#include "port_lib_types.h"
#define private public
#define protected public
#include "PlayerCustomizeMenu.h"
#include "PlayerMenu.h"
#include "ResourceManager.h"
#include "Level.h"
#undef protected
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    Config config; ResourceManager::setConfig(&config);
    sf::Image image; image.create(256,80,sf::Color::White);
    sf::Texture texture; assert(texture.loadFromImage(image));
    ResourceManager::textures["scenes/intro/caravan.png"]=&texture;
    _randEngine.seed(725); auto expected=_randEngine;
    PlayerCustomizeMenu menu;
    assert(_randEngine==expected && !menu.hidden && !menu.open && menu.hasFocus);
    assert(menu.frame==0 && menu.caravanPosition==97.f && menu.caravanPositionY==256.f && !menu.stopped);
    assert(menu.playerClasses==std::vector<Player::PlayerClass>(4,Player::NUM_PLAYER_CLASSES));
    assert(menu.playerPalettes==std::vector<int>(4,0) && menu.playerColors.empty());
    assert(menu.caravanSprite.getSize()==sf::Vector2i(128,80) && menu.caravanSprite.getSpeed()==7);
    assert(menu.caravanSprite.getOrigin()==sf::Vector2f(0,0));
    std::vector<std::shared_ptr<Player>> players(4);
    std::vector<std::shared_ptr<PlayerMenu>> guis(4);
    menu.setPlayersRef(&players); menu.setPlayerMenusRef(&guis);
    assert(menu.players==&players && menu.guis==&guis);
    menu.playerClasses[0]=Player::CLASS_ROGUE; menu.playerPalettes[0]=3;
    menu.reset(false,3,true); assert(menu.isOpen() && menu.caravanDarkness==1.f && menu.frame==0);
    assert(menu.playerClasses[0]==Player::CLASS_ROGUE && menu.playerPalettes[0]==3);
    menu.reset(true,0,false); assert(menu.playerClasses[0]==Player::NUM_PLAYER_CLASSES);
    auto gui=std::make_shared<PlayerMenu>(nullptr,&menu,0);
    gui->inClassSelectionArea=true; guis[0]=gui;
    menu.close(); assert(!menu.isOpen() && !gui->inClassSelectionArea);
    gui->availableClasses={Player::CLASS_KNIGHT,Player::CLASS_MAGE}; gui->playerClass=3;
    assert(gui->getPlayerClass()==Player::CLASS_MAGE);
    gui->availableClasses.clear(); assert(gui->getPlayerClass()==Player::CLASS_KNIGHT);
    Input input(nullptr);
    for(unsigned i=1;i<=31;++i) {
        const float before=menu.caravanPosition;
        const float bounce=((expected()&0x7fffffffu)%20==0) ? .5f : 0.f;
        menu.step(input);
        assert(menu.frame==i && !menu.stopped && !menu.caravanSprite.getFrozen());
        assert(menu.caravanPosition==before-(i<30 ? static_cast<float>(i)*std::nextafter(.01f,1.f) : .3f));
        assert(menu.caravanSprite.getPosition()==sf::Vector2f(menu.caravanPosition,256.f+bounce));
        assert(_randEngine==expected);
    }
    menu.caravanPosition=56.1f; menu.frame=40;
    expected(); menu.step(input); assert(!menu.stopped && menu.caravanPosition<56.f);
    menu.step(input); assert(menu.stopped && menu.caravanPosition==56.f && menu.caravanSprite.getFrozen());
    assert(_randEngine==expected); // Stop is detected on the tick after overshoot; no RNG on stopped ticks.
    Sprite source({},"scenes/intro/caravan.png",128,80,-1,7,7.f,9.f,true,false);
    source.setFlipped(true); source.setPosition(90,100); source.setFrame(1); source.setTime(4);
    menu.syncCaravanSprite(source);
    assert(menu.caravanSprite.getFrame()==1 && menu.caravanSprite.getTime()==4);
    assert(menu.caravanSprite.getPosition()==sf::Vector2f(90,100) && menu.caravanPosition==90.f);
    assert(menu.caravanPositionY==124.f && !menu.stopped);
    menu.setNetworked(true); menu.syncCaravanSprite(source);
    assert(menu.caravanPosition==56.f && menu.stopped);
    sf::RenderTexture target; assert(target.create(256,180));
    menu.caravanDarkness=1.f; target.clear(sf::Color::Transparent);
    menu.drawCaravan(target,.5f); target.display();
    assert(menu.caravanSprite.getColor()==sf::Color(81,81,81,127));
    const auto pixel=target.getTexture().copyToImage().getPixel(100,110);
    assert(pixel.a==127); // The translated draw reaches the SFML render target.
    menu.setHidden(true); const float darkness=menu.caravanDarkness;
    target.clear(sf::Color::Transparent); menu.drawCaravan(target,1); target.display();
    assert(menu.caravanDarkness==darkness && target.getTexture().copyToImage().getPixel(100,110).a==0);
    menu.setHidden(false); menu.setNetworked(false);
    auto other=std::make_shared<Player>(nullptr,0,0,Player::CLASS_KNIGHT,1);
    other->playerClass=Player::CLASS_KNIGHT; other->palette=1;
    players[1]=other;
    for(unsigned i=0;i<20;++i) {
        const int choices[]{0,2,3}; const int selection=choices[(expected()&0x7fffffffu)%3];
        assert(menu.randomPalette(0,Player::CLASS_KNIGHT)==selection && _randEngine==expected);
    }
    const int allChoice=(expected()&0x7fffffffu)%4;
    assert(menu.randomPalette(1,Player::CLASS_KNIGHT)==allChoice && _randEngine==expected);
    menu.currentPlayer={2,0,3}; menu.removePlayer(0,nullptr);
    assert((menu.currentPlayer==std::vector<unsigned>{2,3}) && menu.playerClasses[0]==Player::NUM_PLAYER_CLASSES);
    // Occupied slots return before creating a player or consuming any RNG.
    menu.addPlayer(1,nullptr,Player::CLASS_MAGE,Background::BG_NONE,-1);
    std::vector<Affinity::AffinityType> affinities;
    menu.addRandomizedPlayer(1,nullptr,affinities); assert(_randEngine==expected && players[1]==other);
    gui->choosingClass=true; menu.menu(0); // updateAllPlayerStats remains a dependency stub.
    menu.setNetworked(true); menu.menu(0); assert(_randEngine==expected);
    guis.clear(); players.clear(); ResourceManager::textures.clear(); ResourceManager::setConfig(nullptr);
    std::cout<<"PlayerCustomizeMenu: construction, reset/close, class getter, motion/RNG, stop overshoot, sync, rendering, palettes and early-return paths passed\n";
}
