#include "recon_all.h"
#include <cassert>
#include <iostream>
int main() {
    const std::pair<int,int> counts[]={{0,0},{2,1},{1,1},{1,2},{1,0},{2,1},{2,0},{1,1},{2,0},{1,1},{3,1},{2,0},{0,0},{0,0},{1,1},{1,0},{1,1},{1,0},{1,0},{1,1},{1,0},{2,0},{1,1},{1,0},{0,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,1},{1,1}};
    for (int i=0;i<32;++i) {
        Background bg(static_cast<Background::BackgroundType>(i));
        assert(!bg.name.empty() && !bg.description.empty());
        assert(bg.positive.size()==counts[i].first && bg.negative.size()==counts[i].second);
        auto name=bg.name;auto desc=bg.description;auto pos=bg.positive;auto neg=bg.negative;
        bg.updateNameAndDescription();
        assert(bg.name==name && bg.description==desc && bg.positive==pos && bg.negative==neg);
    }
    Background bg(Background::BG_FLAIL); assert(bg.name=="Flail Wielder" && bg.positive[0]=="+Item: Heavy Flail");
    bg.type=Background::BG_GRAPPLING_HOOK;bg.updateNameAndDescription();assert(bg.name=="Tinkerer" && bg.negative[0]=="-1 SPD");
    bg.type=Background::BG_GOD_CHAOS;bg.updateNameAndDescription();assert(bg.positive[0]=="+???" && bg.negative[0]=="+???");
    bg.type=static_cast<Background::BackgroundType>(255);bg.updateNameAndDescription();assert(bg.name=="Undefined Background" && bg.description=="Undefined Description" && bg.positive.empty() && bg.negative.empty());
    std::cout<<"Background: 32 cases, effects/order, repeat update, corrected enum mapping and invalid type passed\n";
}
