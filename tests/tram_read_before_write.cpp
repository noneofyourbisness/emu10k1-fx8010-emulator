#include "fx8010_engine.h"
#include <cmath>
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.tram.push_back({0x8000,true,0,0,0,0,false});
    p.tram.push_back({0x8002,false,0,0,0,0,false});
    Engine e;e.load(p);
    e.set(0x8000,Engine::q31(0.25));
    e.runSample();
    const double first=Engine::fromQ31(e.get(0x8002)); // read old cell before write commits
    e.setTramAddressRaw(0x8002,(1<<11)); // DBAC moved -1, relative +1 points to prior absolute cell 0
    e.runSample();
    const double second=Engine::fromQ31(e.get(0x8002));
    std::printf("same_cycle_read=%g next_cycle_read=%g\n",first,second);
    return (std::abs(first)<1e-12 && std::abs(second-0.25)<0.002)?0:2;
}
