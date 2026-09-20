#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using namespace fx8010;
int main(){
    // First MAC leaves non-zero low product bits in the 67-bit accumulator.
    // Public FX8010 assembler docs specify that MACINT* with ACCU as A reads
    // the least-significant 32 accumulator bits, unlike fractional MAC*.
    Program p;
    p.gprInit={{0x8000,0x12345678},{0x8001,0x23456789},{0x8002,1},{0x8003,0}};
    p.code.push_back({0,0x0040,0x8000,0x8001,0x8004});
    p.code.push_back({4,0x0056,0x8002,0x8003,0x8005});
    Engine e; e.load(p); e.runSample();
    const std::int64_t product=std::int64_t(0x12345678)*std::int64_t(0x23456789);
    const auto expected=static_cast<std::int32_t>(static_cast<std::uint32_t>(product));
    const auto got=e.get(0x8005);
    std::printf("macint_accu_lowword got=%08x expected=%08x\n",static_cast<unsigned>(got),static_cast<unsigned>(expected));
    return got==expected?0:2;
}
