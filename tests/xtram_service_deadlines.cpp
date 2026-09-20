#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;

static void activateAll(XtramServiceFifo& q){
    for(unsigned i=0;i<32;++i) q.configure(i,true,(i&1u)!=0u,0x300u+i);
}

int main(){
    if(XtramServiceFifo::patentLatencySlackSamples()!=2u) return 1;
    if(XtramServiceFifo::minimumDroppedHeadroomGapSamples()!=14u) return 2;

    XtramServiceFifo q; q.reset();
    q.configure(0,true,false,0x100u); q.configure(1,true,true,0x101u);
    auto s=q.select(0);
    if(s.enqueued!=2 || s.dropped) return 3;
    for(unsigned i=0;i<2;++i){
        const auto& d=s.decisions[i];
        if(!d.active || !d.admitted || d.dropped || d.selectedSample!=0 || d.headroomDeadlineSample!=2 || d.retrySelectionSample!=16) return 4;
    }
    if(s.decisions[0].write || s.decisions[0].burstStart!=0x0eeu) return 5; // even read: -18
    if(!s.decisions[1].write || s.decisions[1].burstStart!=0x102u) return 6; // odd write: +1

    XtramServiceCompletion c{};
    if(!q.completeOneAt(2,&c) || c.request.cache!=0 || c.ageSamples!=2 || c.beyondHeadroom || c.beyondHeadroomSamples) return 7;
    if(!q.completeOneAt(3,&c) || c.request.cache!=1 || c.ageSamples!=3 || !c.beyondHeadroom || c.beyondHeadroomSamples!=1) return 8;

    // Filling four slots and selecting the next pair must return identities for
    // both dropped requests, each with a deadline at s+2 and retry at s+16.
    q.reset(); activateAll(q); q.select(0); q.select(1);
    auto drop=q.select(2);
    if(drop.enqueued || drop.dropped!=2) return 9;
    for(unsigned i=0;i<2;++i){
        const auto& d=drop.decisions[i];
        if(!d.active || d.admitted || !d.dropped || d.selectedSample!=2 || d.headroomDeadlineSample!=4 || d.retrySelectionSample!=18) return 10;
        if(d.retrySelectionSample-d.headroomDeadlineSample!=XtramServiceFifo::minimumDroppedHeadroomGapSamples()) return 11;
    }

    // With exactly one FIFO slot free the count is exact but identity still
    // depends on unresolved intra-pair ordering. Verify both research envelopes.
    XtramServiceRequest tmp{}; q.reset(); activateAll(q); q.select(0); q.select(1); q.completeOne(&tmp);
    auto lo=q.select(2,XtramPairOrder::LowerThenUpper);
    if(!lo.decisions[0].admitted || !lo.decisions[1].dropped) return 12;
    XtramServiceFifo q2; q2.reset(); activateAll(q2); q2.select(0); q2.select(1); q2.completeOne(&tmp);
    auto hi=q2.select(2,XtramPairOrder::UpperThenLower);
    if(!hi.decisions[1].admitted || !hi.decisions[0].dropped) return 13;

    // Timestamped batch completion reports late requests without pretending to
    // know the audible cache-underflow/overflow sample value.
    XtramServiceFifo q3; q3.reset(); q3.configure(0,true,false,0x100); q3.configure(1,true,false,0x101); q3.select(0);
    unsigned late=0; if(q3.completeAt(3,2,&late)!=2 || late!=2) return 14;

    std::printf("xtram_service_deadlines PASS: request deadline=+2, retry=+16, dropped minimum uncovered gap=14, pair-winner ambiguity preserved\n");
    return 0;
}
