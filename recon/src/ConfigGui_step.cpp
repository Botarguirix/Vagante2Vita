#include "recon_all.h"
#include <algorithm>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
namespace {
void mouseHover(GuiElement *element,sf::Vector2f mouse,bool afk,bool &didHover){
    const bool contains=element->contains(int(mouse.x),int(mouse.y));didHover=didHover || contains;element->hover=!afk && contains;
}
}
void ConfigGui::stepMain(){
    if(!exitDialogWindow->isHidden()){
        exitDialogWindow->step(int(mousePos.x),int(mousePos.y));
        for(auto *element:{exitDialogChoiceOne,exitDialogChoiceTwo,exitDialogCancel})mouseHover(element,mousePos,mouseAfk,didMouseHover);
        if(exitDialogChoiceOne->pollAction()){exitGame=true;return;}
        if(exitDialogChoiceTwo->pollAction()){exitToMainMenu=true;return;}
        if(exitDialogCancel->pollAction()){exitDialogWindow->setHidden(true);++selectPosition.y;return;}
    }else for(unsigned i=0;i<mainElements.size();++i){
        auto *element=mainElements[i];mouseHover(element,mousePos,mouseAfk,didMouseHover);element->step(int(mousePos.x),int(mousePos.y));
        if(element->type!=GuiElement::BUTTON || !static_cast<GuiButton *>(element)->pollAction())continue;
        switch(i){case 0:screen=OPTIONS_SCREEN;switchTimer=10;break;case 1:screen=CONTROLS_SCREEN;switchTimer=10;break;case 2:screen=UNLOCKS_SCREEN;break;case 3:runCredits=true;[[fallthrough]];case 5:toggle();break;case 4:exitDialogWindow->setHidden(false);break;}
    }
}
void ConfigGui::stepOptions(){
    for(unsigned i=0;i<configElements.size();++i){
        auto *element=configElements[i];mouseHover(element,mousePos,mouseAfk,didMouseHover);element->step(int(mousePos.x),int(mousePos.y));
        if(element->type==GuiElement::BUTTON){if(static_cast<GuiButton *>(element)->pollAction()){screen=MAIN_SCREEN;switchTimer=10;}}
        else if(element->type==GuiElement::SLIDER){
            auto *slider=static_cast<GuiSlider *>(element);const float value=slider->getValue();
            if(i==0){config.musicVolume=value*100.f*value;ResourceManager::setMusicVolume(config.musicVolume);}
            else if(i==1){
                config.sfxVolume=value*100.f*value;ResourceManager::setGlobalSfxVolume(config.sfxVolume);
                if(slider->pollAction()){
                    const unsigned sample=(_randEngine()&0x7fffffffu)%4;
                    static const char *sounds[]={"player/gold_get1.wav","player/drink_potion1.wav","player/hurt1.wav","player/blade_hit1.wav"};ResourceManager::playSound(sounds[sample],false);
                }
            }else if(i==2){if(config.gamma!=value+1.f){config.gamma=value+1.f;config.updateGamma();}}
            else if(i==3){if(config.viewZoom!=value+1.f)config.viewZoom=value+1.f;}
            else if(i==4){if(config.minimapScale!=value+1.f)config.minimapScale=value+1.f;}
        }else if(element->type==GuiElement::CHECKBOX){
            const bool value=static_cast<GuiCheckbox *>(element)->isChecked();
            switch(i){
            case 5:config.particlesEnabled=value;break;
            case 6:config.loadFromArchive=value;break;
            case 7:if(value!=config.fullScreen){config.fullScreen=value;config.updateWindowSize(window);config.doResized=true;}break;
            case 8:shaderWarningLabel->setHidden(element!=hoveredElement);if(value!=config.postProcessingEnabled){config.postProcessingEnabled=value;gammaSlider->setEnabled(value);config.doResized=true;}break;
            case 9:if(value!=config.useVertexArrays){config.useVertexArrays=value;VLOG(LOG_DEBUG)<<"vertex arrays "<<(value ? "enabled":"disabled");}break;
            case 10:if(value!=config.verticalSync){config.verticalSync=value;VLOG(LOG_DEBUG)<<"vsync "<<(value ? "enabled":"disabled");window.setVerticalSyncEnabled(value);}break;
            case 11:if(value!=config.enableMenuToolTips){config.enableMenuToolTips=value;VLOG(LOG_DEBUG)<<"menu tooltips "<<(value ? "enabled":"disabled");}break;
            case 12:if(value!=config.enableInGameTooltips){config.enableInGameTooltips=value;VLOG(LOG_DEBUG)<<"in-game tooltips "<<(value ? "enabled":"disabled");}break;
            case 13:config.debugLevelGen=value;break;
            case 14:config.cumulativeCombatText=value;break;
            case 15:config.drawNetInformation=value;break;
            case 16:if(value!=config.forcePerfectPixelScaling){config.forcePerfectPixelScaling=value;config.doResized=true;}break;
            case 17:if(value!=config.supportDirectInput){config.supportDirectInput=value;EJoystick::updateDirectInput=value;}break;
            }
        }
    }
    if(selectPosition.y==18)selectPosition.y=17;
}
void ConfigGui::step(){
    updateButtonStates();
    if(closingTimer){switchTimer=0;--closingTimer;}else if(openingTimer)--openingTimer;else if(switchTimer)--switchTimer;
    ++frame;std::fill(directionPressedThisFrame.begin(),directionPressedThisFrame.end(),false);
    const auto previous=mousePos;mousePos=window.mapPixelToCoords(sf::Mouse::getPosition(window));
    const bool left=sf::Mouse::isButtonPressed(sf::Mouse::Left),right=sf::Mouse::isButtonPressed(sf::Mouse::Right);
    didMouseHover=false;std::vector<GuiElement *> *elements=nullptr;
    if(screen==MAIN_SCREEN){const bool dialogHidden=exitDialogWindow->isHidden();stepMain();if(dialogHidden)elements=&mainElements;}
    else if(screen==OPTIONS_SCREEN){elements=&configElements;stepOptions();}
    else if(screen==CONTROLS_SCREEN){elements=&controlsElements;stepControls();}
    if(mousePos==previous || !didMouseHover)++mouseLastMovedTimer;else mouseLastMovedTimer=0;
    mouseAfk=mouseLastMovedTimer>10;selectPosition.y=std::max(selectPosition.y,0);
    if(!elements && exitDialogWindow->isHidden())return;
    const bool dialog=screen==MAIN_SCREEN && !exitDialogWindow->isHidden();
    if(dialog)selectPosition.y=std::min(selectPosition.y,2);
    else{selectPosition.y=std::min(selectPosition.y,int(elements->size())-1);if(screen==OPTIONS_SCREEN && selectPosition.x>1)selectPosition.x=0;}
    int hoverIndex=-1;
    if(!exitDialogWindow->isHidden()){
        // The original tests each child's hover flag here, not its hidden flag.
        if(exitDialogChoiceOne->hover){hoverIndex=0;hoveredElement=exitDialogChoiceOne;}
        else if(exitDialogChoiceTwo->hover){hoverIndex=1;hoveredElement=exitDialogChoiceTwo;}
        else if(exitDialogCancel->hover){hoverIndex=2;hoveredElement=exitDialogCancel;}
    }else for(unsigned i=0;i<elements->size();++i)if((*elements)[i]->hover){hoveredElement=(*elements)[i];hoverIndex=int(i);}
    if(hoverIndex>=0){
        if(screen==CONTROLS_SCREEN || screen==OPTIONS_SCREEN){auto &positions=screen==CONTROLS_SCREEN ? posToControlsIndex:posToSettingsIndex;for(const auto &entry:positions)if(entry.second==unsigned(hoverIndex)){selectPosition={entry.first.x,entry.first.y};break;}}
        else selectPosition.y=hoverIndex;
    }else{
        if(dialog){hoveredElement=selectPosition.y==0 ? exitDialogChoiceOne : selectPosition.y==1 ? exitDialogChoiceTwo:exitDialogCancel;}
        else if(selectPosition.y<int(elements->size())){const unsigned index=screen==CONTROLS_SCREEN || screen==OPTIONS_SCREEN ? selectIndex() : unsigned(selectPosition.y);if(index<elements->size())hoveredElement=(*elements)[index];}
        if(hoveredElement)hoveredElement->hover=true;
    }
    guiCursor.setTarget(hoveredElement);guiCursor.step();
    if(exitDialogWindow->isHidden()){for(auto *element:*elements)element->afterStep(left || mouseAfk,right || mouseAfk);}
    else exitDialogWindow->afterStep(left || mouseAfk,right || mouseAfk);
}
