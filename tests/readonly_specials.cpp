#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,0x12345678}};
    // Invalid result destinations must not overwrite hardware-owned specials.
    p.code.push_back({0,0x8000,0x40,0x40,0x58});
    p.code.push_back({0,0x8000,0x40,0x40,0x59});
    p.code.push_back({0,0x8000,0x40,0x40,0x5b});
    Engine e;e.load(p);e.runSample();
    const auto n0=e.get(0x58),n1=e.get(0x59),db=e.get(0x5b);
    std::printf("noise0=%#x noise1=%#x dbac=%#x\n",(unsigned)n0,(unsigned)n1,(unsigned)db);
    // Noise now legitimately changes every sample; it merely must not become
    // the attempted 0x12345678 write. DBAC is 0 at the start of sample zero.
    return (n0!=0x12345678 && n1!=0x12345678 && db==0) ? 0 : 2;
}
