#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
int main(){
 Program p;
 p.tram.push_back({0x8000,true,0,0,0,0,false});
 p.tram.push_back({0x8002,true,0,0,0,0,true});
 p.tram.push_back({0x8004,false,1,1,0,0,false});
 p.tram.push_back({0x8006,false,1,1,0,0,true});
 Engine e;e.load(p);
 if(e.itramSize()!=8192 || e.xtramSize()!=(1u<<20)){std::printf("sizes I=%zu X=%zu\n",e.itramSize(),e.xtramSize());return 2;}
 e.set(0x8000,Engine::q31(0.125));e.set(0x8002,Engine::q31(-0.375));e.runSample();
 e.runSample();
 const double i=Engine::fromQ31(e.get(0x8004)),x=Engine::fromQ31(e.get(0x8006));
 std::printf("itram=%g xtram=%g dbac=%#x\n",i,x,e.debugDbac());
 if(std::abs(i-0.125)>0.002 || std::abs(x+0.375)>0.002 || std::abs(i-x)<0.1)return 3;
 if(e.debugDbac()!=0x000ffffeu)return 4;
 return 0;
}
