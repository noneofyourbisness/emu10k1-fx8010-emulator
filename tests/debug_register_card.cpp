#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,0x7fffffff},{0x8001,0x7fffffff}};
    // PC0: +1 + (+1 * +1) -> hard positive saturation.
    p.code.push_back({0,0x8000,0x8001,0x8001,0x8002});
    Engine e;e.load(p);e.runSample();
    const std::uint32_t dbg=e.debugRegisterWord();
    const std::uint32_t ccr=(dbg&0x00003e00u)>>9;
    const unsigned satAddr=(dbg&0x01ff0000u)>>16;
    std::printf("dbg=%08x ccr=%02x sat_addr=%u dbac=%05x\n",dbg,ccr,satAddr,e.debugDbac());
    if((dbg&0x02000000u)==0u) return 2;
    if(satAddr!=0u) return 3;
    if(ccr!=(static_cast<std::uint32_t>(e.get(0x57))&0x1fu)) return 4;
    if(e.debugDbac()!=0x000fffffu) return 5;
    e.writeDebugRegister(0x80000000u); // EMU10K1_DBG_ZC
    if(e.debugDbac()!=0u || e.get(0x5b)!=0) return 6;
    return 0;
}
