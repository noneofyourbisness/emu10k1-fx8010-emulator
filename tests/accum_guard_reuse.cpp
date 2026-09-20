#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,Engine::q31(0.5)},{0x8001,Engine::q31(0.6)}};
    // Internal accumulator = 1.0 + 0.5 = 1.5, while R saturates to ~1.0.
    p.code.push_back({0,0x4f,0x8000,0x4f,0x8002});
    // Reuse the unsaturated 1.5 accumulator and subtract 0.6 => 0.9.
    p.code.push_back({1,0x56,0x8001,0x4f,0x8003});
    Engine e;e.load(p);e.runSample();
    const double sat=Engine::fromQ31(e.get(0x8002));
    const double reused=Engine::fromQ31(e.get(0x8003));
    std::printf("first_R=%0.9f reused_ACC=%0.9f\n",sat,reused);
    if(sat<0.9999 || std::abs(reused-0.9)>2e-8) return 2;
    return 0;
}
