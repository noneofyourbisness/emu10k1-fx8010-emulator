#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
using namespace fx8010;
int main(){
    const std::array<std::pair<std::uint32_t,std::uint32_t>,9> v={{
        {0x40004000u,0x70001000u},{0x20002000u,0x60001000u},{0x10001000u,0x50001000u},
        {0x08000800u,0x40001000u},{0x04000400u,0x30001000u},{0x02000200u,0x20001000u},
        {0x01000100u,0x10001000u},{0x00800080u,0x08000800u},{0xff008000u,0xf008000fu}
    }};
    bool ok=true;
    for(auto [in,want]:v){
        Program p; p.gprInit={{0x8000,static_cast<std::int32_t>(in)},{0x8001,7},{0x8002,0}};
        p.code.push_back({12,0x8000,0x8001,0x8002,0x8003});
        Engine e;e.load(p);e.runSample();
        const auto got=static_cast<std::uint32_t>(e.get(0x8003));
        std::printf("%08x -> %08x expected %08x\n",in,got,want);
        ok &= got==want;
    }
    return ok?0:2;
}
