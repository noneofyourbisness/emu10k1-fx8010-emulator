#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
using namespace fx8010;
namespace fs=std::filesystem;

static std::uint64_t mix(std::uint64_t h,std::uint32_t x){ h^=x; h*=1099511628211ull; return h; }
static std::uint64_t runOne(Program p){
    for(auto site:p.inputPatchSites) if(!patchSite(p.code,site,0x40u)) return 0;
    Engine e; e.load(p);
    std::uint64_t h=1469598103934665603ull;
    for(unsigned n=0;n<512u;++n){
        e.runSample();
        h=mix(h,e.debugDbac());
        for(const auto&o:p.outputs) h=mix(h,static_cast<std::uint32_t>(e.get(o.virtualReg)));
    }
    return h;
}
int main(int argc,char**argv){
    if(argc!=3){std::fprintf(stderr,"need APS and LiveWare fixture directories\n");return 2;}
    std::vector<fs::path> files;
    for(int ai=1;ai<3;++ai) for(const auto&de:fs::directory_iterator(argv[ai]))
        if(de.is_regular_file() && de.path().extension()==".rifx") files.push_back(de.path());
    if(files.size()!=41u) return 3;
    std::uint64_t corpus=1469598103934665603ull;
    for(const auto&path:files){
        Program p; std::string err;
        if(!RifxLoader::loadFile(path.string(),p,&err)){std::fprintf(stderr,"%s: %s\n",path.string().c_str(),err.c_str());return 4;}
        const auto a=runOne(p), b=runOne(p);
        if(a==0u || a!=b){std::fprintf(stderr,"nondeterministic/crashed model for %s: %llx %llx\n",path.string().c_str(),(unsigned long long)a,(unsigned long long)b);return 5;}
        corpus=mix(corpus,static_cast<std::uint32_t>(a)); corpus=mix(corpus,static_cast<std::uint32_t>(a>>32));
    }
    std::printf("full 41-program factory corpus executed 512 samples/program deterministically; aggregate=%016llx\n",(unsigned long long)corpus);
    return 0;
}
