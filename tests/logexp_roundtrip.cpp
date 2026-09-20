#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <random>
#include <limits>
using namespace fx8010;
static int eb(unsigned m){int n=0;do{++n;m>>=1;}while(m);return n;}
int main(){std::mt19937 rng(0x8010);unsigned long long n=0;for(unsigned mx=1;mx<=31;++mx){const unsigned loss=(1u<<eb(mx))-1u;for(unsigned i=0;i<5000;++i){const auto x=(std::int32_t)rng();const auto l=Engine::logEncode(x,mx,0);const auto y=Engine::expDecode(l,mx,0);const long long d=std::llabs((long long)y-x);if(d>(long long)loss+1){std::printf("mx=%u x=%d y=%d loss=%u d=%lld\n",mx,x,y,loss,d);return 2;}++n;}}std::printf("logexp_vectors=%llu pass\n",n);return 0;}
