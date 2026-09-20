#include "fx8010_engine.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=2){std::fprintf(stderr,"need Record_Dither.rifx\n");return 2;}
    Program p; std::string err;
    if(!RifxLoader::loadFile(argv[1],p,&err)){std::fprintf(stderr,"load: %s\n",err.c_str());return 3;}
    if(p.name!="Record Dither" || p.code.size()!=8 || p.gprInit.size()!=2 || p.inputPatchSites.size()!=2 || p.outputs.size()!=2) return 4;

    // Native Creative topology: twice per sample it forms NOISE0-NOISE1 and
    // scales the result by GPR 0x8005.  The shipped 16-bit default is 0x10000,
    // exactly one 16-bit PCM LSB in Q31.  This independently corroborates the
    // documented +/-0.5 noise-source amplitude and a TPDF-style dither use.
    auto gpr=[&](std::uint16_t r)->std::uint32_t{for(const auto&g:p.gprInit)if(g.reg==r)return static_cast<std::uint32_t>(g.value);return 0xffffffffu;};
    if(gpr(0x8005)!=0x00010000u || gpr(0x8002)!=0x0000000au) return 5;
    for(unsigned pc: {0u,4u}){
        const auto&i=p.code[pc];
        if(i.op!=0 || i.a!=0x58u || i.x!=0x59u || i.y!=0x4eu || i.r!=0x8007u) return 6;
        const auto&s=p.code[pc+1];
        if(s.op!=0 || s.a!=0x40u || s.x!=0x8007u || s.y!=0x8005u || s.r!=0x8007u) return 7;
    }

    // Silence the two external inputs; then the native RIFX output is pure
    // dither.  Do not assert L/R correlation: whether NOISE0/1 refresh once
    // per sample or during the 512-instruction DSP frame remains silicon-only.
    for(auto site:p.inputPatchSites) if(!patchSite(p.code,site,0x40u)) return 8;
    Engine e; e.load(p);
    std::int32_t lo=std::numeric_limits<std::int32_t>::max();
    std::int32_t hi=std::numeric_limits<std::int32_t>::min();
    bool moved=false,stereoDifferent=false;
    std::int32_t prev=0;
    std::int64_t sum=0;
    constexpr unsigned n=4096;
    for(unsigned k=0;k<n;++k){
        e.runSample();
        const auto l=e.get(0x8000), r=e.get(0x8001);
        // Difference of two [-0.5,+0.5) Q31 sources, scaled by 2^-15,
        // must remain strictly inside +/- one 16-bit LSB.
        if(l < -0x10000 || l > 0x10000 || r < -0x10000 || r > 0x10000) return 9;
        lo=std::min(lo,l); hi=std::max(hi,l); sum+=l;
        if(k) moved |= l!=prev;
        stereoDifferent |= l!=r;
        prev=l;
    }
    if(!moved || lo>=-0x2000 || hi<=0x2000) return 10;
    std::printf("Creative Record Dither: default_scale=0x%08x range=[%d,%d] mean=%.2f stereo_diff=%d (cadence intentionally not asserted)\n",
        gpr(0x8005),lo,hi,double(sum)/double(n),stereoDifferent?1:0);
    return 0;
}
