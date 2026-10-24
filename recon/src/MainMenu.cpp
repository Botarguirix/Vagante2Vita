// MainMenu core: Ghidra 006a2c40-006a69c3 and PDB, register arguments checked against assembly.
#include "recon_all.h"
#include "localization.h"
#include <algorithm>
#include <cmath>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
namespace { enum FadeTarget { Start=0, FadeNetwork=1, Progression=2, ShowCredits=3 }; }
const int MainMenu::FADE_OUT_TIME_NET_MENU=40;
const int MainMenu::FADE_OUT_TIME_GAME=90;
const int MainMenu::FADE_IN_TIME=30;
const int MainMenu::INTRO_END=400;
MainMenu::MainMenu(sf::RenderWindow &surface,Input &controls,ConfigGui &settings,NetMenu &FadeNetwork)
    : window(surface),input(controls),view(surface.getView()),configGui(settings),netMenu(FadeNetwork),
      directionPressedThisFrame(2,false),mousePos(0,0),mouseMovedTimer(0),frame(0),moveRate(1),movePos(0),cloudMovePos(300),
      open(true),hasFocus(true),fadingOut(false),fadingOutTarget(Start),readyToStartGame(false),doOpenProgressionGui(false),doViewCredits(false),
      animateOutTimer(90),animateOutTime(90),newGameButton(nullptr),onlineButton(nullptr),exitButton(nullptr),optionsButton(nullptr),
      unlocksButton(nullptr),creditsButton(nullptr),cursorIndex(0),guiCursor(9999,0),
      skySprite({},"scenes/intro/sky.png",-1,-1,-1,0,0,0,false,false),
      skyStarsSprite({},"scenes/intro/sky_stars.png",-1,-1,-1,0,0,0,false,false),
      moonSprite({},"scenes/intro/moon.png",-1,-1,-1,0,0,0,false,false),
      groundSprite({},"scenes/intro/ground.png",-1,-1,-1,0,0,0,false,false),
      cloudSprite({},"scenes/intro/clouds.png",-1,-1,-1,0,0,0,false,false),
      mountainSprite({},"scenes/intro/mountains.png",-1,-1,-1,0,0,0,false,false),
      treeLineSprite1({},"scenes/intro/midground_treeline_1.png",-1,-1,-1,0,0,0,false,false),
      foregroundSprite({},"scenes/intro/foreground_treeline.png",-1,-1,-1,0,0,0,false,false),
      caravanSprite({},"scenes/intro/caravan.png",128,80,-1,7,0,0,true,false),
      signpostSprite({},"scenes/intro/signpost.png",32,32,-1,3,0,0,false,false),
      titleSprite({},"gui/title.png",-1,-1,-1,0,0,0,false,false),
      titlePos(90),titleVel(2),buttonAlpha(0),fadeInTimer(30),introTimer(0),afterIntroTimer(0),skipIntro(false),fadeInCaravan(false),
      caravanBump(false),titleTimer(0),smoothCaravanTween(Tween::TWEEN_LINEAR,0,1,1),caravanPosition(-814),targetCaravanPosition(0),caravanSpeed(0) {
    for(auto *sprite:{&skySprite,&skyStarsSprite,&moonSprite,&groundSprite,&cloudSprite,&mountainSprite,&caravanSprite,&treeLineSprite1,&foregroundSprite}) sprite->setOrigin(0,0);
    for(unsigned i=0;i<9;++i){
        SceneDoodad doodad;
        doodad.pos={std::sin(float(i)*13.f)*13.f+(float(i)*93.33333587646484f-100.f),std::sin(float(i)*8.168140411376953f)*60.f+202.f};
        doodad.type=i&1;
        doodad.sprite=new Sprite({},doodad.type==0 ? "scenes/intro/entities/little_firefly.png" : "scenes/intro/entities/big_firefly.png",16,16,-1,8,0,0,true,false);
        doodad.sprite->setFrame(static_cast<int>(i)%doodad.sprite->getNumFrames());
        sceneDoodads.push_back(doodad);
    }
    newGameButton=new GuiButton("",526,225,80,16,"",true);
    onlineButton=new GuiButton("",526,245,80,16,"",true);
    optionsButton=new GuiButton("",526,265,80,16,"",true);
    exitButton=new GuiButton("",526,285,80,16,"",true);
    guiElements={newGameButton,onlineButton,optionsButton,exitButton};
    reset(true,false);
}
MainMenu::~MainMenu(){for(auto *element:guiElements) delete element;for(auto &doodad:sceneDoodads){delete doodad.sprite;doodad.sprite=nullptr;}}
void MainMenu::updateStrings(){
    newGameButton->updateName(recon::translate("New Game"));onlineButton->updateName(recon::translate("Play Online"));
    optionsButton->updateName(recon::translate("Options"));exitButton->updateName(recon::translate("Exit Game"));
}
void MainMenu::reset(bool intro,bool keepMusic){
    updateStrings();open=true;cursorIndex=0;readyToStartGame=false;doOpenProgressionGui=false;doViewCredits=false;fadeInCaravan=false;fadingOut=false;animateOutTimer=animateOutTime;
    if(!keepMusic){ResourceManager::stopAllMusic();ResourceManager::playMusic("title_theme.ogg",false,false);fadeInTimer=30;}
    ResourceManager::setSpecificMusicVolume("title_theme.ogg",1.f);
    targetCaravanPosition=307.f-caravanSprite.getSize().x*.5f;
    smoothCaravanTween=Tween(Tween::TWEEN_CUBIC_IN_OUT,caravanPosition,targetCaravanPosition-caravanPosition,520.f);
    if(!intro){buttonAlpha=1;caravanPosition=targetCaravanPosition;skipIntro=true;titlePos=172;}
}
void MainMenu::up(){if(!directionPressedThisFrame[0]){directionPressedThisFrame[0]=true;if(hasFocus && cursorIndex) --cursorIndex;}}
void MainMenu::down(){if(!directionPressedThisFrame[1]){directionPressedThisFrame[1]=true;if(hasFocus && cursorIndex<3) ++cursorIndex;}}
void MainMenu::action(bool down){
    if(!hasFocus) return;
    if(introTimer<400 && !skipIntro){skipIntro=true;introTimer=400;if(caravanPosition<0){fadeInCaravan=true;caravanPosition=0;}return;}
    if(fadingOut) return;
    switch(cursorIndex){case 0:newGameButton->action(down);return;case 1:onlineButton->action(down);return;case 2:optionsButton->action(down);return;case 3:exitButton->action(down);return;}
    VLOG(LOG_DEBUG)<<"Unhandled MainMenu::action() case: "<<cursorIndex;
}
void MainMenu::skip(){readyToStartGame=true;caravanPosition=50;caravanSprite.setPosition(50,232);}
void MainMenu::step(){
    std::fill(directionPressedThisFrame.begin(),directionPressedThisFrame.end(),false);
    if(!input.isKeyBoundToSomething(sf::Keyboard::Return)){if(input.isKeyPressed(sf::Keyboard::Return)) action(true);else if(input.isKeyReleased(sf::Keyboard::Return)) action(false);}
    if(!input.isKeyBoundToSomething(sf::Keyboard::Up) && input.isKeyPressed(sf::Keyboard::Up)) up();
    if(!input.isKeyBoundToSomething(sf::Keyboard::Down) && input.isKeyPressed(sf::Keyboard::Down)) down();
    if(fadeInTimer<1){float goal=std::sin(float(titleTimer)*.025f)+172.f;if(fadingOut)goal=150;const float speed=(goal-titlePos)*.01f+titleVel;titlePos+=speed;titleVel=speed*.8f;}
    else --fadeInTimer;
    const float tween=smoothCaravanTween.step(1);
    moveRate=skipIntro ? std::max(1.f,std::min(25.f,(targetCaravanPosition-caravanPosition)*.1f)) : tween-caravanPosition;
    if(introTimer>399) ++afterIntroTimer;
    ++introTimer;if(introTimer>459 || skipIntro) ++titleTimer;
    stepBackground(moveRate);
    const auto previousMouse=mousePos;
    mousePos=window.mapPixelToCoords(sf::Mouse::getPosition(window),view);
    if(fadingOut && (fadingOutTarget==FadeNetwork || fadingOutTarget==Progression)){buttonAlpha*=.6f;if(buttonAlpha<.05f)buttonAlpha=0;}
    else if(titleTimer>0) buttonAlpha+=(1.f-buttonAlpha)*.05f;
    const int previousTimer=mouseMovedTimer;
    bool anyContains=false,anyHover=false;
    for(std::size_t i=0;i<guiElements.size();++i){
        auto *element=guiElements[i];element->setAlpha(buttonAlpha);
        if(!hasFocus){element->hover=false;continue;}
        const bool contains=element->contains(static_cast<int>(mousePos.x),static_cast<int>(mousePos.y));
        anyContains=anyContains || contains;element->hover=contains && previousTimer<10;
        if(element->hover){cursorIndex=i;anyHover=true;}
        element->step(static_cast<int>(mousePos.x),static_cast<int>(mousePos.y));
        if(element->type!=GuiElement::BUTTON || !static_cast<GuiButton *>(element)->pollAction())continue;
        if(element==newGameButton){fadingOut=true;fadingOutTarget=Start;animateOutTime=animateOutTimer=90;}
        else if(element==onlineButton){fadingOut=true;fadingOutTarget=FadeNetwork;animateOutTime=animateOutTimer=40;}
        else if(element==unlocksButton){fadingOut=true;fadingOutTarget=Progression;animateOutTime=animateOutTimer=10;}
        else if(element==optionsButton) configGui.toggle();
        else if(element==creditsButton){fadingOut=true;fadingOutTarget=ShowCredits;animateOutTime=animateOutTimer=10;}
        else if(element==exitButton) window.close();
    }
    if(mousePos==previousMouse || !anyContains) ++mouseMovedTimer;else mouseMovedTimer=0;
    if(!anyHover) guiElements[cursorIndex]->hover=true;
    guiCursor.setAlpha(buttonAlpha);guiCursor.setTarget(guiElements[cursorIndex]);guiCursor.step();
    const bool left=sf::Mouse::isButtonPressed(sf::Mouse::Left),right=sf::Mouse::isButtonPressed(sf::Mouse::Right);
    for(auto *element:guiElements)element->afterStep(left || previousTimer>=10,right || previousTimer>=10);
    if(fadingOut){
        --animateOutTimer;
        if(animateOutTimer<1){animateOutTimer=0;
            if(fadingOutTarget==Start)readyToStartGame=true;
            else if(fadingOutTarget==FadeNetwork){netMenu.setOpen(true);fadingOut=false;open=false;}
            else if(fadingOutTarget==Progression){doOpenProgressionGui=true;fadingOut=false;buttonAlpha=0;}
            else if(fadingOutTarget==ShowCredits){fadingOut=false;doViewCredits=true;open=false;}
        }else if(fadingOutTarget==Start)ResourceManager::setSpecificMusicVolume("title_theme.ogg",float(animateOutTimer)*.011111111380159855f);
    }
}
// Simple public field wrappers, otherwise inlined in original callers.
bool MainMenu::isOpen(){return open;}
bool MainMenu::openProgressionGui(){const bool result=doOpenProgressionGui;doOpenProgressionGui=false;return result;}
bool MainMenu::viewCredits(){const bool result=doViewCredits;doViewCredits=false;return result;}
bool MainMenu::isReadyToStart(){return readyToStartGame;}
void MainMenu::setReadyToStart(bool value){readyToStartGame=value;}
void MainMenu::close(){open=false;}
void MainMenu::setFocus(bool value){hasFocus=value;}
Sprite &MainMenu::getCaravanSprite(){return caravanSprite;}
