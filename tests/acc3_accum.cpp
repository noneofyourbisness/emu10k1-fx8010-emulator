#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,Engine::q31(0.5)},{0x8001,Engine::q31(-0.5)}};
    p.code.push_back({0,0x4f,0x8000,0x4f,0x8002}); // ACC=1.5, R clips
    p.code.push_back({6,0x56,0x8001,0x40,0x8003}); // 1.5 + -0.5 + 0 = 1.0
    p.code.push_back({0,0x56,0x40,0x40,0x8004});   // copy full ACC high to R
    Engine e;e.load(p);e.runSample();
    const double a=Engine::fromQ31(e.get(0x8003));
    const double b=Engine::fromQ31(e.get(0x8004));
    std::printf("acc3_R=%0.9f copied_ACC=%0.9f\n",a,b);
    if(a<0.9999 || b<0.9999) return 2;
    return 0;
}
