// Credits: parade entities, boarding and firefly animation.
#include "recon_all.h"
#include <algorithm>
#include <cmath>
void Credits::stepParade(){
    if(!paradeLevel)return;
    const float width=float(paradeLevel->w<<4),center=width*.5f;
    auto *font=ResourceManager::getDefaultFont();
    if(paradeEntries.empty()){if(creditStateFrames)creditStateFrames=0;}
    else if(paradeEntries.back().paradeEntity.expired()){
        auto entity=Entity::spawnEntity(paradeEntries.back().type,paradeEntries.back().params);
        // A translated spawn always supplies a valid entity for these IDs. A
        // remaining spawn stub must not be dereferenced as if it were one.
        if(!entity)return;
        paradeLevel->addEntity(entity);entity->paradeEntity=true;
        if(entity->isUnit)static_cast<Unit &>(*entity).facingLeft=false;
        const auto entrance=paradeLevel->entranceLocations.empty() ? sf::Vector2i(0,0) : paradeLevel->entranceLocations.front();
        entity->x=float(entrance.x<<4)+16.f;entity->y=float(entrance.y<<4)-16.f;entity->lightAlpha=0;
        paradeEntries.back().paradeEntity=entity;
        if(font){
            TextWipe wipe;wipe.speed=0;const auto name=entity->getFriendlyName();wipe.text=sf::Text(sf::String::fromUtf8(name.begin(),name.end()),*font,16);
            wipe.text.setOrigin(wipe.text.getLocalBounds().width*.5f,0);
            wipe.position={view.getCenter().x,view.getSize().y+100.f};wipe.desiredPosition={view.getCenter().x,view.getSize().y*.5f+100.f};
            for(auto &old:textWipes)old.desiredPosition={view.getCenter().x,view.getSize().y+800.f};
            textWipes.push_back(wipe);
        }
    }
    // Local lambda @00456d10: remove one matching queue entry, detach ALL
    // current level entities, then clear the level's shared entity vector.
    const auto remove=[&](const std::shared_ptr<Entity> &entity){
        for(std::size_t i=0;i<paradeEntries.size();++i){if(paradeEntries[i].paradeEntity.lock()!=entity)continue;
            for(const auto &current:paradeLevelEntities)current->setLevel(nullptr);
            paradeLevelEntities.clear();paradeEntries.erase(paradeEntries.begin()+i);return;
        }
    };
    for(std::size_t i=0;i<paradeLevelEntities.size();++i){
        auto entity=paradeLevelEntities[i];entity->step();if(entity->doesCollideLevel)entity->collideLevel();
        const float position=entity->w*.5f+entity->x;
        const float alpha=1.f-std::clamp((2.857142925262451f/width)*std::abs(center-position),0.f,1.f);
        entity->lightAlpha=position>=center ? std::min(alpha,entity->lightAlpha) : std::max(alpha,entity->lightAlpha);
        if(entity->lightAlpha>.01f)paradeLevel->revealArea(entity->w*.5f+entity->x,entity->h*.5f+entity->y,entity->lightAlpha*6.f,false);
        if(width*.7f<entity->w*.5f+entity->x)for(auto &wipe:textWipes)wipe.desiredPosition={view.getCenter().x,view.getSize().y+800.f};
        if(float(paradeLevel->w<<4)<=entity->w+entity->x || (center<entity->w*.5f+entity->x && entity->lightAlpha<=0))remove(entity);
    }
    ++paradeLevel->frame;paradeLevel->updateLight(nullptr);paradeLevel->updateParticles();
}
void Credits::stepGoodEnding(){
    for(std::size_t i=0;i<playerSprites.size();){
        auto &entry=playerSprites[i];entry.walkSprite.setColor(sf::Color(200,200,200));entry.jumpSprite.setColor(sf::Color(200,200,200));
        entry.position.x-=float(entry.speedStat)*.1f+1.4f;
        if(entry.position.x<caravanSprite.getSize().x+caravanPosition+5.f)entry.walking=false;
        if(caravanSprite.getSize().x+caravanPosition-10.f<=entry.position.x){
            if(!entry.walking)entry.position.y-=3.5f;
            auto &sprite=entry.walking ? entry.walkSprite : entry.jumpSprite;sprite.step({0,0},{1,1});if(!entry.walking)sprite.setFrame(0);
            ++i;
        }else playerSprites.erase(playerSprites.begin()+i);
    }
    cloudMovePos+=.1f;
    if(playerSprites.empty()){caravanPosition-=.2f;caravanSprite.step({0,0},{1,1});}
    if(fireFlies.empty()){
        Sprite firefly({},"scenes/intro/entities/little_firefly.png",16,16,-1,8,0,0,true,false);
        for(unsigned i=0;i<15;++i){
            fireFlies.push_back(firefly);
            // MSVC generate_canonical<float> @0041c980: one unsigned MT draw
            // rounded to float then divided by 2^32 (no extra rejection).
            const float ry=float(_randEngine())/4294967296.f;
            const float y=ry*160.f+100.f;
            const float left=float(i)*42.66666793823242f;
            const float rx=float(_randEngine())/4294967296.f;
            const float x=rx*(float(i+1)*42.66666793823242f-left)+left;
            auto &sprite=fireFlies.back();sprite.setPosition(x,y);
            sprite.setFrame(static_cast<int>(_randEngine()&0x7fffffffu)%sprite.getNumFrames());
            sprite.setTime(static_cast<int>(_randEngine()&0x7fffffffu)%sprite.getSpeed());
        }
    }
    for(std::size_t i=0;i<fireFlies.size();++i){
        const float n=static_cast<float>(i),frame=static_cast<float>(creditStateFrames);
        float dx=std::cos(n*.27142858505249023f+frame*.014285714365541935f)*.2f;
        dx-=std::sin(n*.8666667342185974f+frame*.06666667014360428f)*.2f;
        const float first=std::cos(n*.85f+frame*.05f);
        float dy=(first+std::sin(n*4.369863033294678f+frame*.013698630034923553f))*.2f;if((i&1)==0)dy=-dy;
        fireFlies[i].setPosition(fireFlies[i].getPosition()+sf::Vector2f(dx,dy));fireFlies[i].step({0,0},{1,1});
    }
}
