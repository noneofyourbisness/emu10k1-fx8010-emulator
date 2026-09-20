#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;

int main(){
    // Source-exact topology from Creative US 6,032,235: old memory is read,
    // data-buffer value is added, and the result is written to the same address.
    auto a=Engine::tramInternalRsaw20(100,23,false);
    if(a.memoryBefore!=100 || a.dataBuffer!=23 || a.mathematicalSum!=123 || a.overflow) return 1;
    if(a.wrap20!=123 || a.saturate20!=123 || a.effectiveWriteIfWrap!=123 || a.effectiveWriteIfSaturate!=123) return 2;

    auto b=Engine::tramInternalRsaw20(-100,23,false);
    if(b.mathematicalSum!=-77 || b.wrap20!=-77 || b.saturate20!=-77 || b.overflow) return 3;

    // Patent clearing mux: while RSAW CLR is active, zero is written instead of
    // the data-buffer/sum path. The arithmetic can be characterized separately.
    auto c=Engine::tramInternalRsaw20(100,23,true);
    if(!c.clearForcedZero || c.effectiveWriteIfWrap!=0 || c.effectiveWriteIfSaturate!=0) return 4;

    // The one remaining arithmetic ambiguity is deliberately surfaced rather
    // than hidden: +20-bit overflow has distinct wrap and saturate candidates.
    auto p=Engine::tramInternalRsaw20(524287,1,false);
    if(!p.overflow || p.mathematicalSum!=524288 || p.wrap20!=-524288 || p.saturate20!=524287) return 5;
    if(p.rawWordIfWrap!=Engine::tramRawWordFromTank20(-524288) || p.rawWordIfSaturate!=Engine::tramRawWordFromTank20(524287) || p.rawWordIfWrap==p.rawWordIfSaturate) return 8;
    auto n=Engine::tramInternalRsaw20(-524288,-1,false);
    if(!n.overflow || n.mathematicalSum!=-524289 || n.wrap20!=524287 || n.saturate20!=-524288) return 6;

    // Inputs presented in the tank domain are clipped to the representable
    // 20-bit range before RSAW, matching the rest of the physical tank model.
    auto cl=Engine::tramInternalRsaw20(900000,-900000,false);
    if(cl.memoryBefore!=524287 || cl.dataBuffer!=-524288 || cl.mathematicalSum!=-1 || cl.wrap20!=-1 || cl.saturate20!=-1) return 7;

    std::printf("tram_rsaw_exact PASS: old+buffer topology exact; CLR=zero; overflow remains explicit wrap/saturate envelope\n");
    return 0;
}
