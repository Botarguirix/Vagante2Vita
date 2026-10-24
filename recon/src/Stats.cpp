// Stats.cpp reconstructed from the PDB and Ghidra 007ba390-007bdefc.
#include "recon_all.h"
#include "localization.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

const int Stats::FILE_FORMAT_VERSION = 1;
const std::string Stats::FILE_NAME = "savedata.sav"; // dynamic initializer 00405cd0

namespace {
int valueOrZero(const std::map<std::string, int> &values, const std::string &name) {
    const auto found = values.find(name);
    return found == values.end() ? 0 : found->second;
}
void checkAchievements() {
    // Steam achievements are excluded. Game unlocks are updated separately.
}
std::string progressionDescription(int level) {
    auto text = recon::translate("Reach progression level {1}.");
    const std::string token = "{1}";
    for (auto position = text.find(token); position != std::string::npos;
         position = text.find(token, position)) {
        const auto number = std::to_string(level);
        text.replace(position, token.size(), number);
        position += number.size();
    }
    return text;
}
const rapidjson::Value *member(const rapidjson::Value &object, const char *name) {
    const auto found = object.FindMember(name);
    return found == object.MemberEnd() ? nullptr : &found->value;
}
}

std::pair<int, int> Stats::getProgressionInterval(int points) {
    std::pair<int, int> interval(0, 0);
    for (const auto &threshold : unlockPointThresholds) {
        if (points <= threshold.second) { interval.second = threshold.second; break; }
        interval.first = threshold.second;
    }
    if (interval.second < interval.first) interval.second = interval.first;
    return interval;
}
int Stats::getValue(std::string name) { return valueOrZero(accumulators, name); }
void Stats::setValue(std::string name, int value) {
    accumulators[name] = value;
    checkAchievements();
}
void Stats::setIfMax(std::string name, int value) {
    const auto found = accumulators.find(name);
    if (found == accumulators.end() || found->second < value) accumulators[name] = value;
    checkAchievements();
}
void Stats::increment(std::string name, int amount) {
    accumulators[name] += amount;
    checkAchievements();
}
void Stats::runIncrement(int slot, std::string name, int amount) {
    if (slot < 0 || static_cast<std::size_t>(slot) >= runStats.size()) return;
    runStats[slot][name] += amount;
    checkAchievements();
}
int Stats::getRunStat(int slot, std::string name) {
    // The native body has no lower bound check; reject negative slots rather
    // than reading before the vector. Positive slots retain the original check.
    if (slot < 0 || static_cast<std::size_t>(slot) >= runStats.size()) return 0;
    return valueOrZero(runStats[slot], name);
}
void Stats::accumulateKills(std::string name, int amount, bool hard) {
    if (amount < 1) return;
    (hard ? killCountHard : killCount)[name] += amount;
    checkAchievements();
}
int Stats::getKilledCount(std::string name, bool hard) {
    return valueOrZero(hard ? killCountHard : killCount, name);
}
int Stats::getKilledCount(int type, bool) {
    const auto found = Entity::entityTypeToString.find(static_cast<Entity::EntityType>(type));
    // 007bd89d clears BL and 007bd8a5 passes CL=0. The compiled integer wrapper
    // only queries normal kills, despite retaining a bool in the PDB signature.
    return found == Entity::entityTypeToString.end() ? 0 : getKilledCount(found->second, false);
}
bool Stats::isUnlocked(std::string name) {
    const auto found = unlocks.find(name);
    return found != unlocks.end() && found->second.unlocked;
}
void Stats::unlock(std::string name) {
    const auto found = unlocks.find(name);
    if (found == unlocks.end()) { VLOG(LOG_ERROR) << "Stats::unlock Invalid unlock key: " << name; return; }
    if (found->second.unlocked) return;
    VLOG(LOG_DEBUG) << "Stats::unlock " << name << " Unlocked!";
    found->second.unlocked = true;
    newlyUnlocked.push_back(name);
}
void Stats::updateUnlocks() {
    newlyUnlocked.clear();
    for (std::size_t index = 0; index < unlockPointThresholds.size(); ++index) {
        const auto &threshold = unlockPointThresholds[index];
        if (threshold.second <= progressionPoints) {
            if (progressionLevel < static_cast<int>(index + 1)) progressionLevel = static_cast<int>(index + 1);
            unlock(threshold.first);
        }
    }
    for (const auto &entry : unlocks) {
        // The executable copies the Unlockable, including its std::function.
        // Keep the copy: mutable captures must not become persistent by accident.
        auto state = entry.second;
        if (state.hasCondition && !state.unlocked && state.conditionFunction()) unlock(entry.first);
    }
}
std::vector<std::pair<Background::BackgroundType, bool>> Stats::getAvailableBackgrounds(int playerClass, bool all) {
    std::vector<std::pair<Background::BackgroundType, bool>> available;
    if (playerClass == 6) return available;
    for (const auto &entry : unlocks) {
        const auto &state = entry.second;
        if (state.type != Unlockable::BACKGROUND || state.background == Background::BG_SYNCHRONIZED) continue;
        if (!all) {
            if (state.background == Background::BG_MAGE_HEIRLOOM && playerClass != 2) continue;
            if (state.background == Background::BG_ROGUE_CROSSBOW && playerClass != 1) continue;
            if (state.background == Background::BG_KNIGHT_ARMORED && playerClass != 0) continue;
            if (state.background == Background::BG_WILDLING_WEAPON && playerClass != 3) continue;
            // Raw immediate 0x13, BG_FLAIL in the PDB header. Ghidra calls it
            // BG_POTION_ADDICT because its imported enum is stale.
            if (state.background == Background::BG_FLAIL && playerClass != 4) continue;
        }
        available.emplace_back(state.background, all || state.unlocked);
    }
    std::sort(available.begin(), available.end(), [](const auto &a, const auto &b) {
        return a.second == b.second ? a.first < b.first : a.second > b.second;
    });
    return available;
}

