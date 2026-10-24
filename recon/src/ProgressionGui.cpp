// PDB contract; Ghidra 004d6a20-004db680. See RAMA_A_PROGRESSIONGUI.md.
#include "recon_all.h"
#include "localization.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
namespace {
std::string selectionKey(sf::Vector2i selection,int page,const std::vector<std::pair<std::string,Unlockable>> &unlocks){
    static const char *buttons[]={"button_back","button_prev","button_next","button_sort"};
    if(selection.y==0 && selection.x>=0 && selection.x<4)return buttons[selection.x];
    const int index=(selection.y+page*4)*3-3+selection.x;
    return index>=0 && index<int(unlocks.size()) ? unlocks[index].first : "";
}
ProgressionGui::PrettyText cardText(const std::string &text,bool unlocked,bool title,Unlockable::UnlockableType type){
    ProgressionGui::PrettyText result;
    auto *font=ResourceManager::getDefaultFont();if(!font)return result;
    auto append=[&](const std::string &s){result.lines.emplace_back(sf::String::fromUtf8(s.begin(),s.end()),*font,16);result.lines.back().setScale(.5f,.5f);};
    if(title){
        static const char *categories[]={"BACKGROUND","CLASS","MUSIC","GAME MODE","SPECIAL"};
        if(unsigned(type)<5)append(recon::translate(categories[type]));
    }
    std::string remaining=title && !unlocked ? recon::translate("LOCKED") : text;
    append(remaining);
    // Split the last line, moving one word at a time to its successor, as in 004d98d0.
    while(result.lines.back().getGlobalBounds().width>=87.f){
        std::vector<std::string> words;std::istringstream stream(remaining);std::string word;
        while(std::getline(stream,word,' '))words.push_back(word);
        int attempts=0;int count=int(words.size());bool split=false;
        while(result.lines.back().getGlobalBounds().width>=87.f){
            --count;
            if(attempts>=21 || count<=0){VLOG(LOG_ERROR)<<"ProgressionGui::doCard: Failed to split string into multiple lines: "<<remaining;break;}
            std::string first;
            for(int i=0;i<count;++i){if(i)first+=' ';first+=words[i];}
            result.lines.back().setString(sf::String::fromUtf8(first.begin(),first.end()));++attempts;
            if(result.lines.back().getGlobalBounds().width<87.f){
                remaining.clear();for(int i=count;i<int(words.size());++i){if(i!=count)remaining+=' ';remaining+=words[i];}
                append(remaining);split=true;break;
            }
        }
        if(!split)break;
    }
    static const sf::Color colors[]={{140,67,67},{60,121,140},{73,150,70},{115,65,147},{60,140,85}};
    for(std::size_t i=0;i<result.lines.size();++i){
        auto &line=result.lines[i];line.setOrigin(line.getLocalBounds().width*.5f,(float(result.lines.size())*.5f-float(i))*14.f);
        sf::Color color(200,200,200);
        if(title && i==0)color=unlocked ? (unsigned(type)<5 ? colors[type] : sf::Color::Black) : sf::Color(87,87,87);
        if(!unlocked){color.r=sf::Uint8(float(color.r)*.65f);color.g=sf::Uint8(float(color.g)*.65f);color.b=sf::Uint8(float(color.b)*.65f);}
        line.setFillColor(color);line.setOutlineColor(sf::Color::Black);line.setOutlineThickness(2.f);
    }
    return result;
}
}
const float ProgressionGui::CARD_W=98.f;
const float ProgressionGui::CARD_H=64.f;
ProgressionGui::ProgressionGui():mouseActive(false),mouseStillTimer(0),mousePosition(0,0),selection(0,0),cardsPage(0),prevMenuOpen(false),sortOrder(0),pageDirection(0),pageAnimTimer(0),gameOverTimer(0),gameOverState(0),gameOverStateProgress(0){
    for(auto *label:{&pageLabel,&sortLabel}){label->setCharacterSize(16);label->setScale(.5f,.5f);label->setFillColor({200,200,200});label->setOutlineColor({20,20,20});label->setOutlineThickness(2.f);}
}
ProgressionGui::~ProgressionGui(){for(auto &entry:guiItems)delete entry.second;}
void ProgressionGui::pushAnimation(GuiItemAnimation animation){currAnimation.push(animation);}
void ProgressionGui::popAnimation(){currAnimation.pop();}
bool ProgressionGui::guiItemContainsPos(sf::Vector2f position,sf::Vector2f size){return mousePosition.x>=position.x-size.x*.5f && mousePosition.x<position.x+size.x*.5f && mousePosition.y>=position.y-size.y*.5f && mousePosition.y<position.y+size.y*.5f;}
bool ProgressionGui::updateStates(std::string key,GuiItem *item){
    bool result=false;
    if(mouseActive){
        const bool contains=guiItemContainsPos(item->position,item->size);
        if(contains)hot=key;
        if(active==key && !sf::Mouse::isButtonPressed(sf::Mouse::Left)){
            if(contains){hot=key;result=true;}else hot.clear();active.clear();
        }else if(hot==key && sf::Mouse::isButtonPressed(sf::Mouse::Left)){focused=key;active=key;}
    }
    return result;
}
bool ProgressionGui::doButton(std::string key,sf::Vector2f position,int frame,bool enabled){
    auto found=guiItems.find(key);
    if(found==guiItems.end()){
        auto *item=new GuiItem{};item->sprite=Sprite({},"gui/unlocks_menu.png",64,16,-1,0,0,0,false,false);item->sprite.setFrame(frame);item->size={64,16};
        found=guiItems.emplace(key,item).first;
    }
    auto *item=found->second;bool result=enabled && updateStates(key,item);
    item->sprite.setColor(enabled ? sf::Color::White : sf::Color(130,130,130));item->alive=true;item->position=position;
    if(item->doAction){if(enabled)result=true;item->doAction=false;}
    return result;
}
bool ProgressionGui::doCard(std::string key,sf::Vector2f position,bool enabled,bool unlockedColor){
    auto found=guiItems.find(key);
    if(found==guiItems.end()){
        auto *item=new GuiItem{};item->isCard=true;
        item->sprite=Sprite({},"gui/secret_card.png",98,64,-1,0,0,0,false,false);item->sprite.setColor(unlockedColor ? sf::Color::White : sf::Color(180,180,200));item->size={98,64};
        auto unlock=Stats::unlocks.find(key);
        if(unlock==Stats::unlocks.end()){VLOG(LOG_ERROR)<<"ProgressionGui::doCard: Unlock not found: "<<key;}
        else{
            const auto &data=unlock->second;
            item->label=cardText(data.name,data.unlocked,true,data.type);item->description=cardText(data.conditions,data.unlocked,false,Unlockable::BACKGROUND);
            switch(data.type){case Unlockable::CLASS:case Unlockable::SPECIAL:item->cardType=0;break;case Unlockable::GAME_MODE:item->cardType=1;break;case Unlockable::MUSIC:item->cardType=2;break;case Unlockable::BACKGROUND:item->cardType=3;break;}
        }
        found=guiItems.emplace(key,item).first;
    }
    auto *item=found->second;bool result=enabled && updateStates(key,item);item->alive=true;item->position=position;
    item->animationType=currAnimation.empty() ? ANIM_NONE : currAnimation.top();
    if(item->doAction){result=true;item->doAction=false;}return result;
}
void ProgressionGui::handleInput(Input *input){
    if(sf::Mouse::isButtonPressed(sf::Mouse::Left)){mouseActive=true;mouseStillTimer=0;}
    bool pressed=false,released=false,left=false,right=false,up=false,down=false;
    for(unsigned i=0;i<4;++i){
        if(input->isPressed(Input::ACTION_MELEE,i)||input->isPressed(Input::ACTION_JUMP,i)||input->isPressed(Input::ACTION_RANGE,i))pressed=true;
        else if(input->isReleased(Input::ACTION_MELEE,i)||input->isReleased(Input::ACTION_JUMP,i)||input->isReleased(Input::ACTION_RANGE,i))released=true;
        if(input->isPressed(Input::ACTION_UP,i))up=true;if(input->isPressed(Input::ACTION_DOWN,i))down=true;
        if(input->isPressed(Input::ACTION_LEFT,i))left=true;if(input->isPressed(Input::ACTION_RIGHT,i))right=true;
    }
    const auto key=selectionKey(selection,cardsPage,unlocksView);
    if(pressed){active=key;focused=key;}
    if(released && active==key){active.clear();auto found=guiItems.find(key);if(found!=guiItems.end())found->second->doAction=true;}
    if(!released){
        if(left){if(selection.x>0)--selection.x;}
        else if(right){if(selection.x<3-(selection.y!=0))++selection.x;}
        else if(up){if(selection.y>0){--selection.y;if(selection.y==0 && selection.x==2)selection.x=3;}}
        else if(down && selection.y<4){if(selection.y==0)selection.x=std::min(selection.x,2);++selection.y;}
    }
    if(left||right||up||down||pressed||released){mouseActive=false;mouseStillTimer=60;hot=selectionKey(selection,cardsPage,unlocksView);}
}
