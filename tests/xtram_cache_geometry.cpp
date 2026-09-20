#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using fx8010::Engine;
int main(){
    if(Engine::xtramPciBurstSamples()!=16u || Engine::xtramDelayCacheSamples()!=18u ||
       Engine::xtramDelayCacheCount()!=32u || Engine::xtramCacheServicePeriodSamples()!=16u ||
       Engine::xtramCachesSelectedPerSample()!=2u || Engine::xtramServiceFifoDepth()!=4u){
        std::puts("cache geometry constant mismatch"); return 1;
    }
    bool seen[32]{};
    for(unsigned phase=0;phase<16;++phase){
        const auto p=Engine::xtramCachesSelected(phase);
        if(p[0]!=phase*2u || p[1]!=phase*2u+1u){
            std::printf("service pair mismatch phase=%u got=%u/%u\n",phase,p[0],p[1]); return 2;
        }
        seen[p[0]]=seen[p[1]]=true;
        const auto again=Engine::xtramCachesSelected(phase+16u);
        if(again!=p){ std::puts("service period is not 16 samples"); return 3; }
    }
    for(bool x:seen) if(!x){std::puts("not all 32 caches selected in one service epoch");return 4;}
    for(unsigned a=0;a<64;++a){
        const int r=Engine::xtramReadBurstStartAdjustment(a);
        const int w=Engine::xtramWriteBurstStartAdjustment(a);
        const int expectedR=(a&1u)?-17:-18;
        const int expectedW=(a&1u)?1:0;
        if(r!=expectedR || w!=expectedW){
            std::printf("alignment mismatch a=%u r=%d/%d w=%d/%d\n",a,r,expectedR,w,expectedW);return 5;
        }
        // Both corrected starts must be even on a 32-bit PCI bus carrying 16-bit samples.
        const int readStart=int(a)+r, writeStart=int(a)+w;
        if((readStart&1)!=0 || (writeStart&1)!=0){std::puts("burst start not even");return 6;}
    }
    // A full four-entry request FIFO spans four 16-sample cache revisit periods.
    // This is the structural 64-sample cadence noted in the research report; it
    // is not by itself proof that any historical audible glitch has this cause.
    const unsigned fourServicePeriods=Engine::xtramServiceFifoDepth()*Engine::xtramCacheServicePeriodSamples();
    if(fourServicePeriods!=64u){std::puts("unexpected FIFO/service product");return 7;}
    std::printf("xtram_cache_geometry PASS: burst=16 cache=18 pair-service=2/32 period=16 fifo=4 structural-span=%u\n",fourServicePeriods);
    return 0;
}
