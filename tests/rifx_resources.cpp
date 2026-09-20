#include "fx8010_engine.h"
#include <cstdio>
#include <string>
using namespace fx8010;
static bool load(const char*p,Program&o){std::string e;if(!RifxLoader::loadFile(p,o,&e)){std::printf("%s: %s\n",p,e.c_str());return false;}return true;}
int main(int argc,char**argv){if(argc!=4)return 9;Program c,p,r;if(!load(argv[1],c)||!load(argv[2],p)||!load(argv[3],r))return 8;
 if(c.tram.size()!=6||c.itramSize!=1539||c.xtramSize!=0) return 2;
 for(auto &t:c.tram) if(t.external) return 3;
 if(p.tram.size()!=3||p.itramSize!=0||p.xtramSize!=48017) return 4;
 for(auto &t:p.tram) if(!t.external) return 5;
 if(r.tram.size()!=47||r.itramSize!=3760||r.xtramSize!=78154)return 6;
 for(std::size_t i=0;i<r.tram.size();++i)if(r.tram[i].external!=(i>=20))return 7;
 // The EAX2 RIFX contains symbolic TRAM resource IDs 0x58 and 0x5a, which
 // numerically collide with NOISE0 and GPR_IRQ.  The loader must patch those
 // declared sites to virtual tank registers (0x8058/0x805a) before execution.
 for(const auto &ins:r.code){
   const std::uint16_t v[4]={ins.a,ins.x,ins.y,ins.r};
   for(auto x:v) if(x==0x58u||x==0x59u||x==0x5au) return 10;
 }
 std::printf("Chorus I=%u X=%u; Echo I=%u X=%u; FDN I=%u X=%u; special aliases patched\n",c.itramSize,c.xtramSize,p.itramSize,p.xtramSize,r.itramSize,r.xtramSize);return 0;}
