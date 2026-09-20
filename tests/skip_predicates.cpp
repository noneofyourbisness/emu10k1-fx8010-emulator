#include "fx8010_engine.h"
#include <cstdint>
#include <cstdio>
using namespace fx8010;
static bool markerRuns(std::uint32_t ccr,std::uint32_t test){
  Program p; p.gprInit={{0x8000,(std::int32_t)test},{0x8001,1},{0x8002,0}};
  p.code.push_back({15,0x57,0x8000,0x8001,0x8003}); // X=test, Y=count
  p.code.push_back({0,0x40,0x4f,0x4f,0x8002});
  Engine e;e.load(p); e.set(0x57,(std::int32_t)ccr); e.runSample(); return e.get(0x8002)!=0;
}
int main(){
 struct V{uint32_t cc,t;bool condition;const char*n;};
 const V v[]={
  {1,1,true,"norm"},{0,1,false,"norm_false"},
  {2,2,true,"borrow"},{0,2,false,"borrow_false"},
  {4,4,true,"minus"},{0,4,false,"minus_false"},
  {8,8,true,"zero"},{0,8,false,"zero_false"},
  {16,16,true,"sat"},{0,16,false,"sat_false"},
  {0,0x100,true,"nonzero"},{8,0x100,false,"nonzero_false"},
  {4,0x1008,true,"le_neg"},{8,0x1008,true,"le_zero"},{0,0x1008,false,"le_false"},
  {0,0x180,true,"gt"},{4,0x180,false,"gt_false"},
  {0,0x80,true,"ge"},{4,0x80,false,"ge_false"}
 };
 for(auto &x:v){auto runs=markerRuns(x.cc,x.t);const bool expectedRuns=!x.condition;if(runs!=expectedRuns){std::printf("%s marker_runs=%d expected=%d\n",x.n,(int)runs,(int)expectedRuns);return 2;}}
 std::printf("skip_predicates=%zu pass\n",sizeof(v)/sizeof(v[0])); return 0;
}
