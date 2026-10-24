// Credits state registry @00458b70. Text positions and numeric entity IDs checked in machine code.
#include "recon_all.h"
#include "localization.h"
void Credits::initializeCurrentState(){
    auto *font=ResourceManager::getDefaultFont();
    textWipes.clear();paradeEntries.clear();paradeLevel.reset();
    const auto wipe=[&](const std::string &value,sf::Vector2f start,sf::Vector2f finish,bool centeredY=false){
        TextWipe item;item.speed=0;item.position=start;item.desiredPosition=finish;
        if(font){item.text=sf::Text(sf::String::fromUtf8(value.begin(),value.end()),*font,16);}
        else item.text.setString(sf::String::fromUtf8(value.begin(),value.end()));
        const auto bounds=item.text.getLocalBounds();item.text.setOrigin(bounds.width*.5f,centeredY ? bounds.height*.5f : 0.f);
        textWipes.push_back(item);
    };
    const auto world=[&](Level::IntermissionType type){paradeLevel.reset(gameEngine.levelGenerator.generateIntermissionLevel(paradeLevelEntities,type));};
    const auto queue=[&](unsigned type,std::vector<int> params=std::vector<int>()){
        EnemyParadeEntry entry;entry.type=static_cast<Entity::EntityType>(type);entry.params=std::move(params);paradeEntries.push_back(entry);
    };
    const float cx=view.getCenter().x,cy=view.getCenter().y;
    switch(state){
    case GOOD_END_SCENE:
        day=.5f;movePos=0;cloudMovePos=300;caravanPosition=100;caravanBump=false;
        if(triggered)initializePlayerSprites();creditStateFrames=1200;break;
    case CREDITS_PROGRAMMING:
        creditStateFrames=240;
        wipe(recon::translate("PROGRAMMING"),{cx,500},{cx,100});
        wipe("Eugene Paik",{cx-100,550},{cx-100,250});wipe("Kyle Tenan",{cx+100,550},{cx+100,250});break;
    case ENEMIES_BRANCH_4:
        world(Level::RIFT_RIFT);for(unsigned type:{66,46,47,45})queue(type);creditStateFrames=65535;break;
    case ENEMIES_ACT_4:
        world(Level::TOWER_RIFT);for(unsigned type:{62,61,38,63,34,35,36,37})queue(type);creditStateFrames=65535;break;
    case CREDITS_GRAPHICS:
        wipe(recon::translate("GRAPHICS"),{200,500},{200,cy-25});wipe("Keo Daniel Bun",{300,550},{300,cy+25});creditStateFrames=240;break;
    case ENEMIES_BRANCH_3:
        world(Level::CATACOMBS_TOWER);queue(60);queue(Entity::TYPE_PIRANHA,{1});queue(40);queue(Entity::TYPE_IMP);creditStateFrames=65535;break;
    case ENEMIES_ACT_3:
        world(Level::FOREST_CATACOMBS);for(unsigned type:{58,33,57,59,31,27})queue(type);queue(Entity::TYPE_GHOST);creditStateFrames=65535;break;
    case CREDITS_SOUND:
        wipe(recon::translate("MUSIC & SOUND"),{200,500},{200,cy-25});wipe("Sam English",{300,550},{300,cy+25});creditStateFrames=240;break;
    case ENEMIES_BRANCH_2:
        world(Level::FOREST_FOREST);for(unsigned type:{64,13,43,42})queue(type);creditStateFrames=65535;break;
    case ENEMIES_ACT_2:
        world(Level::RUINS_FOREST);for(unsigned type:{52,53,55,22,25,20,19})queue(type);queue(Entity::TYPE_WISP,{0});creditStateFrames=65535;break;
    case CREDITS_SPECIAL_THANKS:
        wipe(recon::translate("SPECIAL THANKS"),{cx,500},{cx,cy-150});wipe(recon::translate("Our friends and family..."),{cx,1500},{cx,cy-100});
        wipe(recon::translate("Everyone who supported us during Early Access..."),{cx,3000},{cx,cy});wipe(recon::translate("And YOU!"),{cx,5000},{cx,cy+100});creditStateFrames=600;break;
    case ENEMIES_BRANCH_1:
        world(Level::CAVE_RUINS);for(unsigned type:{56,28,29})queue(type);queue(26,{0});queue(Entity::TYPE_BAT,{1});creditStateFrames=65535;break;
    case ENEMIES_ACT_1:
        world(Level::CAVE_CAVE);for(unsigned type:{50,49,51,15,9,24,11,16,14})queue(type);queue(Entity::TYPE_SLIME);creditStateFrames=65535;break;
    case THE_END:
        wipe(recon::translate("THE END"),{cx,800},{cx,cy},true);creditStateFrames=65535;break;
    default:break;
    }
    for(const auto &entity:paradeLevelEntities)entity->setLevel(nullptr);
    paradeLevelEntities.clear();
    if(paradeLevel){paradeLevel->setClientSide(false);paradeLevel->setServerSide(false);}
}
