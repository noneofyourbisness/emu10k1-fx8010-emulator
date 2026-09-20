#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>

using fx8010::Engine;

static std::int32_t q31FromTank20(std::int32_t x) noexcept {
    return static_cast<std::int32_t>(static_cast<std::int64_t>(x) << 12);
}

// Literal software transcription of the stock Linux raw-IEC958 FX8010 sequence:
// ANDXOR expanded,0xfffff000; LOG(...,7,0); ANDXOR 0xffff0000,^0x70000000;
// SKIP on MINUS; second identical ANDXOR only when the first result is positive.
static std::uint16_t linuxRawIec958Recovery(std::int32_t tank20) noexcept {
    const std::uint32_t q=static_cast<std::uint32_t>(q31FromTank20(tank20)) & 0xfffff000u;
    const std::uint32_t log=static_cast<std::uint32_t>(Engine::logEncode(static_cast<std::int32_t>(q),7u,0u));
    std::uint32_t out=(log & 0xffff0000u) ^ 0x70000000u;
    if((out & 0x80000000u)==0u)
        out=(out & 0xffff0000u) ^ 0x70000000u;
    return static_cast<std::uint16_t>(out>>16);
}

int main(){
    std::array<std::int32_t,65536> lo{},hi{};
    std::array<std::uint32_t,65536> count{};
    lo.fill(std::numeric_limits<std::int32_t>::max());
    hi.fill(std::numeric_limits<std::int32_t>::min());

    // Entire signed-20 input domain: the engine's physical raw-word formatter
    // must be exactly the independently transcribed Linux raw-S/PDIF sequence.
    for(std::int32_t x=-524288;x<=524287;++x){
        const auto expected=linuxRawIec958Recovery(x);
        const auto got=Engine::tramRawWordFromTank20(x);
        if(got!=expected){
            std::printf("linux recovery mismatch x=%d got=%04x expected=%04x\n",x,got,expected);
            return 1;
        }
        auto &l=lo[got], &h=hi[got];
        if(x<l)l=x;
        if(x>h)h=x;
        ++count[got];
    }

    std::uint32_t maxBucket=0;
    std::int32_t maxError=0;
    for(std::uint32_t c=0;c<=0xffffu;++c){
        const auto code=static_cast<std::uint16_t>(c);
        const auto representative=Engine::tramTank20RepresentativeFromRawWord(code);
        if(count[c]==0u){
            std::printf("unpopulated raw code %04x\n",c);
            return 2;
        }
        if(Engine::tramRawWordFromTank20(representative)!=code){
            std::printf("raw roundtrip mismatch c=%04x rep=%d got=%04x\n",c,representative,
                        Engine::tramRawWordFromTank20(representative));
            return 3;
        }
        if(linuxRawIec958Recovery(representative)!=code){
            std::printf("linux roundtrip mismatch c=%04x rep=%d\n",c,representative);
            return 4;
        }
        // Patent-consistent sign reconstruction is symmetric in magnitude:
        // positive truncated LOG words zero-fill to the bucket's low edge;
        // negative one's-complement words restore discarded encoded bits as
        // ones and therefore land on the bucket's high (toward-zero) edge.
        const auto expectedRepresentative=(code&0x8000u)?hi[c]:lo[c];
        if(representative!=expectedRepresentative){
            std::printf("representative bucket-edge mismatch c=%04x rep=%d range=%d..%d\n",
                        c,representative,lo[c],hi[c]);
            return 5;
        }
        if(count[c]>maxBucket)maxBucket=count[c];
        const auto err=hi[c]-lo[c];
        if(err>maxError)maxError=err;
    }

    if(maxBucket!=64u || maxError!=63){
        std::printf("unexpected bucket geometry maxBucket=%u maxError=%d\n",maxBucket,maxError);
        return 6;
    }

    struct V{std::int32_t x;std::uint16_t raw;};
    constexpr V boundaries[]={{0,0x0000},{1,0x0001},{4095,0x0fff},{4096,0x1000},
                              {262144,0x7000},{524287,0x7fff},{-1,0x8fff},{-4096,0x8000},
                              {-262145,0xffff},{-524288,0xf000}};
    for(const auto &v:boundaries){
        const auto got=Engine::tramRawWordFromTank20(v.x);
        if(got!=v.raw){
            std::printf("boundary mismatch x=%d got=%04x expected=%04x\n",v.x,got,v.raw);
            return 7;
        }
    }

    std::printf("tram_raw_spdif_codebook PASS: 1048576 signed20 inputs, 65536 raw words, max bucket=%u, max quantisation span=%d LSB\n",
                maxBucket,maxError);
    return 0;
}
