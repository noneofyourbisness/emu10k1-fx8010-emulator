#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
using fx8010::Engine;
struct Vec{std::uint32_t q31;std::uint32_t log7;std::uint16_t raw;};
int main(){
    constexpr std::array<Vec,6> v{{
        {0x40004000u,0x70001000u,0x7000u},
        {0x20002000u,0x60001000u,0x6000u},
        {0x10001000u,0x50001000u,0x5000u},
        {0x08000800u,0x40001000u,0x4000u},
        {0x04000400u,0x30001000u,0x3000u},
        {0xff008000u,0xf008000fu,0x8008u},
    }};
    for(const auto &x:v){
        const auto gotLog=static_cast<std::uint32_t>(Engine::logEncode(static_cast<std::int32_t>(x.q31),7,0));
        if(gotLog!=x.log7){std::printf("LOG7 vector mismatch in=%08x got=%08x exp=%08x\
",x.q31,gotLog,x.log7);return 1;}
        const int tank=static_cast<std::int32_t>(x.q31&0xfffff000u)/4096;
        const auto raw=Engine::tramEncode20Candidate(tank);
        if(raw!=x.raw){std::printf("raw vector mismatch in=%08x got=%04x exp=%04x\
",x.q31,raw,x.raw);return 2;}
    }
    std::puts("tram_log7_vectors PASS");
    return 0;
}
