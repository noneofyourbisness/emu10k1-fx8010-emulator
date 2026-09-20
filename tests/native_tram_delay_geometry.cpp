#include "fx8010_engine.h"
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc<2){std::fprintf(stderr,"need native RIFX fixtures\n");return 2;}
    unsigned programs=0, reads=0, writes=0; std::uint32_t minDelay=0xffffffffu,maxDelay=0;
    for(int ai=1;ai<argc;++ai){
        Program p; std::string err;
        if(!RifxLoader::loadFile(argv[ai],p,&err)){std::fprintf(stderr,"%s: %s\n",argv[ai],err.c_str());return 3;}
        if(p.tram.empty()) continue;
        ++programs;
        std::set<std::pair<bool,std::uint32_t>> bases;
        for(const auto&t:p.tram) if(t.write){
            ++writes;
            if(t.auxiliary!=0u){std::fprintf(stderr,"%s: write %04x auxiliary=%u\n",p.name.c_str(),t.dataReg,t.auxiliary);return 4;}
            bases.insert({t.external,t.initialAddress});
        }
        for(const auto&t:p.tram) if(!t.write){
            ++reads;
            if(t.auxiliary==0u || t.initialAddress<t.auxiliary){
                std::fprintf(stderr,"%s: invalid read geometry reg=%04x initial=%u aux=%u\n",p.name.c_str(),t.dataReg,t.initialAddress,t.auxiliary);return 5;
            }
            const std::uint32_t base=t.initialAddress-t.auxiliary;
            if(!bases.count({t.external,base})){
                std::fprintf(stderr,"%s: read %04x base=%u not declared as a %s write\n",p.name.c_str(),t.dataReg,base,t.external?"XTRAM":"ITRAM");return 6;
            }
            minDelay=std::min(minDelay,t.auxiliary); maxDelay=std::max(maxDelay,t.auxiliary);
        }
    }
    // The complete July-1999 SBLFX corpus contains eleven tank-using programs.
    // Every read is relocatable as write-base + auxiliary. The smallest native
    // delay is two samples, so none of these programs depends on an ambiguous
    // zero/one-sample same-cell read/write collision.
    if(programs!=11u || reads!=76u || writes!=52u || minDelay!=2u) {
        std::fprintf(stderr,"unexpected corpus totals programs=%u reads=%u writes=%u min=%u\n",programs,reads,writes,minDelay);return 7;
    }
    std::printf("native TRAM geometry PASS: programs=%u reads=%u writes=%u delay=[%u,%u] all reads=write_base+auxiliary\n",programs,reads,writes,minDelay,maxDelay);
    return 0;
}
