#include "port_lib_types.h"
#define private public
#include "ProgressionGui.h"
#include "ResourceManager.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    Config config;ResourceManager::setConfig(&config);ResourceManager::muted=true;
    sf::RenderWindow window(sf::VideoMode(640,360),"Progression fixture",sf::Style::None);window.setVisible(false);sf::View view=window.getView();
    sf::Image image;image.create(784,64,sf::Color::White);sf::Texture texture;assert(texture.loadFromImage(image));
    ResourceManager::textures["gui/secret_card.png"]=&texture;ResourceManager::textures["gui/unlocks_menu.png"]=&texture;
    sf::Font font;assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));ResourceManager::fonts["Tiny_04b.ttf"]=&font;ResourceManager::fonts["NotoSansCJKkr-Regular.otf"]=&font;
    Stats::unlocks.clear();for(int i=0;i<25;++i){Unlockable unlock{};unlock.unlocked=i%2;unlock.hasCondition=i%3==0;unlock.type=static_cast<Unlockable::UnlockableType>(i%5);unlock.name="Card "+std::to_string(i);unlock.conditions="A long sentence with several words that must wrap to fit on this card";unlock.progressionLevel=i;Stats::unlocks["card"+std::to_string(i)]=unlock;}
    {
        ProgressionGui gui;assert(gui.selection==sf::Vector2i(0,0) && gui.currAnimation.empty());
        assert(gui.step(window,view,true));assert(gui.guiItems.size()==16 && gui.pageLabel.getString()=="Page 1/3");
        for(int i=0;i<12;++i)assert(gui.unlocksView[i].second.unlocked);
        auto key=gui.unlocksView[0].first;auto *card=gui.guiItems.at(key);assert(card->size==sf::Vector2f(98,64) && card->isCard && card->alive);
        assert(card->label.lines.size()>=2 && card->description.lines.size()>1);
        assert(card->label.lines[0].getFillColor()==sf::Color(140,67,67));
        auto &locked=Stats::unlocks.at("card0");gui.pushAnimation(ProgressionGui::ANIM_CARD_FLIP);gui.doCard("card0",{100,100},false,false);gui.popAnimation();
        auto *lock=gui.guiItems.at("card0");assert(lock->label.lines[1].getString()=="LOCKED" && lock->label.lines[0].getFillColor()==sf::Color(56,56,56));
        gui.mouseActive=false;gui.guiItems.at("button_next")->doAction=true;assert(gui.step(window,view,true));assert(gui.cardsPage==1 && gui.pageDirection==1 && gui.pageAnimTimer==1);
        gui.guiItems.at("button_sort")->doAction=true;gui.step(window,view,true);assert(gui.cardsPage==0 && gui.sortOrder==1 && gui.pageDirection==0 && !gui.unlocksView.front().second.unlocked);
        Input input(nullptr);for(auto &controller:input.controllers){controller.enabled=true;controller.actions.assign(Input::NUM_ACTIONS,false);controller.prevActions.assign(Input::NUM_ACTIONS,false);}
        auto press=[&](Input::Actions action){input.controllers[0].actions[action]=true;gui.handleInput(&input);input.controllers[0].actions[action]=false;};
        gui.selection={3,0};press(Input::ACTION_DOWN);assert(gui.selection==sf::Vector2i(2,1));press(Input::ACTION_UP);assert(gui.selection==sf::Vector2i(3,0));
        gui.selection={0,0};press(Input::ACTION_JUMP);assert(gui.active=="button_back");input.controllers[0].prevActions[Input::ACTION_JUMP]=true;gui.handleInput(&input);assert(gui.guiItems.at("button_back")->doAction && gui.active.empty());input.controllers[0].prevActions[Input::ACTION_JUMP]=false;
        assert(!gui.step(window,view,true));
        gui.mousePosition={51,68};assert(gui.guiItemContainsPos({100,100},{98,64}));gui.mousePosition={149,100};assert(!gui.guiItemContainsPos({100,100},{98,64}));
        // The previous frame's position is used for hit testing, then the new position is stored.
        gui.mouseActive=true;gui.mousePosition={194,44};gui.hot.clear();gui.doButton("button_back",{200,44},2,true);assert(gui.hot=="button_back");
        gui.mouseActive=false;gui.guiItems.at("button_back")->doAction=true;assert(!gui.doButton("button_back",{200,44},2,false));assert(!gui.guiItems.at("button_back")->doAction);
        gui.doCard("card0",{100,100},false,true);lock=gui.guiItems.at("card0");lock->frame=29;lock->animationType=ProgressionGui::ANIM_CARD_FLIP;
        gui.step(window,view,false);assert(std::abs(lock->sprite.getScale().y-1.f)<1e-6f && lock->sprite.getFrame()==7);
        lock->alive=true;lock->animationType=ProgressionGui::ANIM_CARD_FADE;gui.step(window,view,false);assert(std::abs(lock->sprite.getScale().y-.75f)<1e-6f && lock->sprite.getColor().a==191);
        lock->alive=true;lock->animationType=ProgressionGui::ANIM_CARD_RESPONSIVE;lock->frame=24;lock->animationProgress=0;gui.hot="card0";gui.step(window,view,false);assert(std::abs(lock->animationProgress-.07f)<1e-6f);
        lock->alive=true;gui.step(window,view,false);lock->alive=true;
        sf::RenderTexture target;assert(target.create(640,360));gui.drawMenu(target);
        Stats::progressionPoints=150;Stats::progressionLevel=2;Stats::unlockPointThresholds={{"a",100},{"b",200}};Stats::newlyUnlocked={"card1"};gui.prevProgressionPoints=90;
        sf::Vector2f position(320,180);for(int state=0;state<5;++state){gui.gameOverState=state;gui.gameOverStateProgress=.5f;gui.drawGameOver(target,position);}target.display();
        assert(gui.guiItems.at("card1")->animationType==ProgressionGui::ANIM_CARD_FLIP && gui.currAnimation.empty());
        gui.step(window,view,false);gui.step(window,view,false);assert(gui.guiItems.empty());
    }
    Stats::newlyUnlocked.clear();Stats::unlocks.clear();ResourceManager::textures.clear();ResourceManager::fonts.clear();window.close();std::cout<<"ProgressionGui: paging, sorting, input, animation, text and drawing passed\n";
}

