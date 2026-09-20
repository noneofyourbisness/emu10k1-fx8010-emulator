#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using namespace fx8010;
int main(){
  // Freak Shifter's exact control idiom: MACINT0 A + 4*(-1), then test B.
  Program p; p.gprInit={{0x8000,3},{0x8001,4}};
  p.code.push_back({4,0x8000,0x8001,0x50,0x8002}); // 3-4 => borrow, no saturation
  Engine e; e.load(p); e.runSample();
  auto cc=static_cast<std::uint32_t>(e.get(0x57));
  std::printf("r=%#x cc=%#x\n",static_cast<unsigned>(e.get(0x8002)),cc);
  if((cc&0x02u)==0) return 2;
  if((cc&0x10u)!=0) return 3;
  // No borrow when minuend is large enough.
  p.gprInit={{0x8000,5},{0x8001,4}}; e.load(p); e.runSample(); cc=static_cast<std::uint32_t>(e.get(0x57));
  if((cc&0x02u)!=0) return 4;
  return 0;
}
