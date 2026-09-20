#include "fx8010_engine.h"
#include <array>
#include <cstdio>
using namespace fx8010;
int main(){
 std::array<std::int32_t,16> fx{},in{};std::array<std::int32_t,32> out{};
 for(unsigned i=0;i<16;++i){fx[i]=static_cast<std::int32_t>(0x10000000u+i);in[i]=static_cast<std::int32_t>(0x20000000u+i);}
 // No microcode: hardware-observed automatic route.
 Program empty;Engine e;e.load(empty);e.runCardSample(fx,in,out);
 bool ok=true;for(unsigned i=0;i<16;++i){ok&=out[i]==fx[i];ok&=out[16+i]==in[i];}
 // Writing EXTOUT(3) and FXBUS2(2)/EXTOUT(18) overrides only those routes.
 Program p;p.gprInit={{0x8000,0x33333333},{0x8001,0x44444444}};
 p.code.push_back({6,0x8000,0x40,0x40,0x23});
 p.code.push_back({6,0x8001,0x40,0x40,0x32});
 Engine e2;e2.load(p);e2.runCardSample(fx,in,out);
 for(unsigned i=0;i<16;++i){
   const auto ex=(i==3)?0x33333333:fx[i];
   const auto ex2=(i==2)?0x44444444:in[i];
   ok&=out[i]==ex;ok&=out[16+i]==ex2;
 }
 std::printf("card_default_route=%d override_front=%08x override_capture=%08x\n",ok,(unsigned)out[3],(unsigned)out[18]);
 return ok?0:2;
}
