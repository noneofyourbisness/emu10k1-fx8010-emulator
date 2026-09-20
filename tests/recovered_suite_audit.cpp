#include "fx8010_engine.h"
#include <array>
#include <cstdio>
#include <string>
using namespace fx8010;
int main(int argc,char**argv){
    if(argc!=11){std::fprintf(stderr,"need ten RIFX paths\n");return 2;}
    unsigned total=0,skipCount=0,tramEffects=0,noiseRefs=0,dbacRefs=0,macmv=0,andxor=0,expCount=0,mac23Accu=0,ccrRefs=0;
    for(int f=1;f<argc;++f){
        Program p;std::string err;if(!RifxLoader::loadFile(argv[f],p,&err)){std::fprintf(stderr,"load %s: %s\n",argv[f],err.c_str());return 3;}
        total+=static_cast<unsigned>(p.code.size()); if(!p.tram.empty())++tramEffects;
        for(const auto&i:p.code){
            if(i.op==15){++skipCount;if(i.a!=0x57u){std::fprintf(stderr,"SKIP non-CCR A in %s\n",argv[f]);return 4;}}
            if(i.op==7) ++macmv;
            if(i.op==8) ++andxor;
            if(i.op==13) ++expCount;
            if((i.op==2||i.op==3)&&i.a==0x56u)++mac23Accu;
            for(auto r:{i.a,i.x,i.y}){if(r==0x58u||r==0x59u)++noiseRefs;if(r==0x5bu)++dbacRefs;if(r==0x57u)++ccrRefs;}
        }
    }
    std::printf("instructions=%u tram_effects=%u skips=%u ccr_refs=%u noise_refs=%u dbac_refs=%u macmv=%u andxor=%u exp=%u mac23_accu=%u\n",
        total,tramEffects,skipCount,ccrRefs,noiseRefs,dbacRefs,macmv,andxor,expCount,mac23Accu);
    // These values describe the ten recovered Creative RIFXs in this package.
    // Locking them in makes the accuracy assessment reproducible.
    if(total!=304u || tramEffects!=6u || skipCount!=2u || ccrRefs!=2u) return 5;
    if(noiseRefs||dbacRefs||macmv||andxor||mac23Accu) return 6;
    if(expCount!=1u) return 7;
    return 0;
}