bool Stats::load() {
    VLOG(LOG_INFO) << "Stats :: loading data from file.";
    std::ifstream input(FILE_NAME);
    if (!input) { VLOG(LOG_INFO) << "Stats :: " << FILE_NAME << " not found, a new save file will be created."; return false; }
    rapidjson::IStreamWrapper stream(input);
    rapidjson::Document document;
    document.ParseStream(stream);
    if (document.HasParseError() || !document.IsObject()) {
        VLOG(LOG_ERROR) << "Stats::load() - Error parsing " << FILE_NAME;
        return false;
    }
    const auto *version = member(document, "file_format_version");
    // A parsed file with an unsupported version is ignored but returns true in
    // the original. Do not reinterpret it or overwrite its fields here.
    if (!version || !version->IsInt() || version->GetInt() != FILE_FORMAT_VERSION) return true;
    if (const auto *points = member(document, "points"); points && points->IsInt()) progressionPoints = points->GetInt();
    const auto *integers = member(document, "integers");
    const auto *values = member(document, "accumulators");
    if (integers && integers->IsObject()) values = integers;
    if (values && values->IsObject()) {
        for (auto it = values->MemberBegin(); it != values->MemberEnd(); ++it) {
            if (!it->value.IsInt()) continue; // Corrupt entries are not reinterpreted as int bits.
            std::string name = it->name.GetString();
            const int value = it->value.GetInt();
            if (name.compare(0, 3, "die") == 0) {
                name.erase(0, 3);
                if (Entity::stringToEntityType.count(name)) diedToCount[name] = value;
            } else if (name.compare(0, 7, "unlock_") == 0) {
                name.erase(0, 7);
                const auto found = unlocks.find(name);
                if (found != unlocks.end() && value > 0) found->second.unlocked = true;
            } else accumulators[name] = value;
        }
    }
    const auto loadCounts = [&](const char *key, std::map<std::string, int> &counts) {
        const auto *object = member(document, key);
        if (!object || !object->IsObject()) return;
        for (auto it = object->MemberBegin(); it != object->MemberEnd(); ++it)
            if (it->value.IsInt()) counts[it->name.GetString()] = it->value.GetInt();
    };
    loadCounts("deaths_to_count", diedToCount);
    loadCounts("kill_count", killCount);
    if (const auto *object = member(document, "unlocks"); object && object->IsObject()) {
        for (auto it = object->MemberBegin(); it != object->MemberEnd(); ++it) {
            const std::string name = it->name.GetString();
            const auto found = unlocks.find(name);
            if (found == unlocks.end()) { VLOG(LOG_ERROR) << "Stats::load Invalid unlock: " << name; }
            else found->second.unlocked = it->value.IsTrue();
        }
    }
    return true;
}
bool Stats::save() {
    VLOG(LOG_INFO) << "Stats::save() - Saving ..";
    std::ofstream output(FILE_NAME);
    if (!output) { VLOG(LOG_ERROR) << "Stats::save() - Unable to open " << FILE_NAME << " for writing!"; return false; }
    rapidjson::OStreamWrapper stream(output);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(stream);
    writer.SetIndent(' ', 4);
    writer.StartObject();
    writer.Key("file_format_version"); writer.Uint(FILE_FORMAT_VERSION);
    writer.Key("points"); writer.Int(progressionPoints);
    const auto writeCounts = [&](const char *key, const std::map<std::string, int> &values) {
        writer.Key(key); writer.StartObject();
        for (const auto &entry : values) { writer.Key(entry.first.c_str()); writer.Int(entry.second); }
        writer.EndObject();
    };
    writeCounts("accumulators", accumulators);
    writeCounts("deaths_to_count", diedToCount);
    writeCounts("kill_count", killCount);
    // The original format does not serialize killCountHard or runStats.
    writer.Key("unlocks"); writer.StartObject();
    for (const auto &entry : unlocks) {
        if (entry.second.unlocked) { writer.Key(entry.first.c_str()); writer.Bool(true); }
    }
    writer.EndObject(); writer.EndObject();
    output.close();
    return !output.fail();
}

