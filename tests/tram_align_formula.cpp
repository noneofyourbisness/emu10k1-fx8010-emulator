#include "fx8010_engine.h"
#include <cstdio>
using fx8010::Engine;
int main(){
    // kX loader boundary vectors: ITRAM uses 3 instruction slots/access,
    // XTRAM uses 4 and starts at instruction 128/127 for write/read tests.
    if( Engine::tramAlignFlag(false,true, 2,1)) return 1;
    if(!Engine::tramAlignFlag(false,true, 3,1)) return 2;
    if(!Engine::tramAlignFlag(false,false,0,0)) return 3;
    if(!Engine::tramAlignFlag(false,false,1,0)) return 4;
    if( Engine::tramAlignFlag(false,false,2,0)) return 5;
    if( Engine::tramAlignFlag(true,true,131,1)) return 6;
    if(!Engine::tramAlignFlag(true,true,132,1)) return 7;
    if(!Engine::tramAlignFlag(true,false,0,1)) return 8;
    if(!Engine::tramAlignFlag(true,false,131,1)) return 9;
    if( Engine::tramAlignFlag(true,false,132,1)) return 10;
    std::puts("tram_align_formula PASS");
    return 0;
}
