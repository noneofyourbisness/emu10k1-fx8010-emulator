#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <cmath>
using namespace fx8010;
struct S{long double e{};int first{-1},last{-1};std::int32_t firstValue{};};
static bool load(const char*p,Program&x){std::string e;if(RifxLoader::loadFile(p,x,&e))return true;std::fprintf(stderr,"%s: %s\n",p,e.c_str());return false;}
static std::vector<std::vector<std::int32_t>> render(Program p,unsigned feed,unsigned frames=512){
 std::vector<std::uint16_t> in;for(std::size_t i=0;i<p.inputPatchSites.size();++i){auto r=std::uint16_t(0x9000u+i);if(!patchSite(p.code,p.inputPatchSites[i],r))return{};in.push_back(r);}if(feed>=in.size())return{};
 Engine e;e.load(p);std::vector<std::vector<std::int32_t>>y(p.outputs.size(),std::vector<std::int32_t>(frames));
 for(unsigned n=0;n<frames;++n){for(unsigned i=0;i<in.size();++i)e.set(in[i],n==0&&i==feed?0x20000000:0);e.runSample();for(unsigned o=0;o<p.outputs.size();++o)y[o][n]=e.get(p.outputs[o].virtualReg);}return y;
}
static S st(const std::vector<std::int32_t>&v){S s;for(unsigned n=0;n<v.size();++n){auto x=v[n];long double f=static_cast<long double>(x)/2147483648.0L;s.e+=f*f;if(x){if(s.first<0){s.first=int(n);s.firstValue=x;}s.last=int(n);}}return s;}
static bool only(const std::vector<std::vector<std::int32_t>>&y,unsigned wanted){if(y.size()!=4)return false;for(unsigned o=0;o<4;++o)if((st(y[o]).first>=0)!=(o==wanted))return false;return true;}
static bool equal4(const std::vector<std::vector<std::int32_t>>&y){if(y.size()!=4)return false;auto a=st(y[0]);for(unsigned o=1;o<4;++o){auto b=st(y[o]);if(a.first!=b.first||a.last!=b.last||std::fabs(a.e-b.e)>1e-12L)return false;}return true;}
int main(int argc,char**argv){if(argc!=3)return 90;Program s,e;if(!load(argv[1],s)||!load(argv[2],e))return 91;bool ok=true;
 ok&=s.inputPatchSites.size()==11&&s.outputs.size()==4&&e.inputPatchSites.size()==4&&e.outputs.size()==4;
 // Unity direct corners: FL, FR, RL, RR.
 ok&=only(render(s,0),0)&&only(render(s,1),1)&&only(render(s,6),2)&&only(render(s,7),3);
 // Four -3 dB single-speaker feeds. Their energy should be ~half of unity direct.
 auto u=st(render(s,0)[0]);
 for(auto q: {std::pair<unsigned,unsigned>{2,0},{3,2},{4,1},{5,3}}){auto y=render(s,q.first);ok&=only(y,q.second);auto z=st(y[q.second]);long double ratio=z.e/u.e;ok&=ratio>0.49L&&ratio<0.51L;}
 // Three common paths: long recursive, direct one-frame 0.5-to-all, short filtered.
 auto a=render(s,8),b=render(s,9),c=render(s,10);ok&=equal4(a)&&equal4(b)&&equal4(c);
 auto sa=st(a[0]),sb=st(b[0]),sc=st(c[0]);ok&=sa.first==0&&sa.last==511;ok&=sb.first==0&&sb.last==0;ok&=sc.first==0&&sc.last>=20&&sc.last<64;
 // Encoder: only feeds 0/1 have isolated acoustic output; 2/3 are silent in this RIFX.
 auto e0=render(e,0),e1=render(e,1),e2=render(e,2),e3=render(e,3);ok&=st(e0[0]).first==5&&st(e0[2]).first==5&&st(e0[1]).first<0&&st(e0[3]).first<0;ok&=st(e1[1]).first==5&&st(e1[3]).first==5&&st(e1[0]).first<0&&st(e1[2]).first<0;
 for(auto* y:{&e2,&e3})for(auto&ch:*y)ok&=st(ch).first<0;
 std::printf("surround common_last=%d/%d/%d encoder_feed2_silent=%d ok=%d\n",sa.last,sb.last,sc.last,st(e2[0]).first<0?1:0,ok?1:0);return ok?0:1;
}
