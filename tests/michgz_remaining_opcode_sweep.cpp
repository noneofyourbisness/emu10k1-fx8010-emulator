#include "fx8010_engine.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>
using namespace fx8010;

static std::int32_t sat(std::int64_t v){
    if(v>INT32_MAX)return INT32_MAX;
    if(v<INT32_MIN)return INT32_MIN;
    return static_cast<std::int32_t>(v);
}
static std::int64_t arshift31(std::int64_t v){
    if(v>=0)return v/(std::int64_t(1)<<31);
    const auto m=-v;
    return -((m+((std::int64_t(1)<<31)-1))/(std::int64_t(1)<<31));
}
static std::int32_t ref(std::uint8_t op,std::int32_t a,std::int32_t x,std::int32_t y){
    switch(op){
    case 0:return sat(std::int64_t(a)+arshift31(std::int64_t(x)*std::int64_t(y)));
    case 1:return sat(std::int64_t(a)+arshift31(-std::int64_t(x)*std::int64_t(y)));
    case 4:return sat(std::int64_t(a)+std::int64_t(x)*std::int64_t(y));
    case 5:return static_cast<std::int32_t>(static_cast<std::uint64_t>(std::int64_t(a)+std::int64_t(x)*std::int64_t(y))&0x7fffffffu);
    case 6:return sat(std::int64_t(a)+std::int64_t(x)+std::int64_t(y));
    case 10:return a>=y?x:y;
    case 11:return a<y?x:y;
    case 14:{
        const std::int64_t d=std::int64_t(y)-std::int64_t(a);
        return sat(std::int64_t(a)+arshift31(std::int64_t(x)*d));
    }
    default:return 0;
    }
}
static std::int32_t run(std::uint8_t op,std::int32_t a,std::int32_t x,std::int32_t y){
    Program p; p.gprInit={{0x8000,a},{0x8001,x},{0x8002,y}};
    p.code.push_back({op,0x8000,0x8001,0x8002,0x8003});
    Engine e;e.load(p);e.runSample();return e.get(0x8003);
}
static std::uint32_t lcg(std::uint32_t& s){s=s*1664525u+1013904223u;return s;}
int main(){
    const std::uint8_t ops[]={0,1,4,5,6,10,11,14};
    const std::int32_t edges[]={0,1,-1,2,-2,INT32_MAX,INT32_MIN,0x40000000,-0x40000000,0x7fff0000,(std::int32_t)0x80010000u};
    std::uint64_t checked=0;
    for(auto op:ops){
        for(auto a:edges)for(auto x:edges)for(auto y:edges){
            const auto got=run(op,a,x,y),want=ref(op,a,x,y);++checked;
            if(got!=want){std::printf("edge mismatch op=%u a=%08x x=%08x y=%08x got=%08x want=%08x\n",op,(unsigned)a,(unsigned)x,(unsigned)y,(unsigned)got,(unsigned)want);return 2;}
        }
        std::uint32_t seed=0x80100000u+op;
        for(unsigned i=0;i<4096;++i){
            const auto a=static_cast<std::int32_t>(lcg(seed));
            const auto x=static_cast<std::int32_t>(lcg(seed));
            const auto y=static_cast<std::int32_t>(lcg(seed));
            const auto got=run(op,a,x,y),want=ref(op,a,x,y);++checked;
            if(got!=want){std::printf("random mismatch op=%u i=%u a=%08x x=%08x y=%08x got=%08x want=%08x\n",op,i,(unsigned)a,(unsigned)x,(unsigned)y,(unsigned)got,(unsigned)want);return 3;}
        }
    }
    std::printf("michgz remaining-op sweep PASS vectors=%llu\n",(unsigned long long)checked);
    return 0;
}
