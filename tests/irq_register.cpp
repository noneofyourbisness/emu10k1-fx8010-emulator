#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    Program p; p.name="irq-test";
    // ACC3 -> GPR_IRQ, matching the stock Linux PCM microcode's IRQ form.
    p.code.push_back({6,0x4e,0x40,0x40,0x5a}); // C_80000000 + 0 + 0
    Engine e; e.load(p);
    if(e.irqPending() || e.irqCount()!=0 || e.get(0x5a)!=0) return 1;
    e.runSample();
    if(!e.irqPending() || e.irqCount()!=1 || e.get(0x5a)!=0) return 2;
    if(!e.consumeIrq() || e.irqPending()) return 3;
    e.runSample();
    if(!e.irqPending() || e.irqCount()!=2) return 4;
    e.reset();
    if(e.irqPending() || e.irqCount()!=0) return 5;

    // A zero result to IRQ must not be promoted into a guessed interrupt.
    Program z; z.name="irq-zero"; z.code.push_back({6,0x40,0x40,0x40,0x5a});
    e.load(z); e.runSample();
    if(e.irqPending() || e.irqCount()!=0 || e.get(0x5a)!=0) return 6;
    std::puts("irq_register PASS");
    return 0;
}
