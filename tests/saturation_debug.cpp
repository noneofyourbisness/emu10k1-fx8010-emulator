#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    Program p;
    p.gprInit={{0x8000,0x40000000}};
    p.code.push_back({0,0x8000,0x40,0x40,0x8001}); // pc0 no saturation
    p.code.push_back({0,0x4f,0x4f,0x4f,0x8002});   // pc1 saturating MAC0
    p.code.push_back({2,0x4f,0x4f,0x4f,0x8003});   // pc2 wrap-only MACW
    Engine e;e.load(p);e.runSample();
    const bool latched=e.saturationOccurred() && e.saturationAddress()==1;
    e.clearSaturation();
    const bool cleared=!e.saturationOccurred() && e.saturationAddress()==0;

    Program q; q.code.push_back({2,0x4f,0x4f,0x4f,0x8000});
    Engine w;w.load(q);w.runSample();
    const bool wrapNotHard=!w.saturationOccurred();
    std::printf("latched=%d addr=%u cleared=%d wrap_not_hard=%d\n",latched,e.saturationAddress(),cleared,wrapNotHard);
    return (latched&&cleared&&wrapNotHard)?0:2;
}
