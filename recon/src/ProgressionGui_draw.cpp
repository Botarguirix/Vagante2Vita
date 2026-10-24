#include "recon_all.h"
#include "localization.h"
#include <algorithm>
void ProgressionGui::drawItems(sf::RenderTarget &target){
    for(auto &entry:guiItems){
        auto &item=*entry.second;if(!item.alive)continue;
        item.sprite.draw(target,{});
        if(!item.isCard && active==entry.first){item.sprite.setColor({100,100,100});item.sprite.draw(target,{});}
        else if(hot==entry.first){auto color=item.sprite.getColor();color.a=item.isCard ? 80:255;item.sprite.setColor(color);item.sprite.draw(target,sf::RenderStates(sf::BlendAdd));color.a=255;item.sprite.setColor(color);}
        for(const auto &line:(item.sprite.getFrame()<4 ? item.label : item.description).lines)target.draw(line);
    }
}
void ProgressionGui::drawMenu(sf::RenderTarget &target){
    auto view=target.getView();view.setCenter(320,180);target.setView(view);
    sf::RectangleShape shade(view.getSize());shade.setPosition(view.getCenter()-view.getSize()*.5f);shade.setFillColor({8,9,20,190});target.draw(shade);
    drawItems(target);
    if(auto *font=ResourceManager::getDefaultFont()){
        sortLabel.setFont(*font);pageLabel.setFont(*font);target.draw(sortLabel);target.draw(pageLabel);
        const auto title=recon::translate("Unlocks");sf::Text text(sf::String::fromUtf8(title.begin(),title.end()),*font,16);
        auto bounds=text.getLocalBounds();text.setOrigin(bounds.width*.5f,bounds.height*.5f);text.setPosition(320,14);target.draw(text);
    }
}
void ProgressionGui::drawGameOver(sf::RenderTarget &target,sf::Vector2f &position){
    auto *font=ResourceManager::getDefaultFont();
    const float alpha=gameOverState<1 ? gameOverStateProgress : 1.f;
    const float progress=gameOverState<2 ? 0.f : gameOverState>2 ? 1.f : gameOverStateProgress;
    const sf::Color color(243,245,78,sf::Uint8(alpha*255.f));
    float points=float(prevProgressionPoints)*(1.f-progress)+float(Stats::progressionPoints)*progress;
    const auto interval=Stats::getProgressionInterval(int(points));
    const bool max=interval.first==interval.second;
    float total=100.f,current=100.f;
    if(!max){total=float(interval.second-interval.first);points-=float(interval.first);current=std::min(total,points);}
    const float ratio=current/total;
    sf::RectangleShape bar({400,3});bar.setPosition(position.x-200.f,position.y-70.f);bar.setFillColor(sf::Color::Transparent);bar.setOutlineColor(color);bar.setOutlineThickness(.5f);target.draw(bar);
    if(font){
        sf::Text text(max ? "MAX" : std::to_string(int(current))+" / "+std::to_string(int(total)),*font,16);text.setScale(.5f,.5f);text.setFillColor(color);
        text.setOrigin(text.getLocalBounds().width,0);auto bounds=bar.getGlobalBounds();text.setPosition(bounds.left+bounds.width,bounds.top-10.f);target.draw(text);
        bar.setSize({ratio*400.f,3});bar.setFillColor(color);target.draw(bar);
        text.setString("L"+std::to_string(Stats::progressionLevel));text.setOrigin(0,0);bounds=bar.getGlobalBounds();text.setPosition(bounds.left,bounds.top-10.f);target.draw(text);
    }
    if(!Stats::newlyUnlocked.empty() && gameOverState>2){
        sf::Vector2f card(position.x-float(Stats::newlyUnlocked.size()-1)*52.5f,position.y-20.f);
        pushAnimation(ANIM_CARD_FLIP);for(const auto &key:Stats::newlyUnlocked){doCard(key,card,false,true);card.x+=105.f;}popAnimation();
        if(font){const auto message=recon::translate(Stats::newlyUnlocked.size()<2 ? "New Unlock!":"New Unlocks!");sf::Text text(sf::String::fromUtf8(message.begin(),message.end()),*font,16);text.setScale(.5f,.5f);text.setOrigin(text.getLocalBounds().width*.5f,0);text.setOutlineColor({50,50,20});text.setOutlineThickness(2);text.setPosition(position.x,position.y-63.f);target.draw(text);}
    }
    drawItems(target);
}

