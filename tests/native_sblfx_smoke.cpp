#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=24){std::fprintf(stderr,"need all 23 July-1999 SBLFX RIFX fixtures\n");return 2;}
    std::uint64_t samples=0;
    for(int a=1;a<argc;++a){
        Program p;std::string err;
        if(!RifxLoader::loadFile(argv[a],p,&err)){std::fprintf(stderr,"load %s: %s\n",argv[a],err.c_str());return 3;}
        // Bind every host input patch to one of the ordinary 0x10..0x1f input
        // registers. This is only an interpreter integration smoke test: it
        // deliberately does not assert hardware reference audio.
        unsigned k=0;
        for(auto site:p.inputPatchSites) if(!patchSite(p.code,site,static_cast<std::uint16_t>(0x10u+(k++&15u)))) return 4;
        Engine e; e.load(p);
        for(unsigned n=0;n<64;++n){
            for(unsigned r=0;r<16;++r){
                const std::uint32_t u=(0x01020304u*(r+1u))^(0x00110011u*n);
                e.set(static_cast<std::uint16_t>(0x10u+r),static_cast<std::int32_t>(u&0x3fffffffu));
            }
            e.runSample(); ++samples;
        }
    }
    std::printf("Creative SBLFX 1999 execution smoke: 23/23 programs, %llu aggregate sample frames PASS\n",(unsigned long long)samples);
    return 0;
}
