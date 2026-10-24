#include "port_lib_types.h"
#define private public
#include "MainMenu.h"
#include "GuiButton.h"
#include "ResourceManager.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    Config config;ResourceManager::setConfig(&config);ResourceManager::muted=true;
    sf::RenderWindow window(sf::VideoMode(640,360),"MainMenu fixture",sf::Style::None);window.setVisible(false);
    Input input(nullptr);ConfigGui settings(window,config,input);NetMenu network(window,input,nullptr);
    sf::Image atlas;atlas.create(128,80,sf::Color::White);sf::Texture texture;assert(texture.loadFromImage(atlas));
    const char *names[]={"sky","sky_stars","moon","ground","clouds","mountains","midground_treeline_1","foreground_treeline","caravan","signpost"};
    for(auto *name:names)ResourceManager::textures[std::string("scenes/intro/")+name+".png"]=&texture;
    ResourceManager::textures["gui/title.png"]=&texture;
    ResourceManager::textures["scenes/intro/entities/little_firefly.png"]=&texture;ResourceManager::textures["scenes/intro/entities/big_firefly.png"]=&texture;
    sf::Font font;assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));ResourceManager::fonts["Tiny_04b.ttf"]=&font;ResourceManager::fonts["NotoSansCJKkr-Regular.otf"]=&font;
    {
        _randEngine.seed(123);const auto initial=_randEngine;
        MainMenu menu(window,input,settings,network);
        assert(_randEngine==initial && menu.sceneDoodads.size()==9 && menu.guiElements.size()==4);
        assert(menu.newGameButton->getName()=="New Game" && menu.newGameButton->getX()==526 && menu.exitButton->getY()==285);
        assert(menu.caravanPosition==-814 && menu.targetCaravanPosition==243 && menu.animateOutTime==90);
        menu.down();menu.down();assert(menu.cursorIndex==1);menu.up();assert(menu.cursorIndex==0);
        menu.hasFocus=false;menu.action(true);assert(!menu.skipIntro);menu.hasFocus=true;menu.action(true);
        assert(menu.skipIntro && menu.introTimer==400 && menu.caravanPosition==0 && menu.fadeInCaravan);
        auto expected=_randEngine;expected();menu.stepBackground(1);assert(_randEngine==expected && menu.caravanPosition==0 && menu.movePos==1);
        expected=_randEngine;menu.stepBackground(-.42857143f);assert(_randEngine==expected && !menu.caravanBump);
        menu.sceneDoodads[0].pos.x=741;menu.stepBackground(0);assert(menu.sceneDoodads[0].pos.x<0);
        menu.action(true);menu.action(false);menu.step();assert(menu.fadingOut && menu.fadingOutTarget==0 && menu.animateOutTimer==89);
        for(int i=0;i<89;++i)menu.step();assert(menu.isReadyToStart());
        menu.setReadyToStart(false);menu.reset(false,true);assert(menu.skipIntro && menu.buttonAlpha==1 && menu.titlePos==172 && !menu.fadingOut);
        menu.cursorIndex=1;menu.action(true);menu.action(false);menu.step();assert(menu.fadingOutTarget==1 && menu.animateOutTimer==39);
        menu.fadingOutTarget=2;menu.animateOutTimer=1;menu.step();assert(menu.openProgressionGui() && !menu.openProgressionGui());
        menu.fadingOut=true;menu.fadingOutTarget=3;menu.animateOutTimer=1;menu.step();assert(menu.viewCredits() && !menu.viewCredits() && !menu.isOpen());
        menu.skip();assert(menu.isReadyToStart() && menu.getCaravanSprite().getPosition()==sf::Vector2f(50,232));
        sf::RenderTexture target;assert(target.create(640,360));menu.fadeInTimer=0;menu.fadingOut=false;
        for(float day:{0.f,.5f,1.f}){menu.drawBackground(target,day);assert(menu.skySprite.getColor().a==static_cast<sf::Uint8>(day*255));}
        menu.draw(target,0);assert(menu.skySprite.getColor().a==255); // retained float ignored
        for(int fade=0;fade<4;++fade){menu.fadingOut=true;menu.fadingOutTarget=fade;menu.animateOutTime=90;menu.animateOutTimer=45;menu.draw(target,.1f);}
        menu.fadingOutTarget=0;menu.draw(target,0);assert(menu.skySprite.getColor().a==127);
        menu.fadeInTimer=15;menu.draw(target,1);target.display();
    }
    ResourceManager::fonts.clear();ResourceManager::textures.clear();ResourceManager::muted=false;window.close();
    std::cout<<"MainMenu: intro, navigation, fades/events, RNG schedule, background/title rendering and lifecycle passed\n";
}
