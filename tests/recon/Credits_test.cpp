#include "port_lib_types.h"
#define private public
#define protected public
#include "Credits.h"
#include "Level.h"
#include "ResourceManager.h"
#undef protected
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <iostream>
struct ParadeFixture : Entity {
    int steps=0,draws=0,detaches=0;
    ParadeFixture() : Entity(nullptr,TYPE_BAT,0,0,16,16){x=0;y=0;w=16;h=16;lightAlpha=0;doesCollideLevel=false;isUnit=false;}
    void setLevel(Level *level) override {if(!level)++detaches;}
    void step() override {++steps;}
    void draw(sf::RenderTarget &) override {++draws;}
    void doCollision(const std::shared_ptr<Entity>&) override {}
};
int main(){
    Config config;ResourceManager::setConfig(&config);ResourceManager::muted=true;
    sf::RenderWindow window(sf::VideoMode(640,360),"Credits fixture",sf::Style::None);window.setVisible(false);
    Input input(nullptr);ConfigGui settings(window,config,input);GameEngine engine;
    sf::Image atlas;atlas.create(128,80,sf::Color::White);sf::Texture texture;assert(texture.loadFromImage(atlas));
    for(auto *name:{"sky","sky_stars","moon","ground","clouds","mountains","midground_treeline_1","foreground_treeline","caravan"})ResourceManager::textures[std::string("scenes/intro/")+name+".png"]=&texture;
    ResourceManager::textures["scenes/intro/entities/little_firefly.png"]=&texture;
    sf::Font font;assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));ResourceManager::fonts["Tiny_04b.ttf"]=&font;ResourceManager::fonts["NotoSansCJKkr-Regular.otf"]=&font;
    {
        Credits credits(window,settings,engine);assert(!credits.isTriggered());
        credits.state=Credits::GOOD_END_SCENE;credits.triggered=false;credits.skipAllEnemies=false;credits.skipBranchEnemies=false;credits.fadingOut=false;credits.creditsOver=false;credits.fadeFrames=0;credits.fadeFramesTotal=60;
        credits.initializeCurrentState();assert(credits.day==.5f && credits.caravanPosition==100 && credits.creditStateFrames==1200);
        credits.initializePlayerSprites();assert(credits.playerSprites.empty());
        const int entries[]={0,0,4,8,0,4,7,0,4,8,0,5,10,0};
        const int texts[]={0,3,0,0,2,0,0,2,0,0,4,0,0,1};
        for(int i=0;i<14;++i){credits.state=static_cast<Credits::CreditsState>(i);credits.initializeCurrentState();assert(credits.paradeEntries.size()==entries[i] && credits.textWipes.size()==texts[i]);}
        credits.state=Credits::ENEMIES_BRANCH_3;credits.initializeCurrentState();assert(credits.paradeEntries[1].type==Entity::TYPE_PIRANHA && credits.paradeEntries[1].params==std::vector<int>{1} && credits.paradeEntries[2].params.empty());
        credits.state=Credits::ENEMIES_ACT_2;credits.initializeCurrentState();assert(credits.paradeEntries.back().params==std::vector<int>{0});
        credits.state=Credits::ENEMIES_BRANCH_1;credits.initializeCurrentState();assert(credits.paradeEntries[3].params==std::vector<int>{0} && credits.paradeEntries[4].params==std::vector<int>{1});
        for(int i=0;i<14;++i){credits.state=static_cast<Credits::CreditsState>(i);credits.skipAllEnemies=false;credits.skipBranchEnemies=false;assert(!credits.checkSkipState());credits.skipBranchEnemies=true;assert(credits.checkSkipState()==(i==2 || i==5 || i==8 || i==11));credits.skipAllEnemies=true;assert(credits.checkSkipState()==(entries[i]!=0));}
        credits.skipAllEnemies=false;credits.skipBranchEnemies=false;
        credits.textWipes.clear();Credits::TextWipe wipe;wipe.position={0,0};wipe.desiredPosition={3,4};wipe.speed=0;credits.textWipes.push_back(wipe);credits.stepTextWipes();assert(std::abs(credits.textWipes[0].position.x-.06f)<.00001f && std::abs(credits.textWipes[0].position.y-.08f)<.00001f);
        credits.actionPress();assert(credits.creditStateFrames==0 && credits.fadeFrames==credits.fadeFramesTotal && credits.textWipes[0].position==sf::Vector2f(3,4));
        credits.state=Credits::CREDITS_PROGRAMMING;credits.initializeCurrentState();assert(credits.textWipes[0].text.getString()=="PROGRAMMING" && credits.textWipes[1].text.getString()=="Eugene Paik");
        credits.creditStateFrames=0;credits.fadeFramesTotal=2;credits.fadeFrames=2;credits.fadingOut=false;credits.skipAllEnemies=true;
        credits.step();assert(credits.fadingOut && credits.fadeFrames==0);credits.step();assert(credits.fadeColor.a==127);credits.step();assert(credits.state==Credits::CREDITS_GRAPHICS && !credits.fadingOut && credits.creditStateFrames==240);
        credits.state=Credits::CREDITS_SPECIAL_THANKS;credits.initializeCurrentState();credits.creditStateFrames=0;credits.fadingOut=true;credits.fadeFrames=2;credits.step();assert(credits.state==Credits::THE_END && credits.creditsAreOver());
        credits.skipAllEnemies=false;credits.state=Credits::GOOD_END_SCENE;credits.initializeCurrentState();
        _randEngine.seed(77);auto expected=_randEngine;for(int i=0;i<60;++i)expected();credits.stepGoodEnding();assert(_randEngine==expected && credits.fireFlies.size()==15);
        expected=_randEngine;credits.stepGoodEnding();assert(_randEngine==expected);assert(std::abs(credits.caravanPosition-99.6f)<.0001f);
        for(unsigned i=0;i<15;++i){auto pos=credits.fireFlies[i].getPosition();assert(pos.x>=i*42.666667f-1 && pos.x<=(i+1)*42.666667f+1 && pos.y>=99 && pos.y<=261);}
        Credits::PlayerSpriteCollection player;player.walkSprite=Sprite({},"scenes/intro/entities/little_firefly.png",16,16,-1,8,0,0,true,false);player.jumpSprite=Sprite(player.walkSprite);player.position={250,306};player.walking=true;player.speedStat=3;credits.playerSprites.push_back(player);
        credits.stepGoodEnding();assert(credits.playerSprites[0].walking && credits.playerSprites[0].position.y==306);credits.playerSprites[0].position.x=232;credits.stepGoodEnding();assert(!credits.playerSprites[0].walking && credits.playerSprites[0].position.y==302.5f);
        sf::RenderTexture target;assert(target.create(640,360));credits.drawGoodEnding(target);assert(credits.playerSprites[0].walkSprite.getPosition()==credits.playerSprites[0].position);
        credits.playerSprites[0].position.x=210;credits.stepGoodEnding();assert(credits.playerSprites.empty());
        credits.state=Credits::ENEMIES_ACT_1;credits.paradeLevel=std::make_shared<Level>(1,1,20,10,0);credits.paradeLevel->w=20;credits.paradeLevel->h=10;credits.paradeLevel->frame=0;credits.paradeLevel->particlesEnabled=false;
        auto entity=std::make_shared<ParadeFixture>();auto child=std::make_shared<ParadeFixture>();entity->x=100;credits.paradeEntries.clear();Credits::EnemyParadeEntry entry;entry.type=Entity::TYPE_BAT;entry.paradeEntity=entity;credits.paradeEntries.push_back(entry);credits.paradeLevelEntities={entity,child};
        credits.stepParade();assert(entity->steps==1 && entity->lightAlpha>0 && credits.paradeLevel->frame==1);credits.draw(target);assert(entity->draws==1);
        entity->x=310;credits.stepParade();assert(credits.paradeEntries.empty() && credits.paradeLevelEntities.empty() && entity->detaches==1 && child->detaches==1);
        for(int i=0;i<14;++i){credits.state=static_cast<Credits::CreditsState>(i);credits.initializeCurrentState();credits.draw(target);}target.display();
    }
    ResourceManager::fonts.clear();ResourceManager::textures.clear();ResourceManager::muted=false;window.close();
    std::cout<<"Credits: 14 states, text/fades/skips, parade queue/lighting/removal, boarding, 60-draw fireflies and rendering passed\n";
}
