#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

static std::uint32_t livewareBalance(std::uint16_t left,std::uint16_t right,std::uint32_t max) noexcept {
    // CTSURMIX.EXE 0x40A0A1 ("GetBalance Left/Right") recovered integer law.
    if(left>right) return left ? (std::uint32_t(right)*max)/(2u*std::uint32_t(left)) : 0u;
    if(left<right) return right ? max-(std::uint32_t(left)*max)/(2u*std::uint32_t(right)) : max;
    return max>>1;
}
static float f32(std::uint32_t u){float f{};std::memcpy(&f,&u,4);return f;}
int main(){
    bool ok=true;
    ok &= livewareBalance(100,100,1000)==500;
    ok &= livewareBalance(100,0,1000)==0;
    ok &= livewareBalance(0,100,1000)==1000;
    ok &= livewareBalance(100,50,1000)==250;
    ok &= livewareBalance(50,100,1000)==750;
    ok &= livewareBalance(111,52,1110)==260;
    ok &= livewareBalance(52,111,1110)==850;

    // DEVCON32 stores this exact float bit pattern when Balance/Fade is absent.
    const float unset=f32(0x41b85fd9u);
    ok &= std::fabs(unset-23.0468006134f)<1.0e-5f;
    ok &= unset>1.0f; // clearly outside normalized placement range

    // One shipped Forsaken .sea file contains real normalized placement values.
    ok &= std::fabs(f32(0x3ef914c2u)-0.48648649f)<1.0e-6f;
    ok &= std::fabs(f32(0x3eee5847u)-0.46551725f)<1.0e-6f;
    ok &= std::fabs(f32(0x3f3dcb09u)-0.74137932f)<1.0e-6f;

    std::printf("balance(100,50)=%u balance(50,100)=%u unset=%.7f ok=%d\n",
        livewareBalance(100,50,1000),livewareBalance(50,100,1000),unset,ok?1:0);
    return ok?0:1;
}
