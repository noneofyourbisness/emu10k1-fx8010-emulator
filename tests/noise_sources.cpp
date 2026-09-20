#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
using namespace fx8010;

int main(){
    Program p;
    // Copy the two hardware noise sources into ordinary GPRs every sample.
    p.code.push_back({0,0x58,0x40,0x40,0x8000});
    p.code.push_back({0,0x59,0x40,0x40,0x8001});
    Engine e; e.load(p);
    std::array<std::int32_t,8> a{},b{};
    bool differ=false, movedA=false, movedB=false;
    for(std::size_t i=0;i<a.size();++i){
        e.runSample(); a[i]=e.get(0x8000); b[i]=e.get(0x8001);
        if(a[i] < -0x40000000 || a[i] > 0x3fffffff ||
           b[i] < -0x40000000 || b[i] > 0x3fffffff) return 2;
        differ |= a[i]!=b[i];
        if(i){ movedA |= a[i]!=a[i-1]; movedB |= b[i]!=b[i-1]; }
    }
    // Direct host-style writes must not replace the read-only sources.
    const auto n0=e.get(0x58), n1=e.get(0x59);
    e.set(0x58,0x12345678); e.set(0x59,0x23456789);
    if(e.get(0x58)!=n0 || e.get(0x59)!=n1) return 3;
    // Reset is deterministic in the software emulator. This is a testing aid,
    // not a claim about the unknown EMU10K1 silicon noise sequence.
    e.reset(); e.runSample();
    const bool deterministic=e.get(0x8000)==a[0] && e.get(0x8001)==b[0];
    std::printf("noise0_first=%#x noise1_first=%#x independent=%d moving=%d/%d deterministic=%d\n",
        (unsigned)a[0],(unsigned)b[0],differ?1:0,movedA?1:0,movedB?1:0,deterministic?1:0);
    return (differ && movedA && movedB && deterministic) ? 0 : 4;
}
