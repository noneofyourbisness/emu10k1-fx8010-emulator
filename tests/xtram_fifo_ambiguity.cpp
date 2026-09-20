#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
static void activateAll(XtramServiceFifo& q){ for(unsigned i=0;i<32;++i) q.configure(i,true,(i&1u)!=0u,0x100u+i); }
int main(){
    // Patent geometry constants: M=18, B=16 => two full sample periods of slack.
    if(XtramServiceFifo::transferSamples()!=16u || XtramServiceFifo::cacheSamples()!=18u ||
       XtramServiceFifo::servicePeriodSamples()!=16u || XtramServiceFifo::patentLatencySlackSamples()!=2u ||
       XtramServiceFifo::fifoCapacity()!=4u) return 1;

    // With no completions, an empty four-entry FIFO takes two fully-active cache-pair
    // selections to fill; the third pair is the first one forced to drop.
    XtramServiceFifo q; q.reset(); activateAll(q);
    auto a0=q.admissionEnvelope(0); if(a0.active!=2||a0.freeEntries!=4||a0.enqueued!=2||a0.dropped) return 2;
    q.select(0); if(q.depth()!=2||q.oldestAgeSamples(0)!=0u) return 3;
    q.select(1); if(q.depth()!=4||q.oldestAgeSamples(1)!=1u) return 4;
    auto a2=q.admissionEnvelope(2); if(a2.active!=2||a2.freeEntries!=0||a2.enqueued||a2.dropped!=2||a2.identityAmbiguous) return 5;
    auto s2=q.select(2); if(s2.enqueued||s2.dropped!=2) return 6;

    // A dropped pair is selected again exactly one patented service period later:
    // cache 4/5 at sample 2 -> cache 4/5 at sample 18. There is no source-backed
    // inherent 64-sample retry period from FIFO-depth*service-period multiplication.
    auto p18=Engine::xtramCachesSelected(18); if(p18[0]!=4u||p18[1]!=5u) return 7;
    auto p34=Engine::xtramCachesSelected(34); if(p34!=p18) return 8;
    auto p50=Engine::xtramCachesSelected(50); if(p50!=p18) return 11;
    auto p66=Engine::xtramCachesSelected(66); if(p66!=p18) return 12;

    // The sole admission ambiguity in the patent text: two active caches with one
    // free FIFO entry. Count is fixed (1 accepted, 1 dropped); identity/order is not.
    q.reset(); activateAll(q); q.select(0); q.select(1); XtramServiceRequest r{}; q.completeOne(&r); // depth=3
    auto env=q.admissionEnvelope(2); if(env.active!=2||env.freeEntries!=1||env.enqueued!=1||env.dropped!=1||!env.identityAmbiguous) return 11;
    auto lower=q.select(2,XtramPairOrder::LowerThenUpper); if(lower.enqueued!=1||lower.dropped!=1) return 12;
    // Recreate the same state and take the opposite research tie-break.
    XtramServiceFifo q2; q2.reset(); activateAll(q2); q2.select(0); q2.select(1); q2.completeOne(&r);
    auto upper=q2.select(2,XtramPairOrder::UpperThenLower); if(upper.enqueued!=1||upper.dropped!=1) return 13;
    // Existing queued entries are 1,2,3. The fourth entry reveals which pair member won.
    q.complete(3); q2.complete(3);
    if(!q.completeOne(&r)||r.cache!=4u) return 14;
    if(!q2.completeOne(&r)||r.cache!=5u) return 15;

    // If the service path keeps pace at two completions/sample, even all 32 caches
    // active do not create any deterministic queue overflow or periodic glitch.
    XtramServiceFifo q3; q3.reset(); activateAll(q3);
    for(unsigned smp=0;smp<512;++smp){ auto e=q3.admissionEnvelope(smp); if(e.identityAmbiguous) return 16; auto st=q3.select(smp); if(st.dropped) return 17; if(q3.complete(2)!=2u) return 18; }
    if(q3.acceptedCount()!=1024u||q3.completedCount()!=1024u||q3.droppedCount()!=0u) return 19;

    std::printf("xtram_fifo_ambiguity PASS: M-B=2 slack, first drop after two unretired active pairs, retry=16 samples, one-free pair identity explicitly unresolved, no inherent 64-sample cycle\n");
    return 0;
}
