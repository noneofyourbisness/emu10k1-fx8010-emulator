#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <limits>
using namespace fx8010;
static std::uint32_t run(std::int32_t a,std::uint8_t op=0,std::int32_t x=0,std::int32_t y=0){
 Program p; p.gprInit={{0x8000,a},{0x8001,x},{0x8002,y}}; p.code.push_back({op,0x8000,0x8001,0x8002,0x8003});
 Engine e;e.load(p);e.runSample();return static_cast<std::uint32_t>(e.get(0x57));
}
int main(){
 const auto n=run(0x20000000);         // top bits 00 => N
 const auto notn=run(0x40000000);      // top bits 01 => not N
 const auto minus=run(-0x20000000);    // M plus N (top bits 11)
 const auto zero=run(0);
 const auto borrow=run(std::numeric_limits<std::int32_t>::min(),1,0x7fffffff,0x7fffffff);
 std::printf("N=%#x notN=%#x minus=%#x zero=%#x borrow=%#x\n",n,notn,minus,zero,borrow);
 if((n&1u)==0 || (notn&1u)!=0) return 2;
 if((minus&4u)==0 || (minus&1u)==0) return 3;
 if((zero&8u)==0) return 4;
 if((borrow&2u)==0 || (borrow&0x10u)==0) return 5;
 return 0;
}
