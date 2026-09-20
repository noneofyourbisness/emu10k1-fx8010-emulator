#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
using fx8010::Engine;

static std::int32_t patentDecode(std::uint16_t raw) noexcept {
    // Physical word -> upper 16 bits of the FX8010 LOG(7,0) word.
    std::uint16_t log16=raw;
    if(log16&0x8000u) log16=static_cast<std::uint16_t>(log16^0x7000u);

    std::uint32_t q=0;
    if((log16&0x8000u)==0u) {
        // US5930158 EXP: reconstructed mantissa bits below the retained field are zero.
        const std::uint32_t positive=std::uint32_t(log16)<<16;
        q=static_cast<std::uint32_t>(Engine::expDecode(static_cast<std::int32_t>(positive),7u,0u));
    } else {
        // Patent negative path: one's-complement the LOG operand first.  Since the
        // positive reconstructed mantissa's missing bits are zero, the truncated
        // negative LOG operand necessarily has ones in those missing positions.
        const std::uint32_t negative=(std::uint32_t(log16)<<16)|0xffffu;
        const std::uint32_t positive=~negative;
        const std::uint32_t magnitude=static_cast<std::uint32_t>(
            Engine::expDecode(static_cast<std::int32_t>(positive),7u,0u));
        q=~magnitude;
    }
    q&=0xfffff000u;
    return static_cast<std::int32_t>(q)/4096;
}

int main(){
    std::array<std::int32_t,65536> lo{},hi{};
    lo.fill(std::numeric_limits<std::int32_t>::max());
    hi.fill(std::numeric_limits<std::int32_t>::min());
    for(std::int32_t x=-524288;x<=524287;++x){
        const auto c=Engine::tramRawWordFromTank20(x);
        if(x<lo[c])lo[c]=x;
        if(x>hi[c])hi[c]=x;
    }

    std::uint32_t multiPositive=0,multiNegative=0;
    for(std::uint32_t c=0;c<=0xffffu;++c){
        const auto code=static_cast<std::uint16_t>(c);
        const auto expected=patentDecode(code);
        const auto got=Engine::tramTank20RepresentativeFromRawWord(code);
        if(got!=expected){
            std::printf("patent decode mismatch %04x got=%d expected=%d\n",c,got,expected);
            return 1;
        }
        if(Engine::tramRawWordFromTank20(got)!=code){
            std::printf("roundtrip mismatch %04x rep=%d\n",c,got);
            return 2;
        }
        const auto bucket=Engine::tramTank20BucketFromRawWord(code);
        if(bucket.low!=lo[c] || bucket.high!=hi[c]){
            std::printf("bucket API mismatch %04x got=%d..%d expected=%d..%d\n",
                        c,bucket.low,bucket.high,lo[c],hi[c]);
            return 6;
        }
        if((code&0x8000u)==0u){
            if(got!=lo[c]){
                std::printf("positive bucket edge mismatch %04x rep=%d range=%d..%d\n",c,got,lo[c],hi[c]);
                return 3;
            }
            if(lo[c]!=hi[c]) ++multiPositive;
        }else{
            if(got!=hi[c]){
                std::printf("negative bucket edge mismatch %04x rep=%d range=%d..%d\n",c,got,lo[c],hi[c]);
                return 4;
            }
            if(lo[c]!=hi[c]) ++multiNegative;
        }
    }
    if(multiPositive==0u || multiNegative==0u){
        std::puts("expected multi-value buckets on both signs"); return 5;
    }
    std::printf("tram_decompressor_patent PASS: all 65536 words, positive->bucket low, negative->bucket high, multi=%u/%u\n",
                multiPositive,multiNegative);
    return 0;
}
