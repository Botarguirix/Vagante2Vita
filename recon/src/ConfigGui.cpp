// PDB contract; Ghidra 00583940-0058ae22. Public accessors replace expanded field accesses.
#include "recon_all.h"
#include "localization.h"
#include <algorithm>
const int ConfigGui::ANIMATE_FRAMES=10;
ConfigGui::GridPosition::GridPosition(int x,int y):x(x),y(y){}
bool ConfigGui::GridPosition::operator<(const GridPosition &other)const{return x!=other.x ? x<other.x : y<other.y;}
ConfigGui::ConfigGui(sf::RenderWindow &window,Config &config,Input &input)
    :exitToMainMenu(false),exitGame(false),window(window),view(window.getView()),config(config),input(input),open(false),netGame(false),frame(0),openingTimer(0),closingTimer(0),switchTimer(0),screen(MAIN_SCREEN),mousePos(0,0),mouseLastMovedTimer(70),selectPosition(0,0),
     bgSprite({},"gui/gui_background.png",32,32,-1,0,0,0,false,false),guiCursor(139,110),exitButton(nullptr),exitDialogWindow(nullptr),exitDialogChoiceOne(nullptr),exitDialogChoiceTwo(nullptr),exitDialogCancel(nullptr),creditsButton(nullptr),inputSelection(nullptr),profileSelection(nullptr),shaderWarningLabel(nullptr),statisticsInfoLabel(nullptr),controlsButtonsOffset(0),gammaSlider(nullptr),zoomSlider(nullptr),controlsBackButton(nullptr),loadProfileButton(nullptr),controlsRebindAllButton(nullptr),enablePlayerCheckbox(nullptr),profileSelectLabel(nullptr),inputSelectLabel(nullptr),currentPlayer(0),hoveredElement(nullptr),mouseAfk(false),didMouseHover(false),profileSelectionOpen(false),runCredits(false){initialize();choosePlayer(0);}
ConfigGui::ConfigGui(const ConfigGui &)=default;
ConfigGui::~ConfigGui(){cleanUp();}
void ConfigGui::cleanUp(){
    for(auto *elements:{&mainElements,&configElements,&controlsElements}){for(auto *element:*elements)delete element;elements->clear();}
    posToControlsIndex.clear();posToSettingsIndex.clear();delete exitDialogWindow;exitDialogWindow=nullptr;
}
Config &ConfigGui::getConfig(){return config;}
unsigned ConfigGui::selectIndex(){auto &positions=screen==CONTROLS_SCREEN ? posToControlsIndex:posToSettingsIndex;auto found=positions.find({selectPosition.x,selectPosition.y});return found==positions.end() ? 0:found->second;}
void ConfigGui::left(){
    if(directionPressedThisFrame[0])return;directionPressedThisFrame[0]=true;
    if(hoveredElement && hoveredElement->type==GuiElement::SLIDER && static_cast<GuiSlider *>(hoveredElement)->left())return;
    if(screen!=OPTIONS_SCREEN && screen!=CONTROLS_SCREEN)return;
    const unsigned index=selectIndex();while(selectPosition.x>0 && selectIndex()==index)--selectPosition.x;
}
void ConfigGui::right(){
    if(directionPressedThisFrame[1])return;directionPressedThisFrame[1]=true;
    if(hoveredElement && hoveredElement->type==GuiElement::SLIDER && static_cast<GuiSlider *>(hoveredElement)->right())return;
    if(screen!=OPTIONS_SCREEN && screen!=CONTROLS_SCREEN)return;
    const unsigned index=selectIndex();const int last=screen==OPTIONS_SCREEN ? 1:3;while(selectPosition.x<last && selectIndex()==index)++selectPosition.x;
}
void ConfigGui::up(){
    if(directionPressedThisFrame[2])return;directionPressedThisFrame[2]=true;
    if(screen==CONTROLS_SCREEN && hoveredElement && hoveredElement->type==GuiElement::SELECT && static_cast<GuiSelect *>(hoveredElement)->up())return;
    if(screen!=OPTIONS_SCREEN && screen!=CONTROLS_SCREEN){--selectPosition.y;return;}
    const unsigned index=selectIndex();while(selectPosition.y>0 && selectIndex()==index)--selectPosition.y;
}
void ConfigGui::down(){
    if(directionPressedThisFrame[3])return;directionPressedThisFrame[3]=true;
    if(screen==CONTROLS_SCREEN && hoveredElement && hoveredElement->type==GuiElement::SELECT && static_cast<GuiSelect *>(hoveredElement)->down())return;
    if(screen!=OPTIONS_SCREEN && screen!=CONTROLS_SCREEN){++selectPosition.y;return;}
    const unsigned index=selectIndex();const int last=screen==OPTIONS_SCREEN ? 12:15;while(selectPosition.y<last && selectIndex()==index)++selectPosition.y;
}
void ConfigGui::toggle(){
    if(openingTimer || closingTimer || (input.controllers[currentPlayer].binding && open))return;
    open=!open;
    if(open){runCredits=false;screen=MAIN_SCREEN;selectPosition={0,5};openingTimer=10;updateInputSelection(true);exitDialogWindow->setHidden(true);profileSelection->clearSelection();}
    else{config.saveIni();input.save();closingTimer=10;}
}
void ConfigGui::choosePlayer(unsigned player){
    if(input.controllers[currentPlayer].binding)input.cancelBinding(currentPlayer);
    currentPlayer=player;
    for(unsigned i=0;i<controlsChoosePlayerButtons.size();++i)controlsChoosePlayerButtons[i]->setEnabled(i!=currentPlayer);
    for(auto *element:alternateMovementButtons)element->setHidden(!input.controllers[currentPlayer].usingJoystick);
    enablePlayerCheckbox->setChecked(input.controllers[currentPlayer].enabled);updateInputSelection(true);
}
void ConfigGui::setInGameState(bool network,bool menu,bool inGame,bool characterSelect){
    if(!open)return;
    netGame=network;
    creditsButton->setEnabled(!inGame && menu);
    std::string label=recon::translate("Exit to Main Menu");bool enabled=true;
    if(inGame){}
    else if(menu)enabled=false;
    else if(network)label=recon::translate("Disconnect from Game");
    else if(!characterSelect)label=recon::translate("Exit to Character Select");
    exitDialogChoiceTwo->updateName(label);exitDialogChoiceTwo->setEnabled(enabled);
}
void ConfigGui::updateButtonStates(){
    if(screen!=CONTROLS_SCREEN)return;
    const auto &controller=input.controllers[currentPlayer];const bool enabled=controller.enabled && !controller.binding;
    for(GuiElement *element:std::initializer_list<GuiElement *>{inputSelection,profileSelection,inputSelectLabel,profileSelectLabel})element->setEnabled(enabled);
    for(auto *element:alternateMovementButtons)element->setHidden(!controller.usingJoystick);
    for(unsigned i=0;i<controlsChoosePlayerButtons.size();++i)controlsChoosePlayerButtons[i]->setEnabled(i!=currentPlayer);
    for(unsigned i=0;i<bindElements.size();++i)bindElements[i]->setEnabled(!controller.usingSteamController && controller.enabled && (!controller.binding || controller.currentlyBinding==i/2));
    loadProfileButton->setEnabled(profileSelection->getFirstSelected()>=0);
    controlsRebindAllButton->setEnabled(enabled && (!controller.tryToUseJoystick || controller.usingJoystick));
}
