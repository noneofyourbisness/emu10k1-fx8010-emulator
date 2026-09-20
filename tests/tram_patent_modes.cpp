#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    struct V{bool r,w; TramAccessMode m; const char* n;};
    const V v[]={{false,false,TramAccessMode::Off,"off"},
                 {true,false,TramAccessMode::Read,"read"},
                 {false,true,TramAccessMode::Write,"write"},
                 {true,true,TramAccessMode::ReadSumWrite,"rsaw"}};
    for(const auto&t:v){
        TramHardwareControl c{}; c.address=0x54321; c.read=t.r; c.write=t.w;
        auto word=Engine::encodeTramHardwareControl(c);
        auto d=Engine::decodeTramHardwareControl(word);
        if(d.read!=t.r || d.write!=t.w || d.address!=0x54321u || Engine::tramAccessMode(d)!=t.m) return 2;
        std::printf("%s word=%08x\n",t.n,word);
    }
    return 0;
}
