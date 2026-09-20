#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    TramDecl t; t.dataReg=0x8000; t.initialAddress=0; t.write=false; t.external=false;
    p.tram.push_back(t);
    Engine e;e.load(p);
    e.set(0x8001,static_cast<std::int32_t>(0x80000d55u));
    const auto visible=static_cast<std::uint32_t>(e.get(0x8001));
    const bool signCleared=(visible&0x80000000u)==0;
    const bool fractionKept=(visible&0x7ffu)==0x555u;
    const bool physicalKept=((visible>>11)&0xfffffu)==((0x00000d55u>>11)&0xfffffu);
    e.runSample();
    const bool dbacWrapped=e.debugDbac()==0xfffffu;
    std::printf("visible=%08x signCleared=%d fractionKept=%d physicalKept=%d dbac=%05x\n",visible,signCleared,fractionKept,physicalKept,e.debugDbac());
    return (signCleared&&fractionKept&&physicalKept&&dbacWrapped)?0:2;
}
