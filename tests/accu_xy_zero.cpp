#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,Engine::q31(0.5)}};
    // First create a nonzero accumulator. ACCU is special only as A: when used
    // as X/Y the hardware operand reads zero (as10k1 and emu10kJ agree).
    p.code.push_back({0,0x8000,0x8000,0x4f,0x8001});
    p.code.push_back({0,0x8000,0x56,0x4f,0x8002});
    p.code.push_back({0,0x8000,0x4f,0x56,0x8003});
    Engine e;e.load(p);e.runSample();
    const double x=Engine::fromQ31(e.get(0x8002));
    const double y=Engine::fromQ31(e.get(0x8003));
    std::printf("accu_as_x=%0.9f accu_as_y=%0.9f\n",x,y);
    return std::abs(x-0.5)<2e-8 && std::abs(y-0.5)<2e-8 ? 0 : 2;
}
