#include "recon_all.h"
#include "localization.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rapidjson/istreamwrapper.h>

namespace {
void reset() {
    Stats::accumulators.clear(); Stats::diedToCount.clear();
    Stats::killCount.clear(); Stats::killCountHard.clear();
    Stats::unlocks.clear(); Stats::newlyUnlocked.clear();
    Stats::unlockPointThresholds.clear(); Stats::runStats.clear();
    Stats::progressionPoints = 0; Stats::progressionLevel = 0;
}
void write(const std::string &text) { std::ofstream(Stats::FILE_NAME) << text; }
bool has(const std::vector<std::pair<Background::BackgroundType, bool>> &list, Background::BackgroundType bg) {
    return std::any_of(list.begin(), list.end(), [=](const auto &entry) { return entry.first == bg; });
}
void removeFixtureSave() { std::filesystem::remove(Stats::FILE_NAME); }

void testRegistryAndCounters() {
    reset();
    _randEngine.seed(91); const auto rngBefore = _randEngine;
    assert(!Stats::init()); // New profile; init does not invent a successful load.
    assert(!std::filesystem::exists(Stats::FILE_NAME));
    assert(Stats::FILE_FORMAT_VERSION == 1 && Stats::FILE_NAME == "savedata.sav");
    assert(Stats::unlocks.size() == 39 && Stats::unlockPointThresholds.size() == 26);
    for (const auto &entry : Stats::unlocks) assert(!entry.second.unlocked);
    const int thresholds[] = {100,210,327,451,581,717,858,1005,1157,1314,1477,1645,1818,
        1995,2177,2364,2555,2751,2951,3156,3365,3579,3797,4019,4246,4477};
    for (unsigned i = 0; i < 26; ++i) {
        assert(Stats::unlockPointThresholds[i].second == thresholds[i]);
        const auto &unlock = Stats::unlocks.at(Stats::unlockPointThresholds[i].first);
        assert(!unlock.hasCondition && unlock.progressionLevel == int(i + 1));
    }
    assert(Stats::unlocks.at("ranger").name == "Houndmaster");
    assert(Stats::unlocks.at("potion_addict").background == Background::BG_POTION_ADDICT);
    assert(Stats::unlocks.at("stoic").background == Background::BG_STOIC);
    assert(Stats::unlocks.at("boomerang").background == Background::BG_BOOMERANG);
    assert(Stats::unlocks.at("tinkerer").background == Background::BG_GRAPPLING_HOOK);
    assert(Stats::unlocks.at("houndmaster_flail").background == Background::BG_FLAIL);
    assert(Stats::getProgressionInterval(99) == std::make_pair(0,100));
    assert(Stats::getProgressionInterval(100) == std::make_pair(0,100));
    assert(Stats::getProgressionInterval(101) == std::make_pair(100,210));
    assert(Stats::getProgressionInterval(5000) == std::make_pair(4477,4477));
    assert(!Stats::isUnlocked("missing") && Stats::unlocks.size() == 39);
    assert(Stats::getValue("missing") == 0 && Stats::accumulators.empty());
    Stats::setIfMax("max", -3); Stats::setIfMax("max", -5);
    assert(Stats::getValue("max") == -3);
    Stats::setIfMax("max", 7); Stats::increment("max", -2);
    assert(Stats::getValue("max") == 5);
    Stats::setValue("max", 4); assert(Stats::getValue("max") == 4);
    Stats::runStats.resize(2);
    Stats::runIncrement(0, "damage", 3); Stats::runIncrement(0, "damage", 4);
    Stats::runIncrement(-1, "damage", 100); Stats::runIncrement(2, "damage", 100);
    assert(Stats::getRunStat(0, "damage") == 7 && Stats::getRunStat(1, "damage") == 0);
    assert(Stats::getRunStat(-1, "damage") == 0 && Stats::getRunStat(2, "damage") == 0);
    Stats::accumulateKills("Goblin", 0, false); Stats::accumulateKills("Goblin", -1, false);
    assert(Stats::killCount.empty());
    Stats::accumulateKills("Goblin", 2, false); Stats::accumulateKills("Goblin", 7, true);
    assert(Stats::getKilledCount("Goblin", false) == 2 && Stats::getKilledCount("Goblin", true) == 7);
    assert(Stats::getKilledCount(Entity::TYPE_GOBLIN, true) == 2); // Native integer wrapper specializes to normal.
    assert(Stats::getKilledCount(255, false) == 0);
    Stats::progressionPoints = 99; Stats::updateUnlocks(); assert(!Stats::isUnlocked("experienced"));
    Stats::progressionPoints = 100; Stats::updateUnlocks();
    assert(Stats::isUnlocked("experienced") && Stats::progressionLevel == 1);
    assert(Stats::newlyUnlocked == std::vector<std::string>{"experienced"});
    Stats::unlock("experienced"); Stats::unlock("missing");
    assert(Stats::newlyUnlocked.size() == 1 && Stats::unlocks.size() == 39);
    Stats::progressionPoints = 0; Stats::updateUnlocks();
    assert(Stats::progressionLevel == 1 && Stats::isUnlocked("experienced") && Stats::newlyUnlocked.empty());
    // updateUnlocks evaluates copies: mutable captured state must not persist.
    Unlockable probe{}; probe.hasCondition = true;
    probe.conditionFunction = [calls = 0]() mutable { return ++calls == 2; };
    Stats::unlocks.emplace("copy-probe", probe);
    Stats::updateUnlocks(); Stats::updateUnlocks();
    assert(!Stats::isUnlocked("copy-probe"));
    Stats::unlocks.erase("copy-probe");
    assert(_randEngine == rngBefore);
}

void testConditionsAndBackgrounds() {
    reset(); assert(!Stats::init());
    Stats::setValue("victory_count", 1); Stats::setValue("good_victory_count", 1);
    Stats::setValue("Knight_victory_count", 1); Stats::setValue("Rogue_victory_count", 1);
    Stats::setValue("Mage_victory_count", 1); Stats::setValue("Wildling_victory_count", 1);
    Stats::setValue("Ranger_victory_count", 1); Stats::setValue("mortality_floors", 2);
    for (int type : {49,50,51,53,52,55,58,59,57,61,62,63}) {
        const auto &name = Entity::entityTypeToString.at(static_cast<Entity::EntityType>(type));
        Stats::accumulateKills(name, 1, true);
    }
    Stats::updateUnlocks();
    assert(Stats::isUnlocked("hard_mode") && Stats::isUnlocked("boss_trophy"));
    for (const char *key : {"knight_armored", "rogue_boomerang", "mage_heirloom", "wildling_weapon", "houndmaster_flail"})
        assert(Stats::isUnlocked(key));
    assert(!Stats::isUnlocked("death") && !Stats::isUnlocked("music_cave") && !Stats::isUnlocked("tinkerer"));
    Stats::setValue("mortality_floors", 3);
    for (int type : {49,50,51,53,52,55,58,59,57,61,62})
        Stats::accumulateKills(Entity::entityTypeToString.at(static_cast<Entity::EntityType>(type)), 1, false);
    Stats::updateUnlocks();
    assert(Stats::isUnlocked("death") && Stats::isUnlocked("tinkerer"));
    assert(Stats::isUnlocked("music_cave") && Stats::isUnlocked("music_forest") && Stats::isUnlocked("music_catacombs"));
    assert(!Stats::isUnlocked("music_rift"));
    Stats::accumulateKills(Entity::entityTypeToString.at(Entity::TYPE_BOSS_SHADOW_KNIGHT), 1, false);
    Stats::updateUnlocks(); assert(Stats::isUnlocked("music_rift"));
    assert(Stats::getAvailableBackgrounds(6, true).empty());
    for (int playerClass = 0; playerClass < 5; ++playerClass) {
        const auto list = Stats::getAvailableBackgrounds(playerClass, false);
        assert(has(list, Background::BG_KNIGHT_ARMORED) == (playerClass == 0));
        assert(has(list, Background::BG_ROGUE_CROSSBOW) == (playerClass == 1));
        assert(has(list, Background::BG_MAGE_HEIRLOOM) == (playerClass == 2));
        assert(has(list, Background::BG_WILDLING_WEAPON) == (playerClass == 3));
        assert(has(list, Background::BG_FLAIL) == (playerClass == 4));
        assert(has(list, Background::BG_POTION_ADDICT));
        assert(std::is_sorted(list.begin(), list.end(), [](const auto &a, const auto &b) {
            return a.second == b.second ? a.first < b.first : a.second > b.second;
        }));
    }
    Unlockable synchronized{};
    synchronized.type = Unlockable::BACKGROUND; synchronized.background = Background::BG_SYNCHRONIZED;
    synchronized.unlocked = true;
    Stats::unlocks.emplace("synchronized-probe", synchronized);
    const auto all = Stats::getAvailableBackgrounds(0, true);
    assert(all.size() == 29 && std::all_of(all.begin(), all.end(), [](const auto &v) { return v.second; }));
    assert(!has(all, Background::BG_SYNCHRONIZED));
    Stats::unlocks.erase("synchronized-probe");
}

void testPersistence() {
    reset(); assert(!Stats::init());
    Stats::progressionPoints = 210; Stats::updateUnlocks();
    Stats::setValue("counter", -5); Stats::setValue("quoted\"key", 8);
    Stats::diedToCount["Goblin"] = 3;
    Stats::accumulateKills("Goblin", 7, false); Stats::accumulateKills("Goblin", 11, true);
    Stats::runStats.resize(1); Stats::runIncrement(0, "session", 42);
    assert(Stats::save());
    {
        std::ifstream input(Stats::FILE_NAME); rapidjson::IStreamWrapper wrapper(input);
        rapidjson::Document document; document.ParseStream(wrapper);
        assert(!document.HasParseError() && document.MemberCount() == 6);
        assert(document["file_format_version"].GetInt() == 1 && document["points"].GetInt() == 210);
        assert(document["accumulators"]["quoted\"key"].GetInt() == 8);
        assert(document["unlocks"].MemberCount() == 2 && document["unlocks"]["experienced"].IsTrue());
        assert(!document.HasMember("kill_count_hard") && !document.HasMember("runStats"));
    }
    reset(); assert(Stats::init());
    assert(Stats::progressionPoints == 210 && Stats::progressionLevel == 2);
    assert(Stats::getValue("counter") == -5 && Stats::diedToCount.at("Goblin") == 3);
    assert(Stats::getKilledCount("Goblin", false) == 7 && Stats::getKilledCount("Goblin", true) == 0);
    assert(Stats::runStats.empty() && Stats::isUnlocked("experienced") && Stats::isUnlocked("survivor"));
    write(R"({"file_format_version":1,"points":12,"integers":{"dieGoblin":4,"dieUnknown":8,"unlock_survivor":1,"unlock_missing":1,"unlock_wildling":0,"legacy":9},"accumulators":{"ignored":100},"deaths_to_count":{"Goblin":6},"kill_count":{"Bat":2},"unlocks":{"experienced":false,"strong":true,"missing":true}})");
    assert(Stats::load());
    assert(Stats::progressionPoints == 12 && Stats::getValue("legacy") == 9 && Stats::getValue("ignored") == 0);
    assert(Stats::diedToCount.at("Goblin") == 6 && !Stats::diedToCount.count("Unknown"));
    assert(Stats::isUnlocked("survivor") && Stats::isUnlocked("strong") && !Stats::isUnlocked("experienced"));
    assert(!Stats::unlocks.count("missing") && !Stats::isUnlocked("wildling"));
    const auto savedCounters = Stats::accumulators;
    write(R"({"file_format_version":99,"points":999})");
    assert(Stats::load() && Stats::progressionPoints == 12 && Stats::accumulators == savedCounters);
    write("{"); assert(!Stats::load() && Stats::accumulators == savedCounters);
    write("[]"); assert(!Stats::load());
    write(R"({"file_format_version":1,"points":"bad","accumulators":{"bad":[],"valid":3},"unlocks":{"strong":123}})");
    assert(Stats::load() && Stats::progressionPoints == 12 && Stats::getValue("valid") == 3);
    assert(!Stats::accumulators.count("bad") && !Stats::isUnlocked("strong"));
    removeFixtureSave();
    std::filesystem::create_directory(Stats::FILE_NAME);
    assert(!Stats::save());
    std::filesystem::remove(Stats::FILE_NAME);
    assert(!Stats::load());
}
}
int main() {
    const auto run = std::filesystem::current_path() / ("run-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(run); std::filesystem::current_path(run);
    Entity::initEntityStringIdentifiers();
    testRegistryAndCounters(); testConditionsAndBackgrounds(); testPersistence();
    const auto previous = std::locale();
    auto *catalog = new recon::MessageCatalog("no-catalog-fixture.mo");
    catalog->messages["Experienced"] = "Nombre fixture";
    catalog->messages["Reach progression level {1}."] = "Nivel {1}.";
    std::locale::global(std::locale(previous, catalog));
    reset(); assert(!Stats::init());
    assert(Stats::unlocks.at("experienced").name == "Nombre fixture");
    assert(Stats::unlocks.at("experienced").conditions == "Nivel 1.");
    assert(!Stats::init() && Stats::unlockPointThresholds.size() == 52); // Original init retains the vector.
    std::locale::global(previous);
    std::cout << "Stats: 39 unlocks, 26 thresholds, 13 conditions, class filtering, counters, legacy/JSON round-trip, malformed input, I/O failure and RNG passed\n";
}
