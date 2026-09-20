#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using namespace fx8010;
static std::uint32_t expected(std::int32_t r,bool s=false,bool b=false){
    std::uint32_t c=0;
    const std::uint32_t u=static_cast<std::uint32_t>(r);
    if(s) c|=0x10;
    if(r==0) c|=0x08;
    if(r<0) c|=0x04;
    if(((u>>31)&1u)==((u>>30)&1u)) c|=0x01;
    if(b) c|=0x02;
    return c;
}
static bool runOne(Instruction op,std::int32_t expect,bool sat=false,bool borrow=false){
    Program p;
    p.gprInit={{0x8000,0x20000000},{0x8001,0x40000000},{0x8002,0x10000000},{0x8003,7}};
    p.code.push_back(op);
    // SKIP with zero test/count copies prior CCR to R without replacing CCR.
    p.code.push_back({15,0x57,0x40,0x40,0x800f});
    Engine e;e.load(p);e.runSample();
    const std::int32_t got=e.get(op.r);
    const std::uint32_t cc=static_cast<std::uint32_t>(e.get(0x800f));
    if(got!=expect || cc!=expected(expect,sat,borrow)){
        std::printf("op=%u got=%#x exp=%#x cc=%#x expcc=%#x\n",op.op,(unsigned)got,(unsigned)expect,(unsigned)cc,(unsigned)expected(expect,sat,borrow));
        return false;
    }
    return true;
}
int main(){
    bool ok=true;
    ok &= runOne({8,0x8000,0x8001,0x8002,0x8004},0x10000000); // ANDXOR
    ok &= runOne({9,0x8000,0x8001,0x8003,0x8004},0x40000000); // TSTNEG true
    ok &= runOne({10,0x8000,0x8001,0x8003,0x8004},0x40000000); // LIMIT
    ok &= runOne({11,0x8003,0x8001,0x8000,0x8004},0x40000000); // LIMIT1
    ok &= runOne({14,0x40,0x8001,0x8000,0x8004},0x10000000); // 0 + .5*.25
    std::printf("ccr_result_matrix=%s\n",ok?"PASS":"FAIL");
    return ok?0:2;
}
