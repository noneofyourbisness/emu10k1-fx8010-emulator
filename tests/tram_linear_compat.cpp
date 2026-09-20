#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>

int main() {
    fx8010::Program p;
    p.name = "linear TRAM compatibility self-test";
    p.linearTramCompatibility = true;
    // Write relative address 0; read relative address 1. After DBAC decrements,
    // the second sample's read resolves to the first sample's write location.
    p.tram = {
        {0x8000, true,  0, 0, 0},
        {0x8002, false, 1, 0, 0},
    };

    fx8010::Engine e;
    e.load(p, 32);
    const std::int32_t v = static_cast<std::int32_t>(0x12345678u);
    e.set(0x8000, v);
    e.runSample();
    e.runSample();

    const auto got = e.get(0x8002);
    if(got != v) {
        std::fprintf(stderr, "linear TRAM compatibility mismatch: got=%08x expected=%08x\n",
                     static_cast<unsigned>(got), static_cast<unsigned>(v));
        return 1;
    }
    return 0;
}
