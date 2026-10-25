#include "recon_all.h"
#include <set>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

const int PlayerCustomizeMenu::TARGET_POSITION=56;
PlayerCustomizeMenu::PlayerCustomizeMenu()
    : hidden(false), open(false), hasFocus(true), gameMode(0), frame(0), caravanDarkness(0.f),
      caravanPosition(97.f), caravanPositionY(256.f), stopped(false), selectingClass(true),
      currentClass(0), players(nullptr), guis(nullptr),
      caravanSprite({},"scenes/intro/caravan.png",128,80,-1,7,0.f,0.f,true,false),
      playerClasses(4,Player::NUM_PLAYER_CLASSES), playerPalettes(4,0), networked(false) {
    caravanSprite.setOrigin(0.f,0.f);
}
PlayerCustomizeMenu::~PlayerCustomizeMenu()=default;

void PlayerCustomizeMenu::reset(bool clearClasses,int mode,bool initial) {
    caravanDarkness=1.f; frame=0; hidden=false; open=true;
    if(guis) for(auto &gui:*guis) if(gui) gui->reset(clearClasses,mode,initial);
    if(clearClasses) playerClasses.assign(4,Player::NUM_PLAYER_CLASSES);
}
void PlayerCustomizeMenu::close() {
    open=false;
    if(guis) for(auto &gui:*guis) if(gui) gui->inClassSelectionArea=false;
}
void PlayerCustomizeMenu::drawCaravan(sf::RenderTarget &target,float alpha) {
    if(hidden) return;
    caravanDarkness=std::max(caravanDarkness-.01f,0.f);
    const auto brightness=static_cast<sf::Uint8>(180-static_cast<int>(caravanDarkness*100.f));
    caravanSprite.setColor(sf::Color(brightness,brightness,brightness,static_cast<sf::Uint8>(alpha*255.f)));
    caravanSprite.draw(target,sf::RenderStates::Default);
}
void PlayerCustomizeMenu::syncCaravanSprite(Sprite &source) {
    caravanSprite.setFrame(source.getFrame());
    caravanSprite.setTime(source.getTime());
    caravanSprite.setPosition(source.getPosition());
    stopped=networked;
    caravanPosition=networked ? static_cast<float>(TARGET_POSITION) : caravanSprite.getPosition().x;
    caravanPositionY=caravanSprite.getPosition().y+24.f;
}
void PlayerCustomizeMenu::step(Input &input) {
    ++frame;
    if(caravanPosition<=static_cast<float>(TARGET_POSITION)) {
        caravanPosition=static_cast<float>(TARGET_POSITION); stopped=true;
    } else {
        // 00b1fd80 = 0x3c23d70b, one ULP above the usual .01f literal.
        caravanPosition-=frame<30 ? static_cast<float>(frame)*0.010000000707805157f : .3f;
        stopped=false;
    }
    caravanSprite.setFrozen(stopped);
    caravanSprite.step({0,0},{1.f,1.f});
    const float bounce=!stopped && (_randEngine()&0x7fffffffu)%20==0 ? .5f : 0.f;
    caravanSprite.setPosition(caravanPosition,caravanPositionY+bounce);
    if(!caravan || !caravan->getLevel()) return;
    if(!stopped && (_randEngine()&0x7fffffffu)%3==0)
        caravan->getLevel()->addParticle(Particle::DUST,caravanPosition+114.f,caravanPositionY+80.f,0,true);
    caravan->x=caravanPosition+68.f; caravan->y=caravanPositionY+17.f;
    bool anyMenu=false;
    if(guis) for(const auto &gui:*guis) if(gui && gui->menuOpen) anyMenu=true;
    if(players) for(unsigned i=0;i<players->size();++i) {
        auto player=(*players)[i];
        if(!player) continue;
        player->canExitLevel=!anyMenu;
        if(!networked && i<input.controllers.size() && !input.controllers[i].enabled) {
            removePlayer(i,nullptr);
            if(guis && i<guis->size() && (*guis)[i]) (*guis)[i]->reset(networked,i,false);
        }
    }
    selectingClass=false;
}
int PlayerCustomizeMenu::randomPalette(unsigned index,Player::PlayerClass playerClass) {
    std::set<int> available{0,1,2,3};
    if(players) for(unsigned i=0;i<players->size();++i) {
        const auto &player=(*players)[i];
        if(i!=index && player && player->playerClass==playerClass) available.erase(player->palette);
    }
    if(available.empty()) { VLOG(LOG_ERROR)<<"PlayerCustomizeMenu::randomPalette: no unused palettes";return 0; }
    auto selected=available.begin();
    std::advance(selected,(_randEngine()&0x7fffffffu)%available.size());
    return *selected;
}
void PlayerCustomizeMenu::addPlayer(unsigned index,Level *level,Player::PlayerClass playerClass,
                                    Background::BackgroundType background,int palette) {
    if(!players || index>=players->size() || (*players)[index]) return;
    VLOG(LOG_DEBUG)<<"PlayerCustomizeMenu::addPlayer("<<index<<", "<<playerClass<<", "<<static_cast<unsigned>(background)<<")";
    if(palette<0) palette=randomPalette(index,playerClass);
    if(!caravan) { VLOG(LOG_ERROR)<<"PlayerCustomizeMenu::addPlayer: invalid caravan";return; }
    // Level in EDX, coordinates XMM2/3; trailing index/bool optimized out at this call.
    auto player=PlayerCustomizer::getNewPlayer(level,caravan->x+35.f,caravan->y+13.f,
                                              playerClass,background,palette,index,false);
    if(!player) { VLOG(LOG_ERROR)<<"PlayerCustomizeMenu::addPlayer: getNewPlayer returned null";return; }
    const auto gui=guis && index<guis->size() ? (*guis)[index] : nullptr;
    player->setPlayerMenu(gui);
    VLOG(LOG_DEBUG)<<"PlayerCustomizeMenu::added new player at ("<<player->x<<", "<<player->y<<")";
    (*players)[index]=player;
    if(gui) gui->setPlayer(player);
    playerClasses[index]=playerClass; playerPalettes[index]=palette;
    player->facingLeft=false; player->dx=3.6f; player->dy=-2.f;
}
void PlayerCustomizeMenu::addRandomizedPlayer(unsigned index,Level *level,std::vector<Affinity::AffinityType> &affinities) {
    if(!players || index>=players->size() || (*players)[index]) return;
    bool forceHoundmaster=false;
    for(auto affinity:affinities) if(affinity==22 || affinity==23 || affinity==24) { forceHoundmaster=true;break; }
    // A draw still occurs on the forced branch @00708867, before the background draw.
    const auto random=_randEngine()&0x7fffffffu;
    const auto playerClass=static_cast<Player::PlayerClass>(forceHoundmaster ? 4 : random%4);
    const auto options=Stats::getAvailableBackgrounds(playerClass,false);
    std::vector<Background::BackgroundType> backgrounds;
    for(const auto &option:options) if(option.second) backgrounds.push_back(option.first);
    auto background=Background::BG_NONE;
    if(!backgrounds.empty()) background=backgrounds[((_randEngine()&0x7fffffffu)%backgrounds.size())&0xffu];
    const int palette=randomPalette(index,playerClass);
    if(!caravan) { VLOG(LOG_ERROR)<<"PlayerCustomizeMenu::addRandomizedPlayer: invalid caravan";return; }
    auto player=std::make_shared<Player>(level,caravan->x+35.f,caravan->y+13.f,playerClass,palette);
    if(level) level->addEntity(player);
    PlayerCustomizer::initializePlayer(player,level,background,index,affinities,false);
    const auto gui=guis && index<guis->size() ? (*guis)[index] : nullptr;
    player->setPlayerMenu(gui);
    VLOG(LOG_DEBUG)<<"PlayerCustomizeMenu::addRandomizedPlayer Randomized new player at ("<<player->x<<", "<<player->y<<")";
    (*players)[index]=player;
    if(gui) gui->setPlayer(player);
    playerClasses[index]=playerClass; playerPalettes[index]=palette;
    player->facingLeft=false; player->dx=3.6f; player->dy=-2.f; player->randomizedClass=true;
}
void PlayerCustomizeMenu::removePlayer(unsigned index,std::map<int,std::shared_ptr<Entity>> *netEntities) {
    if(!players || index>=players->size()) return;
    auto player=(*players)[index];
    if(!player) { VLOG(LOG_ERROR)<<"PlayerCustomizeMenu: Attempting to remove a player that doesn't exist!"; }
    else {
        if(netEntities && player->netId>=0) netEntities->erase(player->netId);
        auto *level=player->getLevel();
        player->inventory.destroyAll();
        auto *entities=level ? level->getEntities() : nullptr;
        const auto found=entities ? std::find(entities->begin(),entities->end(),player) : std::vector<std::shared_ptr<Entity>>::iterator{};
        if(entities && found!=entities->end()) { entities->erase(found); player->setLevel(nullptr); }
        else { VLOG(LOG_ERROR)<<"Attempted to remove an entity that wasn't in the entities vector. type:"<<Entity::entityTypeToString[player->type]; }
        const auto weapon=player->getCurrWeapon();
        if(weapon && weapon!=player->unarmedWeapon) weapon->die=true;
        if(player->pet) { player->pet->die=true; player->pet->removeOnDie=true; }
        // Client/server log levels retained; no network API is invoked.
        const auto logLevel=player->clientSide ? LOG_CLIENT : player->serverSide ? LOG_SERVER : LOG_DEBUG;
        VLOG(logLevel)<<"PlayerCustomizeMenu: Removed and deleted player id: "<<index;
    }
    (*players)[index].reset();
    if(guis && index<guis->size() && (*guis)[index]) (*guis)[index]->setPlayer(nullptr);
    playerClasses[index]=Player::NUM_PLAYER_CLASSES;
    const auto current=std::find(currentPlayer.begin(),currentPlayer.end(),index);
    if(current!=currentPlayer.end()) currentPlayer.erase(current);
}
void PlayerCustomizeMenu::menu(unsigned index) {
    if(networked || hidden || !guis || index>=guis->size() || !(*guis)[index] || !players) return;
    const auto gui=(*guis)[index];
    if(!gui->choosingClass) {
        if(!gui->classChosen) removePlayer(index,nullptr);
        else {
            int palette=gui->playerPalette;
            for(unsigned attempts=0;;++attempts) {
                const auto playerClass=gui->getPlayerClass();
                const bool used=std::any_of(players->begin(),players->end(),[&](const auto &player) {
                    return player && player->playerClass==playerClass && player->palette==palette;
                });
                if(!used || attempts>=4) break;
                palette=(palette+1)%4; gui->playerPalette=palette;
            }
            if(!caravan || !caravan->getLevel()) { VLOG(LOG_ERROR)<<"Could not add player "<<index<<"! Invalid caravan or level."; }
            else if(gui->getPlayerClass()==Player::CLASS_RANDOM) addRandomizedPlayer(index,caravan->getLevel(),gui->randomClassAffinities);
            else {
                auto background=Background::BG_NONE;
                const int selected=gui->selectedBackgroundIndex;
                if(selected>=0 && static_cast<unsigned>(selected)<gui->availableBackgrounds.size() && gui->availableBackgrounds[selected].second)
                    background=gui->availableBackgrounds[selected].first;
                gui->playerBackground=background;
                addPlayer(index,caravan->getLevel(),gui->getPlayerClass(),background,palette);
            }
        }
    }
    PlayerCustomizer::updateAllPlayerStats(*players);
}
