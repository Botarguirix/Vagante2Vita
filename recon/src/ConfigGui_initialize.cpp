#include "recon_all.h"
#include "localization.h"
#include <cmath>
namespace {
std::string playerText(const char *pattern,unsigned player){auto text=recon::translate(pattern);const auto pos=text.find("{1}");if(pos!=std::string::npos)text.replace(pos,3,std::to_string(player));return text;}
}
void ConfigGui::initialize(){
    const bool dialogHidden=!exitDialogWindow || exitDialogWindow->isHidden();cleanUp();
    directionPressedThisFrame.resize(4,false);bgSprite.setOrigin(0,0);
    const char *mainLabels[]={"Settings","Controls","Unlocks","View Credits","Exit","Return"};
    for(int i=0;i<6;++i)mainElements.push_back(new GuiButton(recon::translate(mainLabels[i]),196,92+i*20,120,16,"",true));
    creditsButton=static_cast<GuiButton *>(mainElements[3]);exitButton=static_cast<GuiButton *>(mainElements[4]);
    exitDialogWindow=new GuiWindow(196,142,140,140);exitDialogWindow->setHasBg(false);
    exitDialogChoiceOne=new GuiButton(recon::translate("Exit Game"),196,92,120,16,"",true);
    exitDialogChoiceTwo=new GuiButton(recon::translate("Exit to Main Menu"),196,112,120,16,"",true);exitDialogChoiceTwo->disable();
    exitDialogCancel=new GuiButton(recon::translate("Cancel"),196,132,120,16,"",true);
    exitDialogWindow->add(exitDialogChoiceOne);exitDialogWindow->add(exitDialogChoiceTwo);exitDialogWindow->add(exitDialogCancel);exitDialogWindow->setHidden(dialogHidden);
    int row=0,fullRow=0,column=0;
    // lambda_d710...: map logical cells before appending, retaining insert-first semantics.
    auto addOption=[&](GuiElement *element,bool fullWidth,bool place,int height){
        if(place){
            const unsigned index=unsigned(configElements.size());
            if(fullWidth){
                posToSettingsIndex.emplace(GridPosition(0,row),index);posToSettingsIndex.emplace(GridPosition(1,row),index);fullRow=row;
                element->setPosition(int(256.f+(element->type==GuiElement::SLIDER ? 30.f:0.f)-float(element->getW())*.5f),52+row*20);
            }else{
                posToSettingsIndex.emplace(GridPosition(column,row),index);if(height>1)posToSettingsIndex.emplace(GridPosition(column,row+1),index);
                element->setPosition(column==0 ? 196:376,52+row*20);
            }
            row+=height;if(row>11 && column==0){column=1;row=fullRow+1;}
        }
        configElements.push_back(element);
    };
    addOption(new GuiSlider(196,62,std::sqrt(config.musicVolume*.01f),recon::translate("Music Volume")),true,true,1);
    addOption(new GuiSlider(196,82,std::sqrt(config.sfxVolume*.01f),recon::translate("Effects Volume")),true,true,1);
    gammaSlider=new GuiSlider(196,102,config.gamma-1.f,recon::translate("Gamma Correction"));addOption(gammaSlider,true,true,1);
    zoomSlider=new GuiSlider(196,122,config.viewZoom-1.f,recon::translate("Zoom"));addOption(zoomSlider,true,true,1);
    addOption(new GuiSlider(0,0,config.minimapScale-1.f,recon::translate("Minimap Scale")),true,true,1);
    if(!config.postProcessingEnabled)gammaSlider->disable();
    auto checkbox=[&](const char *label,bool value,int height=1){auto *element=new GuiCheckbox(0,0,value,recon::translate(label),"");addOption(element,false,true,height);return element;};
    checkbox("Particles",config.particlesEnabled);checkbox("Load from Archive",config.loadFromArchive)->disable();checkbox("Fullscreen",config.fullScreen);
    const bool shaders=sf::Shader::isAvailable();if(!shaders)config.postProcessingEnabled=false;
    auto *shader=checkbox("Enable Shader",config.postProcessingEnabled);if(!shaders)shader->disable();
    checkbox("Use Vertex Arrays",config.useVertexArrays);checkbox("Enable Vertical Sync",config.verticalSync);checkbox("Menu Tooltips",config.enableMenuToolTips);
    checkbox("In-Game Tooltips",config.enableInGameTooltips);checkbox("Show Debug Info",false);checkbox("Cumulative Combat Numbers",config.cumulativeCombatText);
    checkbox("Show Network Information",config.drawNetInformation);checkbox("Pixel-perfect Scaling",config.forcePerfectPixelScaling);checkbox("DirectInput Support",config.supportDirectInput,2);
    auto *back=new GuiButton(recon::translate("Back"),100,332,80,16,recon::translate("Back to the previous menu"),true);addOption(back,true,true,1);
    shaderWarningLabel=new GuiLabel(recon::translate("Warning: Enabling shader may cause issues on some machines."),back->getX(),back->getY()+20);shaderWarningLabel->setColor({255,255,255,170});shaderWarningLabel->setHidden(true);addOption(shaderWarningLabel,false,false,1);
    auto cell=[&](int x,int y){posToControlsIndex.emplace(GridPosition(x,y),unsigned(controlsElements.size()-1));};
    for(unsigned i=0;i<4;++i){auto *button=new GuiButton(playerText("Player {1}",i+1),43+int(i)*112,32,90,23,"",true);controlsChoosePlayerButtons.push_back(button);controlsElements.push_back(button);cell(i,0);}
    loadProfileButton=new GuiButton(recon::translate("Load Preset"),40,285,80,16,"",true);
    controlsRebindAllButton=new GuiButton(recon::translate("Rebind All"),40,305,80,16,"",true);
    controlsBackButton=new GuiButton(recon::translate("Back"),40,325,80,16,"",true);
    enablePlayerCheckbox=new GuiCheckbox(40,71,input.controllers[0].enabled,recon::translate("  Player Enabled"),"");enablePlayerCheckbox->useRightLabel=true;
    controlsElements.push_back(enablePlayerCheckbox);cell(0,1);cell(1,1);
    inputSelectLabel=new GuiLabel(recon::translate("Input Devices"),40,91);controlsElements.push_back(inputSelectLabel);
    // The linked GuiSelect constructor ignores its retained width/height parameters.
    inputSelection=new GuiSelect(40,104,0,0);controlsElements.push_back(inputSelection);for(int x=0;x<2;++x)for(int y=2;y<10;++y)cell(x,y);
    profileSelectLabel=new GuiLabel("",40,211);controlsElements.push_back(profileSelectLabel);
    profileSelection=new GuiSelect(40,224,0,0);controlsElements.push_back(profileSelection);for(int y=10;y<13;++y){cell(0,y);cell(1,y);}
    std::vector<std::string> devices;for(const auto &device:input.controllersDetected)devices.push_back(device.name);inputSelection->setEntries(devices);
    controlsButtonsOffset=unsigned(controlsElements.size());bindElements.clear();
    for(unsigned i=0;i<input.actionNames.size();++i){
        const int y=71+int(i)*18;auto *label=new GuiLabel(input.actionNames[i],280,y+1);label->alignRight=true;controlsElements.push_back(label);
        auto *primary=new GuiButton(input.getBindName(static_cast<Input::Actions>(i),currentPlayer,false),290,y,60,17,"",true);primary->fgColor={255,255,255,20};
        auto *alternate=new GuiButton(input.getBindName(static_cast<Input::Actions>(i),currentPlayer,true),386,y,60,17,"",true);alternate->fgColor={255,255,255,20};
        controlsElements.push_back(primary);cell(2,i+1);
        if(i<4){controlsElements.push_back(alternate);cell(3,i+1);alternateMovementButtons.push_back(alternate);}
        else{cell(3,i+1);alternate->setHidden(true);controlsElements.push_back(alternate);}
        bindElements.push_back(primary);bindElements.push_back(alternate);
    }
    controlsElements.push_back(loadProfileButton);cell(0,13);cell(1,13);
    controlsElements.push_back(controlsRebindAllButton);cell(0,14);cell(1,14);
    controlsElements.push_back(controlsBackButton);cell(0,15);cell(1,15);
}
