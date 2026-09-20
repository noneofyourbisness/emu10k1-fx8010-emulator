#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <string>
using namespace fx8010;

int main(int argc,char **argv){
    if(argc!=2){std::fprintf(stderr,"need ac3pass.rifx\n");return 2;}
    Program p; std::string err;
    if(!RifxLoader::loadFile(argv[1],p,&err)){std::fprintf(stderr,"load: %s\n",err.c_str());return 3;}
    if(p.name!="ac3pass" || p.code.size()!=24 || p.tram.size()!=2 || p.xtramSize!=6146u) return 4;

    unsigned skips=0,irqWrites=0,dbacReads=0,andxor=0,logs=0;
    bool nativeSkip180=false,nativeSkipAll=false;
    for(const auto&i:p.code){
        if(i.op==15){
            ++skips;
            if(i.a!=0x57u) return 5; // native SKIP takes CCR in A
            if(i.x==0x8007u && i.y==0x8005u) nativeSkip180=true; // GPRs = 0x180, 5
            if(i.x==0x4fu && i.y==0x42u) nativeSkipAll=true;      // C_7fffffff, C_2
        }
        if(i.r==0x5au){++irqWrites;if(i.op!=0 || i.a!=0x4eu) return 6;}
        for(auto r:{i.a,i.x,i.y}) if(r==0x5bu) ++dbacReads;
        if(i.op==8) ++andxor;
        if(i.op==12) ++logs;
    }
    if(skips!=3 || irqWrites!=2 || dbacReads!=1 || andxor!=4 || logs!=2 || !nativeSkip180 || !nativeSkipAll) return 7;

    // Default-state execution reaches Creative's first IRQ request and captures
    // DBAC+1 into 0x8002.  This checks the special-register semantics together
    // with the native control flow without claiming cycle-exact XTRAM timing.
    Engine e; e.load(p); e.runSample();
    if(!e.irqPending() || e.irqCount()!=1u || e.get(0x8002u)!=1) return 8;
    if(!e.consumeIrq() || e.irqPending()) return 9;

    std::printf("Creative ac3pass: skips=%u irq_writes=%u dbac_reads=%u andxor=%u log=%u first_irq=PASS DBAC+1=%d\n",
        skips,irqWrites,dbacReads,andxor,logs,e.get(0x8002u));
    return 0;
}
