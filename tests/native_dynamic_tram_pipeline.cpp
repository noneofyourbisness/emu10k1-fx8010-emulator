#include "fx8010_engine.h"
#include <cstdio>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=4){std::fprintf(stderr,"need Chorus, Flanger, PitchShift fixtures\n");return 2;}
    const unsigned expectedWrites[3]={4,4,4};
    for(int ai=1;ai<argc;++ai){
        Program p; std::string err;
        if(!RifxLoader::loadFile(argv[ai],p,&err)){std::fprintf(stderr,"%s: %s\n",argv[ai],err.c_str());return 3;}
        unsigned dynamic=0; int minGap=999;
        for(const auto&t:p.tram){
            const auto ar=std::uint16_t(t.dataReg+1);
            int addressWrite=-1,lastDataUse=-1;
            for(int pc=0;pc<(int)p.code.size();++pc){
                const auto&i=p.code[(std::size_t)pc];
                if(i.r==ar) addressWrite=pc;
                if(i.a==t.dataReg||i.x==t.dataReg||i.y==t.dataReg) lastDataUse=pc;
            }
            if(addressWrite>=0){
                ++dynamic;
                if(lastDataUse<0 || addressWrite<=lastDataUse){
                    std::fprintf(stderr,"%s: address %04x written at pc%d before/at final data use pc%d\n",p.name.c_str(),ar,addressWrite,lastDataUse);return 4;
                }
                minGap=std::min(minGap,addressWrite-lastDataUse);
            }
        }
        std::printf("%s: dynamic read addresses=%u minimum address-update-after-data-use gap=%d instructions\n",p.name.c_str(),dynamic,minGap);
        if(dynamic!=expectedWrites[ai-1] || minGap<6) return 5+ai;
    }
    std::puts("native dynamic TRAM pipeline PASS: Creative consumes current TRAM data before computing next read addresses");
    return 0;
}
