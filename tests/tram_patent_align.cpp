#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    // Patent law: unaligned address is rel+DBAC; aligned read subtracts one
    // sample, aligned write adds one. Exercise ordinary and wrap boundaries.
    if(Engine::tramPhysicalReadAddress(0,0,false)!=0) return 2;
    if(Engine::tramPhysicalReadAddress(0,0,true)!=0xfffffu) return 3;
    if(Engine::tramPhysicalWriteAddress(0,0,false)!=0) return 4;
    if(Engine::tramPhysicalWriteAddress(0,0,true)!=1) return 5;
    if(Engine::tramPhysicalReadAddress(0x20,0x10,true)!=0x2fu) return 6;
    if(Engine::tramPhysicalWriteAddress(0xfffff,1,true)!=1u) return 7;
    std::printf("read_aligned_zero=%05x write_aligned_zero=%05x\n",
        Engine::tramPhysicalReadAddress(0,0,true),Engine::tramPhysicalWriteAddress(0,0,true));
    return 0;
}
