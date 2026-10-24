// PaletteDatabase 004b6f70-004b831a. Move/destruction of nested records is implicit STL C++.
#include "recon_all.h"
#include <fstream>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
namespace {
// getConfig is the existing accessor, but the generated header marks it private.
// Recover access without modifying the regenerated PDB contract (as in Sprite.cpp).
template<class Tag, typename Tag::Type Member> struct GetterAccess {
    friend typename Tag::Type configGetter(Tag) { return Member; }
};
struct ResourceConfig { using Type=Config *(*)(); friend Type configGetter(ResourceConfig); };
template struct GetterAccess<ResourceConfig,&ResourceManager::getConfig>;
// Original parser lambda @004b8320. First palette is the default, subsequent
// duplicate names keep their first insertion, matching unordered_map::insert.
bool readGroup(const rapidjson::Value &value,PaletteDatabase::PaletteGroup &group) {
    if(!value.IsObject() || !value.HasMember("name") || !value["name"].IsString() ||
       !value.HasMember("texture-names") || !value["texture-names"].IsArray() ||
       !value.HasMember("palettes") || !value["palettes"].IsArray()) return false;
    group.name=value["name"].GetString();
    for(const auto &texture:value["texture-names"].GetArray()) {
        if(!texture.IsString()) return false;
        group.textureNames.emplace_back(texture.GetString());
    }
    bool first=true;
    for(const auto &entry:value["palettes"].GetArray()) {
        if(!entry.IsObject() || !entry.HasMember("name") || !entry["name"].IsString() ||
           !entry.HasMember("colors") || !entry["colors"].IsArray()) return false;
        PaletteDatabase::Palette palette;
        palette.name=entry["name"].GetString();
        for(const auto &color:entry["colors"].GetArray()) {
            if(!color.IsArray() || color.Size()<4) return false;
            for(unsigned i=0;i<4;++i) if(!color[i].IsInt()) return false;
            palette.colors.emplace_back(static_cast<sf::Uint8>(color[0].GetInt()),static_cast<sf::Uint8>(color[1].GetInt()),
                                        static_cast<sf::Uint8>(color[2].GetInt()),static_cast<sf::Uint8>(color[3].GetInt()));
        }
        if(first) { group.defaultPalette=palette;first=false; }
        else { const auto key=palette.name;group.palettes.emplace(key,std::move(palette)); }
    }
    return true;
}
}
std::vector<std::string> PaletteDatabase::getPaletteGroupList() const {
    std::vector<std::string> names;
    for(const auto &entry:globalPaletteDb.paletteGroups) names.push_back(entry.first);
    return names;
}
PaletteDatabase::PaletteGroup PaletteDatabase::getPaletteGroup(const std::string &name) const {
    const auto found=globalPaletteDb.paletteGroups.find(name);
    if(found!=globalPaletteDb.paletteGroups.end()) return found->second;
    VLOG(LOG_ERROR)<<"PaletteDatabase::getPaletteGroup: palette group doesn't exist! \""<<name<<"\"";
    return {};
}
void PaletteDatabase::setPaletteGroup(const PaletteGroup &group) { globalPaletteDb.paletteGroups[group.name]=group; }
void PaletteDatabase::loadScripts() {
    rapidjson::Document document;
    if(configGetter(ResourceConfig{})() && ResourceManager::isArchiveLoaded()) {
        const char *data=ResourceManager::getPaletteData();
        if(!data) { VLOG(LOG_ERROR)<<"PaletteDatabase::loadScripts: No palette data found!";return; }
        VLOG(LOG_INFO)<<"PaletteDatabase::loadScripts: Got palette data!";
        document.Parse(data);
        if(document.HasParseError()) { VLOG(LOG_ERROR)<<"PaletteDatabase::loadScripts: Error parsing palettes.json in data.vra!";return; }
    } else {
        std::ifstream input("palettes.json");
        if(!input) { VLOG(LOG_ERROR)<<"PaletteDatabase::loadScripts: Failed to open palettes.json! Does it exist?";return; }
        rapidjson::IStreamWrapper stream(input);document.ParseStream(stream);
        if(document.HasParseError()) { VLOG(LOG_ERROR)<<"PaletteDatabase::loadScripts: Error parsing attacks.json!";return; }
    }
    // Invalid external JSON is rejected instead of dereferencing wrong RapidJSON types.
    if(!document.IsObject() || !document.HasMember("palette-groups") || !document["palette-groups"].IsArray()) return;
    globalPaletteDb.paletteGroups.clear();
    for(const auto &value:document["palette-groups"].GetArray()) {
        PaletteGroup group;
        if(!readGroup(value,group)) { VLOG(LOG_ERROR)<<"PaletteDatabase::loadScripts: Invalid palette group";continue; }
        const auto key=group.name;globalPaletteDb.paletteGroups.emplace(key,std::move(group));
    }
}
void PaletteDatabase::saveScripts() {
    VLOG(LOG_INFO)<<"PaletteDatabase::saveScripts: saving palettes to JSON...";
    std::ofstream output("palettes.json");
    if(!output) { VLOG(LOG_ERROR)<<"PaletteDatabase::saveScripts: Unable to open palettes.json for writing";return; }
    rapidjson::OStreamWrapper stream(output);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(stream);writer.SetIndent(' ',4);
    // Writer lambdas @004b74c0 and @004b7630, with the same field/array order.
    const auto writePalette=[&](const Palette &palette) {
        writer.StartObject();writer.Key("name");writer.String(palette.name.c_str());writer.Key("colors");writer.StartArray();
        for(const auto color:palette.colors) { writer.StartArray();writer.Uint(color.r);writer.Uint(color.g);writer.Uint(color.b);writer.Uint(color.a);writer.EndArray(); }
        writer.EndArray();writer.EndObject();
    };
    const auto writeGroup=[&](const PaletteGroup &group) {
        writer.StartObject();writer.Key("name");writer.String(group.name.c_str());writer.Key("texture-names");writer.StartArray();
        for(const auto &texture:group.textureNames) writer.String(texture.c_str());
        writer.EndArray();writer.Key("palettes");writer.StartArray();writePalette(group.defaultPalette);
        for(const auto &entry:group.palettes) writePalette(entry.second);
        writer.EndArray();writer.EndObject();
    };
    writer.StartObject();writer.Key("_comment");
    writer.String("Similar to rooms.json, do not modify this file manually.  Use the in-game attack script editor!");
    writer.Key("palette-groups");writer.StartArray();
    for(const auto &entry:globalPaletteDb.paletteGroups) writeGroup(entry.second);
    writer.EndArray();writer.EndObject();output.close();
    VLOG(LOG_INFO)<<"PaletteDatabase::saveScripts: DONE!";
}
// Inlined initialize from ResourceManager::runTasks; accessors preserve the
// executable's specialization to globalPaletteDb (ECX=0xb93094 for its map).
void PaletteDatabase::initialize() { if(!globalPaletteDb.initialized) { loadScripts();globalPaletteDb.initialized=true; } }
bool PaletteDatabase::isInitialized() const { return globalPaletteDb.initialized; }
bool PaletteDatabase::containsPaletteGroup(const std::string &name) const { return globalPaletteDb.paletteGroups.count(name)!=0; }
void PaletteDatabase::erasePaletteGroup(const std::string &name) { globalPaletteDb.paletteGroups.erase(name); }
