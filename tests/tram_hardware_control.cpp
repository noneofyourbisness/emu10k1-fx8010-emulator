#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    if(Engine::internalTramAccessSlots()!=128 || Engine::externalTramAccessSlots()!=32 || Engine::totalTramAccessSlots()!=160) return 2;
    TramHardwareControl c{};c.address=0xabcde;c.clear=true;c.align=true;c.write=true;c.read=false;
    const auto w=Engine::encodeTramHardwareControl(c);
    if(w!=0x00eabcdeu){std::printf("word=%08x\n",w);return 3;}
    const auto d=Engine::decodeTramHardwareControl(w|0xff000000u|0x00100000u);
    if(d.address!=0xabcdeu || !d.clear || !d.align || !d.write || !d.read) return 4;
    // Undefined/unrelated high bits must not leak back through the encoder.
    const auto round=Engine::encodeTramHardwareControl(d);
    if(round!=0x00fabcdeu) return 5;
    std::printf("TRAM physical slots=%u (%u internal + %u external), control=%08x\n",Engine::totalTramAccessSlots(),Engine::internalTramAccessSlots(),Engine::externalTramAccessSlots(),round);
    return 0;
}
