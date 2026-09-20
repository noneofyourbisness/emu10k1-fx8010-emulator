#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using namespace fx8010;

static std::int32_t run(std::int32_t x) {
    Program p; p.name="macintw-test";
    p.code.push_back({5,0x0040,0x8000,0x0055,0x8001});
    Engine e; e.load(p); e.set(0x8000,x); e.runSample(); return e.get(0x8001);
}

int main() {
    const auto below=run(1024);       // 1024 * 0x00100000 = 0x40000000
    const auto boundary=run(2048);    // 0x80000000 -> wraps to 0 in 31 bits
    const auto above=run(3072);       // 0xc0000000 -> 0x40000000 in 31 bits

    Program negp; negp.gprInit={{0x8000,-1},{0x8001,1}};
    negp.code.push_back({5,0x0040,0x8000,0x8001,0x8002});
    Engine ne; ne.load(negp); ne.runSample();
    const auto neg=ne.get(0x8002);
    const auto negcc=static_cast<std::uint32_t>(ne.get(0x57));
    std::printf("below=%08x boundary=%08x above=%08x negative=%08x negcc=%02x\n",
        static_cast<unsigned>(below),static_cast<unsigned>(boundary),static_cast<unsigned>(above),
        static_cast<unsigned>(neg),static_cast<unsigned>(negcc));
    return (below==0x40000000 && boundary==0 && above==0x40000000 &&
            neg==0x7fffffff && (negcc&0x10u)!=0u)?0:2;
}
