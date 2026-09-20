#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
using namespace fx8010;
struct H{const char* n;std::uint64_t h;};
static std::uint64_t fnv(const std::vector<unsigned char>&b){std::uint64_t h=0xcbf29ce484222325ULL;for(auto x:b){h^=x;h*=0x100000001b3ULL;}return h;}
static const H expected[]={
{"2HeadphoneSpatializer.rifx",0xcdf2d640a3ea2fd6ULL},{"Auto_Wah.rifx",0x3e7526c5a84af20eULL},{"Chorus.rifx",0x17b7e639066f07ceULL},{"FDNReverb_EAX2_0_version.rifx",0xe4a99578b56bb67fULL},{"Formant_Filter.rifx",0x711454eba360725fULL},{"Freak_Shifter.rifx",0x739eeb60aaa1d992ULL},{"Fuzz.rifx",0x20569a0c4399475aULL},{"Panning_Echo.rifx",0x05ec39976de29d69ULL},{"Parametric_EQ.rifx",0xcfd07174d1a49bf9ULL},{"Peak_Meter.rifx",0x84718ae641d2e719ULL},{"PitchShift.rifx",0x515767b44faf79d3ULL},{"Record_Dither.rifx",0x6fedbffa0c4c7bddULL},{"RingMod.rifx",0xb4fc3bb05fda70e4ULL},{"SB2Mixer.rifx",0xfc4d5ba93b8e244dULL},{"SB2SpeakerSpatializer.rifx",0x6f590361725ceb1dULL},{"SB4Mixer.rifx",0x97a86d078a00d0afULL},{"SB4SpeakerSpatializer.rifx",0x7514f7ac2f1777e7ULL},{"SBLTone4.rifx",0x3f401976899db386ULL},{"SBLTone4_2.rifx",0x17780f39d199d203ULL},{"Stereo_Flanger.rifx",0x3a61929c072724baULL},{"SurroundSpatializer.rifx",0xbc3d83e5b6652c8dULL},{"Surround_Encoder.rifx",0x4fa51ac09e46b54bULL},{"ac3pass.rifx",0x2f86e2cf7a7cadeeULL}};
int main(int argc,char**argv){if(argc!=3)return 2;std::string d=argv[1],later=argv[2];for(const auto&e:expected){std::ifstream f(d+"/"+e.n,std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});if(b.empty()||fnv(b)!=e.h){std::fprintf(stderr,"identity mismatch %s\n",e.n);return 3;}}
 struct S{const char*n;unsigned c,t;}; const S native[]={{"2HeadphoneSpatializer.rifx",134,12},{"SB2SpeakerSpatializer.rifx",97,12},{"SB4SpeakerSpatializer.rifx",60,8},{"SurroundSpatializer.rifx",15,0},{"Surround_Encoder.rifx",38,12},{"FDNReverb_EAX2_0_version.rifx",80,43}};
 for(auto&s:native){Program p;std::string er;if(!RifxLoader::loadFile(d+"/"+s.n,p,&er)||p.code.size()!=s.c||p.tram.size()!=s.t)return 4;}
 // The broader later factory corpus is a distinct revision family; don't conflate it with PCI1024 LiveWare 2.5.
 const S lv[]={{"2HeadphoneSpatializer.rifx",131,12},{"SB2SpeakerSpatializer.rifx",95,12},{"SB4SpeakerSpatializer.rifx",58,8},{"SurroundSpatializer.rifx",10,0},{"Surround_Encoder.rifx",39,12},{"FDNReverb-EAX2.0_version.rifx",84,47}};
 for(auto&s:lv){Program p;std::string er;if(!RifxLoader::loadFile(later+"/"+s.n,p,&er)||p.code.size()!=s.c||p.tram.size()!=s.t)return 5;}
 std::puts("PCI1024 SBLFX LiveWare 2.5: exact 23-program fixture identity locked; spatializer/reverb revision family kept distinct from later corpus");return 0;}
