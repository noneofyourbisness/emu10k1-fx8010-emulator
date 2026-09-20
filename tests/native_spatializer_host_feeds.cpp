#include "fx8010_engine.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using namespace fx8010;

struct Stats { long double e{}; long double delayed{}; int first{-1}; int last{-1}; };

static bool load(const char* path, Program& p){
    std::string err;
    if(RifxLoader::loadFile(path,p,&err)) return true;
    std::fprintf(stderr,"%s: %s\n",path,err.c_str()); return false;
}
static std::vector<std::uint16_t> bindInputs(Program& p){
    std::vector<std::uint16_t> r;
    for(std::size_t i=0;i<p.inputPatchSites.size();++i){
        auto v=static_cast<std::uint16_t>(0x9000u+i);
        if(!patchSite(p.code,p.inputPatchSites[i],v)) return {};
        r.push_back(v);
    }
    return r;
}
static std::vector<std::vector<std::int32_t>> render(Program p,unsigned pin,unsigned frames=256){
    auto in=bindInputs(p); if(in.size()!=p.inputPatchSites.size() || pin>=in.size()) return {};
    Engine e; e.load(p);
    std::vector<std::vector<std::int32_t>> y(p.outputs.size(),std::vector<std::int32_t>(frames));
    for(unsigned n=0;n<frames;++n){
        for(unsigned i=0;i<in.size();++i) e.set(in[i], (n==0u && i==pin)?0x20000000:0);
        e.runSample();
        for(unsigned o=0;o<p.outputs.size();++o) y[o][n]=e.get(p.outputs[o].virtualReg);
    }
    return y;
}
static Stats stats(const std::vector<std::int32_t>& v){
    Stats s;
    for(std::size_t n=0;n<v.size();++n){
        const long double f=static_cast<long double>(v[n])/2147483648.0L;
        s.e+=f*f; if(n) s.delayed+=f*f;
        if(v[n]){if(s.first<0)s.first=static_cast<int>(n);s.last=static_cast<int>(n);}
    }
    return s;
}
static bool near(long double a,long double b,long double rel=2.0e-4L){
    const auto d=std::fabs(a-b), m=std::max(std::fabs(a),std::fabs(b));
    return d <= rel*std::max(1.0e-18L,m);
}
static bool mirrorPair(const Program& p,unsigned a,unsigned b){
    auto A=render(p,a),B=render(p,b); if(A.size()!=2||B.size()!=2)return false;
    auto a0=stats(A[0]),a1=stats(A[1]),b0=stats(B[0]),b1=stats(B[1]);
    // The A4 programs are not cycle-symmetric at the first sample because the
    // two sides occupy different instruction/TRAM service slots.  Host-feed
    // identity is therefore checked by swapped energy/dominance, not onset.
    return near(a0.e,b1.e,5.0e-3L)&&near(a1.e,b0.e,5.0e-3L)&&a0.e>a1.e&&b1.e>b0.e;
}

int main(int argc,char**argv){
    if(argc!=4) return 90;
    Program hp,s2,s4;
    if(!load(argv[1],hp)||!load(argv[2],s2)||!load(argv[3],s4)) return 91;

    bool ok=true;
    ok &= hp.inputPatchSites.size()==8u && hp.outputs.size()==2u;
    ok &= s2.inputPatchSites.size()==9u && s2.outputs.size()==2u;
    ok &= s4.inputPatchSites.size()==9u && s4.outputs.size()==4u;

    // Headphone network: three exact L/R mirror feed pairs, one equal direct
    // center feed, and one equal filtered/common feed. This is the topology a
    // host-side panner can crossfade between without rewriting the HRTF GPRs.
    ok &= mirrorPair(hp,0,1) && mirrorPair(hp,2,3) && mirrorPair(hp,4,5);
    auto hc=render(hp,6), hd=render(hp,7);
    if(hc.size()!=2||hd.size()!=2) return 2;
    auto hc0=stats(hc[0]),hc1=stats(hc[1]),hd0=stats(hd[0]),hd1=stats(hd[1]);
    ok &= hc0.first==0 && hc1.first==0 && hc0.last==0 && hc1.last==0 && near(hc0.e,hc1.e);
    ok &= hd0.first==0 && hd1.first==0 && hd0.last>=40 && hd1.last>=40 && near(hd0.e,hd1.e);

    // Two-speaker network: hard/direct endpoint feeds 0/1, three mirrored
    // processed pairs 2/3, 4/5, 6/7, and a symmetric common/filter feed 8.
    auto s20=render(s2,0),s21=render(s2,1),s28=render(s2,8);
    if(s20.size()!=2||s21.size()!=2||s28.size()!=2)return 3;
    auto s20L=stats(s20[0]),s20R=stats(s20[1]),s21L=stats(s21[0]),s21R=stats(s21[1]);
    ok &= s20L.first==0 && s20L.last==0 && s20R.first<0;
    ok &= s21R.first==0 && s21R.last==0 && s21L.first<0;
    ok &= mirrorPair(s2,2,3)&&mirrorPair(s2,4,5)&&mirrorPair(s2,6,7);
    auto s28L=stats(s28[0]),s28R=stats(s28[1]);
    ok &= near(s28L.e,s28R.e)&&s28L.last>100&&s28R.last>100;

    // Four-speaker network: exact direct front L/R are feeds 0/1 and exact
    // direct rear L/R are 4/5. Processed L/R pairs 2/3 and 6/7 feed both
    // front+rear with opposite-side delay; feed 8 is common to all four.
    auto q0=render(s4,0),q1=render(s4,1),q4=render(s4,4),q5=render(s4,5),q8=render(s4,8);
    if(q0.size()!=4||q1.size()!=4||q4.size()!=4||q5.size()!=4||q8.size()!=4)return 4;
    for(unsigned o=0;o<4;++o){
        const bool a=stats(q0[o]).first>=0,b=stats(q1[o]).first>=0,c=stats(q4[o]).first>=0,d=stats(q5[o]).first>=0;
        ok &= a==(o==0); ok &= b==(o==1); ok &= c==(o==2); ok &= d==(o==3);
    }
    // Exact pair mirror relation across FL<->FR and RL<->RR.
    const std::array<unsigned,4> mirror{{1,0,3,2}};
    for(auto pair: {std::pair<unsigned,unsigned>{2,3}, {6,7}}){
        auto A=render(s4,pair.first),B=render(s4,pair.second);
        for(unsigned o=0;o<4;++o){auto as=stats(A[o]),bs=stats(B[mirror[o]]);ok &= near(as.e,bs.e,5.0e-3L);}
    }
    auto q8s0=stats(q8[0]);
    for(unsigned o=1;o<4;++o){auto z=stats(q8[o]);ok &= near(q8s0.e,z.e)&&q8s0.first==z.first&&q8s0.last==z.last;}

    // The exact host pixel/coordinate interpolation coefficients are NOT in the
    // RIFX. This test intentionally proves only the fixed feed-bank symmetry and
    // direct endpoint/corner identities used by the reconstructed CLAP panners.
    std::printf("hp=3_mirror_pairs+direct_center+common sb2=direct_endpoints+3_pairs+common sb4=FL_FR_RL_RR+2_pairs+common ok=%d\n",ok?1:0);
    return ok?0:1;
}
