// Credits 00455690-0045ad2f: core, fade transitions and text motion.
#include "recon_all.h"
#include <algorithm>
#include <cmath>
Credits::Credits(sf::RenderWindow &window,ConfigGui &settings,GameEngine &engine)
    : configGui(settings),gameEngine(engine),view(window.getView()),
      skySprite({},"scenes/intro/sky.png",-1,-1,-1,0,0,0,false,false),
      skyStarsSprite({},"scenes/intro/sky_stars.png",-1,-1,-1,0,0,0,false,false),
      moonSprite({},"scenes/intro/moon.png",-1,-1,-1,0,0,0,false,false),
      groundSprite({},"scenes/intro/ground.png",-1,-1,-1,0,0,0,false,false),
      cloudSprite({},"scenes/intro/clouds.png",-1,-1,-1,0,0,0,false,false),
      mountainSprite({},"scenes/intro/mountains.png",-1,-1,-1,0,0,0,false,false),
      treeLineSprite1({},"scenes/intro/midground_treeline_1.png",-1,-1,-1,0,0,0,false,false),
      foregroundSprite({},"scenes/intro/foreground_treeline.png",-1,-1,-1,0,0,0,false,false),
      caravanSprite({},"scenes/intro/caravan.png",128,80,-1,7,0,0,true,false),fadeColor(),triggered(false) {
    // Remaining scalar state is initialized by the original trigger sequence,
    // not by this constructor. Do not invent its initial values here.
    for(auto *sprite:{&skySprite,&skyStarsSprite,&moonSprite,&groundSprite,&cloudSprite,&mountainSprite,&caravanSprite,&treeLineSprite1,&foregroundSprite})sprite->setOrigin(0,0);
    view.setCenter(320,180);
}
void Credits::actionPress(){creditStateFrames=0;fadeFrames=fadeFramesTotal;for(auto &wipe:textWipes)wipe.position=wipe.desiredPosition;}
bool Credits::checkSkipState() const {
    if(state==THE_END)return false;
    const bool branch=state==ENEMIES_BRANCH_1 || state==ENEMIES_BRANCH_2 || state==ENEMIES_BRANCH_3 || state==ENEMIES_BRANCH_4;
    const bool act=state==ENEMIES_ACT_1 || state==ENEMIES_ACT_2 || state==ENEMIES_ACT_3 || state==ENEMIES_ACT_4;
    return (skipAllEnemies && (branch || act)) || (skipBranchEnemies && branch);
}
void Credits::stepFadeEffect(){
    if(fadeFramesTotal && fadeFrames<fadeFramesTotal){++fadeFrames;const float ratio=float(fadeFrames)/float(fadeFramesTotal);fadeColor.a=static_cast<sf::Uint8>(static_cast<int>((fadingOut ? ratio : 1.f-ratio)*255.f));}
}
void Credits::stepTextWipes(){
    for(auto &wipe:textWipes){
        auto direction=wipe.desiredPosition-wipe.position;
        const float distance=std::sqrt(direction.x*direction.x+direction.y*direction.y);
        if(std::abs(distance)>=9.999999717180685e-10f)direction/=distance;
        float speed=std::clamp(distance*.0033333334140479565f,0.f,1.f)*29.899999618530273f+.1f;
        speed=std::min(speed,wipe.speed+.1f);wipe.speed=speed;
        if(speed<=distance)wipe.position+=direction*speed;
        else {wipe.speed=0;wipe.position=wipe.desiredPosition;}
    }
}
void Credits::step(){
    stepFadeEffect();stepTextWipes();stepParade();if(state==GOOD_END_SCENE)stepGoodEnding();
    if(creditStateFrames){--creditStateFrames;return;}
    if(fadeFrames<fadeFramesTotal){if(!fadingOut)return;}
    else if(!fadingOut){fadingOut=true;fadeFrames=0;fadeColor=sf::Color(0,0,0,0);return;}
    if(fadeFrames>=fadeFramesTotal){
        if(state<THE_END){
            fadingOut=false;fadeFrames=0;fadeColor=sf::Color::Black;
            do{state=static_cast<CreditsState>(static_cast<unsigned>(state)+1);}while(checkSkipState());
            if(!skipAllEnemies || state!=THE_END)initializeCurrentState();else creditsOver=true;
        }else creditsOver=true;
    }
    if(state==THE_END){const float volume=std::clamp(1.f-float(fadeFrames)/float(fadeFramesTotal),0.f,1.f);ResourceManager::setSpecificMusicVolume("credits-victory.ogg",volume);ResourceManager::setSpecificMusicVolume("credits.ogg",volume);}
}
void Credits::initializePlayerSprites(){
    playerSprites.clear();
    const auto &players=gameEngine.getPlayers();
    for(std::size_t i=0;i<players.size();++i){if(!players[i])continue;
        PlayerSpriteCollection entry;
        entry.position={0,0};
        entry.jumpSprite=Sprite(players[i]->getJumpSprite());entry.walkSprite=Sprite(players[i]->getWalkSprite());
        entry.jumpSprite.reset();entry.walkSprite.reset();entry.jumpSprite.setFlipped(true);entry.walkSprite.setFlipped(true);
        entry.position={caravanPosition+600.f+float(i*16),306.f};entry.speedStat=players[i]->getStat(Player::SPD);entry.walking=true;
        playerSprites.push_back(entry);
    }
}
void Credits::draw(sf::RenderTarget &target){
    target.setView(view);
    switch(state){
    case GOOD_END_SCENE:drawGoodEnding(target);break;
    case ENEMIES_BRANCH_4:case ENEMIES_ACT_4:case ENEMIES_BRANCH_3:case ENEMIES_ACT_3:case ENEMIES_BRANCH_2:case ENEMIES_ACT_2:case ENEMIES_BRANCH_1:case ENEMIES_ACT_1:
        if(paradeLevel){auto &level=*paradeLevel;const int right=level.w-1,bottom=level.h-1;
            level.drawSky(target,view,0.f);level.drawBackground(target,0,right,0,bottom,true,false);level.drawFarground(target,0,right,0,bottom,true,false);
            level.drawMidground(target,0,right,0,bottom,true,false);level.drawForeground(target,0,right,0,bottom,true,false);level.drawCloseground(target,0,right,0,bottom,true,false);level.drawLightSmooth(target,view);
            std::sort(paradeLevelEntities.begin(),paradeLevelEntities.end(),compareEntityDrawOrder);
            for(const auto &entity:paradeLevelEntities)entity->draw(target);
            if(level.particlesEnabled)level.drawParticles(target);
        }break;
    default:break;
    }
    for(auto &wipe:textWipes){wipe.text.setPosition(wipe.position);target.draw(wipe.text);}
    sf::RectangleShape cover(view.getSize());cover.setOrigin(view.getSize()*.5f);cover.setFillColor(fadeColor);cover.setPosition(view.getCenter());target.draw(cover);
}
bool Credits::isTriggered() const {return triggered;}
bool Credits::creditsAreOver() const {return creditsOver;}
