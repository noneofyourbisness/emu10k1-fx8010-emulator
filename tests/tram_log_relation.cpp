#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using fx8010::Engine;
static std::int32_t q31FromTank(int x){return static_cast<std::int32_t>(static_cast<std::int64_t>(x)*4096ll);}
static std::uint16_t tracedEncode(int x){
    const auto l=static_cast<std::uint32_t>(Engine::logEncode(q31FromTank(x),7,0));
    std::uint16_t u=static_cast<std::uint16_t>(l>>16);
    if(u&0x8000u)u=static_cast<std::uint16_t>(u^0x7000u);
    return u;
}
static int tracedDecode(std::uint16_t c){
    std::uint16_t u=c;
    if(u&0x8000u)u=static_cast<std::uint16_t>(u^0x7000u);
    std::uint32_t log=std::uint32_t(u)<<16;
    if(u&0x8000u) log|=0xffffu;
    const std::uint32_t q=static_cast<std::uint32_t>(Engine::expDecode(static_cast<std::int32_t>(log),7,0))&0xfffff000u;
    return static_cast<std::int32_t>(q)/4096;
}
int main(){
    for(int x=-524288;x<=524287;++x){
        const auto expected=tracedEncode(x);
        const auto got=Engine::tramEncode20Candidate(x);
        if(got!=expected){std::printf("encode mismatch x=%d got=%04x exp=%04x\
",x,got,expected);return 1;}
    }
    for(unsigned c=0;c<=0xffffu;++c){
        const int expected=tracedDecode(static_cast<std::uint16_t>(c));
        const int got=Engine::tramDecode20Candidate(static_cast<std::uint16_t>(c));
        if(got!=expected){std::printf("decode mismatch c=%04x got=%d exp=%d\
",c,got,expected);return 2;}
    }
    std::puts("tram_log_relation PASS exhaustive signed20/raw-log16 relation");
    return 0;
}
