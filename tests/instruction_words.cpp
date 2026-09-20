#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
 Instruction i{4,0x040,0x003,0x041,0x033};
 const auto w=Engine::encodeInstructionWords(i);
 const std::uint32_t elo=((0x003u&0x3ffu)<<10)|(0x041u&0x3ffu);
 const std::uint32_t ehi=((4u&0xfu)<<20)|((0x033u&0x3ffu)<<10)|(0x040u&0x3ffu);
 const auto d=Engine::decodeInstructionWords(w[0],w[1]);
 std::printf("lo=%08x hi=%08x\n",w[0],w[1]);
 return (w[0]==elo&&w[1]==ehi&&d.op==i.op&&d.a==i.a&&d.x==i.x&&d.y==i.y&&d.r==i.r)?0:2;
}
