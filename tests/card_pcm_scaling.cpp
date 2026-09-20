#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    const auto measured=Engine::approximatePlayback16ToBus(0x1000);
    std::printf("play 0x1000 -> %08x capture -> %04x\n",(unsigned)measured,(unsigned)(std::uint16_t)Engine::capture16FromBus(measured));
    if(static_cast<std::uint32_t>(measured)!=0x03fbfc00u) return 2;
    if(Engine::capture16FromBus(measured)!=0x03fb) return 3;
    if(Engine::capture16FromBus(0x7fffffff)!=32767) return 4;
    if(Engine::capture16FromBus((std::int32_t)0x80000000u)!=-32768) return 5;
    const auto neg=Engine::approximatePlayback16ToBus(-0x1000);
    if((static_cast<std::uint32_t>(neg)&0xffu)!=0u) return 6;
    // Send-volume zero must mute exactly.
    if(Engine::approximatePlayback16ToBus(12345,65535,0)!=0) return 7;
    return 0;
}
