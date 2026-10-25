#include "port_lib_types.h"
#define private public
#define protected public
#include "ConfigGui.h"
#include "GuiButton.h"
#include "GuiSelect.h"
#include "GuiSlider.h"
#include "GuiCheckbox.h"
#include "GuiLabel.h"
#include "ResourceManager.h"
#undef protected
#undef private
#include "recon_all.h"
#include <cassert>
#include <iostream>
int main(){
    Config config;ResourceManager::setConfig(&config);ResourceManager::muted=true;
    sf::RenderWindow window(sf::VideoMode(640,360),"Config fixture",sf::Style::None);window.setVisible(false);Input input(&window);
    sf::Font font;assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));ResourceManager::fonts["Tiny_04b.ttf"]=&font;ResourceManager::fonts["NotoSansCJKkr-Regular.otf"]=&font;
    auto click=[](GuiButton *button){button->action(true);button->action(false);};
    {
        ConfigGui gui(window,config,input);
        assert(!gui.isOpen() && gui.currentPlayer==0 && !input.controllers[0].binding);
        assert(gui.mainElements.size()==6 && gui.configElements.size()==20 && gui.controlsElements.size()==57);
        assert(gui.bindElements.size()==30 && gui.alternateMovementButtons.size()==4 && gui.controlsButtonsOffset==9);
        assert(gui.mainElements[0]->getX()==196 && gui.mainElements[0]->getY()==92 && gui.mainElements[5]->getY()==192);
        assert(gui.posToSettingsIndex.at({0,0})==0 && gui.posToSettingsIndex.at({1,0})==0);
        assert(gui.posToSettingsIndex.at({0,5})==5 && gui.posToSettingsIndex.at({1,5})==12);
        assert(gui.posToSettingsIndex.at({1,10})==17 && gui.posToSettingsIndex.at({1,11})==17);
        assert(gui.posToSettingsIndex.at({0,12})==18 && gui.posToSettingsIndex.at({1,12})==18);
        assert(gui.posToControlsIndex.at({2,1})==10 && gui.posToControlsIndex.at({3,1})==11);
        assert(gui.posToControlsIndex.at({2,5})==gui.posToControlsIndex.at({3,5}));
        assert(!gui.controlsChoosePlayerButtons[0]->isEnabled() && gui.controlsChoosePlayerButtons[1]->isEnabled());
        gui.choosePlayer(2);assert(gui.currentPlayer==2 && !gui.controlsChoosePlayerButtons[2]->isEnabled() && gui.controlsChoosePlayerButtons[0]->isEnabled());gui.choosePlayer(0);
        // Navigation checks exercise the logical map even while the slider/select widgets are stubs.
        auto directions=[&](){std::fill(gui.directionPressedThisFrame.begin(),gui.directionPressedThisFrame.end(),false);gui.hoveredElement=nullptr;};
        gui.screen=ConfigGui::OPTIONS_SCREEN;gui.selectPosition={0,5};directions();gui.right();assert(gui.selectPosition==sf::Vector2i(1,5) && gui.selectIndex()==12);gui.right();assert(gui.selectPosition.x==1);directions();gui.left();assert(gui.selectIndex()==5);
        gui.selectPosition={0,0};directions();gui.down();assert(gui.selectPosition.y==1);gui.down();assert(gui.selectPosition.y==1);directions();gui.up();assert(gui.selectPosition.y==0);
        gui.screen=ConfigGui::CONTROLS_SCREEN;gui.selectPosition={0,0};directions();gui.right();assert(gui.selectPosition.x==1 && gui.selectIndex()==1);directions();gui.down();assert(gui.selectPosition.y==1 && gui.selectIndex()==4);
        gui.screen=ConfigGui::MAIN_SCREEN;gui.toggle();assert(gui.isOpen() && gui.openingTimer==10 && gui.selectPosition==sf::Vector2i(0,5));gui.toggle();assert(gui.open);
        for(int i=0;i<10;++i)gui.step();assert(gui.openingTimer==0 && gui.hoveredElement==gui.mainElements[5]);
        auto otherView=window.getView();otherView.setCenter(1000,1000);window.setView(otherView);const auto mapped=window.mapPixelToCoords(sf::Mouse::getPosition(window));gui.step();assert(gui.mousePos==mapped);window.setView(gui.view);
        gui.setInGameState(false,true,false,false);assert(gui.creditsButton->isEnabled() && !gui.exitDialogChoiceTwo->isEnabled());
        gui.setInGameState(false,false,true,false);assert(!gui.creditsButton->isEnabled() && gui.exitDialogChoiceTwo->isEnabled() && gui.exitDialogChoiceTwo->getName()=="Exit to Main Menu");
        gui.setInGameState(false,false,false,false);assert(gui.exitDialogChoiceTwo->getName()=="Exit to Character Select");
        gui.setInGameState(true,false,false,false);assert(gui.exitDialogChoiceTwo->getName()=="Disconnect from Game");
        gui.setInGameState(false,false,false,true);assert(gui.exitDialogChoiceTwo->getName()=="Exit to Main Menu");
        gui.mouseAfk=true;click(static_cast<GuiButton *>(gui.mainElements[0]));gui.stepMain();assert(gui.screen==ConfigGui::OPTIONS_SCREEN && gui.switchTimer==10);
        click(static_cast<GuiButton *>(gui.configElements[18]));gui.stepOptions();assert(gui.screen==ConfigGui::MAIN_SCREEN && gui.switchTimer==10);
        click(gui.exitButton);gui.stepMain();assert(!gui.exitDialogWindow->isHidden());gui.selectPosition.y=1;click(gui.exitDialogCancel);gui.stepMain();assert(gui.exitDialogWindow->isHidden() && gui.selectPosition.y==2);
        click(gui.exitButton);gui.stepMain();click(gui.exitDialogChoiceOne);gui.stepMain();assert(gui.exitGame);gui.exitGame=false;gui.exitDialogWindow->setHidden(true);
        click(gui.exitButton);gui.stepMain();gui.exitDialogChoiceTwo->enable();click(gui.exitDialogChoiceTwo);gui.stepMain();assert(gui.exitToMainMenu);gui.exitToMainMenu=false;gui.exitDialogWindow->setHidden(true);
        Input::ControllerIdentifier device("fixture");device.keyboard=false;device.steamController=false;device.joystickId=5;gui.updateInputController(device);assert(input.controllers[0].tryToUseJoystick && input.controllers[0].joystickId==5 && !input.controllers[0].usingJoystick);
        device.keyboard=true;gui.updateInputController(device);assert(!input.controllers[0].tryToUseJoystick && !input.controllers[0].usingSteamController);
        InputProfile keyboard;keyboard.keyboard=true;keyboard.profileName="Keyboard fixture";InputProfile gamepad;gamepad.keyboard=false;gamepad.profileName="Gamepad fixture";input.inputProfiles={keyboard,gamepad};gui.updateInputSelection(false);assert(gui.profileIndices==std::vector<int>{0});
        gui.screen=ConfigGui::CONTROLS_SCREEN;auto &controller=input.controllers[0];controller.enabled=true;controller.binding=true;controller.currentlyBinding=3;controller.usingSteamController=false;gui.updateButtonStates();
        assert(!gui.inputSelection->isEnabled() && gui.bindElements[6]->isEnabled() && !gui.bindElements[4]->isEnabled());
        controller.binding=false;gui.controlsRebindAllButton->enable();click(gui.controlsRebindAllButton);gui.stepControls();assert(controller.binding && controller.currentlyBinding==0 && controller.bindTimer==0 && !controller.bindingAlternate);
        input.cancelBinding(0);controller.enabled=true;auto *bind=static_cast<GuiButton *>(gui.bindElements[0]);bind->enable();click(bind);gui.stepControls();assert(controller.binding && controller.currentlyBinding==0 && !controller.binded[0]);
        gui.controlsBackButton->enable();click(gui.controlsBackButton);gui.stepControls();assert(gui.screen==ConfigGui::MAIN_SCREEN && !controller.binding);
        sf::RenderTexture target;assert(target.create(640,360));for(int screen=0;screen<3;++screen){gui.screen=static_cast<ConfigGui::ConfigScreen>(screen);gui.draw(target,true);}gui.screen=ConfigGui::MAIN_SCREEN;gui.exitDialogWindow->setHidden(false);gui.draw(target,true);target.display();
        gui.exitDialogWindow->setHidden(true);gui.screen=ConfigGui::MAIN_SCREEN;gui.switchTimer=0;gui.openingTimer=0;gui.closingTimer=0;gui.creditsButton->enable();click(gui.creditsButton);gui.stepMain();assert(gui.doRunCredits() && !gui.doRunCredits() && !gui.open && gui.closingTimer==10);
        gui.cleanUp();assert(gui.mainElements.empty() && gui.controlsElements.empty() && gui.configElements.empty() && gui.posToSettingsIndex.empty() && !gui.exitDialogWindow);
    }
    ResourceManager::fonts.clear();window.close();std::cout<<"ConfigGui: grids, navigation, menus, controls and drawing passed; slider/select/checkbox dependencies remain stubbed\n";
}
