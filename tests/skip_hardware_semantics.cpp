#include "fx8010_engine.h"
#include <cstdio>
using namespace fx8010;

static bool nonzeroSkipsOne(){
    Program p;
    p.gprInit={{0x8000,0x12345678},{0x8001,0x11111111},{0x8002,0x22222222}};
    p.code.push_back({0,0x8000,0x40,0x40,0x8003});
    p.code.push_back({15,0x57,0x48,0x41,0x8004}); // X=0x100 (!Z), Y=1
    p.code.push_back({0,0x8001,0x40,0x40,0x8005});
    p.code.push_back({0,0x8002,0x40,0x40,0x8006});
    Engine e;e.load(p);e.runSample();
    return e.get(0x8005)==0 && e.get(0x8006)==0x22222222;
}

static bool zeroDoesNotSkipNonzero(){
    Program p;
    p.gprInit={{0x8001,0x11111111}};
    p.code.push_back({0,0x40,0x40,0x40,0x8003});
    p.code.push_back({15,0x57,0x48,0x41,0x8004});
    p.code.push_back({0,0x8001,0x40,0x40,0x8005});
    Engine e;e.load(p);e.runSample();
    return e.get(0x8005)==0x11111111;
}

static bool pitchNoSaturationSkipsReset(){
    Program p;
    // Real-card iSKIP decoder: 0x200 is inverted CCR bit4, i.e. !S.
    p.gprInit={{0x57,0x00000000},{0x8006,0x200},{0x8003,0x11111111},{0x8004,0x22222222}};
    p.code.push_back({15,0x57,0x8006,0x42,0x8007});
    p.code.push_back({0,0x8003,0x40,0x40,0x8008});
    p.code.push_back({0,0x8004,0x40,0x40,0x8009});
    Engine e;e.load(p);e.runSample();
    return e.get(0x8008)==0 && e.get(0x8009)==0;
}

static bool saturationAllowsReset(){
    Program p;
    p.gprInit={{0x57,0x00000010},{0x8006,0x200},{0x8003,0x11111111},{0x8004,0x22222222}};
    p.code.push_back({15,0x57,0x8006,0x42,0x8007});
    p.code.push_back({0,0x8003,0x40,0x40,0x8008});
    p.code.push_back({0,0x8004,0x40,0x40,0x8009});
    Engine e;e.load(p);e.runSample();
    return e.get(0x8008)==0x11111111 && e.get(0x8009)==0x22222222;
}

int main(){
    const bool a=nonzeroSkipsOne(),b=zeroDoesNotSkipNonzero(),c=pitchNoSaturationSkipsReset(),d=saturationAllowsReset();
    std::printf("nonzero_skip=%d zero_no_skip=%d pitch_no_S=%d pitch_S_reset=%d\n",a,b,c,d);
    return (a&&b&&c&&d)?0:2;
}
