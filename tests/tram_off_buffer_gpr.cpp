#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    TramHardwareControl off{};
    if(Engine::tramAccessMode(off)!=TramAccessMode::Off) return 2;
    // EMU10K1 patent: OFF ITRAM address/data buffers persist and can be used as
    // 20-bit pseudo-GPRs. Core models the physical register width explicitly.
    const std::uint32_t a=Engine::tramBuffer20(0x12345678u);
    const std::uint32_t b=Engine::tramBuffer20(0xffffffffu);
    if(a!=0x00045678u || b!=0x000fffffu) return 3;
    // The value is unchanged by merely being in OFF mode: no TRAM operation is
    // selected. Persistence itself is an architectural property of the slot.
    if(Engine::tramAccessMode(off)!=TramAccessMode::Off || Engine::tramBuffer20(a)!=a) return 4;
    std::printf("off_itram_pseudogpr=%05x mask20=%05x\n",a,b);
    return 0;
}
