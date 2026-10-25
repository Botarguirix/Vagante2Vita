#include "recon_all.h"
#include <iterator>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

RoomDatabase::RoomDatabase(bool base, boost::filesystem::path file)
    : actDimensions(12, {12, 8}), rooms(12), defaultVagante(base), path(std::move(file)),
      steamUgcFileId(0), worldAllowBackgrounds(true), debugCountRoomFrequency(false) {
    VLOG(LOG_DEBUG) << "RoomDatabase::RoomDatabase Begin.";
    loadRooms();
    VLOG(LOG_DEBUG) << "RoomDatabase::RoomDatabase End.";
}
RoomDatabase::~RoomDatabase() {
    for (auto &entry : roomUidLookup) delete entry.second;
}

Room *RoomDatabase::getRoom(std::string uid) {
    const auto found = roomUidLookup.find(uid);
    return found == roomUidLookup.end() ? nullptr : found->second;
}
Room *RoomDatabase::getRoom(Room::ActType, unsigned, unsigned) {
    // Indexed room selection is an editor operation; the runtime selects by UID or randomly.
    return nullptr;
}
int RoomDatabase::getRoomCountOfAct(Room::ActType act) {
    if (unsigned(act) >= rooms.size()) return 0;
    int count = 0;
    for (const auto &entry : rooms[act]) count += int(entry.second.size());
    return count;
}
std::vector<std::pair<std::string, std::string>> RoomDatabase::getAllRoomUidAndNames() {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto &entry : roomUidLookup)
        result.emplace_back(entry.first, entry.second ? entry.second->roomName : "");
    return result;
}
std::set<unsigned> RoomDatabase::getAllTypesOfActWithBitSet(Room::ActType act,
                                                          Room::RoomTypeBitfield bits) {
    std::set<unsigned> result;
    if (unsigned(act) < rooms.size())
        for (const auto &entry : rooms[act])
            if (entry.first & unsigned(bits)) result.insert(entry.first & ~unsigned(bits));
    return result;
}
Room *RoomDatabase::getFirstRoom() {
    for (auto &act : rooms)
        for (auto &entry : act)
            if (!entry.second.empty()) return entry.second.front();
    return nullptr;
}
boost::filesystem::path RoomDatabase::getFolderPath() { return path.parent_path(); }

int RoomDatabase::getNextType(int act, int type, int direction) {
    auto &types = rooms[act];
    const auto found = types.find(unsigned(type));
    if (found != types.end() && direction > 0) {
        const auto next = std::next(found);
        return int((next == types.end() ? types.begin() : next)->first);
    }
    if (found != types.end() && direction < 0)
        return int((found == types.begin() ? std::prev(types.end()) : std::prev(found))->first);
    VLOG(LOG_ERROR) << "Unable to get next type. Act " << act << ", type " << type
                    << ", dir: " << direction;
    return -1;
}

Room *RoomDatabase::getRandomRoom(Room *model, std::map<std::string, int> &used) {
    auto &candidates = rooms[model->act][model->roomType];
    if (candidates.empty()) {
        VLOG(LOG_ERROR) << "RoomDatabase::getRandomRoom - Found no rooms for Act "
                       << model->act << ", type: " << model->roomType;
        sf::sleep(sf::milliseconds(10));
        return nullptr;
    }
    std::vector<std::string> choices;
    int frequency = 0;
    for (;;) {
        choices.clear();
        std::size_t lower = 0;
        for (const auto *room : candidates) {
            const auto found = used.find(room->uid);
            if (found == used.end()) {
                if (frequency == 0) choices.push_back(room->uid);
            } else if (found->second == frequency) choices.push_back(room->uid);
            else if (found->second < frequency) ++lower;
        }
        if (!choices.empty() || lower >= candidates.size()) break;
        ++frequency;
    }
    // Same one-draw MSVC uniform_int<int> followed by modulo, not std::uniform_int_distribution.
    const unsigned random = _randEngine() & 0x7fffffffu;
    if (choices.empty()) { VLOG(LOG_ERROR) << "Invalid negative room-use counts"; return nullptr; }
    auto *room = getRoom(choices[random % choices.size()]);
    if (!room) return nullptr; // Broken UID index, not a generated room.
    room->shopRoom = model->shopRoom;
    room->treasureRoom = model->treasureRoom;
    room->brightSkyRoom = model->brightSkyRoom;
    room->bookPedestalRoom = model->bookPedestalRoom;
    room->act = model->act;
    if (debugCountRoomFrequency) ++debugRoomFrequencyCount[room];
    return room;
}

void RoomDatabase::debugPrintRoomFrequencies() {
    VLOG(LOG_INFO) << "Printing out room frequencies...";
    for (const auto &entry : debugRoomFrequencyCount)
        VLOG(LOG_INFO) << entry.first->act << '.' << entry.first->roomType << '['
                       << entry.first->uid << "] :" << entry.second;
}

bool RoomDatabase::saveRoom(Room *) { /* Room editor is excluded. */ return false; }
bool RoomDatabase::saveRooms() { /* Room editor is excluded. */ return false; }
Room *RoomDatabase::createNewRoomOfType(Room::ActType, unsigned, int, int) {
    /* Room editor is excluded. */ return nullptr;
}
bool RoomDatabase::deleteRoom(std::string) { /* Room editor is excluded. */ return false; }
bool RoomDatabase::resizeAct(int, int, int) { /* Room editor is excluded. */ return false; }
