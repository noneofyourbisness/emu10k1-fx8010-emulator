#include "fx8010_engine.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
using namespace fx8010;
namespace fs=std::filesystem;

static bool physicalInput(std::uint16_t r){ return r<=0x20u; }

int main(int argc,char**argv){
    if(argc!=3){std::fprintf(stderr,"need APS and LiveWare fixture directories\n");return 2;}
    std::vector<fs::path> files;
    for(int ai=1;ai<3;++ai){
        for(const auto&de:fs::directory_iterator(argv[ai]))
            if(de.is_regular_file() && de.path().extension()==".rifx") files.push_back(de.path());
    }
    std::array<std::uint64_t,16> ops{};
    std::size_t programs=0,instructions=0,trams=0,a0=0,a4=0,tramPrograms=0,xtramPrograms=0,noisePrograms=0,dbacPrograms=0,irqPrograms=0,ccrPrograms=0,accuPrograms=0,readDecls=0,writeDecls=0;
    bool everbSeen=false;
    for(const auto&path:files){
        Program p; std::string err;
        if(!RifxLoader::loadFile(path.string(),p,&err)){
            std::fprintf(stderr,"%s: %s\n",path.string().c_str(),err.c_str()); return 3;
        }
        ++programs; instructions+=p.code.size(); trams+=p.tram.size();
        if(!p.tram.empty()) ++tramPrograms;
        bool hasXtram=false,hasNoise=false,hasDbac=false,hasIrq=false,hasCcr=false,hasAccu=false;
        if(p.target=="EMU8010_A0") ++a0; else if(p.target=="EMU8010_A4") ++a4;
        else {std::fprintf(stderr,"unexpected target %s in %s\n",p.target.c_str(),path.string().c_str()); return 4;}
        if(p.rsrc[0]!=p.code.size()){
            std::fprintf(stderr,"instruction count mismatch in %s: rsrc=%u code=%zu\n",path.string().c_str(),p.rsrc[0],p.code.size());return 5;
        }
        for(const auto&t:p.tram){ if(t.external)hasXtram=true; if(t.write)++writeDecls; else ++readDecls; }
        if(hasXtram) ++xtramPrograms;
        const std::size_t extExpected=std::min<std::size_t>(p.rsrc[5],p.tram.size());
        std::size_t ext=0;
        for(const auto&t:p.tram) ext+=t.external?1u:0u;
        if(ext!=extExpected){
            std::fprintf(stderr,"XTRAM count mismatch in %s: expected=%zu parsed=%zu\n",path.string().c_str(),extExpected,ext);return 6;
        }
        if(p.target=="EMU8010_A0" && p.name=="Everb"){
            const std::size_t in=p.tram.size()-ext;
            if(p.tram.size()!=50u || in!=22u || ext!=28u){
                std::fprintf(stderr,"Everb domains wrong: total=%zu I=%zu X=%zu\n",p.tram.size(),in,ext); return 7;
            }
            everbSeen=true;
        }
        for(const auto&i:p.code){
            if(i.op>=16u) return 8;
            ++ops[i.op];
            const std::uint16_t regs[4]={i.a,i.x,i.y,i.r};
            for(auto r:regs){ hasNoise|=(r==0x58u||r==0x59u); hasDbac|=(r==0x5bu); hasIrq|=(r==0x5au); hasCcr|=(r==0x57u); hasAccu|=(r==0x56u); }
            const unsigned direct=unsigned(physicalInput(i.a))+unsigned(physicalInput(i.x))+unsigned(physicalInput(i.y));
            if(direct>1u){
                std::fprintf(stderr,"factory undefined multi-physical-input case in %s\n",path.string().c_str());return 9;
            }
        }
        noisePrograms+=hasNoise?1u:0u; dbacPrograms+=hasDbac?1u:0u; irqPrograms+=hasIrq?1u:0u; ccrPrograms+=hasCcr?1u:0u; accuPrograms+=hasAccu?1u:0u;
    }
    const std::array<std::uint64_t,16> expected={1051,259,2,4,133,13,10,0,4,7,8,1,11,3,279,7};
    if(programs!=41u || instructions!=1792u || trams!=236u || a0!=15u || a4!=26u || tramPrograms!=19u || xtramPrograms!=5u || noisePrograms!=2u || dbacPrograms!=1u || irqPrograms!=1u || ccrPrograms!=5u || accuPrograms!=11u || readDecls!=140u || writeDecls!=96u || !everbSeen){
        std::fprintf(stderr,"corpus totals: programs=%zu insn=%zu tram=%zu A0=%zu A4=%zu tramP=%zu xtramP=%zu noiseP=%zu dbacP=%zu irqP=%zu ccrP=%zu accuP=%zu read=%zu write=%zu Everb=%d\n",programs,instructions,trams,a0,a4,tramPrograms,xtramPrograms,noisePrograms,dbacPrograms,irqPrograms,ccrPrograms,accuPrograms,readDecls,writeDecls,everbSeen?1:0);return 10;
    }
    if(ops!=expected){
        for(unsigned i=0;i<16;++i) std::fprintf(stderr,"op%u=%llu expected=%llu\n",i,(unsigned long long)ops[i],(unsigned long long)expected[i]);
        return 11;
    }
    std::printf("factory corpus: 41 programs, 1792 instructions, 236 TRAM declarations (140 read/96 write), 19 TRAM programs/5 XTRAM, 15 A0 + 26 A4; NOISE=2 DBAC/IRQ=1 CCR=5 ACCU=11; opcode histogram and Everb 22I/28X domains verified\n");
    return 0;
}
