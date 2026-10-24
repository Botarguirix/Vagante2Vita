#include "port_lib_types.h"
#define private public
#include "Tween.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    const float quarter[]={.25f,.0625f,.125f,.015625f,.578125f,.0625f,.00390625f,.031754163f,.6614378f,.14644662f,.47265625f,.015625f};
    for(int kind=0;kind<12;++kind){Tween t(static_cast<Tween::TweenType>(kind),10,8,4);assert(t.val()==10);t.time=1;assert(std::abs(t.val()-(10+8*quarter[kind]))<.00001f);t.time=4;assert(std::abs(t.val()-18)<.00001f);t.time=8;assert(std::abs(t.val()-18)<.00001f);}
    Tween t(Tween::TWEEN_LINEAR,2,4,4);assert(t.step(1)==3);t.pauseTime=2.5f;assert(t.step(2)==3 && t.time==1);assert(t.step(1)==3.5f && t.time==1.5f);assert(t.step(1)==4.5f);
    t.type=Tween::TWEEN_EASE_BOUNCE;for(float q:{.36363637447f,.72727274894f,.90909093618f}){t.time=q*4;assert(std::abs(t.val()-6)<.00001f);}
    t.type=static_cast<Tween::TweenType>(99);assert(t.val()==0);
    std::cout<<"Tween: twelve curves, endpoints/clamp, bounce boundaries and pause overshoot passed\n";
}
