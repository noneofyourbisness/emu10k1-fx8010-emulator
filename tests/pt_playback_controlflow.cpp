#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
static int run(std::int32_t input){
    Program p;
    p.gprInit={{0x8000,input},{0x8001,7},{0x8002,0},{0x8003,0x11111111},{0x8004,0x22222222}};
    // Mirrors the control dependency in Linux PT playback: LOG updates CCR,
    // then SKIP tests the MINUS condition. The following move is skipped only
    // for a negative LOG result.
    p.code.push_back({12,0x8000,0x8001,0x8002,0x8005});
    p.code.push_back({15,0x57,0x44,0x41,0x8006}); // X=C_4 (MINUS), Y=C_1 count
    p.code.push_back({0,0x8003,0x40,0x40,0x8007});
    p.code.push_back({0,0x8004,0x40,0x40,0x8008});
    Engine e;e.load(p);e.runSample();
    const bool firstRan=e.get(0x8007)==0x11111111;
    const bool lastRan=e.get(0x8008)==0x22222222;
    std::printf("input=%08x ccr=%08x first=%d last=%d\n",(unsigned)input,(unsigned)e.get(0x57),firstRan,lastRan);
    return (firstRan?1:0)|(lastRan?2:0);
}
int main(){
    const int pos=run(0x10000000);
    const int neg=run((std::int32_t)0xf0000000u);
    if(pos!=3) return 2; // positive: MINUS clear, no skip
    if(neg!=2) return 3; // negative: MINUS set, skip first move only
    return 0;
}
