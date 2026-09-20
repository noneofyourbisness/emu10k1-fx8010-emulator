#include "fx8010_engine.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
using namespace fx8010;
namespace fs=std::filesystem;
int main(int argc,char**argv){
    if(argc!=3)return 2;
    std::vector<fs::path> files;
    for(int ai=1;ai<3;++ai)for(const auto&de:fs::directory_iterator(argv[ai]))
        if(de.is_regular_file()&&de.path().extension()==".rifx")files.push_back(de.path());
    if(files.size()!=41u)return 3;
    unsigned programsWithTram=0,decls=0,aligned=0,xdecls=0;
    for(const auto&f:files){
        Program p;std::string err;if(!RifxLoader::loadFile(f.string(),p,&err))return 4;
        if(!p.scheduledTramSequencer)return 5;
        if(!p.tram.empty())++programsWithTram;
        unsigned is=0,xs=0;
        for(const auto&t:p.tram){
            const unsigned slot=t.external?xs++:is++;
            if(t.hardwareSlot!=slot){std::fprintf(stderr,"%s slot mismatch\n",f.string().c_str());return 6;}
            const unsigned pc=t.patchSite&0x0fffu;
            const bool want=Engine::tramAlignFlag(t.external,t.write,pc,slot);
            if(t.align!=want){std::fprintf(stderr,"%s ALIGN mismatch pc=%u slot=%u\n",f.string().c_str(),pc,slot);return 7;}
            ++decls;aligned+=t.align?1u:0u;xdecls+=t.external?1u:0u;
        }
    }
    std::printf("RIFX phase metadata PASS: 41 programs, %u TRAM programs, %u declarations (%u XTRAM), %u ALIGN corrections\n",programsWithTram,decls,xdecls,aligned);
    return 0;
}
