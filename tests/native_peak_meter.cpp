#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=2){std::fprintf(stderr,"need Peak_Meter.rifx\n");return 2;}
    Program p; std::string err;
    if(!RifxLoader::loadFile(argv[1],p,&err)){std::fprintf(stderr,"load: %s\n",err.c_str());return 3;}
    if(p.name!="Peak Meter" || p.code.size()!=3 || p.inputPatchSites.size()!=2) return 4;
    const auto&i0=p.code[0]; const auto&i1=p.code[1]; const auto&i2=p.code[2];
    // Creative averages the two inputs with INTERP(0.5), LOGs the result with
    // max exponent 31/sign mode 1, then LIMITGE-latches the larger code.
    if(i0.op!=14 || i0.x!=0x4du || i0.r!=0x8003u) return 5;
    if(i1.op!=12 || i1.a!=0x8003u || i1.x!=0x8000u || i1.y!=0x41u || i1.r!=0x8003u) return 6;
    if(i2.op!=10 || i2.a!=0x8003u || i2.x!=0x8003u || i2.y!=0x8001u || i2.r!=0x8001u) return 7;
    bool exp31=false; for(const auto&g:p.gprInit) if(g.reg==0x8000u && static_cast<std::uint32_t>(g.value)==31u) exp31=true;
    if(!exp31) return 8;

    // Patch the two inputs to test GPRs and verify the native peak latch works
    // with our INTERP/LOG/LIMITGE implementation.
    if(!patchSite(p.code,p.inputPatchSites[0],0x8004u) || !patchSite(p.code,p.inputPatchSites[1],0x8005u)) return 9;
    Engine e; e.load(p);
    const std::array<std::int32_t,5> in{{0x00100000,0x01000000,0x10000000,0x08000000,0x00100000}};
    std::uint32_t last=0;
    for(auto v:in){
        e.set(0x8004u,v); e.set(0x8005u,v); e.runSample();
        const auto cur=static_cast<std::uint32_t>(e.get(0x8003u));
        const auto peak=static_cast<std::uint32_t>(e.get(0x8001u));
        if(peak<last || peak<cur) return 10;
        last=peak;
    }
    if(last!=0x74000000u) return 11;
    std::printf("Creative Peak Meter: INTERP->LOG(31,1)->LIMITGE native path peak=0x%08x\n",last);
    return 0;
}
