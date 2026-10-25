#include "recon_all.h"
#include "localization.h"
#include <algorithm>
#include <cmath>
void ConfigGui::draw(sf::RenderTarget &target,bool inGame){
    float alpha=openingTimer>0 ? 1.f-float(openingTimer)*.1f : closingTimer>0 ? float(closingTimer)*.1f : 1.f;
    target.setView(view);sf::RectangleShape shade(view.getSize());shade.setPosition(view.getCenter()-view.getSize()*.5f);shade.setFillColor({16,17,40,sf::Uint8(alpha*190.f)});target.draw(shade);
    if(!closingTimer)alpha=std::min(1.f-float(switchTimer)*.1f,alpha);
    const float entrance=std::sqrt(alpha);auto *font=ResourceManager::getDefaultFont();
    // lambda_e9f140... draws a UTF-8 heading at the view's horizontal center.
    auto title=[&](const std::string &message,float y){if(!font)return;sf::Text text(sf::String::fromUtf8(message.begin(),message.end()),*font,16);text.setPosition(target.getView().getCenter().x-text.getLocalBounds().width*.5f,y);text.setFillColor({230,230,230,sf::Uint8(alpha*255.f)});text.setOutlineColor(sf::Color::Black);text.setOutlineThickness(2.f);target.draw(text);};
    if(screen==MAIN_SCREEN){if(!exitDialogWindow->isHidden())title(recon::translate("Really Exit?"),62.f);else if(inGame)title(recon::translate("PAUSED"),entrance*36.f+26.f);}
    else if(screen==OPTIONS_SCREEN)title(recon::translate("OPTIONS"),entrance*36.f-16.f);
    else if(screen==CONTROLS_SCREEN && !input.controllers[currentPlayer].binding){auto label=recon::translate("Player {1} Controls");const auto where=label.find("{1}");if(where!=std::string::npos)label.replace(where,3,std::to_string(currentPlayer+1));title(label,entrance*36.f-32.f);}
    std::vector<GuiElement *> *elements=nullptr;
    if(screen==MAIN_SCREEN){if(!exitDialogWindow->isHidden()){exitDialogWindow->draw(target);exitDialogWindow->setAlpha(alpha);}else elements=&mainElements;}
    else if(screen==OPTIONS_SCREEN)elements=&configElements;
    else if(screen==CONTROLS_SCREEN)elements=&controlsElements;
    if(elements)for(auto *element:*elements){element->setAlpha(alpha);element->draw(target);}
    if(screen==CONTROLS_SCREEN){
        unsigned action=99999;bool alternate=false;
        for(unsigned i=0;i<controlsElements.size();++i){
            if(i==controlsButtonsOffset){action=0;alternate=false;}auto *element=controlsElements[i];
            if(element->type!=GuiElement::BUTTON)continue;
            if(action<input.actionNames.size() && !element->isHidden()){
                const auto &controller=input.controllers[currentPlayer];
                if(!controller.enabled || controller.binded[action])if(auto *sprite=input.getControllerButtonSprite(currentPlayer,static_cast<Input::Actions>(action),alternate)){
                    auto color=controller.enabled ? sf::Color::White : sf::Color(100,100,100);color.a=sf::Uint8(alpha*255.f);sprite->setColor(color);
                    sprite->setPosition(float(element->getX())+element->getOffset().x+float(element->getW())+20.f,float(element->getY())+float(element->getH())*.5f);sprite->draw(target,{});
                }
            }
            if(alternate)++action;alternate=!alternate;
        }
    }
    guiCursor.draw(target);
    if(screen==CONTROLS_SCREEN && font){int y=0;for(const auto &state:input.debugStates){
        sf::Text text(state,*font,8);text.setPosition(5.f,float(y)+5.f);text.setFillColor(sf::Color::Green);const auto bounds=text.getLocalBounds();
        sf::RectangleShape background({bounds.width,bounds.height+3.f});background.setFillColor(sf::Color::Black);background.setPosition(text.getPosition());background.setScale(.75f,.75f);text.setScale(.75f,.75f);target.draw(background);target.draw(text);y+=7;
    }}
}
