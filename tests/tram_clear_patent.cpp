#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    // US6032235 switches from CLR to valid read exactly when the global
    // zeroed-sample counter reaches the delay length.
    for(std::uint32_t n=0;n<3;++n) if(!Engine::tramClearReadReturnsZero(n,3)) return 2;
    if(Engine::tramClearReadReturnsZero(3,3)) return 3;
    if(Engine::tramClearReadReturnsZero(4,3)) return 4;
    if(Engine::tramClearReadReturnsZero(0,0)) return 5;
    // RSAW initialization uses the same strict-before-equality countdown to
    // force zero writes, preventing stale memory from entering the sum chain.
    for(std::uint32_t n=0;n<5;++n) if(!Engine::tramClearRsawForcesZero(n,5)) return 6;
    if(Engine::tramClearRsawForcesZero(5,5)) return 7;
    std::printf("clear_delay3: zero at 0,1,2; valid at 3\n");
    return 0;
}
