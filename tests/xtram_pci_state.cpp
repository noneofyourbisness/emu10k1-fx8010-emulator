#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;

int main(){
    XtramServiceFifo q; q.reset();
    // One read + one write cache request at sample 0.
    q.configure(0,true,false,0x100u);
    q.configure(1,true,true, 0x101u);
    auto sel=q.select(0);
    if(sel.enqueued!=2u || q.depth()!=2u) return 1;

    auto t0=q.timingState(0);
    if(t0.depth!=2u || t0.oldestAge!=0u || t0.withinLatencyBudget!=2u || t0.atLatencyLimit || t0.beyondLatencyBudget) return 2;
    auto t1=q.timingState(1);
    if(t1.oldestAge!=1u || t1.withinLatencyBudget!=2u || t1.beyondLatencyBudget) return 3;
    auto t2=q.timingState(2);
    if(t2.oldestAge!=2u || t2.withinLatencyBudget!=2u || t2.atLatencyLimit!=2u || t2.beyondLatencyBudget) return 4;
    // The patent says M-B=2 samples of latency tolerance. Age 3 is therefore
    // past the source-backed cache headroom, though exact audible failure phase
    // is intentionally not invented by the emulator.
    auto t3=q.timingState(3);
    if(t3.oldestAge!=3u || t3.withinLatencyBudget || t3.atLatencyLimit || t3.beyondLatencyBudget!=2u) return 5;

    XtramServiceRequest done{};
    if(!q.completeOne(&done) || done.cache!=0u) return 6;
    auto ta=q.timingState(3);
    if(ta.depth!=1u || ta.beyondLatencyBudget!=1u) return 7;

    // Strictly-later service opportunity is exactly +16 samples for either
    // member of the pair, including across the 4-bit counter wrap.
    if(XtramServiceFifo::nextSelectionAfter(0,0)!=16u) return 8;
    if(XtramServiceFifo::nextSelectionAfter(1,0)!=16u) return 9;
    if(XtramServiceFifo::nextSelectionAfter(30,15)!=31u) return 10;
    if(XtramServiceFifo::nextSelectionAfter(31,15)!=31u) return 11;
    if(XtramServiceFifo::nextSelectionAfter(4,2)!=18u) return 12;

    std::printf("depth0=%u age_limit=%u overdue_at_3=%u retry0=%llu retry4=%llu\n",
        t0.depth,t2.atLatencyLimit,t3.beyondLatencyBudget,
        (unsigned long long)XtramServiceFifo::nextSelectionAfter(0,0),
        (unsigned long long)XtramServiceFifo::nextSelectionAfter(4,2));
    return 0;
}
