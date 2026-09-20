#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;
int main(){
    XtramServiceFifo q; q.reset();
    // Activate every cache, alternating read/write, with parity-varying addresses.
    for(unsigned i=0;i<32;++i) q.configure(i,true,(i&1u)!=0u,0x100u+i);
    auto s0=q.select(0); if(s0.selected!=std::array<unsigned,2>{0,1}||s0.enqueued!=2||s0.dropped||q.depth()!=2) return 1;
    auto s1=q.select(1); if(s1.selected!=std::array<unsigned,2>{2,3}||s1.enqueued!=2||s1.dropped||q.depth()!=4) return 2;
    // Full FIFO: the next selected active pair is ignored exactly as the patent says.
    auto s2=q.select(2); if(s2.enqueued||s2.dropped!=2||q.depth()!=4||q.droppedCount()!=2) return 3;
    // FIFO completion is strict head order.
    XtramServiceRequest r{}; if(!q.completeOne(&r)||r.cache!=0||r.selectedSample!=0||q.depth()!=3) return 4;
    if(r.write || r.currentAddress!=0x100u || r.burstStart!=0x0eeu) return 5; // even read: -18
    // One free position: pair 6/7 has source-known 1 accept + 1 drop; this test chooses the lower-first research tie-break.
    auto s3=q.select(3,XtramPairOrder::LowerThenUpper); if(s3.enqueued!=1||s3.dropped!=1||q.depth()!=4) return 6;
    // Cache 4/5 were dropped at sample 2 and cannot be selected again before sample 18.
    for(unsigned smp=4;smp<18;++smp){ const auto p=Engine::xtramCachesSelected(smp); if(p[0]==4u||p[1]==4u||p[0]==5u||p[1]==5u) return 7; }
    const auto p18=Engine::xtramCachesSelected(18); if(p18!=std::array<unsigned,2>{4,5}) return 8;
    // Drain and verify queued FIFO order was 1,2,3,6.
    unsigned expect[]={1,2,3,6};
    for(unsigned e:expect){ if(!q.completeOne(&r)||r.cache!=e) return 9; }
    if(!q.empty()||q.completedCount()!=5u) return 10;
    // Inactive selections produce no requests.
    q.reset(); q.configure(0,true,false,0x101u); q.configure(1,false,false,0x102u);
    auto only=q.select(0); if(only.enqueued!=1||only.dropped||q.depth()!=1) return 11;
    if(!q.completeOne(&r)||r.cache!=0||r.currentAddress!=0x101u||r.burstStart!=0x0f0u) return 12; // odd read: -17
    // Write parity corrections: even +0, odd +1, both even starts.
    q.reset(); q.configure(0,true,true,0x200u); q.configure(1,true,true,0x201u); q.select(0);
    if(!q.completeOne(&r)||r.burstStart!=0x200u) return 13;
    if(!q.completeOne(&r)||r.burstStart!=0x202u) return 14;
    // A deterministic queue overflow is NOT inherent: if both requests complete each sample, no drops occur.
    q.reset(); for(unsigned i=0;i<32;++i) q.configure(i,true,(i&1u)!=0u,0x400u+i);
    for(unsigned smp=0;smp<256;++smp){ auto st=q.select(smp); if(st.dropped) return 15; if(q.complete(2)!=2u) return 16; }
    if(q.droppedCount()!=0||q.acceptedCount()!=512u||q.completedCount()!=512u) return 17;
    std::printf("xtram_fifo_controller PASS: exact pair selection, depth=4, drop-on-full, FIFO completion, 16-sample retry, parity starts; zero drops with prompt service\n");
    return 0;
}
