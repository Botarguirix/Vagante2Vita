// MainMenu scene animation and rendering, 006a3ff0/006a4ba0/006a5b50.
#include "recon_all.h"
#include <algorithm>
#include <cmath>
namespace { sf::Uint8 byte(float value){return static_cast<sf::Uint8>(static_cast<int>(value));} }
void MainMenu::stepBackground(float rate){
    const float speed=rate*.3f-.3f;
    ++frame;caravanSpeed=speed;movePos=rate+movePos;
    const float relative=speed-rate;
    caravanPosition=speed+caravanPosition;cloudMovePos+=.005f;
    caravanBump=false;
    if(std::abs(relative)>.001){caravanSprite.step({0,0},{1,1});caravanBump=(_randEngine()&0x7fffffffu)%20==0;}
    for(std::size_t i=0;i<sceneDoodads.size();++i){
        auto &doodad=sceneDoodads[i];const float n=static_cast<float>(i);
        const float dx=std::cos(n*.27142858505249023f+float(frame)*.014285714365541935f)*.2f;
        const float sx=std::sin(n*.8666667342185974f+float(frame)*.06666667014360428f)*.2f;
        const float drift=moveRate*.3f;
        const float first=std::cos(n*.85f+float(frame)*.05f);
        float dy=(first+std::sin(n*4.369863033294678f+float(frame)*.013698630034923553f))*.2f;
        if((i&1)==0)dy=-dy;
        doodad.pos.x=(dx-sx)+drift+doodad.pos.x;doodad.pos.y=dy+doodad.pos.y;
        if(doodad.sprite){
            doodad.sprite->setPosition(doodad.pos);doodad.sprite->step({0,0},{1,1});
            float alpha=1.f;
            if(doodad.pos.x<0)alpha=1.f-doodad.pos.x*-.01f;
            else if(doodad.pos.x>640)alpha=1.f-(doodad.pos.x-640.f)*.01f;
            doodad.sprite->setAlpha(byte(alpha*255.f));
        }
        if(doodad.pos.x>740.f)doodad.pos.x-=840.f;
    }
}
void MainMenu::drawBackground(sf::RenderTarget &target,float day){
    view.setCenter(320,180);target.setView(view);
    const float width=view.getSize().x;
    const int columns=static_cast<int>(std::ceil(width*.0025f));
    const int half=static_cast<int>(std::ceil(float(columns)*.5f+1.f));
    const int begin=-half,end=columns+2-half;
    caravanSprite.setPosition(caravanPosition,caravanBump ? 232.5f : 232.f);
    const auto brightness=byte(day*255.f);
    skySprite.setAlpha(brightness);skyStarsSprite.setAlpha(static_cast<sf::Uint8>(~brightness));
    for(int i=begin;i<end;++i){skySprite.setPosition(float(i)*400.f,0);skySprite.draw(target,{});if(day<1.f){skyStarsSprite.setPosition(skySprite.getPosition());skyStarsSprite.draw(target,{});}}
    const float night=std::pow(1.f-day,2.f);
    auto tint=static_cast<sf::Uint8>(230-byte(day*-25.f));moonSprite.setColorComponents(tint,tint,tint);
    moonSprite.setPosition(night*3.f,56.f-night*11.f);moonSprite.draw(target,{});
    const float cloud=std::fmod(night*55.f+cloudMovePos+movePos*.0035f,width+400.f)-300.f;
    const float mountains=std::fmod(movePos*.004f,400.f);
    const float trees=std::fmod(movePos*.19f,400.f);
    const float ground=std::fmod(movePos*.3f+22.f,400.f);
    const float foreground=std::fmod(movePos*.6f,400.f);
    const float sign=(movePos-3130.f)*.3f;
    cloudSprite.setAlpha(byte(day*day*255.f));cloudSprite.setPosition(cloud,0);cloudSprite.draw(target,{});
    const auto layer=[&](Sprite &sprite,float shift){for(int i=begin;i<end;++i){sprite.setPosition(float(i)*400.f+shift,0);sprite.draw(target,{});}};
    tint=static_cast<sf::Uint8>(110-byte(day*-145.f));mountainSprite.setColorComponents(tint,tint,tint);layer(mountainSprite,mountains);
    tint=static_cast<sf::Uint8>(30-byte(day*-225.f));treeLineSprite1.setColorComponents(tint,tint,tint);layer(treeLineSprite1,trees);
    groundSprite.setColorComponents(tint,tint,tint);layer(groundSprite,ground);
    signpostSprite.setColorComponents(tint,tint,tint);signpostSprite.setPosition(sign,291);signpostSprite.draw(target,{});
    tint=static_cast<sf::Uint8>(80-byte(day*-175.f));caravanSprite.setColorComponents(tint,tint,tint);
    if(fadeInCaravan && skipIntro)caravanSprite.setAlpha(byte(std::min(1.f,float(afterIntroTimer)*.03333333507180214f)*255.f));
    caravanSprite.draw(target,{});
    for(auto &doodad:sceneDoodads)if(doodad.sprite && doodad.sprite->getPosition().y>0)doodad.sprite->draw(target,{});
    foregroundSprite.setColorComponents(brightness,brightness,brightness);layer(foregroundSprite,foreground);
}
void MainMenu::draw(sf::RenderTarget &target,float){
    // The linked body ignores its retained float parameter; day depends on the start fade.
    float day=1.f;
    if(fadingOut && fadingOutTarget==0)day=std::min(1.f,std::max(0.f,float(animateOutTimer)/float(animateOutTime)));
    drawBackground(target,day);
    if(titleTimer>0){
        const float fade=std::min(1.f,float(titleTimer)*.01666666753590107f);
        const float out=fadingOut ? 1.f-std::min(1.f,float(animateOutTime-animateOutTimer)*.025f) : 1.f;
        titleSprite.setColor(sf::Color(0,0,0,byte(fade*120.f*out)));titleSprite.setPosition(322,titlePos+2);titleSprite.draw(target,{});
        titleSprite.setColor(sf::Color(255,255,255,byte(fade*255.f*out)));titleSprite.setPosition(320,titlePos);titleSprite.draw(target,{});
        titleSprite.setColor(sf::Color(90,90,90,byte((std::sin(float(frame)*.027027027681469917f)+1.f)*127.5f*fade*out)));
        titleSprite.draw(target,sf::RenderStates(sf::BlendAdd));
    }
    const auto cover=[&](float alpha){sf::RectangleShape rectangle(view.getSize());rectangle.setOrigin(view.getSize()*.5f);rectangle.setPosition(view.getCenter());rectangle.setFillColor(sf::Color(0,0,0,byte(alpha)));target.draw(rectangle);};
    if(!fadingOut){for(auto *element:guiElements)element->draw(target);guiCursor.draw(target);}
    else if(fadingOutTarget==0 || fadingOutTarget==3){cover(std::max(0.f,1.f-float(animateOutTimer)*.03333333507180214f)*255.f);caravanSprite.draw(target,{});}
    else if(fadingOutTarget==1 || fadingOutTarget==2)cover(std::max(0.f,1.f-float(animateOutTimer)/float(animateOutTime))*230.f);
    if(fadeInTimer>0)cover(fadeInTimer<30 ? float(fadeInTimer*255)*.03333333507180214f : 255.f);
}
