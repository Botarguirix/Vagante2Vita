#include "recon_all.h"
#include <algorithm>
#include <cmath>
bool ProgressionGui::step(sf::RenderWindow &window,sf::View &view,bool menuOpen){
    view.setCenter(320,180);
    auto mouse=window.mapPixelToCoords(sf::Mouse::getPosition(window),view);
    if(std::abs(mouse.x-mousePosition.x)<1.f && std::abs(mouse.y-mousePosition.y)<1.f)++mouseStillTimer;else mouseStillTimer=0;
    mousePosition=mouse;mouseActive=mouseStillTimer<30;
    for(auto it=guiItems.begin();it!=guiItems.end();){
        auto &item=*it->second;
        if(!item.alive){delete it->second;it=guiItems.erase(it);continue;}
        ++item.frame;item.alive=false;item.sprite.setPosition(item.position);
        if(item.isCard && item.animationType!=ANIM_SHIFT_LEFT){
            float angle=0;bool setFrame=true;
            switch(item.animationType){
            case ANIM_CARD_RESPONSIVE:
                if(item.frame<25)item.animationProgress=.5f-float(item.frame)*.02f;
                else if(hot==it->first)item.animationProgress=std::min(item.animationProgress+.07f,1.f);
                else item.animationProgress=std::max(item.animationProgress-.05f,0.f);
                angle=item.animationProgress*3.1415927410125732f+1.5707963705062866f;
                item.sprite.setScale((item.sprite.getFlipped() ? -1.f:1.f)*std::abs(std::sin(angle)),1.f);break;
            case ANIM_CARD_FLIP:{
                const float scale=std::sqrt(std::min(float(item.frame)*.03333333507180214f,1.f));
                const int time=std::max(item.frame-120,0);if(time>=300)item.frame=120;
                const int phase=time<240 ? std::min(time,60) : time-60;
                angle=float(phase)*.05235988274216652f+4.71238899230957f;
                item.sprite.setScale((item.sprite.getFlipped() ? -1.f:1.f)*std::abs(std::sin(angle)*scale),scale);break;
            }
            case ANIM_CARD_FADE:{
                const float scale=item.sprite.getScale().y*.75f;item.sprite.setScale((item.sprite.getFlipped() ? -1.f:1.f)*std::abs(scale),scale);
                auto color=item.sprite.getColor();color.a=sf::Uint8(float(color.a)*.75f);item.sprite.setColor(color);setFrame=false;break;
            }
            default:item.sprite.setFrame(item.cardType+4);setFrame=false;break;
            }
            if(setFrame)item.sprite.setFrame(item.cardType+(std::fmod(angle,6.2831854820251465f)>=3.1415927410125732f ? 4:0));
        }
        for(auto *text:{&item.label,&item.description})for(auto &line:text->lines){
            line.setPosition(item.position);auto scale=item.sprite.getScale();line.setScale(std::abs(scale.x)*.5f,scale.y*.5f);line.setRotation(item.sprite.getRotation());
        }
        ++it;
    }
    bool result=menuOpen;
    if(menuOpen){
        const int lastPage=(int(Stats::unlocks.size())-1)/12;
        auto center=[](sf::Text &label){auto bounds=label.getLocalBounds();label.setOrigin(bounds.width*.5f,bounds.height*.5f);};
        auto pageText=[&](){pageLabel.setString("Page "+std::to_string(cardsPage+1)+"/"+std::to_string(lastPage+1));pageLabel.setPosition(320,56);center(pageLabel);};
        auto rebuild=[&](){
            unlocksView.clear();for(const auto &entry:Stats::unlocks)unlocksView.emplace_back(entry.first,entry.second);
            sortLabel.setString(sortOrder==0 ? "Unlocked First":"Locked First");
            std::sort(unlocksView.begin(),unlocksView.end(),[&](const auto &a,const auto &b){
                const auto &x=a.second;const auto &y=b.second;
                if(x.unlocked!=y.unlocked)return sortOrder==0 ? x.unlocked : !x.unlocked;
                if(x.type!=y.type)return x.type<y.type;
                if(x.hasCondition!=y.hasCondition)return !x.hasCondition;
                return x.progressionLevel<y.progressionLevel;
            });center(sortLabel);
        };
        if(!prevMenuOpen){rebuild();pageText();}
        auto sound=[](){ResourceManager::playSound("menu/menu_use_button.wav",false);};
        if(doButton("button_back",{194,44},2,true)){sound();result=false;}
        if(doButton("button_prev",{278,44},0,cardsPage>0)){
            sound();if(cardsPage>0){--cardsPage;pageText();pageAnimTimer=0;pageDirection=-1;}
        }
        if(doButton("button_next",{362,44},1,cardsPage<lastPage)){
            sound();if(cardsPage<lastPage){++cardsPage;pageText();pageAnimTimer=0;pageDirection=1;}
        }
        sortLabel.setPosition(446,56);
        if(doButton("button_sort",{446,44},3,true)){
            cardsPage=0;sound();sortOrder=(sortOrder+1)%2;rebuild();pageText();
            for(auto &entry:guiItems)if(entry.second->isCard)entry.second->frame=0;pageDirection=0;
        }
        pushAnimation(ANIM_CARD_RESPONSIVE);
        for(std::size_t i=0;i<unlocksView.size();++i){
            const auto &entry=unlocksView[i];const int page=int(i)/12,index=int(i)%12;
            sf::Vector2f position(float(index%3)*147.f+173.f,float(index/3)*74.f+102.f);
            if(page==cardsPage){
                position.x+=float(pageDirection)*std::max(1.f-float(pageAnimTimer)*.06666667014360428f,0.f)*500.f;
                doCard(entry.first,position,true,entry.second.unlocked);
                if(hot==entry.first)selection={index%3,index/3+1};
            }else if(pageAnimTimer<60 && (page==cardsPage-1 || page==cardsPage+1) && guiItems.find(entry.first)!=guiItems.end()){
                position.x+=float(pageAnimTimer)*50.f*(page<cardsPage ? -1.f:1.f);
                pushAnimation(ANIM_SHIFT_LEFT);doCard(entry.first,position,true,entry.second.unlocked);popAnimation();
            }
        }
        popAnimation();
    }
    ++pageAnimTimer;prevMenuOpen=menuOpen;return result;
}


