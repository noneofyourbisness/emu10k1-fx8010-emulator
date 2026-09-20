#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using namespace fx8010;

struct SigStats { long double energy{}; long double delayed{}; int first{-1}; int last{-1}; unsigned nonzero{}; };

static std::vector<std::uint16_t> bindInputs(Program& p){
    std::vector<std::uint16_t> regs;
    for(std::size_t j=0;j<p.inputPatchSites.size();++j){
        const auto r=static_cast<std::uint16_t>(0x9000u+j);
        if(!patchSite(p.code,p.inputPatchSites[j],r)) return {};
        regs.push_back(r);
    }
    return regs;
}

static std::vector<std::vector<std::int32_t>> impulse(Program p,unsigned pin,unsigned frames=512){
    const auto ins=bindInputs(p);
    if(ins.size()!=p.inputPatchSites.size() || pin>=ins.size()) return {};
    Engine e; e.load(p);
    std::vector<std::vector<std::int32_t>> out(p.outputs.size(),std::vector<std::int32_t>(frames));
    for(unsigned n=0;n<frames;++n){
        for(unsigned j=0;j<ins.size();++j) e.set(ins[j],(n==0u && j==pin)?0x20000000:0);
        e.runSample();
        for(unsigned o=0;o<p.outputs.size();++o) out[o][n]=e.get(p.outputs[o].virtualReg);
    }
    return out;
}

static SigStats stats(const std::vector<std::int32_t>& v){
    SigStats s{};
    for(unsigned n=0;n<v.size();++n){
        const auto x=v[n];
        const long double f=static_cast<long double>(x)/2147483648.0L;
        s.energy+=f*f; if(n) s.delayed+=f*f;
        if(x){ if(s.first<0) s.first=static_cast<int>(n); s.last=static_cast<int>(n); ++s.nonzero; }
    }
    return s;
}

static bool load(const char* path,Program& p){std::string e; if(RifxLoader::loadFile(path,p,&e)) return true; std::fprintf(stderr,"%s: %s\n",path,e.c_str()); return false;}
static bool active(const SigStats&s){return s.energy>1.0e-10L;}

int main(int argc,char**argv){
    if(argc!=6) return 90;
    Program hp,s2,s4,sur,enc;
    if(!load(argv[1],hp)||!load(argv[2],s2)||!load(argv[3],s4)||!load(argv[4],sur)||!load(argv[5],enc)) return 91;

    // Headphone path: isolated left-side source must reach the opposite ear only
    // after the native 12-sample tank tap, while the ipsilateral ear is immediate.
    auto h=impulse(hp,0); if(h.size()!=2) return 1;
    auto hL=stats(h[0]),hR=stats(h[1]);
    if(hL.first!=0 || hR.first!=12 || !active(hR) || hR.delayed<=0 || hL.delayed<=0) return 2;
    if((hL.delayed+hR.delayed)/(hL.energy+hR.energy)<0.20L) return 3;

    // Two-speaker virtualizer: a processed source creates delayed contralateral
    // crossfeed at 8 samples rather than simply panning or bypassing.
    auto a=impulse(s2,2); if(a.size()!=2) return 4;
    auto aL=stats(a[0]),aR=stats(a[1]);
    if(aL.first!=0 || aR.first!=8 || !active(aR) || aR.delayed<=0) return 5;
    if((aL.delayed+aR.delayed)/(aL.energy+aR.energy)<0.20L) return 6;

    // Four-speaker virtualizer: one processed input fans into all four outputs;
    // the opposite-side pair starts at the same 8-sample spatializing tap.
    auto b=impulse(s4,2); if(b.size()!=4) return 7;
    SigStats bs[4]={stats(b[0]),stats(b[1]),stats(b[2]),stats(b[3])};
    for(const auto& x:bs) if(!active(x)) return 8;
    if(bs[0].first!=0 || bs[2].first!=0 || bs[1].first!=8 || bs[3].first!=8) return 9;

    // SurroundSpatializer has no TRAM but is not a no-op: its diffuse path fans
    // to all four outputs and carries recursive/filter state well past sample 0.
    auto c=impulse(sur,8); if(c.size()!=4) return 10;
    for(unsigned o=0;o<4;++o){auto x=stats(c[o]); if(!active(x)||x.first!=0||x.last<20||x.delayed/x.energy<0.70L) return 11;}

    // Surround Encoder is entirely delayed for its principal input and duplicates
    // the encoded component into front/rear corresponding outputs.
    auto d=impulse(enc,0); if(d.size()!=4) return 12;
    auto d0=stats(d[0]),d1=stats(d[1]),d2=stats(d[2]),d3=stats(d[3]);
    if(d0.first!=5 || d2.first!=5 || !active(d0)||!active(d2) || active(d1)||active(d3)) return 13;
    if(d0.delayed!=d0.energy || d2.delayed!=d2.energy || d0.last<60 || d2.last<60) return 14;

    std::printf("headphone_crossfeed_first=%d hp_delayed_pct=%.2Lf sb2_crossfeed_first=%d sb4_crossfeed_first=%d surround_tail_last=%d encoder_first=%d\n",
        hR.first,100.0L*(hL.delayed+hR.delayed)/(hL.energy+hR.energy),aR.first,bs[1].first,stats(c[0]).last,d0.first);
    return 0;
}
