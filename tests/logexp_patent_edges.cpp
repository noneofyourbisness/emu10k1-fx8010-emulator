#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
using namespace fx8010;
int main(){
    // Patent exponent-field partitions change at max-exponent 2/4/8/16.
    // Exercise both sides of every production-supported boundary. Golden data
    // for the detailed values remain in logexp_michgz_vectors; this test checks
    // the architectural sign-format and partition invariants described by the
    // patent without overriding real-card measurements.
    const std::array<unsigned,8> xs{{2,3,4,7,8,15,16,31}};
    const std::array<std::int32_t,8> pos{{0,1,0x1000,0x00ffffff,0x20000000,0x40000000,0x60000000,0x7fffffff}};
    const std::array<std::int32_t,5> neg{{-1,-0x1000,-0x01000000,-0x20000000,-0x40000000}};
    unsigned checks=0;
    for(unsigned x:xs){
        for(auto v:pos){
            auto y0=static_cast<std::uint32_t>(Engine::logEncode(v,x,0));
            auto y1=static_cast<std::uint32_t>(Engine::logEncode(v,x,1));
            auto y2=static_cast<std::uint32_t>(Engine::logEncode(v,x,2));
            auto y3=static_cast<std::uint32_t>(Engine::logEncode(v,x,3));
            if(y0!=y1 || y2!=y3 || y2!=~y0) return 2;
            checks+=3;
        }
        for(auto v:neg){
            auto y0=static_cast<std::uint32_t>(Engine::logEncode(v,x,0));
            auto y1=static_cast<std::uint32_t>(Engine::logEncode(v,x,1));
            auto y2=static_cast<std::uint32_t>(Engine::logEncode(v,x,2));
            auto y3=static_cast<std::uint32_t>(Engine::logEncode(v,x,3));
            if(y0!=y2 || y1!=y3 || y1!=~y0) return 3;
            checks+=3;
        }
    }
    // Explicit real-card golden values at every exponent-width transition.
    struct G{std::uint32_t v,x,y,z;};
    const G g[]={{0x7fffffffu,2,0,0x5fffffffu},{0x7fffffffu,3,0,0x7fffffffu},
                 {0x1fffffffu,4,3,0xd0000000u},{0x40000000u,7,0,0x70000000u},
                 {0x20000000u,8,0,0x38000000u},{0x00000002u,15,0,0x00001000u},
                 {0x00ffffffu,16,0,0x27fffff8u},{0x00000002u,31,0,0x08000000u}};
    for(const auto&t:g){if(static_cast<std::uint32_t>(Engine::logEncode(static_cast<std::int32_t>(t.v),t.x,t.y))!=t.z)return 4;++checks;}
    std::printf("patent_partition_and_sign_checks=%u\n",checks);
    return 0;
}
