// Credits good ending draw @004579c0; the walk sprite remains the one drawn
// during boarding in this compiled body, including after walking becomes false.
#include "recon_all.h"
#include <cmath>
namespace { sf::Uint8 byte(float value){return static_cast<sf::Uint8>(static_cast<int>(value));} }
void Credits::drawGoodEnding(sf::RenderTarget &target){
    const float width=view.getSize().x;
    const int columns=static_cast<int>(std::ceil(width*.0025f));
    const int half=static_cast<int>(std::ceil(float(columns)*.5f+1.f));
    const int begin=-half,end=columns+2-half;
    skySprite.setAlpha(byte(day*255.f));skyStarsSprite.setAlpha(static_cast<sf::Uint8>(~byte(day*255.f)));
    for(int i=begin;i<end;++i){skySprite.setPosition(float(i)*400.f,0);skySprite.draw(target,{});if(day<1){skyStarsSprite.setPosition(skySprite.getPosition());skyStarsSprite.draw(target,{});}}
    const float night=std::pow(1.f-day,2.f);
    auto tint=static_cast<sf::Uint8>(230-byte(day*-25.f));moonSprite.setColorComponents(tint,tint,tint);moonSprite.setPosition(night*3.f,80.f-night*11.f);moonSprite.draw(target,{});
    const float cloud=std::fmod(night*55.f+cloudMovePos+movePos*.0035f,width+400.f)-300.f;
    const float mountains=std::fmod(movePos*.004f,400.f),trees=std::fmod(movePos*.19f,400.f),ground=std::fmod(movePos*.3f+22.f,400.f),foreground=std::fmod(movePos*.6f,400.f);
    cloudSprite.setAlpha(byte(day*day*255.f));cloudSprite.setPosition(cloud,0);cloudSprite.draw(target,{});
    const auto layer=[&](Sprite &sprite,float shift){for(int i=begin;i<end;++i){sprite.setPosition(float(i)*400.f+shift,0);sprite.draw(target,{});}};
    tint=static_cast<sf::Uint8>(110-byte(day*-145.f));mountainSprite.setColorComponents(tint,tint,tint);layer(mountainSprite,mountains);
    tint=static_cast<sf::Uint8>(30-byte(day*-225.f));treeLineSprite1.setColorComponents(tint,tint,tint);layer(treeLineSprite1,trees);
    groundSprite.setColorComponents(tint,tint,tint);layer(groundSprite,ground);
    for(auto &entry:playerSprites){entry.walkSprite.setPosition(entry.position);entry.walkSprite.draw(target,{});}
    tint=static_cast<sf::Uint8>(80-byte(day*-175.f));caravanSprite.setPosition(caravanPosition,caravanBump ? 232.5f : 232.f);caravanSprite.setColorComponents(tint,tint,tint);caravanSprite.draw(target,{});
    tint=byte(day*255.f);foregroundSprite.setColorComponents(tint,tint,tint);layer(foregroundSprite,foreground);
    for(auto &sprite:fireFlies)sprite.draw(target,{});
}
