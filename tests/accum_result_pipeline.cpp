#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    // ANDXOR: (A & X) ^ Y => 0x20000000, then MACS copies ACCU.
    p.gprInit={{0x8000,0x60000000},{0x8001,0x3fffffff},{0x8002,0}};
    p.code.push_back({8,0x8000,0x8001,0x8002,0x8003});
    p.code.push_back({0,0x56,0x40,0x40,0x8004});
    Engine e;e.load(p);e.runSample();
    std::printf("andxor=%#x accum_copy=%#x\n",(unsigned)e.get(0x8003),(unsigned)e.get(0x8004));
    return e.get(0x8003)==0x20000000 && e.get(0x8004)==0x20000000 ? 0 : 2;
}
