#include <random>
#include <iostream>
#include <string>
#include <cstdint>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 unsigned long long seed=std::stoull(argv[1]);if(seed>UINT32_MAX)return 2;
 std::mt19937 rng(static_cast<uint32_t>(seed));
 for(int i=0;i<1000;i++)std::cout<<rng()<<'\n';
 return 0;
}
