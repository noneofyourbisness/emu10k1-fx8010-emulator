#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    // Deliberately no TRAM declarations: DBAC is global and must still tick.
    p.code.push_back({0,0x5b,0x40,0x40,0x8000});
    Engine e; e.load(p);
    e.runSample(); const auto r0=(std::uint32_t)e.get(0x8000), d0=e.debugDbac();
    e.runSample(); const auto r1=(std::uint32_t)e.get(0x8000), d1=e.debugDbac();
    e.runSample(); const auto r2=(std::uint32_t)e.get(0x8000), d2=e.debugDbac();
    std::printf("dbac_read=%#x,%#x,%#x internal=%#x,%#x,%#x\n",r0,r1,r2,d0,d1,d2);
    return r0==0x00000000u && r1==0x7ffff800u && r2==0x7ffff000u &&
           d0==0x000fffffu && d1==0x000ffffeu && d2==0x000ffffdu ? 0 : 2;
}
