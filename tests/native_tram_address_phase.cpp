#include "fx8010_engine.h"
#include <cstdio>
#include <set>
#include <string>
using namespace fx8010;

static unsigned addressWrites(const Program& p){
    std::set<std::uint16_t> addressRegs;
    for(const auto&t:p.tram) addressRegs.insert(std::uint16_t(t.dataReg+1));
    unsigned n=0;
    for(const auto&i:p.code) if(addressRegs.count(i.r)) ++n;
    return n;
}

int main(int argc,char **argv){
    if(argc!=8){std::fprintf(stderr,"need headphone,sb2,sb4,surround,chorus,flanger,pitch fixtures\n");return 2;}
    const unsigned expected[7]={0,0,0,0,4,4,4};
    for(int ai=1;ai<argc;++ai){
        Program p; std::string err;
        if(!RifxLoader::loadFile(argv[ai],p,&err)){std::fprintf(stderr,"%s: %s\n",argv[ai],err.c_str());return 3;}
        const auto n=addressWrites(p);
        std::printf("%s: DSP writes to TRAM address regs=%u\n",p.name.c_str(),n);
        if(n!=expected[ai-1]) return 4+ai;
    }
    std::puts("native TRAM phase classification PASS: hidden spatializers are fixed-address; Chorus/Flanger/PitchShift dynamically update four read addresses per frame");
    return 0;
}