// Unlock registry: the three local lambdas at 007bc0e0/007bc250/007bc520.
bool Stats::init() {
    unlocks.clear();
    int level = 0, totalPoints = 0;
    const auto add = [](Unlockable::UnlockableType type, const std::string &key,
                        const std::string &name) -> Unlockable & {
        auto &state = unlocks[key];
        state.type = type;
        state.name = name;
        state.unlocked = false;
        return state;
    };
    const auto progression = [&](Unlockable::UnlockableType type, const std::string &key,
                                const std::string &name) -> Unlockable & {
        auto &state = add(type, key, name);
        state.hasCondition = false;
        const float power = std::pow(static_cast<float>(level), 0.8f);
        totalPoints += static_cast<int>(power * 10.0f + 100.0f);
        unlockPointThresholds.emplace_back(key, totalPoints);
        state.progressionLevel = ++level;
        state.conditions = progressionDescription(state.progressionLevel);
        return state;
    };
    const auto conditional = [&](Unlockable::UnlockableType type, const std::string &key,
                                 const std::string &name, const std::string &description,
                                 std::function<bool()> condition) -> Unlockable & {
        auto &state = add(type, key, name);
        state.hasCondition = true;
        state.conditions = description;
        state.conditionFunction = std::move(condition);
        state.progressionLevel = 0;
        return state;
    };
    progression(Unlockable::BACKGROUND, "experienced", recon::translate("Experienced")).background = Background::BG_EXPERIENCED;
    progression(Unlockable::BACKGROUND, "survivor", recon::translate("Survivor")).background = Background::BG_SURVIVOR;
    progression(Unlockable::CLASS, "wildling", recon::translate("Wildling"));
    progression(Unlockable::BACKGROUND, "strong", recon::translate("Strong")).background = Background::BG_STRONG;
    progression(Unlockable::BACKGROUND, "nimble", recon::translate("Nimble")).background = Background::BG_NIMBLE;
    progression(Unlockable::SPECIAL, "dummy", recon::translate("Training Dummy"));
    progression(Unlockable::BACKGROUND, "bookworm", recon::translate("Bookworm")).background = Background::BG_BOOKWORM;
    progression(Unlockable::BACKGROUND, "illiterate", recon::translate("Illiterate")).background = Background::BG_ILLITERATE;
    progression(Unlockable::BACKGROUND, "merchant", recon::translate("Merchant")).background = Background::BG_MERCHANT;
    progression(Unlockable::BACKGROUND, "ascetic", recon::translate("Ascetic")).background = Background::BG_ASCETIC;
    progression(Unlockable::BACKGROUND, "farsighted", recon::translate("Farsighted")).background = Background::BG_FARSIGHTED;
    progression(Unlockable::BACKGROUND, "snakeeater", recon::translate("Snake Eater")).background = Background::BG_SNAKEEATER;
    progression(Unlockable::BACKGROUND, "wanted", recon::translate("Wanted")).background = Background::BG_WANTED;
    progression(Unlockable::BACKGROUND, "potion_addict", recon::translate("Potion Addict")).background = Background::BG_POTION_ADDICT;
    progression(Unlockable::BACKGROUND, "stoic", recon::translate("Stoic")).background = Background::BG_STOIC;
    progression(Unlockable::BACKGROUND, "boomerang", recon::translate("Boomerang")).background = Background::BG_BOOMERANG;
    progression(Unlockable::CLASS, "random", recon::translate("Random Class"));
    progression(Unlockable::CLASS, "ranger", recon::translate("Houndmaster"));
    progression(Unlockable::BACKGROUND, "mimic", recon::translate("Mimic Whisperer")).background = Background::BG_MIMIC;
    progression(Unlockable::BACKGROUND, "bloody", recon::translate("Bloody Adherent")).background = Background::BG_GOD_BLOOD;
    progression(Unlockable::BACKGROUND, "smith", recon::translate("Disciple of the Smith")).background = Background::BG_GOD_SMITH;
    progression(Unlockable::BACKGROUND, "scholar", recon::translate("Disciple of the Scholar")).background = Background::BG_GOD_SPELL;
    progression(Unlockable::BACKGROUND, "shackled", recon::translate("Cursed Adherent")).background = Background::BG_GOD_SHACKLED;
    progression(Unlockable::BACKGROUND, "light", recon::translate("Acolyte of Light")).background = Background::BG_GOD_LIGHT;
    progression(Unlockable::BACKGROUND, "chaos", recon::translate("Acolyte of Chaos")).background = Background::BG_GOD_CHAOS;
    progression(Unlockable::BACKGROUND, "faithless", recon::translate("Faithless")).background = Background::BG_FAITHLESS;
    {
        const auto description = recon::translate("Defeat ???");
        const auto name = recon::translate("Trophy");
        conditional(Unlockable::SPECIAL, "boss_trophy", name, description, [] { return getValue("good_victory_count") > 0; });
    }
    {
        const auto description = recon::translate("Defeat Vine Lord");
        const auto name = recon::translate("Tinkerer");
        conditional(Unlockable::BACKGROUND, "tinkerer", name, description, [] { return getKilledCount(Entity::TYPE_BOSS_SPRIGGAN, false) > 0; }).background = Background::BG_GRAPPLING_HOOK;
    }
    {
        const auto description = recon::translate("Complete 3 floors with mortality armor.");
        const auto name = recon::translate("Death's Acolyte");
        conditional(Unlockable::BACKGROUND, "death", name, description, [] { return getValue("mortality_floors") > 2; }).background = Background::BG_DEATH;
    }
    {
        const auto description = recon::translate("Beat the game as a knight.");
        const auto name = recon::translate("Armored");
        conditional(Unlockable::BACKGROUND, "knight_armored", name, description, [] { return getValue("Knight_victory_count") > 0; }).background = Background::BG_KNIGHT_ARMORED;
    }
    {
        const auto description = recon::translate("Beat the game as a rogue.");
        const auto name = recon::translate("Crossbowman");
        conditional(Unlockable::BACKGROUND, "rogue_boomerang", name, description, [] { return getValue("Rogue_victory_count") > 0; }).background = Background::BG_ROGUE_CROSSBOW;
    }
    {
        const auto description = recon::translate("Beat the game as a mage.");
        const auto name = recon::translate("Heirloom");
        conditional(Unlockable::BACKGROUND, "mage_heirloom", name, description, [] { return getValue("Mage_victory_count") > 0; }).background = Background::BG_MAGE_HEIRLOOM;
    }
    {
        const auto description = recon::translate("Beat the game as a wildling.");
        const auto name = recon::translate("Will of the Elements");
        conditional(Unlockable::BACKGROUND, "wildling_weapon", name, description, [] { return getValue("Wildling_victory_count") > 0; }).background = Background::BG_WILDLING_WEAPON;
    }
    {
        const auto description = recon::translate("Beat the game as a Hound Master.");
        const auto name = recon::translate("Flail Wielder");
        conditional(Unlockable::BACKGROUND, "houndmaster_flail", name, description, [] { return getValue("Ranger_victory_count") > 0; }).background = Background::BG_FLAIL;
    }
    {
        const auto description = recon::translate("Kill all Cave bosses.");
        const auto name = recon::translate("Alternate Cave Theme");
        conditional(Unlockable::MUSIC, "music_cave", name, description, [] { return getKilledCount(Entity::TYPE_BOSS_GOBLIN, false) > 0 && getKilledCount(Entity::TYPE_BOSS_DRAGON, false) > 0 && getKilledCount(Entity::TYPE_BOSS_WORM, false) > 0; });
    }
    {
        const auto description = recon::translate("Kill all Forest bosses.");
        const auto name = recon::translate("Alternate Forest Theme");
        conditional(Unlockable::MUSIC, "music_forest", name, description, [] { return getKilledCount(Entity::TYPE_BOSS_SPRIGGAN, false) > 0 && getKilledCount(Entity::TYPE_BOSS_WISP, false) > 0 && getKilledCount(Entity::TYPE_BOSS_GOLEM, false) > 0; });
    }
    {
        const auto description = recon::translate("Kill all Catacombs bosses.");
        const auto name = recon::translate("Alternate Catacombs Theme");
        conditional(Unlockable::MUSIC, "music_catacombs", name, description, [] { return getKilledCount(Entity::TYPE_BOSS_ABOMINATION, false) > 0 && getKilledCount(Entity::TYPE_BOSS_ZOMBIE_DRAGON, false) > 0 && getKilledCount(Entity::TYPE_BOSS_NECROMANCER, false) > 0; });
    }
    {
        const auto description = recon::translate("Kill all Rift bosses.");
        const auto name = recon::translate("Alternate Rift Theme");
        conditional(Unlockable::MUSIC, "music_rift", name, description, [] { return getKilledCount(Entity::TYPE_BOSS_BROOD_DEMON, false) > 0 && getKilledCount(Entity::TYPE_BOSS_BEHEMOTH, false) > 0 && getKilledCount(Entity::TYPE_BOSS_SHADOW_KNIGHT, false) > 0; });
    }
    {
        const auto description = recon::translate("Beat the game on normal difficulty.");
        const auto name = recon::translate("Hard Mode");
        conditional(Unlockable::GAME_MODE, "hard_mode", name, description, [] { return getValue("victory_count") > 0; });
    }
    const bool loaded = load();
    updateUnlocks();
    // Steam Achievements construction/check are excluded from this port.
    return loaded;
}
