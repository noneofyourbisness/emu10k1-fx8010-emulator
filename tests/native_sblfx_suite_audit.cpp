#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=24){std::fprintf(stderr,"need all 23 July-1999 SBLFX RIFX fixtures\n");return 2;}
    std::array<unsigned,16> ops{};
    unsigned instructions=0,tramEffects=0,itramEffects=0,xtramEffects=0;
    unsigned noiseRefs=0,dbacRefs=0,irqWrites=0,ccrRefs=0,accuRefs=0;
    std::size_t tramDecls=0;
    for(int a=1;a<argc;++a){
        Program p;std::string err;
        if(!RifxLoader::loadFile(argv[a],p,&err)){std::fprintf(stderr,"load %s: %s\n",argv[a],err.c_str());return 3;}
        instructions+=static_cast<unsigned>(p.code.size());
        tramDecls+=p.tram.size(); if(!p.tram.empty())++tramEffects; if(p.itramSize)++itramEffects; if(p.xtramSize)++xtramEffects;
        for(const auto&i:p.code){
            if(i.op>15) return 4;
            ++ops[i.op];
            for(auto r:{i.a,i.x,i.y}){if(r==0x58u||r==0x59u)++noiseRefs;if(r==0x5bu)++dbacRefs;if(r==0x57u)++ccrRefs;if(r==0x56u)++accuRefs;}
            if(i.r==0x5au)++irqWrites;
        }
    }
    // Lock the complete July-1999 Creative SBLFX corpus into the regression
    // suite.  This is native-program coverage, not a claim that every program
    // is cycle-exact on silicon.
    const std::array<unsigned,16> expected{{588,125,1,2,62,7,12,0,4,3,3,0,6,1,244,5}};
    if(instructions!=1063u || tramEffects!=11u || itramEffects!=9u || xtramEffects!=3u || tramDecls!=128u) return 5;
    if(noiseRefs!=4u || dbacRefs!=1u || irqWrites!=2u || ccrRefs!=5u || accuRefs!=22u || ops!=expected) return 6;
    std::printf("Creative SBLFX 1999 corpus: 23 programs, %u instructions, 128 TRAM declarations; opcode mask covers 14/16 opcodes; NOISE=%u DBAC=%u IRQ=%u CCR=%u ACCU=%u\n",
        instructions,noiseRefs,dbacRefs,irqWrites,ccrRefs,accuRefs);
    return 0;
}
