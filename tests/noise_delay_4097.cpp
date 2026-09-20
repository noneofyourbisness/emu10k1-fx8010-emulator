#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <vector>
using namespace fx8010;
int main(){
    Program p;
    p.code.push_back({0,0x58,0x40,0x40,0x8000});
    p.code.push_back({0,0x59,0x40,0x40,0x8001});
    Engine e; e.load(p);
    std::vector<std::int32_t> n0(4104),n1(4104);
    for(std::size_t i=0;i<n0.size();++i){e.runSample();n0[i]=e.get(0x8000);n1[i]=e.get(0x8001);}
    for(std::size_t i=4097;i<n0.size();++i){
        if(n1[i]!=n0[i-4097]){
            std::fprintf(stderr,"delay mismatch at %zu: n1=%08x n0[-4097]=%08x\n",i,(unsigned)n1[i],(unsigned)n0[i-4097]);return 2;
        }
    }
    e.reset(); e.runSample();
    if(e.get(0x8000)!=n0[0] || e.get(0x8001)!=n1[0]) return 3;
    std::printf("NOISE1 follows the single NOISE0 placeholder stream at exactly 4097 samples; silicon polynomial/seed remain intentionally unspecified\n");
    return 0;
}
