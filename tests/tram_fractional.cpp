#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
static double test(std::uint32_t frac){
 Program p;p.tram.push_back({0x8000,true,0,0,0,0,false});p.tram.push_back({0x8002,false,1,1,0,0,false});
 Engine e;e.load(p);e.set(0x8000,Engine::q31(0.3125));e.runSample();e.setTramAddressRaw(0x8002,(1<<11)|(frac&0x7ff));e.runSample();return Engine::fromQ31(e.get(0x8002));
}
int main(){const double a=test(0),b=test(1),c=test(1023),d=test(2047);std::printf("frac0=%g frac1=%g frac1023=%g frac2047=%g\n",a,b,c,d);return (a==b&&a==c&&a==d&&std::abs(a)>0.1)?0:2;}
