#include "fx8010_engine.h"
#include <cstdio>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=2){std::fprintf(stderr,"need Record_Dither.rifx\n");return 2;}
    Program p; std::string err;
    if(!RifxLoader::loadFile(argv[1],p,&err)){std::fprintf(stderr,"load: %s\n",err.c_str());return 3;}
    if(p.name!="Record Dither" || p.code.size()!=8) return 4;
    // Creative emits two explicit NOISE0-NOISE1 expressions at PCs 0 and 4.
    // This proves two reads occur in one sample frame, but not their refresh
    // cadence: the object contains no captured output/reference sequence and
    // the compiler emits independent channel expressions rather than a shared
    // dither temporary. Keep the emulator's conservative per-sample default.
    for(unsigned pc: {0u,4u}){
        const auto&i=p.code[pc];
        if(i.op!=0 || i.a!=0x58u || i.x!=0x59u || i.y!=0x4eu) return 5;
    }
    // With zero inputs, a sample-static source necessarily gives correlated
    // L/R dither. Record that as a diagnostic, not as a silicon assertion.
    for(auto site:p.inputPatchSites) if(!patchSite(p.code,site,0x40u)) return 6;
    Engine e; e.load(p);
    bool any=false,allSame=true; std::int32_t prev=0;
    for(unsigned n=0;n<256;++n){e.runSample();const auto l=e.get(0x8000),r=e.get(0x8001);any|=(n==0?l!=0:l!=prev);allSame&=l==r;prev=l;}
    if(!any || !allSame) return 7;
    std::printf("noise cadence diagnostic: native reads at pc0/pc4; current per-sample model => correlated stereo. Silicon intra-frame refresh remains unproven.\n");
    return 0;
}
