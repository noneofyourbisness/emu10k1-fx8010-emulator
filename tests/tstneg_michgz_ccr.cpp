#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
static unsigned run(std::int32_t a,std::int32_t x,std::int32_t y,std::int32_t &r){
 Program p;p.gprInit={{0x8000,a},{0x8001,x},{0x8002,y}};p.code.push_back({9,0x8000,0x8001,0x8002,0x8003});Engine e;e.load(p);e.runSample();r=e.get(0x8003);return static_cast<unsigned>(e.get(0x57));
}
int main(){
 std::int32_t r1=0,r2=0,r3=0;
 const auto c1=run(5,0x12345678,4,r1); // A>=Y: X, no comparison borrow
 const auto c2=run(3,0x12345678,4,r2); // A<Y: ~X, B set
 const auto c3=run(3,static_cast<std::int32_t>(0xffffffffu),4,r3); // ~X=0 -> Z+B
 std::printf("r1=%08x c1=%02x r2=%08x c2=%02x r3=%08x c3=%02x\n",(unsigned)r1,c1,(unsigned)r2,c2,(unsigned)r3,c3);
 return ((unsigned)r1==0x12345678u && c1==0u && (unsigned)r2==0xedcba987u && c2==0x06u && r3==0 && c3==0x0au)?0:2;
}
