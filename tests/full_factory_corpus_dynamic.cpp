#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
using namespace fx8010;
namespace fs=std::filesystem;
static std::uint64_t mix(std::uint64_t h,std::uint32_t x){h^=x;h*=1099511628211ull;return h;}
static std::uint32_t lcg(std::uint32_t& s){s=s*1664525u+1013904223u;return s;}
static std::uint64_t runOne(Program p){
    std::vector<std::uint16_t> ins;
    ins.reserve(p.inputPatchSites.size());
    for(std::size_t j=0;j<p.inputPatchSites.size();++j){
        const auto r=static_cast<std::uint16_t>(0x9000u+j);
        if(!patchSite(p.code,p.inputPatchSites[j],r)) return 0;
        ins.push_back(r);
    }
    Engine e;e.load(p);
    std::uint32_t rng=0x31415926u;
    std::uint64_t h=1469598103934665603ull;
    for(unsigned n=0;n<4096u;++n){
        for(std::size_t j=0;j<ins.size();++j){
            // Moderately hot bipolar deterministic stimulus, deliberately
            // different per input and sample to exercise filters/TRAM/control flow.
            const std::uint32_t q=lcg(rng) ^ std::uint32_t(j*0x9e3779b9u);
            e.set(ins[j],static_cast<std::int32_t>(q>>2));
        }
        e.runSample();
        h=mix(h,e.debugDbac());
        h=mix(h,e.debugRegisterWord());
        for(const auto&o:p.outputs) h=mix(h,static_cast<std::uint32_t>(e.get(o.virtualReg)));
    }
    return h;
}
int main(int argc,char**argv){
    if(argc!=3)return 2;
    std::vector<fs::path> files;
    for(int ai=1;ai<3;++ai) for(const auto&de:fs::directory_iterator(argv[ai]))
        if(de.is_regular_file()&&de.path().extension()==".rifx")files.push_back(de.path());
    if(files.size()!=41u)return 3;
    std::uint64_t all=1469598103934665603ull;unsigned saturated=0;
    for(const auto&f:files){
        Program p;std::string err;if(!RifxLoader::loadFile(f.string(),p,&err))return 4;
        const auto a=runOne(p),b=runOne(p);if(a==0||a!=b){std::fprintf(stderr,"dynamic mismatch %s %llx %llx\n",f.string().c_str(),(unsigned long long)a,(unsigned long long)b);return 5;}
        all=mix(all,std::uint32_t(a));all=mix(all,std::uint32_t(a>>32));
        // Count is informational only: real factory programs legitimately saturate on hot synthetic input.
        Program q=p; std::vector<std::uint16_t> ins;
        for(std::size_t j=0;j<q.inputPatchSites.size();++j){auto r=std::uint16_t(0x9000u+j);patchSite(q.code,q.inputPatchSites[j],r);ins.push_back(r);}Engine e;e.load(q);std::uint32_t rng=0x31415926u;
        for(unsigned n=0;n<64;++n){for(std::size_t j=0;j<ins.size();++j)e.set(ins[j],std::int32_t((lcg(rng)^std::uint32_t(j*0x9e3779b9u))>>2));e.runSample();} saturated+=e.saturationOccurred()?1u:0u;
    }
    std::printf("41 factory programs executed 4096 nonzero-stimulus samples each deterministically; aggregate=%016llx saturated_programs(first64)=%u\n",(unsigned long long)all,saturated);
    return 0;
}
