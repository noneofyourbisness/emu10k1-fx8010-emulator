#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;

int main(){
    XtramServiceFifo q; q.reset();
    if(q.tankCacheLocked() || q.mappingGeneration()!=0 || q.cancelledCount()!=0 || q.lockSuppressedCount()!=0) return 1;
    if(XtramServiceFifo::samplesForTcbsCode(0)!=8192u || XtramServiceFifo::samplesForTcbsCode(7)!=1048576u) return 2;

    // Old Creative/Linux initialization installs a TCB/TCBS mapping, then
    // releases LOCKTANKCACHE.  Requests are tagged with that mapping epoch.
    q.programTankMapping(0x01000000u,3u,true);
    const auto g1=q.mappingGeneration();
    if(g1!=1 || !q.tankMappingValid() || q.tankBase()!=0x01000000u || q.tankSizeCode()!=3u) return 3;
    q.configure(0,true,false,0x100u); q.configure(1,true,true,0x101u);
    auto s=q.select(0);
    if(s.enqueued!=2 || q.depth()!=2 || !q.front() || q.front()->mappingGeneration!=g1) return 4;

    // HCFG_LOCKTANKCACHE is documented as cancelling bus-master accesses.
    // The driver frees the old coherent DMA immediately after this barrier, so
    // no queued request from the old mapping is allowed to survive it.
    q.setTankCacheLocked(true);
    if(!q.tankCacheLocked() || q.depth()!=0 || q.cancelledCount()!=2) return 5;
    XtramServiceRequest dead{}; if(q.completeOne(&dead)) return 6;

    // Tank sequencer selection can continue conceptually while bus-master
    // requests are locked, but no request is admitted or counted as FIFO drop.
    auto blocked=q.select(16);
    if(blocked.enqueued || blocked.dropped || blocked.blockedByTankLock!=2 || q.depth()!=0) return 7;
    if(!blocked.decisions[0].blockedByTankLock || !blocked.decisions[1].blockedByTankLock || q.lockSuppressedCount()!=2) return 8;

    // Rebind to new TCB/TCBS while locked. Cache register configuration remains;
    // only external DMA mapping generation changes.
    q.programTankMapping(0x02000000u,4u,true);
    const auto g2=q.mappingGeneration();
    if(g2!=g1+1 || q.tankBase()!=0x02000000u || q.tankSizeCode()!=4u) return 9;
    q.setTankCacheLocked(false);
    auto fresh=q.select(32);
    if(fresh.enqueued!=2 || q.depth()!=2 || !q.front() || q.front()->mappingGeneration!=g2) return 10;
    if(q.front()->mappingGeneration==g1) return 11;

    // Reprogramming the same tuple is not a new epoch; invalidating mapping is.
    q.programTankMapping(0x02000000u,4u,true); if(q.mappingGeneration()!=g2) return 12;
    q.setTankCacheLocked(true); q.programTankMapping(0u,0u,false);
    if(q.tankMappingValid() || q.mappingGeneration()!=g2+1 || q.cancelledCount()!=4) return 13;

    std::printf("xtram_tankcache_lock PASS: LOCK cancels queued busmaster work, rebind advances mapping epoch, locked selections suppressed; TCBS 8K..1M samples\n");
    return 0;
}
