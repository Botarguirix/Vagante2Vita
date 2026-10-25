#include "recon_all.h"
#include <cstdlib>
#include <fstream>
#include <memory>
#include <rapidjson/istreamwrapper.h>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
namespace {
// Existing getConfig is private in the generated PDB header (same recovery as PaletteDatabase).
template<class Tag, typename Tag::Type Member> struct GetterAccess {
    friend typename Tag::Type roomConfigGetter(Tag) { return Member; }
};
struct ResourceConfig { using Type = Config *(*)(); friend Type roomConfigGetter(ResourceConfig); };
template struct GetterAccess<ResourceConfig, &ResourceManager::getConfig>;
void readString(const rapidjson::Value &object, const char *key, std::string &value) {
    if (object.HasMember(key) && object[key].IsString()) value = object[key].GetString();
}
}

bool RoomDatabase::loadRooms() {
    VLOG(LOG_DEBUG) << "RoomDatabase::loadRooms()";
    rapidjson::Document document;
    if (defaultVagante && roomConfigGetter(ResourceConfig{})() && ResourceManager::isArchiveLoaded()) {
        const char *data = ResourceManager::getRoomData();
        if (!data) { VLOG(LOG_ERROR) << "RoomDatabase::loadRooms() - No room data found!"; return false; }
        VLOG(LOG_DEBUG) << "Got room data!";
        document.Parse(data);
    } else {
        std::ifstream file(path);
        if (!file) { VLOG(LOG_ERROR) << "RoomDatabase::loadRooms() - Failed to open " << path.string() << ". Does it exist?"; return false; }
        rapidjson::IStreamWrapper stream(file);
        document.ParseStream(stream);
    }
    if (document.HasParseError() || !document.IsObject()) {
        VLOG(LOG_ERROR) << "RoomDatabase::loadRooms() - Error parsing " << path.string();
        return false;
    }
    if (!defaultVagante) {
        readString(document, "world_name", worldName);
        readString(document, "world_author", worldAuthorName);
        readString(document, "world_description", worldDescription);
        if (document.HasMember("world_allow_backgrounds") && document["world_allow_backgrounds"].IsBool())
            worldAllowBackgrounds = document["world_allow_backgrounds"].GetBool();
        if (document.HasMember("ugcFileId") && document["ugcFileId"].IsUint64())
            steamUgcFileId = document["ugcFileId"].GetUint64(); // Metadata only, no Steam calls.
        readString(document, "ugcImagePreviewPath", ugcImagePreviewFilename);
    }
    if (!document.HasMember("act_specifications") || !document["act_specifications"].IsArray() ||
        !document.HasMember("acts") || !document["acts"].IsArray()) {
        VLOG(LOG_ERROR) << "RoomDatabase::loadRooms() - Invalid act arrays";
        return false;
    }
    actDimensions.clear();
    std::map<int, std::pair<int, int>> resizes;
    for (const auto &spec : document["act_specifications"].GetArray()) {
        if (!spec.IsObject() || !spec.HasMember("width") || !spec["width"].IsInt() ||
            !spec.HasMember("height") || !spec["height"].IsInt()) return false;
        const int act = int(actDimensions.size());
        actDimensions.emplace_back(spec["width"].GetInt(), spec["height"].GetInt());
        // Presence of resize is the original condition; its boolean value is not read.
        if (spec.HasMember("resize")) {
            if (!spec.HasMember("resize_width") || !spec["resize_width"].IsInt() ||
                !spec.HasMember("resize_height") || !spec["resize_height"].IsInt()) return false;
            resizes.emplace(act, std::make_pair(spec["resize_width"].GetInt(), spec["resize_height"].GetInt()));
        }
    }
    unsigned act = 0;
    for (const auto &data : document["acts"].GetArray()) {
        if (act >= rooms.size() || act >= actDimensions.size()) return false;
        if (data.IsObject() && data.MemberCount() && data.HasMember("rooms") && data["rooms"].IsObject()) {
            for (auto entry = data["rooms"].MemberBegin(); entry != data["rooms"].MemberEnd(); ++entry) {
                if (!entry->value.IsArray()) return false;
                const unsigned type = unsigned(std::atoi(entry->name.GetString()));
                for (const auto &roomData : entry->value.GetArray()) {
                    auto room = std::make_unique<Room>(act, 0, 1, 1, actDimensions[act].first, actDimensions[act].second);
                    room->load(roomData, type);
                    const auto resize = resizes.find(int(act));
                    if (resize != resizes.end()) room->resizeRoomTo(resize->second.first, resize->second.second);
                    if (room->uid.empty()) room->uid = Room::generateRandomUniqueId();
                    while (!room->uid.empty() && roomUidLookup.find(room->uid) != roomUidLookup.end()) {
                        VLOG(LOG_WARNING) << "Room uid collision! Regenerating " << room->uid;
                        room->uid = Room::generateRandomUniqueId();
                    }
                    if (room->uid.empty()) {
                        VLOG(LOG_ERROR) << "Room loader/UID generator unavailable; refusing an invalid room";
                        return false; // Room and its UID generator are still dependency stubs.
                    }
                    if (unsigned(room->act) >= rooms.size()) return false;
                    Room *loaded = room.get();
                    roomUidLookup.emplace(room->uid, loaded);
                    room.release();
                    rooms[loaded->act][loaded->roomType].push_back(loaded);
                    if (debugCountRoomFrequency) debugRoomFrequencyCount.emplace(loaded, 0);
                }
            }
        }
        ++act;
    }
    for (const auto &resize : resizes) {
        VLOG(LOG_INFO) << "RoomDatabase::loadRooms Resizing act: " << resize.first;
        actDimensions[resize.first] = resize.second;
    }
    VLOG(LOG_INFO) << "Set the room dimensions for " << actDimensions.size() << " distinct acts.";
    VLOG(LOG_INFO) << "Loaded " << path.string() << " successfully.";
    std::size_t count = 0;
    for (const auto &level : rooms) for (const auto &entry : level) count += entry.second.size();
    VLOG(LOG_INFO) << "RoomDatabase::loadRooms() - Total # of rooms: " << count;
    return true;
}

RoomDatabase::DatabaseDetails RoomDatabase::getDatabaseDetails(std::string fileName) {
    const auto cached = databaseCache.find(fileName);
    if (cached != databaseCache.end()) return cached->second;
    DatabaseDetails result;
    std::ifstream file(fileName);
    if (!file) { VLOG(LOG_ERROR) << "RoomDatabase::getDatabaseDetails - Failed to open " << fileName; return result; }
    rapidjson::IStreamWrapper stream(file);
    rapidjson::Document document;
    document.ParseStream(stream);
    if (document.HasParseError() || !document.IsObject()) {
        VLOG(LOG_ERROR) << "RoomDatabase::getDatabaseDetails - Error parsing " << fileName;
        return result;
    }
    readString(document, "world_name", result.title);
    readString(document, "world_author", result.author);
    readString(document, "world_description", result.description);
    databaseCache[fileName] = result;
    return result;
}
