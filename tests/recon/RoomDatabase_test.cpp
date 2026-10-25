#include "port_lib_types.h"
#define private public
#include "RoomDatabase.h"
#include "ResourceManager.h"
#include "ResourceArchive.h"
#include "Room.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <fstream>
#include <iostream>

int main() {
    const std::string json = R"({"world_name":"Fixture","world_author":"Author","world_description":"Description","world_allow_backgrounds":false,"ugcFileId":1234567890123,"ugcImagePreviewPath":"preview.png","act_specifications":[{"width":12,"height":8,"resize":false,"resize_width":6,"resize_height":4}],"acts":[{"rooms":{}}]})";
    std::filesystem::create_directories("world");
    std::ofstream("world/rooms.json") << json;
    ResourceManager::setConfig(nullptr);
    _randEngine.seed(781);
    auto expected = _randEngine;
    {
        RoomDatabase database(false, "world/rooms.json");
        assert((database.rooms.size() == 12 && database.actDimensions == std::vector<std::pair<int,int>>({{6,4}})));
        assert(!database.worldAllowBackgrounds && !database.debugCountRoomFrequency);
        assert(database.worldName == "Fixture" && database.worldAuthorName == "Author" && database.worldDescription == "Description");
        assert(database.steamUgcFileId == 1234567890123ULL && database.ugcImagePreviewFilename == "preview.png");
        assert(database.getFolderPath() == std::filesystem::path("world") && _randEngine == expected);
        assert(database.getFirstRoom() == nullptr && database.getRoom("missing") == nullptr);
        auto make = [](const char *uid, const char *name) {
            auto *room = new Room(nullptr);
            room->act = Room::ACT_ONE; room->roomType = 5; room->uid = uid; room->roomName = name;
            room->shopRoom = room->treasureRoom = room->brightSkyRoom = room->bookPedestalRoom = false;
            room->isRuins = false;
            return room;
        };
        auto *a = make("a", "Alpha"), *b = make("b", "Beta");
        database.roomUidLookup = {{"a", a}, {"b", b}};
        database.rooms[0][5] = {a, b};
        database.rooms[0][261] = {a};
        assert(database.getRoomCountOfAct(Room::ACT_ONE) == 3 && database.getRoomCountOfAct(Room::NUM_ACTS) == 0);
        assert((database.getAllRoomUidAndNames() == std::vector<std::pair<std::string,std::string>>({{"a","Alpha"},{"b","Beta"}})));
        assert(database.getFirstRoom() == a && database.getRoom("b") == b);
        assert(database.getAllTypesOfActWithBitSet(Room::ACT_ONE, Room::BIT_SHOP) == std::set<unsigned>{5});
        assert(database.getNextType(0, 5, 1) == 261 && database.getNextType(0, 261, 1) == 5 && database.getNextType(0, 5, -1) == 261);
        assert(database.getNextType(0, 5, 0) == -1);
        Room model(nullptr);
        model.act = Room::ACT_ONE; model.roomType = 5;
        model.shopRoom = model.brightSkyRoom = true; model.treasureRoom = model.bookPedestalRoom = false;
        std::map<std::string,int> used{{"a",2},{"b",1}};
        expected();
        assert(database.getRandomRoom(&model, used) == b && _randEngine == expected && used.at("b") == 1);
        assert(b->shopRoom && b->brightSkyRoom && !b->treasureRoom && !b->isRuins);
        used.clear(); const unsigned index = (expected() & 0x7fffffffu) % 2;
        database.debugCountRoomFrequency = true;
        assert(database.getRandomRoom(&model, used) == (index ? b : a));
        assert(database.debugRoomFrequencyCount.at(index ? b : a) == 1 && _randEngine == expected);
        model.roomType = 99;
        assert(database.getRandomRoom(&model, used) == nullptr && _randEngine == expected);
        // Tile fixtures avoid claiming that the still-stubbed Room loader has been translated.
        std::vector<std::vector<Tile>> upper(3, std::vector<Tile>(4)), lower(3, std::vector<Tile>(4));
        std::vector<Tile *> upperColumns, lowerColumns;
        for (int x=0; x<3; ++x) {
            upperColumns.push_back(upper[x].data()); lowerColumns.push_back(lower[x].data());
            for (int y=0; y<4; ++y) upper[x][y].type = lower[x][y].type = 1;
        }
        a->tileW = b->tileW = 3; a->tileH = b->tileH = 4;
        a->tiles = upperColumns.data(); b->tiles = lowerColumns.data();
        assert(!database.roomsFitHorizontally(a,b) && !database.roomsFitVertically(a,b));
        upper[1][3].type = 3; lower[1][0].type = 0;
        assert(database.roomsFitVertically(a,b));
        lower[1][1].type = 0;
        assert(!database.roomsFitVertically(a,b));
        lower[1][0].type = 6;
        assert(database.roomsFitVertically(a,b));
        lower[1][0].type = 1;
        assert(!database.roomsFitVertically(a,b));
        upper[2][2].type = 0; lower[0][2].type = 0;
        assert(database.roomsFitHorizontally(a,b));
        Room *outColumn[1] = {a}; Room **output[1] = {outColumn};
        database.randomizeRooms(output, nullptr, 1, 1, true);
        assert(outColumn[0] == a && _randEngine == expected);
        database.debugPrintRoomFrequencies();
        assert(!database.saveRoom(a) && !database.saveRooms() && !database.deleteRoom("a") && !database.resizeAct(0,1,1));
        assert(database.roomUidLookup.size() == 2);
    }
    RoomDatabase::databaseCache.clear();
    auto details = RoomDatabase::getDatabaseDetails("world/rooms.json");
    assert(details.title == "Fixture" && details.author == "Author" && details.description == "Description");
    std::ofstream("world/rooms.json") << "{";
    assert(RoomDatabase::getDatabaseDetails("world/rooms.json").title == "Fixture");
    RoomDatabase::databaseCache.clear();
    assert(RoomDatabase::getDatabaseDetails("world/rooms.json").title.empty());
    assert(RoomDatabase::databaseCache.empty());
    Config config; ResourceManager::setConfig(&config);
    ResourceArchive archive("missing.vra", false);
    archive.loaded = true; archive.roomData = new char[json.size()+1];
    std::copy(json.begin(), json.end(), archive.roomData); archive.roomData[json.size()] = 0;
    ResourceManager::archive = &archive;
    {
        RoomDatabase base(true, "missing.json");
        assert(base.worldName.empty() && base.worldAllowBackgrounds && !base.debugCountRoomFrequency);
        assert((base.actDimensions == std::vector<std::pair<int,int>>({{6,4}})));
        archive.roomData[0] = '!'; assert(!base.loadRooms());
    }
    ResourceManager::archive = nullptr; ResourceManager::setConfig(nullptr);
    std::cout << "RoomDatabase: metadata/archive, dimensions, queries, least-used RNG, fitting, cache and editor exclusions passed\n";
}
