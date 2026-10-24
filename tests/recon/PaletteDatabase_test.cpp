#include "port_lib_types.h"
#define private public
#include "PaletteDatabase.h"
#include "ResourceManager.h"
#include "ResourceArchive.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
int main(){
    auto work=std::filesystem::current_path()/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(work);std::filesystem::current_path(work);
    ResourceManager::archive=nullptr;ResourceManager::setConfig(nullptr);globalPaletteDb.paletteGroups.clear();globalPaletteDb.initialized=false;
    const std::string json=R"({"palette-groups":[{"name":"group","texture-names":["a","b"],"palettes":[{"name":"base","colors":[[1,2,3,255],[4,5,6,8]]},{"name":"alt","colors":[[11,12,13,14],[14,15,16,17]]},{"name":"alt","colors":[]}]},{"name":"group","texture-names":[],"palettes":[]}]})";
    std::ofstream("palettes.json")<<json;globalPaletteDb.initialize();assert(globalPaletteDb.isInitialized());
    assert(globalPaletteDb.containsPaletteGroup("group") && globalPaletteDb.getPaletteGroupList().size()==1);
    auto group=globalPaletteDb.getPaletteGroup("group");assert(group.textureNames==std::vector<std::string>({"a","b"}));
    assert(group.defaultPalette.name=="base" && group.palettes.size()==1 && group.palettes.at("alt").colors.size()==2);
    auto copy=group;copy.defaultPalette.colors[0]=sf::Color::Green;assert(group.defaultPalette.colors[0]==sf::Color(1,2,3));
    group.name="new";globalPaletteDb.setPaletteGroup(group);assert(globalPaletteDb.containsPaletteGroup("new"));
    globalPaletteDb.erasePaletteGroup("group");assert(!globalPaletteDb.containsPaletteGroup("group"));
    assert(globalPaletteDb.getPaletteGroup("missing").name.empty() && globalPaletteDb.getPaletteGroupList().size()==1);
    globalPaletteDb.saveScripts();globalPaletteDb.paletteGroups.clear();globalPaletteDb.loadScripts();
    auto loaded=globalPaletteDb.getPaletteGroup("new");assert(loaded.defaultPalette.colors==group.defaultPalette.colors && loaded.palettes.at("alt").colors==group.palettes.at("alt").colors);
    const auto before=globalPaletteDb.getPaletteGroupList();std::ofstream("palettes.json")<<"{";globalPaletteDb.loadScripts();assert(globalPaletteDb.getPaletteGroupList()==before);
    sf::Image image;image.create(2,2,sf::Color(1,2,3,77));image.setPixel(1,1,sf::Color(99,99,99,0));
    auto alt=group.palettes.at("alt");assert(alt.generatePaletteSwap(image,group.defaultPalette,99));assert(image.getPixel(0,0)==sf::Color(11,12,13,77) && image.getPixel(1,1).a==0);
    image.setPixel(0,0,sf::Color(1,2,3,44));assert(alt.generatePaletteSwap(image,group.defaultPalette,0)==false); // later RGB already swapped: ordered, partial mutation
    assert(image.getPixel(0,0)==sf::Color(255,0,255,44));
    auto shorter=group.defaultPalette;shorter.colors.clear();assert(!alt.generatePaletteSwap(image,shorter,99));
    Config config;ResourceManager::setConfig(&config);ResourceArchive archive("missing.vra",false);
    ResourceManager::archive=&archive;archive.loaded=true;archive.paletteData=new char[json.size()+1];std::copy(json.begin(),json.end(),archive.paletteData);archive.paletteData[json.size()]=0;
    globalPaletteDb.loadScripts();assert(globalPaletteDb.containsPaletteGroup("group"));
    globalPaletteDb.paletteGroups.clear();globalPaletteDb.initialize();assert(globalPaletteDb.paletteGroups.empty()); // initialize is one-shot
    archive.paletteData[0]='!';globalPaletteDb.loadScripts();assert(globalPaletteDb.paletteGroups.empty());
    ResourceManager::archive=nullptr;globalPaletteDb.paletteGroups.clear();ResourceManager::setConfig(nullptr);
    std::cout<<"PaletteDatabase: JSON/archive load/save, duplicate keys, copy/move, one-shot initialization and RGB/alpha swap passed\n";
}
