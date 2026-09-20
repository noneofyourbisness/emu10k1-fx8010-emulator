#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;

int main(){
    TramClearController c;
    c.reset();
    c.configure(7, TramClearKind::Read, 3);
    c.configure(8, TramClearKind::InternalRsaw, 5);

    // READ remains CLR for counts 0,1,2 and becomes valid exactly at equality.
    for(unsigned n=0;n<3;++n){
        c.setZeroedSamples(n);
        auto r=c.state(7);
        if(!r.clearActive || !r.readReturnsZero || !r.microcodeDataBufferWriteAllowed) return 1;
    }
    c.setZeroedSamples(3);
    auto r=c.state(7);
    if(r.clearActive || r.readReturnsZero || !r.microcodeDataBufferWriteAllowed) return 2;

    // RSAW clearing forces a zero memory write and forbids a DSP/microcode write
    // into the countdown-bearing data-buffer location until equality.
    for(unsigned n=0;n<5;++n){
        c.setZeroedSamples(n);
        auto s=c.state(8);
        if(!s.clearActive || !s.rsawForcesZeroWrite || s.microcodeDataBufferWriteAllowed) return 3;
    }
    c.setZeroedSamples(5);
    auto s=c.state(8);
    if(s.clearActive || s.rsawForcesZeroWrite || !s.microcodeDataBufferWriteAllowed) return 4;

    // Disabled/off/out-of-range slots have no clearing side effects.
    c.disable(8);
    if(c.state(8).clearActive || !c.state(8).microcodeDataBufferWriteAllowed) return 5;
    if(c.state(999).clearActive || !c.state(999).microcodeDataBufferWriteAllowed) return 6;

    // Saturating global counter cannot wrap and accidentally re-enter CLR.
    c.reset(0xffffffffu);
    c.advanceSample();
    if(c.zeroedSamples()!=0xffffffffu) return 7;

    std::puts("TRAM CLR controller exact read/RSAW transition PASS");
    return 0;
}
