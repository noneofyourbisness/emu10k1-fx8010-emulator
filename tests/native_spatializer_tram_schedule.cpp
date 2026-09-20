#include "fx8010_engine.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
using namespace fx8010;

struct Expect { const char *name; std::uint32_t itram; std::vector<std::uint32_t> delays; unsigned alignedW, alignedR; };

static int audit(const char *path,const Expect&e){
    Program p; std::string err;
    if(!RifxLoader::loadFile(path,p,&err)){std::fprintf(stderr,"%s: %s\n",path,err.c_str());return 1;}
    if(p.name!=e.name || p.itramSize!=e.itram){std::fprintf(stderr,"%s: identity/size mismatch %s %u\n",path,p.name.c_str(),p.itramSize);return 2;}
    std::vector<std::uint32_t>d; unsigned aw=0,ar=0,w=0,r=0;
    for(unsigned slot=0;slot<p.tram.size();++slot){
        const auto&t=p.tram[slot];
        if(t.external){std::fprintf(stderr,"%s: expected pure ITRAM\n",p.name.c_str());return 3;}
        const unsigned pc=t.patchSite&0x0fffu;
        const bool align=Engine::tramAlignFlag(false,t.write,pc,slot);
        if(t.write){++w;aw+=align?1u:0u;}
        else {++r;ar+=align?1u:0u;d.push_back(t.auxiliary);}
    }
    std::sort(d.begin(),d.end()); auto want=e.delays; std::sort(want.begin(),want.end());
    if(d!=want || aw!=e.alignedW || ar!=e.alignedR){
        std::fprintf(stderr,"%s: delay/alignment mismatch w=%u aw=%u r=%u ar=%u\n",p.name.c_str(),w,aw,r,ar);return 4;
    }
    std::printf("%s: itram=%u delays=",p.name.c_str(),p.itramSize);
    for(auto x:d) std::printf("%u,",x);
    std::printf(" standalone_ALIGN(w/r)=%u/%u of %u/%u\n",aw,ar,w,r);
    return 0;
}

int main(int argc,char **argv){
    if(argc!=5){std::fprintf(stderr,"need Headphone SB2 SB4 SurroundEncoder fixtures\n");return 2;}
    const Expect ex[]={
      {"2HeadphoneSpatializer",116,{12,12,12,12,31,31},6,0},
      {"SB2SpeakerSpatializer",130,{8,8,12,12,31,31},6,0},
      {"SB4SpeakerSpatializer",104,{8,8,31,31},4,0},
      {"Surround Encoder",24,{2,2,2,2,2,2},5,6}
    };
    for(int i=0;i<4;++i){const int rc=audit(argv[i+1],ex[i]);if(rc)return 10+i*10+rc;}
    // The three spatializer networks compile to a strikingly regular schedule:
    // all writes request ALIGN under standalone slot allocation while reads do
    // not. The surround encoder intentionally has the opposite read phase.
    // This strongly supports ALIGN as loader-level phase compensation, while
    // the programmer-visible delay relation remains write-base + N samples.
    return 0;
}
