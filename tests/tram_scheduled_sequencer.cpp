#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <vector>
using namespace fx8010;

static Program makeProgram(bool scheduled,bool external){
    Program p;
    p.linearTramCompatibility=true; // isolate timing from the independently-tested TRAM codec
    p.scheduledTramSequencer=scheduled;
    constexpr std::uint16_t in=0x9000, addr=0x9001, out=0x9002;
    constexpr std::uint16_t rd=0x8000, wr=0x8010;
    const unsigned addrPc=external?127u:1u;
    const unsigned readPc=external?129u:3u;
    const unsigned writeSlot=1u;
    const unsigned codeCount=external?130u:4u;
    p.code.reserve(codeCount);
    for(unsigned i=0;i<codeCount;++i)p.code.push_back({7,0x40,0x40,0x40,0x9100});
    p.code[0]={7,in,0x40,0x40,wr};
    p.code[addrPc]={7,addr,0x40,0x40,std::uint16_t(rd+1)};
    p.code[readPc]={7,rd,0x40,0x40,out};
    // Read slot zero becomes visible before PC 2 (ITRAM) / PC 128 (XTRAM),
    // after the address update and before the consumer. Write slot one is
    // serviced after the producer. Neither access needs ALIGN in this layout.
    p.tram.push_back({rd,false,2,2,std::uint16_t(readPc),0,external,0,false});
    p.tram.push_back({wr,true,0,0,0,0,external,std::uint16_t(writeSlot),false});
    return p;
}

static int audit(bool external){
    Engine scheduled,legacy;
    scheduled.load(makeProgram(true,external));
    legacy.load(makeProgram(false,external));
    std::vector<std::int32_t> history;
    bool observedLegacyLag=false;
    for(unsigned n=0;n<16;++n){
        const std::int32_t input=1000+std::int32_t(n);
        const unsigned delay=(n&1u)?3u:2u;
        history.push_back(input);
        for(Engine* e:{&scheduled,&legacy}){
            e->set(0x9000,input);
            e->set(0x9001,std::int32_t(delay<<11));
            e->runSample();
        }
        const std::int32_t expected=n>=delay?history[n-delay]:0;
        if(scheduled.get(0x9002)!=expected){
            std::fprintf(stderr,"%s scheduled phase mismatch n=%u d=%u got=%d expected=%d\n",
                         external?"XTRAM":"ITRAM",n,delay,scheduled.get(0x9002),expected);
            return 1;
        }
        if(n>=3u && legacy.get(0x9002)!=expected) observedLegacyLag=true;
    }
    if(!observedLegacyLag){
        std::fprintf(stderr,"%s probe failed to distinguish old begin/end model\n",external?"XTRAM":"ITRAM");
        return 2;
    }
    return 0;
}

int main(){
    if(int rc=audit(false))return 10+rc;
    if(int rc=audit(true))return 20+rc;
    std::puts("tram_scheduled_sequencer PASS: dynamic address writes are observed at the recovered ITRAM/XTRAM service boundaries; old frame-begin reads lag by one frame");
    return 0;
}
