#include "fx8010_engine.h"
#include <cstdio>
#include <string>
using namespace fx8010;
static std::uint32_t initValue(const Program&p,std::uint16_t r){
    switch(r){case 0x40:return 0;case 0x41:return 1;case 0x42:return 2;case 0x43:return 3;case 0x44:return 4;case 0x45:return 8;
      case 0x46:return 0x10;case 0x47:return 0x20;case 0x48:return 0x100;case 0x49:return 0x10000;case 0x4a:return 0x80000;
      case 0x4b:return 0x10000000;case 0x4c:return 0x20000000;case 0x4d:return 0x40000000;case 0x4e:return 0x80000000;
      case 0x4f:return 0x7fffffff;case 0x50:return 0xffffffff;case 0x51:return 0xfffffffe;case 0x52:return 0xc0000000;
      case 0x53:return 0x4f1bbcdc;case 0x54:return 0x5a7ef9db;case 0x55:return 0x00100000;default:break;}
    for(const auto&g:p.gprInit) if(g.reg==r) return static_cast<std::uint32_t>(g.value);
    return 0;
}
int main(int argc,char**argv){
    if(argc!=4) return 1;
    unsigned seen=0, decisive=0;
    for(int f=1;f<argc;++f){Program p;std::string err;if(!RifxLoader::loadFile(argv[f],p,&err)) return 2;
        for(std::size_t pc=0;pc<p.code.size();++pc){const auto&i=p.code[pc];if(i.op!=15)continue;++seen;
            const auto xv=initValue(p,i.x), yv=initValue(p,i.y);
            const unsigned productionCount=yv&0x3ffu;
            const unsigned patentCount=xv&0x3ffu;
            // In the shipped programs X holds recognizable condition masks
            // (0x180, 0x200, 0x7fffffff) while Y is a tiny count (2 or 5).
            if(productionCount<=5u && patentCount>=0x180u) ++decisive;
            std::printf("%s pc=%zu X=%08x Y=%08x production_count=%u patent_count=%u\n",
                        p.name.c_str(),pc,xv,yv,productionCount,patentCount);
        }
    }
    // Freak Shifter, PitchShift, and both 0x180 AC3 SKIPs are decisive. The
    // AC3 all-ones condition instruction is useful but not needed for proof.
    return (seen==5u && decisive>=4u)?0:3;
}
