#include "recon_all.h"
#include "localization.h"
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
// SDK wrapper listed by --unit ConfigGui (@0058b910). No Steam APIs in this port.
void SteamInternal_Init_SteamController(ISteamController **) { /* Steam initialization excluded. */ }
void ConfigGui::updateInputController(Input::ControllerIdentifier &device){
    auto &controller=input.controllers[currentPlayer];controller.tryToUseJoystick=!device.keyboard;
    if(!device.keyboard){
        controller.usingJoystick=false;controller.usingSteamController=device.steamController;
        if(!device.steamController)controller.joystickId=device.joystickId;
        else{controller.steamControllerHandle=device.steamControllerHandle;VLOG(LOG_DEBUG)<<"Successfully selected steam controller from inputSelection. Assigning controller...";VLOG(LOG_DEBUG)<<"handle: "<<controller.steamControllerHandle;}
    }else controller.usingSteamController=false;
    controlsRebindAllButton->updateName(recon::translate(!device.keyboard && device.steamController ? "Rebind Steam":"Rebind All"));
}
void ConfigGui::updateInputSelection(bool restoreSelection){
    input.updateGamepadStatus();const auto &controller=input.controllers[currentPlayer];
    const auto devices=input.controllersDetected;std::vector<std::string> names;for(const auto &device:devices)names.push_back(device.name);
    inputSelection->setEntries(names);
    profileSelectLabel->setString(recon::translate(controller.usingJoystick ? "Gamepad Presets":"Keyboard Presets"));
    profileIndices.clear();for(unsigned i=0;i<input.inputProfiles.size();++i)if(input.inputProfiles[i].keyboard==!controller.usingJoystick)profileIndices.push_back(i);
    names.clear();for(const int index:profileIndices){
        if(index<0 || index>=int(input.inputProfiles.size())){VLOG(LOG_ERROR)<<"ConfigGui::updateInputSelection Invalid profile index: "<<index;}
        else names.push_back(input.inputProfiles[index].profileName);
    }
    profileSelection->setEntries(names);loadProfileButton->setY(profileSelection->getY()+profileSelection->getH()+2);
    if(restoreSelection){
        VLOG(LOG_DEBUG)<<"Loaded input selection";
        if(!controller.usingJoystick){VLOG(LOG_DEBUG)<<"!usingJoystick, selecting keyboard. "<<currentPlayer;inputSelection->select(0);}
        else if(!controller.usingSteamController){
            VLOG(LOG_DEBUG)<<"Update input selection. "<<devices.size();
            for(unsigned i=0;i<devices.size();++i)if(devices[i].joystickId==int(controller.joystickId) && devices[i].name==controller.joystickName){VLOG(LOG_DEBUG)<<"found joystick";inputSelection->select(i);break;}
        }else for(unsigned i=0;i<devices.size();++i)if(devices[i].steamController && devices[i].steamControllerHandle==controller.steamControllerHandle){
            VLOG(LOG_DEBUG)<<"Found steam controller handle from inputEntries, updating inputSelection.";inputSelection->select(i);break;
        }
    }
}
void ConfigGui::stepControls(){
    if(input.controllers[currentPlayer].binding && !input.controllers[currentPlayer].enabled)input.cancelBinding(currentPlayer);
    input.updateGamepadStatus();updateInputSelection(input.someConnectEvent);input.someConnectEvent=false;
    unsigned action=99999;bool alternate=false;
    for(unsigned i=0;i<controlsElements.size();++i){
        auto &controller=input.controllers[currentPlayer];if(i==controlsButtonsOffset){action=0;alternate=false;}
        auto *element=controlsElements[i];const bool contains=element->contains(int(mousePos.x),int(mousePos.y));didMouseHover=didMouseHover || contains;element->hover=!mouseAfk && contains;element->step(int(mousePos.x),int(mousePos.y));
        if(element->type==GuiElement::SELECT){
            auto *select=static_cast<GuiSelect *>(element);
            if(select==inputSelection && select->pollAction()){
                if(!controller.binding){const int index=select->getFirstSelected();if(index>=0 && index<int(input.controllersDetected.size()))updateInputController(input.controllersDetected[index]);}
                else updateInputSelection(true);
            }else if(select==profileSelection)select->pollAction();
        }else if(element->type==GuiElement::BUTTON){
            auto *button=static_cast<GuiButton *>(element);
            if(action<input.actionNames.size())button->updateName(input.getBindName(static_cast<Input::Actions>(action),currentPlayer,alternate));
            if(button->pollAction()){
                if(action<input.actionNames.size()){
                    if(!controller.binding){
                        if(!alternate || (controller.usingJoystick && action<4)){controller.currentlyBinding=action;controller.binded[action]=false;controller.binding=true;controller.bindingAlternate=alternate;}
                        button->disable();
                    }
                }else if(button==controlsBackButton){if(controller.binding)input.cancelBinding(currentPlayer);screen=MAIN_SCREEN;switchTimer=10;}
                else if(button==loadProfileButton){
                    const int selected=profileSelection->getFirstSelected();
                    if(selected>=0 && selected<int(profileIndices.size())){
                        const int index=profileIndices[selected];
                        if(index<0 || index>=int(input.inputProfiles.size())){VLOG(LOG_ERROR)<<"ConfigGui::stepControls Invalid profileIndex: "<<index;}
                        else{controller.loadFromProfile(input.inputProfiles[index]);profileSelection->clearSelection();}
                    }
                }else if(button==controlsRebindAllButton){
                    if(!controller.usingSteamController){
                        controller.binded.clear();controller.validAxes.clear();controller.actions.clear();controller.prevActions.clear();controller.currentlyBinding=0;controller.bindingAlternate=false;
                        controller.keyBindings.resize(15,ActionBind(sf::Keyboard::F12));controller.joystickBindings.resize(15,ActionBind());controller.alternateJoystickBindings.resize(15,ActionBind());
                        controller.binded.resize(15,false);controller.actions.resize(15,false);controller.prevActions.resize(15,false);
                        controller.bindTimer=0;controller.binding=true;controller.controllerChooseTimer=0;input.enteringText=false;window.setKeyRepeatEnabled(false);
                    }else{ /* Steam controller binding overlay is intentionally excluded. */ }
                }else for(unsigned player=0;player<controlsChoosePlayerButtons.size();++player)if(button==controlsChoosePlayerButtons[player])choosePlayer(player);
            }
            if(alternate)++action;alternate=!alternate;
        }else if(element->type==GuiElement::CHECKBOX){
            if(element!=enablePlayerCheckbox)element->setEnabled(controller.enabled);
            if(element==enablePlayerCheckbox)controller.enabled=enablePlayerCheckbox->isChecked();
        }else if(element->type==GuiElement::LABEL && element!=inputSelectLabel && element!=profileSelectLabel)element->setEnabled(controller.enabled);
    }
}
