#include "fx8010_engine.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

namespace fx8010 {
namespace {
constexpr std::uint16_t ACCU=0x56, CCR=0x57, NOISE0=0x58, NOISE1=0x59, IRQ=0x5a, DBAC=0x5b;
constexpr std::pair<std::uint16_t,std::uint32_t> C[]={{0x40,0},{0x41,1},{0x42,2},{0x43,3},{0x44,4},{0x45,8},{0x46,0x10},{0x47,0x20},{0x48,0x100},{0x49,0x10000},{0x4a,0x80000},{0x4b,0x10000000},{0x4c,0x20000000},{0x4d,0x40000000},{0x4e,0x80000000},{0x4f,0x7fffffff},{0x50,0xffffffff},{0x51,0xfffffffe},{0x52,0xc0000000},{0x53,0x4f1bbcdc},{0x54,0x5a7ef9db},{0x55,0x00100000}};
static std::uint16_t rd16(const std::uint8_t* p,bool be){return be?std::uint16_t(p[0]<<8|p[1]):std::uint16_t(p[0]|p[1]<<8);} 
static std::uint32_t rd32(const std::uint8_t* p,bool be){return be?(std::uint32_t(p[0])<<24|std::uint32_t(p[1])<<16|std::uint32_t(p[2])<<8|p[3]):(std::uint32_t(p[0])|std::uint32_t(p[1])<<8|std::uint32_t(p[2])<<16|std::uint32_t(p[3])<<24);} 
static bool tag(const std::uint8_t* p,const char* s){return std::memcmp(p,s,4)==0;}
static int floorLog2(std::uint32_t v) noexcept { int n=-1; while(v){++n; v>>=1;} return n; }
}

bool RifxLoader::loadFile(const std::string& path,Program& out,std::string* error){std::ifstream f(path,std::ios::binary); if(!f){if(error)*error="open failed";return false;} std::vector<std::uint8_t>b((std::istreambuf_iterator<char>(f)),{}); return loadBytes(b.data(),b.size(),out,error);} 

bool RifxLoader::loadBytes(const std::uint8_t* b,std::size_t n,Program& o,std::string* e){
    if(n<12 || (!tag(b,"RIFX")&&!tag(b,"RIFF"))){if(e)*e="not RIFX/RIFF";return false;}
    o=Program{}; o.bigEndian=tag(b,"RIFX"); const bool be=o.bigEndian; const std::size_t end=std::min<std::size_t>(n,8u+rd32(b+4,be));
    if(end<12||!tag(b+8,"PTXT")){if(e)*e="form is not PTXT";return false;}
    auto parseChunks=[&](auto&& self,std::size_t s,std::size_t ee,const char* listType)->void{
        for(std::size_t p=s;p+8<=ee;){const auto* h=b+p; std::uint32_t z=rd32(h+4,be); std::size_t d=p+8, q=d+z; if(q>ee||q>n)break;
            if(tag(h,"LIST")&&z>=4){char t[5]{};std::memcpy(t,b+d,4);self(self,d+4,q,t);} 
            else if(tag(h,"ckid")){std::vector<std::string> ss;std::size_t k=d;while(k<q){std::size_t j=k;while(j<q&&b[j])++j;if(j>k)ss.emplace_back((const char*)b+k,j-k);k=j+1;}if(ss.size()>0)o.target=ss[0];if(ss.size()>1)o.version=ss[1];if(ss.size()>2)o.name=ss[2];}
            else if(tag(h,"rsrc")){
                for(int i=0;i<13&&d+2*i+1<q;++i)o.rsrc[i]=rd16(b+d+2*i,be);
                if(q-d>=22){o.itramSize=rd32(b+d+14,be);o.xtramSize=rd32(b+d+18,be);}
            } 
            else if(tag(h,"code")){for(std::size_t k=d;k+9<=q;k+=9)o.code.push_back({b[k],rd16(b+k+1,be),rd16(b+k+3,be),rd16(b+k+5,be),rd16(b+k+7,be)});} 
            else if(listType&&std::strcmp(listType,"gpri")==0&&tag(h,"gprs")){for(std::size_t k=d;k+6<=q;k+=6)o.gprInit.push_back({rd16(b+k,be),(std::int32_t)rd32(b+k+2,be)});} 
            else if(listType&&std::strcmp(listType,"gpri")==0&&tag(h,"tram")){for(std::size_t k=d;k+12<=q;k+=12){auto r=rd16(b+k,be);o.tram.push_back({std::uint16_t(0x8000|(r&0x0fff)),bool(r&0x1000),rd32(b+k+2,be),rd32(b+k+6,be),rd16(b+k+10,be),r});}} 
            else if(listType&&std::strcmp(listType,"patc")==0&&tag(h,"inp ")){for(std::size_t k=d;k+2<=q;k+=2)o.inputPatchSites.push_back(rd16(b+k,be));}
            else if(listType&&std::strcmp(listType,"patc")==0&&tag(h,"outp")){for(std::size_t k=d;k+4<=q;k+=4)o.outputs.push_back({rd16(b+k,be),rd16(b+k+2,be)});} 
            p=q+(z&1u);
        }};
    parseChunks(parseChunks,12,end,nullptr);
    if(o.code.empty()){if(e)*e="no code chunk";return false;}
    // E-mu's 26-byte rsrc block reports allocated ITRAM/XTRAM access-pair
    // resources, not necessarily the number of active `tram` declarations.
    // Factory APS programs can reserve extra ITRAM pairs as OFF/pseudo-GPR
    // storage (Everb is the smallest clear example).  The active declarations
    // are serialized internal first, external last, and rsrc[5] gives the
    // external-pair count exactly across the recovered APS/LiveWare corpus.
    const std::size_t nx=std::min<std::size_t>(o.rsrc[5],o.tram.size());
    const std::size_t firstExternal=o.tram.size()-nx;
    unsigned islot=0, xslot=0;
    for(std::size_t i=0;i<o.tram.size();++i){
        auto &t=o.tram[i];
        t.external=(i>=firstExternal);
        t.hardwareSlot=static_cast<std::uint16_t>(t.external?xslot++:islot++);
        const unsigned pc=static_cast<unsigned>(t.patchSite&0x0fffu);
        // Literal EMUAPS.VXD phase equations.  The equality cases tell us the
        // service-event ordering as well: an ITRAM write slot is serviced before
        // PC 3*s, its read result becomes visible before PC 3*s+2, and XTRAM
        // read/write service occurs before PC 128+4*s.
        const std::int64_t instr=static_cast<std::int64_t>(pc);
        const std::int64_t slot=static_cast<std::int64_t>(t.hardwareSlot);
        t.align=!t.external ? (t.write ? instr>=slot*3 : instr-1<=slot*3)
                            : (t.write ? instr-128>=slot*4 : instr-127<=slot*4);
    }
    o.scheduledTramSequencer=true;
    // Creative loader uses bit15 as symbolic-resource marker. Keeping the
    // symbolic ID as our virtual register preserves the same aliasing while
    // avoiding dependence on physical-card allocation addresses.
    for(auto&t:o.tram) patchSite(o.code,t.patchSite,t.dataReg);
    for(auto&p:o.outputs) patchSite(o.code,p.site,p.virtualReg);
    return true;
}

Engine::Engine(){initConstants();}
void Engine::initConstants() noexcept{
    reg_.fill(0);constant_.fill(false);tramAddress_.fill(false);
    for(auto [r,v]:C){reg_[r]=(std::int32_t)v;constant_[r]=true;}
    acc_={};reg_[ACCU]=0;reg_[CCR]=0;reg_[NOISE0]=0;reg_[NOISE1]=0;reg_[IRQ]=0;reg_[DBAC]=0;
    dbac_=0;noiseState0_=0x13579bdfu;noiseDelayPos_=0;
    // Seed with 4097 immediately preceding samples from the same deterministic
    // placeholder stream. This preserves the project's compatibility topology
    // while the NOISE1 relationship and silicon PRNG remain unconfirmed.
    for(auto&v:noiseDelay_) v=noiseQ31(noiseState0_);
    irqPending_=false;irqCount_=0;saturationOccurred_=false;saturationAddress_=0;currentPc_=0;
    cardMode_=false;cardOutputWritten_.fill(false);
}
void Engine::load(const Program&p,std::size_t minTram){
    program_=p;initConstants();code_=p.code;tram_=p.tram;minimumTramSamples_=minTram;
    for(auto&t:tram_) tramAddress_[std::uint16_t(t.dataReg+1)]=true;
    for(auto&g:p.gprInit)set(g.reg,g.value);
    bool hasI=false,hasX=false;
    for(auto&t:tram_){
        hasX|=t.external;hasI|=!t.external;
        set(t.dataReg,0);
        set(std::uint16_t(t.dataReg+1),(std::int32_t)(t.initialAddress<<11));
    }
    // EMU10K1 exposes separate 8K-sample ITRAM and up-to-1M-sample XTRAM.
    // A larger ITRAM is permitted only for our non-48-kHz convenience mode.
    const std::size_t iNeed=std::max<std::size_t>({std::size_t(8192),minTram,std::size_t(p.itramSize)});
    itramMem_.assign(hasI?iNeed:0,0);
    xtramMem_.assign(hasX?(std::size_t(1)<<20):0,0);
    if(p.linearTramCompatibility){
        itramLinearCompat_.assign(itramMem_.size(),0);
        xtramLinearCompat_.assign(xtramMem_.size(),0);
    }else{
        itramLinearCompat_.clear();
        xtramLinearCompat_.clear();
    }
    dbac_=0;
}
void Engine::reset(bool keep){auto p=program_; if(keep)load(p,minimumTramSamples_); else initConstants();}
void Engine::set(std::uint16_t r,std::int32_t v) noexcept{
    if(constant_[r] || r==NOISE0 || r==NOISE1) return;
    // DSP-visible TRAM address registers use bits 30..11 as the physical
    // 20-bit address and bits 10..0 as fractional workspace; bit31 reads 0.
    if(isTramAddressReg(r)) v=static_cast<std::int32_t>(static_cast<std::uint32_t>(v)&0x7fffffffu);
    reg_[r]=v;
}
std::int32_t Engine::get(std::uint16_t r)const noexcept{return r==ACCU?highAcc(acc_):reg_[r];}
std::int32_t Engine::sat32(std::int64_t v) noexcept{return v>INT32_MAX?INT32_MAX:v<INT32_MIN?INT32_MIN:(std::int32_t)v;}
std::int32_t Engine::q31(double v) noexcept{v=std::clamp(v,-1.0,2147483647.0/2147483648.0);return sat32((std::int64_t)std::llround(v*2147483648.0));}
double Engine::fromQ31(std::int32_t v) noexcept{return double(v)/2147483648.0;}
Engine::Acc Engine::wrap67(Acc a) noexcept { a.hi &= 0x07u; return a; }
Engine::Acc Engine::accFromI64(std::int64_t v) noexcept { return {std::uint64_t(v), std::uint8_t(v < 0 ? 0x07u : 0x00u)}; }
Engine::Acc Engine::accFromShift31(std::int64_t v) noexcept { const std::uint64_t u=std::uint64_t(v); return {u << 31, std::uint8_t((u >> 33) & 0x07u)}; }
Engine::Acc Engine::accAdd(Acc a, Acc b) noexcept { const std::uint64_t lo=a.lo+b.lo; const std::uint8_t carry=std::uint8_t(lo<a.lo); return {lo, std::uint8_t((a.hi+b.hi+carry)&0x07u)}; }
std::int64_t Engine::highAccRaw(Acc a) noexcept {
    a=wrap67(a);
    const std::uint64_t q=(std::uint64_t(a.hi)<<33)|(a.lo>>31); // signed 36-bit bits 66..31
    return (q&(std::uint64_t(1)<<35))
        ? std::int64_t(q | (~std::uint64_t(0)<<36))
        : std::int64_t(q);
}
std::int32_t Engine::highAcc(Acc a) noexcept {
    const auto h=highAccRaw(a);
    if(h>INT32_MAX) return INT32_MAX;
    if(h<INT32_MIN) return INT32_MIN;
    return std::int32_t(h);
}
std::int32_t Engine::highAccWrap(Acc a) noexcept {
    a=wrap67(a);
    // Fractional MAC word-wrap returns bits 31..62 of the 67-bit accumulator
    // directly, rather than first saturating the 36-bit guarded value.
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(a.lo >> 31));
}
std::int32_t Engine::lowAcc(Acc a) noexcept{return std::int32_t(std::uint32_t(a.lo));}
std::int32_t Engine::mulHigh(std::int32_t x,std::int32_t y) noexcept{return (std::int32_t)((std::int64_t(x)*std::int64_t(y))>>31);}
std::uint32_t Engine::nextNoiseWord(std::uint32_t& state) noexcept {
    // xorshift32 is only a deterministic software stand-in for the undocumented
    // EMU10K1 generator sequence.  It is not presented as a silicon LFSR.
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
std::int32_t Engine::noiseQ31(std::uint32_t& state) noexcept {
    // Contemporary EMU10K1 documentation gives the source range as
    // [-0.5,+0.5).  The exact silicon PRNG remains unknown; this helper is
    // therefore only the deterministic stand-in used to generate NOISE0.
    const std::uint32_t u=nextNoiseWord(state)&0x7fffffffu;
    return static_cast<std::int32_t>(u)-0x40000000;
}
std::int32_t Engine::readOperand(std::uint16_t r)const noexcept{return r==ACCU?0:reg_[r];}
std::int32_t Engine::readA(std::uint16_t r,std::uint8_t op)const noexcept{if(r!=ACCU)return reg_[r]; if(op==4||op==5)return lowAcc(acc_); return highAcc(acc_);} 
bool Engine::tramAlignFlag(bool external,bool write,unsigned instructionIndex,unsigned slot) noexcept {
    // Literal signed form of E-mu APS 1.5 EMUAPS.VXD loader equations. Keep these
    // calculations signed: for reads, instructions before the nominal tank
    // service origin intentionally satisfy the <= relation.
    const std::int64_t instr=static_cast<std::int64_t>(instructionIndex);
    const std::int64_t s=static_cast<std::int64_t>(slot);
    if(!external) return write ? instr >= s*3
                               : instr-1 <= s*3;
    return write ? instr-128 >= s*4
                 : instr-127 <= s*4;
}

bool Engine::skipConditionWord(std::uint32_t ccr,std::uint32_t x) noexcept {
    // Hardware-tested reconstruction from michgz/emu10k/OP/iSKIP.py.
    // CCR is five bits.  Each 10-bit bracket contains five positive literals
    // followed by five inverted literals.  The high two condition-word bits
    // select one of four AND/OR forms used by the FX8010 compiler.
    ccr &= 0x1fu;
    const unsigned form=((x>>30)&3u)+1u;
    auto all=[&](unsigned group) noexcept {
        const std::uint32_t g=(x>>(group*10u))&0x3ffu;
        if(g==0u) return false;
        const std::uint32_t pos=g&0x1fu;
        const std::uint32_t neg=(g>>5)&0x1fu;
        return (ccr&pos)==pos && (((~ccr)&0x1fu)&neg)==neg;
    };
    auto any=[&](unsigned group) noexcept {
        const std::uint32_t g=(x>>(group*10u))&0x3ffu;
        const std::uint32_t pos=g&0x1fu;
        const std::uint32_t neg=(g>>5)&0x1fu;
        return (ccr&pos)!=0u || ((((~ccr)&0x1fu)&neg)!=0u);
    };
    switch(form){
    case 1: return all(0)||all(1)||all(2);
    case 2: return any(0)&&any(1)&&any(2);
    case 3: return all(0)||all(1)||any(2);
    default:return any(0)&&any(1)&&all(2);
    }
}

std::array<std::uint32_t,2> Engine::encodeInstructionWords(const Instruction&i) noexcept {
    return { ((std::uint32_t(i.x)&0x3ffu)<<10)|(std::uint32_t(i.y)&0x3ffu),
             ((std::uint32_t(i.op)&0x0fu)<<20)|((std::uint32_t(i.r)&0x3ffu)<<10)|(std::uint32_t(i.a)&0x3ffu) };
}

Instruction Engine::decodeInstructionWords(std::uint32_t lo,std::uint32_t hi) noexcept {
    Instruction i;
    i.op=static_cast<std::uint8_t>((hi>>20)&0x0fu);
    i.r=static_cast<std::uint16_t>((hi>>10)&0x3ffu);
    i.a=static_cast<std::uint16_t>(hi&0x3ffu);
    i.x=static_cast<std::uint16_t>((lo>>10)&0x3ffu);
    i.y=static_cast<std::uint16_t>(lo&0x3ffu);
    return i;
}

void Engine::updateCCRMasked(std::int32_t r,bool sat,bool borrow,std::uint32_t mask) noexcept{
    // Confirmed EMU10K1 CCR values: N=1, B=2, M=4, Z=8, S=16.
    // The instruction patent is explicit that opcodes write different subsets
    // of these five bits; do not accidentally synthesize unsupported flags.
    std::uint32_t c=0;
    if(sat)c|=0x10;
    if(r==0)c|=0x08;
    if(r<0)c|=0x04;
    const std::uint32_t u=static_cast<std::uint32_t>(r);
    if(((u>>31)&1u)==((u>>30)&1u))c|=0x01;
    if(borrow)c|=0x02;
    reg_[CCR]=static_cast<std::int32_t>(c&mask&0x1fu);
}
void Engine::updateCCR(std::int32_t r,bool sat,bool borrow) noexcept{
    updateCCRMasked(r,sat,borrow,0x1fu);
}

std::int32_t Engine::logEncode(std::int32_t linear,std::uint32_t X,std::uint32_t Y) noexcept{
    // Bit-exact output path reconstructed and measured on real EMU10K1 by
    // michgz/emu10k.  Unlike the older patent-only approximation, the hardware
    // aliases X modulo 32 and Y modulo 4 and has defined behavior for X=0.
    std::uint32_t V=static_cast<std::uint32_t>(linear);
    X%=32u; Y%=4u;
    std::uint32_t Z=0u;
    bool sign=false;
    if(V>=0x80000000u){
        V=(V&0x7fffffffu)^0x7fffffffu;
        if(Y==0u||Y==2u) sign=true;
    }else if(Y==2u||Y==3u){
        sign=true;
    }
    if(V!=0u){
        const int M=floorLog2(V);
        if(M < static_cast<int>(32u-X)){
            if(X>=16u) Z=V<<(X-5u);
            else if(X>=8u) Z=V<<(X-4u);
            else if(X>=4u) Z=V<<(X-3u);
            else if(X>=2u) Z=V<<(X-2u);
            else if(X==1u) Z=V;
            else Z=V>>1;
        }else{
            const std::uint32_t P=static_cast<std::uint32_t>(M-static_cast<int>(30u-X));
            std::uint32_t Q=0u;
            if(X>=16u){
                Q=(M>=26 ? (V>>static_cast<unsigned>(M-26)) : (V<<static_cast<unsigned>(26-M)))-0x04000000u;
                Z=0x04000000u*P+Q;
            }else if(X>=8u){
                Q=(M>=27 ? (V>>static_cast<unsigned>(M-27)) : (V<<static_cast<unsigned>(27-M)))-0x08000000u;
                Z=0x08000000u*P+Q;
            }else if(X>=4u){
                Q=(M>=28 ? (V>>static_cast<unsigned>(M-28)) : (V<<static_cast<unsigned>(28-M)))-0x10000000u;
                Z=0x10000000u*P+Q;
            }else if(X>=2u){
                // In reachable hardware states M>=29 here for X=2/3.
                Q=(M>=29 ? (V>>static_cast<unsigned>(M-29)) : (V>>static_cast<unsigned>(29-M)))-0x20000000u;
                Z=0x20000000u*P+Q;
            }
        }
    }
    if(sign) Z=(Z^0x7fffffffu)+0x80000000u;
    return static_cast<std::int32_t>(Z);
}

std::int32_t Engine::expDecode(std::int32_t encoded,std::uint32_t X,std::uint32_t Y) noexcept{
    // Bit-exact measured EMU10K1 iEXP output path from michgz/emu10k.
    std::uint32_t V=static_cast<std::uint32_t>(encoded);
    X%=32u; Y%=4u;
    bool sign=false;
    if(V>=0x80000000u){ V=(V&0x7fffffffu)^0x7fffffffu; sign=true; }
    if(Y==1u) sign=false; else if(Y==2u) sign=true; else if(Y==3u) sign=!sign;
    std::uint64_t Z=0u;
    if(X==0u) Z=std::uint64_t(V)*2u;
    else if(X==1u) Z=V;
    else if(X<4u){
        const std::uint32_t M=V>>29;
        if(M<=1u) Z=V>>(X-2u);
        else{
            V&=0x1fffffffu;
            if(M>X+1u) Z=(std::uint64_t(1u)<<(M-X-2u)) | (V>>(X+31u-M));
            else Z=std::uint64_t(V|0x20000000u)<<(M-X+1u);
        }
    }else if(X<8u){
        const std::uint32_t M=V>>28;
        if(M<=1u) Z=V>>(X-3u);
        else{
            V&=0x0fffffffu;
            if(M>X+1u) Z=(std::uint64_t(1u)<<(M-X-2u)) | (V>>(X+30u-M));
            else { const int sh=int(M)-int(X)+2; Z=sh>=0 ? (std::uint64_t(V|0x10000000u)<<sh) : ((V|0x10000000u)>>(-sh)); }
        }
    }else if(X<16u){
        const std::uint32_t M=V>>27;
        if(M<=1u) Z=V>>(X-4u);
        else{
            V&=0x07ffffffu;
            if(M>X+1u) Z=(std::uint64_t(1u)<<(M-X-2u)) | (V>>(X+29u-M));
            else { const int sh=int(M)-int(X)+3; Z=sh>=0 ? (std::uint64_t(V|0x08000000u)<<sh) : ((V|0x08000000u)>>(-sh)); }
        }
    }else{
        const std::uint32_t M=V>>26;
        if(M<=1u) Z=V>>(X-5u);
        else{
            // This apparently asymmetric mask is intentional: it is the value
            // reported by the real-card exhaustive implementation.
            V&=0x07ffffffu;
            if(M>X+1u) Z=(std::uint64_t(1u)<<(M-X-2u)) | (V>>(X+28u-M));
            else { const int sh=int(M)-int(X)+4; Z=sh>=0 ? (std::uint64_t(V|0x04000000u)<<sh) : ((V|0x04000000u)>>(-sh)); }
        }
    }
    std::uint32_t out=static_cast<std::uint32_t>(Z);
    if(sign && out<0xfffffffcu) out^=0xffffffffu;
    return static_cast<std::int32_t>(out);
}

std::int32_t Engine::q31ToTank20(std::int32_t q31) noexcept {
    // The tank data path is 20 bits wide.  Later tracing of the Creative/ALSA
    // formatter path shows the low 12 DSP bits are discarded, not rounded.
    // Clearing them first also gives the hardware-like sign extension for
    // tiny negative values (for example -1 -> tank value -1).
    const std::uint32_t masked=static_cast<std::uint32_t>(q31)&0xfffff000u;
    const std::int32_t aligned=static_cast<std::int32_t>(masked);
    return aligned/4096; // exact because the low 12 bits are zero
}

std::int32_t Engine::tank20ToQ31(std::int32_t tank20) noexcept {
    tank20=std::clamp(tank20,-524288,524287);
    return static_cast<std::int32_t>(static_cast<std::int64_t>(tank20) << 12);
}

std::uint16_t Engine::tramRawWordFromTank20(std::int32_t x) noexcept {
    // Stock Linux EMU10K1 raw-IEC958 microcode reconstructs arbitrary physical
    // XTRAM words from the hardware-expanded 20-bit tank value with exactly:
    //   tmp = LOG(tank_q31 & 0xfffff000, 7, 0)
    //   raw = high16(tmp)
    //   if (raw is negative) raw ^= 0x7000
    // The driver expresses the conditional XOR as two ANDXORs separated by a
    // CC_REG_MINUS SKIP.  This is therefore no longer merely a guessed field
    // split: it is the codeword recovery law used by the stock raw-S/PDIF path.
    x=std::clamp(x,-524288,524287);
    const std::int32_t q=tank20ToQ31(x);
    const std::uint32_t encoded=static_cast<std::uint32_t>(logEncode(q,7u,0u));
    std::uint16_t raw=static_cast<std::uint16_t>(encoded>>16);
    if((raw&0x8000u)!=0u) raw=static_cast<std::uint16_t>(raw^0x7000u);
    return raw;
}

std::int32_t Engine::tramTank20RepresentativeFromRawWord(std::uint16_t code) noexcept {
    // Undo the physical negative-word exponent transform and reconstruct the
    // truncated FX8010 LOG(7,0) operand consumed by the decompressor.  Creative
    // US 5,930,158 specifies that EXP first one's-complements a negative LOG
    // operand, reconstructs the positive mantissa with all omitted low bits = 0,
    // then one's-complements the final linear value.  Therefore discarded bits
    // of a *negative* truncated LOG word must be restored as ones.  Restoring
    // zero for both signs (the 1.16 model) biased every multi-value negative
    // bucket away from zero.
    std::uint16_t log16=code;
    if((log16&0x8000u)!=0u) log16=static_cast<std::uint16_t>(log16^0x7000u);
    std::uint32_t encoded=std::uint32_t(log16)<<16;
    if((log16&0x8000u)!=0u) encoded|=0xffffu;
    const std::uint32_t expanded=static_cast<std::uint32_t>(
        expDecode(static_cast<std::int32_t>(encoded),7u,0u))&0xfffff000u;
    return static_cast<std::int32_t>(expanded)/4096;
}

TramTank20Bucket Engine::tramTank20BucketFromRawWord(std::uint16_t code) noexcept {
    const auto rep=tramTank20RepresentativeFromRawWord(code);
    const unsigned exponent=(std::uint32_t(code)>>12)&7u;
    const std::int32_t span=exponent<=1u ? 0 : static_cast<std::int32_t>((1u<<(exponent-1u))-1u);
    if((code&0x8000u)!=0u) return {rep-span,rep};
    return {rep,rep+span};
}

std::uint16_t Engine::tramEncode20Candidate(std::int32_t x) noexcept {
    return tramRawWordFromTank20(x);
}

std::int32_t Engine::tramDecode20Candidate(std::uint16_t code) noexcept {
    return tramTank20RepresentativeFromRawWord(code);
}

void Engine::execute(const Instruction&i) noexcept{
    auto A=readA(i.a,i.op),X=readOperand(i.x),Y=readOperand(i.y);std::int32_t R=0;bool sat=false,borrow=false,writeCcr=true,hardSaturation=false;
    switch(i.op){
    case 0:case 1:case 2:case 3:{
        // Fractional MACs can take ACCU as A specifically to retain the
        // accumulator's guard bits and low precision into the next MAC.
        // Collapsing ACCU through highAcc() first destroys that documented
        // 67-bit headroom (for example 1.5 - 0.6 must produce 0.9).
        const std::int64_t product=std::int64_t(X)*std::int64_t(Y);
        const std::int64_t signedProduct=(i.op&1)?-product:product;
        const Acc base=(i.a==ACCU)?acc_:accFromShift31(A);
        acc_=accAdd(base,accFromI64(signedProduct));
        // Decide saturation/wrap from the full guarded accumulator. Doing
        // this after an early >>31 can miss a one-LSB boundary on subtract MACs.
        const std::int64_t guarded=highAccRaw(acc_);
        const bool wordEvent=guarded>INT32_MAX || guarded<INT32_MIN;
        sat=wordEvent; hardSaturation=wordEvent && i.op<2;
        borrow=(i.op&1) && wordEvent;
        if(i.op<2){
            R=highAcc(acc_);
        }else{
            R=highAccWrap(acc_);
            if(i.a!=ACCU){
                // Real-card exhaustive MAC2/MAC3 measurements from michgz.
                // These wrap opcodes generate CCR from the unwrapped 32-bit
                // A + high-product expression, not from the generic result rule.
                const std::int64_t contribution=signedProduct>>31;
                const std::int64_t full=std::int64_t(A)+contribution;
                const auto outsidePositive=[](std::int64_t v) noexcept {return v>0x7fffffffll || v<0;};
                const bool measuredB = outsidePositive(contribution)
                    ? (outsidePositive(full)!=outsidePositive(A))
                    : (outsidePositive(full)==outsidePositive(A));
                const bool measuredS = full>0x7fffffffll || full<-0x7fffffffll;
                const bool measuredN = full>0x3fffffffll || full<-0x3fffffffll;
                std::uint32_t c=0u;
                if(R==0)c|=0x08u;
                if(R<0)c|=0x04u;
                if(measuredS)c|=0x10u;
                if(measuredN)c|=0x01u;
                if(measuredB)c|=0x02u;
                reg_[CCR]=static_cast<std::int32_t>(c);
                writeCcr=false;
                sat=measuredS; borrow=measuredB;
            }
        }
        break;}
    case 4:case 5:{
        // MACINTS/MACINTW use integer Y; no Q31 down-shift is performed.
        // Linux/Creative FX8010 documentation defines MACINTW/MACINT1 as
        // wraparound in a 31-bit result word.  Creative's chorus/flanger use
        // this opcode to fold a moving Q11 delay address into a non-negative
        // Q31 INTERP fraction; retaining bit 31 turns half of that sweep into
        // a negative interpolation coefficient and causes a periodic click.
        const std::int64_t full=std::int64_t(A)+std::int64_t(X)*std::int64_t(Y);
        acc_=accFromI64(full);
        // S means saturated OR wrapped. MACINTW/MACINT1 wraps in a 31-bit
        // result domain, so negative values have wrapped too even though they
        // are still representable in signed 32-bit C++. MACINTS retains the
        // ordinary signed-32 saturation boundaries.
        const bool wordEvent=(i.op==5) ? (full<0 || full>0x7fffffffll)
                                      : (full>INT32_MAX || full<INT32_MIN);
        sat=wordEvent; hardSaturation=wordEvent && i.op==4;
        // B is independent from S.  Creative Freak Shifter proves that MACINT0
        // can generate BORROW when a negative product makes the effective
        // operation a subtraction (abs(phase) + 4 * -1).  Model unsigned
        // underflow of the A word against that subtraction magnitude.
        const std::int64_t addend=std::int64_t(X)*std::int64_t(Y);
        borrow = addend < 0 && std::uint64_t(static_cast<std::uint32_t>(A)) < std::uint64_t(-addend);
        if(i.op==4) R=sat32(full);
        else R=static_cast<std::int32_t>(static_cast<std::uint32_t>(full)&0x7fffffffu);
        break;}
    case 6:{
        // ACC3 is a fractional three-input accumulate in the normal audio
        // path.  Like the fractional MACs, ACCU as A must reuse the full
        // guarded accumulator rather than its already-saturated 32-bit view.
        // Ordinary operands are aligned into the high/fractional half.
        Acc sum=(i.a==ACCU)?acc_:accFromShift31(A);
        sum=accAdd(sum,accFromShift31(X));
        sum=accAdd(sum,accFromShift31(Y));
        acc_=sum;
        const auto guarded=highAccRaw(acc_);
        R=highAcc(acc_);
        sat=guarded>INT32_MAX || guarded<INT32_MIN; hardSaturation=sat;
        break;}
    case 7:{
        // MACMV moves A to R while accumulating X*Y into the existing 67-bit
        // accumulator.  The guard/LS bits are intentionally retained.
        R=A;acc_=accAdd(acc_,accFromI64(std::int64_t(X)*std::int64_t(Y)));break;}
    case 8:{
        R=(std::int32_t)(((std::uint32_t)A&(std::uint32_t)X)^(std::uint32_t)Y);
        acc_=accFromShift31(R);
        // US 5,930,158 saves only Minus/Zero/Normalized for ANDXOR.
        updateCCRMasked(R,false,false,0x0du); writeCcr=false;
        break;}
    case 9:{
        const bool cmpBorrow=A<Y;
        R=A>=Y?X:(std::int32_t)~(std::uint32_t)X;
        acc_=accFromShift31(R);
        // Real-card iTSTNEG vectors show a selective CCR write: Z/M from R and
        // B from the A-Y comparison; N and S are clear.
        std::uint32_t c=0u;
        if(R==0)c|=0x08u;
        if(R<0)c|=0x04u;
        if(cmpBorrow)c|=0x02u;
        reg_[CCR]=static_cast<std::int32_t>(c);
        writeCcr=false;
        break;}
    case 10:R=A>=Y?X:Y;acc_=accFromShift31(R);break;
    case 11:R=A<Y?X:Y;acc_=accFromShift31(R);break;
    case 12:{
        R=logEncode(A,(std::uint32_t)X,(std::uint32_t)Y);acc_=accFromShift31(R);
        // Hardware-tested iLOG CCR behavior.  Bit0 is the LOG normalization
        // threshold flag, bit1 is set by this opcode, plus ordinary M/Z.
        const std::uint32_t ux=static_cast<std::uint32_t>(X)%32u;
        std::uint32_t mag=static_cast<std::uint32_t>(A);
        if(mag>=0x80000000u)mag=(mag&0x7fffffffu)^0x7fffffffu;
        bool n=false; if(mag!=0u)n=floorLog2(mag)>=static_cast<int>(31u-ux);
        std::uint32_t c=0x02u; if(n)c|=0x01u; if(R==0)c|=0x08u; if(R<0)c|=0x04u;
        reg_[CCR]=static_cast<std::int32_t>(c);
        writeCcr=false;
        break;}
    case 13:{
        R=expDecode(A,(std::uint32_t)X,(std::uint32_t)Y);acc_=accFromShift31(R);
        // US 5,930,158 saves only Minus/Zero/Normalized for EXP.  LOG is not
        // folded into this rule because real-card vectors contradict the
        // patent's preferred-embodiment LOG CCR description (hardware sets B).
        updateCCRMasked(R,false,false,0x0du); writeCcr=false;
        break;}
    case 14:{
        // INTERP R=(1-X)*A + X*Y = A + X*(Y-A).  Preserve the full product
        // in ACC so a following instruction can address ACCU without losing
        // the LS precision/guard bits.
        const std::int64_t diff=std::int64_t(Y)-std::int64_t(A);
        acc_=accAdd(accFromShift31(A),accFromI64(std::int64_t(X)*diff));
        R=highAcc(acc_);
        const auto guarded=highAccRaw(acc_);
        sat=guarded>INT32_MAX || guarded<INT32_MIN; hardSaturation=sat;
        break;}
    case 15:{
        // Hardware form measured on real EMU10K1: A supplies GPR_COND for
        // conditional branches, X is the 32-bit boolean condition word, and Y
        // is the skip count.  This decoder also covers compiler-generated masks
        // such as 0x180 (>0), 0x1008 (<=0), 0x200 (!S), and 0x7fffffff.
        R=A;
        const auto cc=static_cast<std::uint32_t>(A);
        const auto test=static_cast<std::uint32_t>(X);
        const unsigned count=static_cast<unsigned>(static_cast<std::uint32_t>(Y)&0x3ffu);
        skip_=skipConditionWord(cc,test)?count:0u;
        acc_=accFromShift31(R);
        writeCcr=false;
        break;}
    default:return;}
    // Noise sources and DBAC are hardware read-only result destinations.
    // GPR_IRQ is different: it is a write-side-effect register. Linux's stock
    // EMU10K1 PCM FX program raises IPR_FXDSP by writing C_80000000 here.
    // Latch only that evidenced high-bit form; do not invent semantics for
    // other values. The special register itself remains read-as-zero.
    if(hardSaturation){saturationOccurred_=true;saturationAddress_=currentPc_ & 0x01ffu;}
    if(cardMode_ && i.r>=0x20u && i.r<0x40u) cardOutputWritten_[i.r-0x20u]=true;
    if(i.r==IRQ){
        if((static_cast<std::uint32_t>(R)&0x80000000u)!=0u){irqPending_=true;++irqCount_;}
    } else if(i.r!=NOISE0 && i.r!=NOISE1 && i.r!=DBAC) {
        set(i.r,R);
    }
    if(writeCcr)updateCCR(R,sat,borrow);
}

void Engine::tramReadService(TramDecl& t,bool physicalAlign) noexcept{
    const auto ar=std::uint16_t(t.dataReg+1);
    // Tank hardware ignores address bits 0..10; the DSP still sees them and can
    // explicitly derive a fractional-delay INTERP coefficient.
    const std::uint32_t rel=(static_cast<std::uint32_t>(get(ar))>>11)&0x000fffffu;
    const std::uint32_t absolute=tramPhysicalReadAddress(rel,dbac_,physicalAlign);
#ifdef SBLIVE_TESTING
    debugLastTramRelative_[t.dataReg]=rel;
#endif
    auto&mem=t.external?xtramMem_:itramMem_;
    if(mem.empty()){reg_[t.dataReg]=0;return;}
    if(program_.linearTramCompatibility){
        auto&linear=t.external?xtramLinearCompat_:itramLinearCompat_;
        reg_[t.dataReg]=linear[std::size_t(absolute)%linear.size()];
    }else{
        reg_[t.dataReg]=tank20ToQ31(tramTank20RepresentativeFromRawWord(mem[std::size_t(absolute)%mem.size()]));
    }
}

void Engine::tramWriteService(TramDecl& t,bool physicalAlign) noexcept{
    const auto ar=std::uint16_t(t.dataReg+1);
    const std::uint32_t rel=(static_cast<std::uint32_t>(get(ar))>>11)&0x000fffffu;
    const std::uint32_t absolute=tramPhysicalWriteAddress(rel,dbac_,physicalAlign);
#ifdef SBLIVE_TESTING
    debugLastTramRelative_[t.dataReg]=rel;
#endif
    auto&mem=t.external?xtramMem_:itramMem_;
    if(mem.empty())return;
    if(program_.linearTramCompatibility){
        auto&linear=t.external?xtramLinearCompat_:itramLinearCompat_;
        linear[std::size_t(absolute)%linear.size()]=get(t.dataReg);
    }else{
        mem[std::size_t(absolute)%mem.size()]=tramRawWordFromTank20(q31ToTank20(get(t.dataReg)));
    }
}

unsigned Engine::tramWriteServicePc(bool external,unsigned slot) noexcept{
    // Equality is classified as "already serviced" by the recovered loader:
    // ITRAM write ALIGN iff pc >= 3*slot, XTRAM iff pc >= 128+4*slot.
    return external ? 128u+4u*slot : 3u*slot;
}
unsigned Engine::tramReadServicePc(bool external,unsigned slot) noexcept{
    // Read ALIGN iff consumer is still before the fresh-data boundary:
    // ITRAM pc <= 3*slot+1 => service before pc 3*slot+2;
    // XTRAM pc <= 127+4*slot => service before pc 128+4*slot.
    return external ? 128u+4u*slot : 3u*slot+2u;
}

void Engine::tramBegin() noexcept{
    // Legacy/sample-level compatibility path: every read is serviced before the
    // DSP and every write after it. Native RIFX programs use the interleaved
    // sequencer below instead.
    reg_[DBAC]=static_cast<std::int32_t>((dbac_&0x000fffffu)<<11);
    for(auto&t:tram_)if(!t.write) tramReadService(t,false);
}
void Engine::tramEnd() noexcept{
    for(auto&t:tram_)if(t.write) tramWriteService(t,false);
    dbac_=(dbac_-1u)&0x000fffffu;
}

void Engine::tramScheduledBegin() noexcept{
    reg_[DBAC]=static_cast<std::int32_t>((dbac_&0x000fffffu)<<11);
}

void Engine::tramScheduledServiceBefore(unsigned pc) noexcept{
    // The TRAM sequencer is independent of DSP control flow.  Service every
    // access whose fixed hardware slot boundary has been crossed, including
    // boundaries in instructions skipped by SKIP.  Each declaration is serviced
    // exactly once per sample; a small per-sample bitset would be overkill here,
    // so callers invoke this on monotonically increasing PC and we key on the
    // exact boundary. Skipped ranges are expanded by runSample().
    for(auto&t:tram_){
        const unsigned event=t.write?tramWriteServicePc(t.external,t.hardwareSlot)
                                    :tramReadServicePc(t.external,t.hardwareSlot);
        if(event!=pc)continue;
        if(t.write)tramWriteService(t,t.align);
        else tramReadService(t,t.align);
    }
}

void Engine::tramScheduledFinish() noexcept{
    // Hardware continues its fixed tank schedule after the last active DSP
    // instruction. Service any slots whose boundary lies in the unused tail of
    // the 512-instruction frame before advancing DBAC.
    const unsigned begin=static_cast<unsigned>(std::min<std::size_t>(code_.size(),512u));
    for(unsigned pc=begin;pc<512u;++pc)tramScheduledServiceBefore(pc);
    dbac_=(dbac_-1u)&0x000fffffu;
}

void Engine::runCardSample(const std::array<std::int32_t,16>& fxbus,
                           const std::array<std::int32_t,16>& extin,
                           std::array<std::int32_t,32>& outputs){
    for(unsigned i=0;i<16;++i){reg_[0x00u+i]=fxbus[i];reg_[0x10u+i]=extin[i];}
    cardMode_=true;
    cardOutputWritten_.fill(false);
    runSample();
    for(unsigned i=0;i<16;++i){
        outputs[i]=cardOutputWritten_[i]?reg_[0x20u+i]:fxbus[i];
        outputs[16u+i]=cardOutputWritten_[16u+i]?reg_[0x30u+i]:extin[i];
    }
    cardMode_=false;
}

void Engine::runSample(){
    // Both registers refresh once per sample. NOISE1's 4097-sample relationship
    // remains a compatibility hypothesis from the surviving project lineage,
    // not a bit-exact silicon claim; the exact generator/relationship still
    // requires primary-source or card confirmation.
    const std::int32_t n0=noiseQ31(noiseState0_);
    const std::int32_t n1=noiseDelay_[noiseDelayPos_];
    noiseDelay_[noiseDelayPos_]=n0;
    noiseDelayPos_=(noiseDelayPos_+1u)%kNoiseDelaySamples;
    reg_[NOISE0]=n0;
    reg_[NOISE1]=n1;
    if(!program_.scheduledTramSequencer){
        tramBegin();
        for(std::size_t pc=0;pc<code_.size();){skip_=0;currentPc_=static_cast<std::uint16_t>(pc);execute(code_[pc]);pc+=1+skip_;}
        tramEnd();
        return;
    }

    tramScheduledBegin();
    std::size_t pc=0;
    while(pc<code_.size() && pc<512u){
        tramScheduledServiceBefore(static_cast<unsigned>(pc));
        skip_=0; currentPc_=static_cast<std::uint16_t>(pc); execute(code_[pc]);
        const std::size_t next=std::min<std::size_t>(pc+1u+skip_,code_.size());
        // SKIP changes which instructions execute, not the free-running TRAM
        // slot cadence. Walk crossed slot boundaries without executing skipped DSP.
        for(std::size_t q=pc+1u;q<next && q<512u;++q)tramScheduledServiceBefore(static_cast<unsigned>(q));
        pc=next;
    }
    // Service the remainder of the fixed frame. If code_.size()<512 this begins
    // at code_.size(); if the program fills the frame, the loop is empty.
    tramScheduledFinish();
}

void Engine::setTramAddressRaw(std::uint16_t d,std::int32_t a) noexcept{set(std::uint16_t(d+1),a);}
void Engine::setTramAddress(std::uint16_t d,double samples) noexcept{samples=std::max(0.0,samples);auto raw=(std::int64_t)std::llround(samples*2048.0);setTramAddressRaw(d,sat32(raw));}

std::uint32_t Engine::debugRegisterWord() const noexcept {
    // Linux/ALSA EMU10K1 DBG layout:
    // bit25 saturation occurred, bits24..16 saturation instruction address,
    // bits13..9 current five-bit condition code.  Single-step fields are zero
    // because this sample-level interpreter does not claim cycle stepping.
    std::uint32_t w=0u;
    if(saturationOccurred_) w|=0x02000000u;
    w|=(std::uint32_t(saturationAddress_&0x01ffu)<<16);
    w|=(static_cast<std::uint32_t>(reg_[CCR])&0x1fu)<<9;
    return w;
}

void Engine::writeDebugRegister(std::uint32_t word) noexcept {
    // EMU10K1_DBG_ZC is the one documented write-side effect we can model
    // without inventing single-step timing: writing bit31 zeros the TRAM DBAC.
    if((word&0x80000000u)!=0u){dbac_=0u;reg_[DBAC]=0;}
}

std::uint32_t Engine::encodeTramHardwareControl(const TramHardwareControl& c) noexcept {
    std::uint32_t w=c.address&0x000fffffu;
    if(c.clear) w|=0x00800000u;
    if(c.align) w|=0x00400000u;
    if(c.write) w|=0x00200000u;
    if(c.read)  w|=0x00100000u;
    return w;
}

TramHardwareControl Engine::decodeTramHardwareControl(std::uint32_t w) noexcept {
    TramHardwareControl c{};
    c.address=w&0x000fffffu;
    c.clear=(w&0x00800000u)!=0u;
    c.align=(w&0x00400000u)!=0u;
    c.write=(w&0x00200000u)!=0u;
    c.read=(w&0x00100000u)!=0u;
    return c;
}

TramAccessMode Engine::tramAccessMode(const TramHardwareControl& c) noexcept {
    if(c.read && c.write) return TramAccessMode::ReadSumWrite;
    if(c.read) return TramAccessMode::Read;
    if(c.write) return TramAccessMode::Write;
    return TramAccessMode::Off;
}

TramRsaw20Result Engine::tramInternalRsaw20(std::int32_t memoryTank20,
                                                    std::int32_t dataBufferTank20,
                                                    bool clearForcedZero) noexcept {
    // US 6,032,235 is unusually explicit about RSAW sequencing: fetch the
    // addressed internal-TRAM sample, sum it with the corresponding data buffer,
    // and write that sum back to the same address.  What it does *not* specify is
    // whether an overflowing 20-bit tank sum wraps or saturates, so expose both
    // outcomes and let future silicon vectors collapse the ambiguity.
    TramRsaw20Result r{};
    r.memoryBefore=std::clamp(memoryTank20,-524288,524287);
    r.dataBuffer=std::clamp(dataBufferTank20,-524288,524287);
    r.mathematicalSum=static_cast<std::int64_t>(r.memoryBefore)+static_cast<std::int64_t>(r.dataBuffer);
    r.overflow=(r.mathematicalSum < -524288ll || r.mathematicalSum > 524287ll);
    const std::uint32_t raw20=static_cast<std::uint32_t>(r.mathematicalSum)&0x000fffffu;
    r.wrap20=(raw20&0x00080000u)!=0u
        ? static_cast<std::int32_t>(raw20|0xfff00000u)
        : static_cast<std::int32_t>(raw20);
    r.saturate20=static_cast<std::int32_t>(std::clamp<std::int64_t>(r.mathematicalSum,-524288ll,524287ll));
    r.clearForcedZero=clearForcedZero;
    r.effectiveWriteIfWrap=clearForcedZero ? 0 : r.wrap20;
    r.effectiveWriteIfSaturate=clearForcedZero ? 0 : r.saturate20;
    r.rawWordIfWrap=tramRawWordFromTank20(r.effectiveWriteIfWrap);
    r.rawWordIfSaturate=tramRawWordFromTank20(r.effectiveWriteIfSaturate);
    return r;
}

std::uint32_t Engine::tramPhysicalReadAddress(std::uint32_t relative,
                                               std::uint32_t dbac,
                                               bool align) noexcept {
    // WO1999001953: a read-side ALIGN compensates stale-buffer phase by
    // addressing one sample earlier than the unaligned DBAC-relative address.
    return (relative + dbac - (align ? 1u : 0u)) & 0x000fffffu;
}

std::uint32_t Engine::tramPhysicalWriteAddress(std::uint32_t relative,
                                                std::uint32_t dbac,
                                                bool align) noexcept {
    // The write-side hazard is the mirror image: ALIGN advances one sample.
    return (relative + dbac + (align ? 1u : 0u)) & 0x000fffffu;
}

std::array<unsigned,2> Engine::xtramCachesSelected(std::uint32_t sampleCounter) noexcept {
    // US 6,275,899 specific embodiment: a 4-bit sample counter selects cache
    // pairs 0/1, 2/3, ... 30/31 in round-robin order.
    const unsigned phase=static_cast<unsigned>(sampleCounter&0x0fu);
    return {phase*2u,phase*2u+1u};
}

int Engine::xtramReadBurstStartAdjustment(std::uint32_t currentAddress) noexcept {
    // Main-memory delay addressing proceeds downward while PCI bursts upward.
    // Start at an even 32-bit boundary: even current -> -18 samples, odd -> -17.
    return (currentAddress&1u)!=0u ? -17 : -18;
}

int Engine::xtramWriteBurstStartAdjustment(std::uint32_t currentAddress) noexcept {
    // Post-write bursts likewise begin on an even 32-bit PCI boundary.
    return (currentAddress&1u)!=0u ? 1 : 0;
}

void XtramServiceFifo::reset() noexcept {
    active_.fill(false); write_.fill(false); currentAddress_.fill(0);
    fifo_.fill({}); head_=size_=0; accepted_=dropped_=completed_=0;
    cancelled_=lockSuppressed_=0; tankCacheLocked_=false;
    tankMappingValid_=false; tankBase_=0; tankSizeCode_=0; mappingGeneration_=0;
}

void XtramServiceFifo::setTankCacheLocked(bool locked) noexcept {
    if(locked==tankCacheLocked_) return;
    if(locked){
        // HCFG_LOCKTANKCACHE is documented as cancelling tank-cache busmaster
        // accesses.  Discard every outstanding request from the old DMA mapping
        // at the lock edge, matching the Creative/ALSA rebind sequence which may
        // immediately free the old coherent allocation after asserting the bit.
        cancelled_ += size_;
        fifo_.fill({}); head_=0; size_=0;
    }
    tankCacheLocked_=locked;
}

void XtramServiceFifo::programTankMapping(std::uint32_t tcbBase, unsigned tcbsCode, bool valid) noexcept {
    tcbsCode &= 7u;
    if(tankBase_==tcbBase && tankSizeCode_==tcbsCode && tankMappingValid_==valid) return;
    tankBase_=tcbBase; tankSizeCode_=tcbsCode; tankMappingValid_=valid;
    ++mappingGeneration_;
}

void XtramServiceFifo::configure(unsigned cache, bool active, bool write, std::uint32_t currentAddress) noexcept {
    if(cache>=active_.size()) return;
    active_[cache]=active; write_[cache]=write; currentAddress_[cache]=currentAddress&0x000fffffu;
}

XtramAdmissionEnvelope XtramServiceFifo::admissionEnvelope(std::uint64_t sampleCounter) const noexcept {
    XtramAdmissionEnvelope e{};
    e.selected=Engine::xtramCachesSelected(static_cast<std::uint32_t>(sampleCounter));
    for(unsigned cache:e.selected) if(cache<active_.size() && active_[cache]) ++e.active;
    e.freeEntries=static_cast<unsigned>(fifo_.size())-size_;
    if(tankCacheLocked_){
        e.enqueued=0u; e.dropped=0u;
    }else{
        e.enqueued=std::min(e.active,e.freeEntries);
        e.dropped=e.active-e.enqueued;
        e.identityAmbiguous=(e.active==2u && e.freeEntries==1u);
    }
    return e;
}

XtramServiceSelection XtramServiceFifo::select(std::uint64_t sampleCounter, XtramPairOrder order) noexcept {
    XtramServiceSelection out{};
    out.selected=Engine::xtramCachesSelected(static_cast<std::uint32_t>(sampleCounter));
    for(unsigned i=0;i<2u;++i){
        const unsigned cache=out.selected[i];
        auto& d=out.decisions[i];
        d.cache=cache;
        d.selectedSample=sampleCounter;
        d.headroomDeadlineSample=sampleCounter+patentLatencySlackSamples();
        d.retrySelectionSample=nextSelectionAfter(cache,sampleCounter);
        if(cache<active_.size()){
            d.active=active_[cache]; d.write=write_[cache]; d.currentAddress=currentAddress_[cache];
            const int adjust=d.write ? Engine::xtramWriteBurstStartAdjustment(d.currentAddress)
                                     : Engine::xtramReadBurstStartAdjustment(d.currentAddress);
            d.burstStart=static_cast<std::uint32_t>((std::int64_t(d.currentAddress)+adjust)&0x000fffffll);
        }
    }
    std::array<unsigned,2> visit=out.selected;
    if(order==XtramPairOrder::UpperThenLower) std::swap(visit[0],visit[1]);
    for(unsigned cache:visit){
        const unsigned di=(cache==out.selected[0])?0u:1u;
        auto& d=out.decisions[di];
        if(cache>=active_.size() || !active_[cache]) continue;
        if(tankCacheLocked_){
            d.blockedByTankLock=true; ++lockSuppressed_; ++out.blockedByTankLock;
            continue;
        }
        XtramServiceRequest req{};
        req.cache=static_cast<std::uint8_t>(cache);
        req.write=write_[cache];
        req.currentAddress=currentAddress_[cache];
        req.burstStart=d.burstStart;
        req.selectedSample=sampleCounter;
        req.headroomDeadlineSample=d.headroomDeadlineSample;
        req.retrySelectionSample=d.retrySelectionSample;
        req.mappingGeneration=mappingGeneration_;
        if(size_==fifo_.size()){ ++dropped_; ++out.dropped; d.dropped=true; continue; }
        const unsigned tail=(head_+size_)%fifo_.size();
        fifo_[tail]=req; ++size_; ++accepted_; ++out.enqueued; d.admitted=true;
    }
    return out;
}

bool XtramServiceFifo::completeOne(XtramServiceRequest* completed) noexcept {
    if(size_==0) return false;
    if(completed) *completed=fifo_[head_];
    head_=(head_+1u)%fifo_.size(); --size_; ++completed_; return true;
}

bool XtramServiceFifo::completeOneAt(std::uint64_t completedSample, XtramServiceCompletion* completed) noexcept {
    XtramServiceRequest r{};
    if(!completeOne(&r)) return false;
    if(completed){
        completed->request=r;
        completed->completedSample=completedSample;
        const std::uint64_t age64=completedSample>r.selectedSample ? completedSample-r.selectedSample : 0u;
        completed->ageSamples=age64>0xffffffffull ? 0xffffffffu : static_cast<unsigned>(age64);
        const std::uint64_t late64=completedSample>r.headroomDeadlineSample ? completedSample-r.headroomDeadlineSample : 0u;
        completed->beyondHeadroomSamples=late64>0xffffffffull ? 0xffffffffu : static_cast<unsigned>(late64);
        completed->beyondHeadroom=late64!=0u;
    }
    return true;
}

unsigned XtramServiceFifo::complete(unsigned count) noexcept {
    unsigned done=0; while(done<count && completeOne(nullptr)) ++done; return done;
}

unsigned XtramServiceFifo::completeAt(std::uint64_t completedSample, unsigned count, unsigned* late) noexcept {
    unsigned done=0, lateCount=0;
    XtramServiceCompletion c{};
    while(done<count && completeOneAt(completedSample,&c)){ if(c.beyondHeadroom) ++lateCount; ++done; }
    if(late) *late=lateCount;
    return done;
}

const XtramServiceRequest* XtramServiceFifo::front() const noexcept {
    return size_?&fifo_[head_]:nullptr;
}

unsigned XtramServiceFifo::oldestAgeSamples(std::uint64_t now) const noexcept {
    if(size_==0) return 0u;
    const auto then=fifo_[head_].selectedSample;
    if(now<=then) return 0u;
    const auto age=now-then;
    return age>0xffffffffull ? 0xffffffffu : static_cast<unsigned>(age);
}

XtramServiceTimingState XtramServiceFifo::timingState(std::uint64_t now) const noexcept {
    XtramServiceTimingState s{};
    s.depth=size_;
    for(unsigned i=0;i<size_;++i){
        const auto& r=fifo_[(head_+i)%fifo_.size()];
        const std::uint64_t age64=now>r.selectedSample ? now-r.selectedSample : 0u;
        const unsigned age=age64>0xffffffffull ? 0xffffffffu : static_cast<unsigned>(age64);
        if(age>s.oldestAge) s.oldestAge=age;
        if(age<=patentLatencySlackSamples()) ++s.withinLatencyBudget;
        else ++s.beyondLatencyBudget;
        if(age==patentLatencySlackSamples()) ++s.atLatencyLimit;
    }
    return s;
}

std::uint64_t XtramServiceFifo::nextSelectionAfter(unsigned cache, std::uint64_t selectedSample) noexcept {
    // The patented 4-bit round-robin counter selects pair floor(cache/2) once
    // every 16 samples.  This is the strictly-later selection, matching the
    // patent statement that a dropped request waits an additional 16 periods.
    if(cache>=32u) return selectedSample;
    const unsigned target=(cache>>1u)&0x0fu;
    const std::uint64_t start=selectedSample+1u;
    const unsigned phase=static_cast<unsigned>(start&0x0fu);
    const unsigned delta=(target+16u-phase)&0x0fu;
    return start+delta;
}

void TramClearController::reset(std::uint32_t zeroedSamples) noexcept {
    slots_.fill(Slot{});
    zeroedSamples_=zeroedSamples;
}

void TramClearController::configure(unsigned slot, TramClearKind kind,
                                    std::uint32_t countdownLength, bool enabled) noexcept {
    if(slot>=slots_.size()) return;
    slots_[slot].kind=kind;
    slots_[slot].countdownLength=countdownLength;
    slots_[slot].enabled=enabled && kind!=TramClearKind::Off;
}

void TramClearController::disable(unsigned slot) noexcept {
    if(slot>=slots_.size()) return;
    slots_[slot]=Slot{};
}

void TramClearController::advanceSample() noexcept {
    if(zeroedSamples_!=0xffffffffu) ++zeroedSamples_;
}

TramClearSlotState TramClearController::state(unsigned slot) const noexcept {
    TramClearSlotState out{};
    if(slot>=slots_.size()) return out;
    const Slot& s=slots_[slot];
    out.kind=s.kind;
    out.countdownLength=s.countdownLength;
    out.enabled=s.enabled;
    out.clearActive=s.enabled && zeroedSamples_<s.countdownLength;
    out.readReturnsZero=out.clearActive && s.kind==TramClearKind::Read;
    out.rsawForcesZeroWrite=out.clearActive && s.kind==TramClearKind::InternalRsaw;
    // US6032235 explicitly blocks microinstruction writes to the corresponding
    // data buffer only during CLR-active RSAW initialization.
    out.microcodeDataBufferWriteAllowed=!out.rsawForcesZeroWrite;
    return out;
}

bool Engine::tramClearReadReturnsZero(std::uint32_t zeroedSamples,
                                      std::uint32_t delayLength) noexcept {
    // US6032235: CLR remains active while the global zeroed-sample count is
    // strictly below the delay length; equality switches the slot to valid RD.
    return zeroedSamples < delayLength;
}

bool Engine::tramClearRsawForcesZero(std::uint32_t zeroedSamples,
                                     std::uint32_t countdownLength) noexcept {
    // During RSAW initialization, stale RAM must not be accumulated. The same
    // countdown relation forces a zero memory write until the path is valid.
    return zeroedSamples < countdownLength;
}

std::uint32_t Engine::tramBuffer20(std::uint32_t value) noexcept {
    return value & 0x000fffffu;
}

std::int32_t Engine::approximatePlayback16ToBus(std::int16_t sample,
                                                 std::uint16_t volume,
                                                 std::uint8_t sendVolume) noexcept {
    // Measured EMU10K1 playback relation: input * volume * send / 1024,
    // followed by loss of the low 8 bits. Use floor-style fixed-point shifts
    // for negatives so this mirrors clearing the low byte in two's-complement.
    const std::int64_t product=std::int64_t(sample)*std::int64_t(volume)*std::int64_t(sendVolume);
    std::int64_t scaled=product>=0 ? product/1024 : -(((-product)+1023)/1024);
    scaled=scaled>=0 ? (scaled/256)*256 : -(((-scaled)+255)/256)*256;
    return sat32(scaled);
}

std::int16_t Engine::capture16FromBus(std::int32_t sample) noexcept {
    const std::int64_t v=sample;
    const std::int64_t top=v>=0 ? v/65536 : -(((-v)+65535)/65536);
    return static_cast<std::int16_t>(std::clamp<std::int64_t>(top,-32768,32767));
}

bool patchSite(std::vector<Instruction>&c,std::uint16_t s,std::uint16_t r) noexcept{std::size_t i=s&0xfff;if(i>=c.size())return false;switch(s&0xf000){case 0x2000:c[i].a=r;return true;case 0x4000:c[i].x=r;return true;case 0x6000:c[i].y=r;return true;case 0x8000:c[i].r=r;return true;default:return false;}}
}
