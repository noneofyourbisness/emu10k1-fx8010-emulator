#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
using namespace fx8010;

static std::int64_t arshift31(std::int64_t v) noexcept {
    constexpr std::int64_t d = (std::int64_t{1} << 31);
    std::int64_t q = v / d;
    const std::int64_t r = v % d;
    if (v < 0 && r != 0) --q; // arithmetic-right-shift / floor semantics
    return q;
}
static std::int32_t refSat(std::int64_t v){
    const auto q=arshift31(v);
    if(q>INT32_MAX)return INT32_MAX;
    if(q<INT32_MIN)return INT32_MIN;
    return static_cast<std::int32_t>(q);
}
static std::int32_t refWrap(std::int64_t v){
    const auto q=arshift31(v);
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(q));
}
static std::int32_t run(std::uint8_t op,std::int32_t a,std::int32_t x,std::int32_t y,std::uint32_t &cc){
    Program p;p.gprInit={{0x8000,a},{0x8001,x},{0x8002,y}};p.code.push_back({op,0x8000,0x8001,0x8002,0x8003});Engine e;e.load(p);e.runSample();cc=(std::uint32_t)e.get(0x57);return e.get(0x8003);
}
int main(){
    const std::array<std::int32_t,9> v={INT32_MIN,INT32_MIN+1,-0x40000000,-1,0,1,0x3fffffff,0x7ffffffe,INT32_MAX};
    unsigned checked=0;
    constexpr std::int64_t scale=(std::int64_t{1}<<31);
    for(auto a:v)for(auto x:v)for(auto y:v)for(std::uint8_t op=0;op<4;++op){
        const std::int64_t prod=std::int64_t(x)*std::int64_t(y);
        const std::int64_t acc=std::int64_t(a)*scale+((op&1)?-prod:prod);
        const auto expect=op<2?refSat(acc):refWrap(acc);
        std::uint32_t cc=0;const auto got=run(op,a,x,y,cc);
        if(got!=expect){std::printf("mismatch op=%u a=%d x=%d y=%d got=%d exp=%d\n",op,a,x,y,got,expect);return 2;}
        const auto hi=arshift31(acc);
        bool event=hi>INT32_MAX||hi<INT32_MIN;
        if(op>=2){
            const std::int64_t contribution=arshift31((op&1)?-prod:prod);
            const std::int64_t full=std::int64_t(a)+contribution;
            event=full>0x7fffffffll || full<-0x7fffffffll;
        }
        if(((cc&0x10u)!=0)!=event){std::printf("S mismatch op=%u a=%d x=%d y=%d cc=%#x\n",op,a,x,y,cc);return 3;}
        ++checked;
    }
    std::printf("fractional_MAC_edge_vectors=%u pass\n",checked);return 0;
}
