#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,Engine::q31(0.125)},{0x8001,Engine::q31(0.5)},{0x8002,Engine::q31(0.5)}};
    // MACMV moves A to R while accumulating the raw 64-bit product. A following
    // fractional MAC fetch of ACCU's high half should therefore see +0.25.
    p.code.push_back({7,0x8000,0x8001,0x8002,0x8003});
    p.code.push_back({0,0x56,0x40,0x40,0x8004});
    Engine e;e.load(p);e.runSample();
    const double moved=Engine::fromQ31(e.get(0x8003));
    const double accum=Engine::fromQ31(e.get(0x8004));
    std::printf("macmv_move=%0.9f accum_high=%0.9f\n",moved,accum);
    return std::abs(moved-0.125)<2e-8 && std::abs(accum-0.25)<2e-8 ? 0 : 2;
}
