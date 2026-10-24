#include "port_lib_types.h"
#define private public
#include "ItemAttribute.h"
#undef private
#include "recon_all.h"
#include "localization.h"
#include <cassert>
#include <iostream>

int main() {
    _randEngine.seed(91);
    const auto rngBefore = _randEngine;
    Entity::initEntityStringIdentifiers();
    ItemAttribute::initAttributeStrings();
    Skill::initSkillStringIdentifiers();
    assert(Entity::entityTypeToString.size() == 123 && Entity::stringToEntityType.size() == 123);
    assert(ItemAttribute::attributeTypeToString.size() == 101 && ItemAttribute::stringToAttributeType.size() == 101);
    assert(Skill::skillTypeToString.size() == 27 && Skill::stringToSkillType.size() == 27);
    for (const auto &entry : Entity::entityTypeToString) assert(Entity::stringToEntityType.at(entry.second) == entry.first);
    for (int i = 0; i <= ItemAttribute::NUM_ATTRIBUTE_TYPES; ++i) {
        const auto type = static_cast<ItemAttribute::AttributeType>(i);
        const auto &name = ItemAttribute::attributeTypeToString.at(type);
        assert(ItemAttribute::stringToAttributeType.at(name) == type);
    }
    for (const auto &entry : Skill::skillTypeToString) assert(Skill::stringToSkillType.at(entry.second) == entry.first);
    assert(Entity::entityTypeToString.at(Entity::TYPE_PLAYER) == "Player");
    assert(Entity::entityTypeToString.at(Entity::TYPE_SHOP_KEEPER) == "ShopKeeper");
    assert(Entity::entityTypeToString.at(Entity::TYPE_PLAYER_MOD) == "PlayerMod");
    assert(ItemAttribute::attributeTypeToString.at(ItemAttribute::ATTRIBUTE_PRIMARY_BEGIN) == "ATTRIBUTE_PRIMARY_BEGIN");
    assert(ItemAttribute::attributeTypeToString.at(ItemAttribute::ATTRIBUTE_WEAPON_POISON) == "ATTRIBUTE_WEAPON_POISON");
    assert(Skill::skillTypeToString.at(Skill::ICE_BOLT) == "Iceball");
    assert(Skill::skillTypeToString.at(Skill::TELEPORT) == "Blink");
    assert(Skill::skillTypeToString.at(Skill::HEAL) == "Circle of Light");
    assert(Skill::skillTypeToString.at(Skill::DOG_WHISTLE) == "Whistle");
    const auto before = std::locale();
    auto *catalog = new recon::MessageCatalog("nonexistent-fixture.mo");
    catalog->messages["Dash"] = "Prueba";
    std::locale::global(std::locale(before, catalog));
    Skill::initSkillStringIdentifiers();
    assert(Skill::skillTypeToString.at(Skill::DASH) == "Prueba");
    assert(Skill::stringToSkillType.at("Prueba") == Skill::DASH);
    assert(Skill::stringToSkillType.at("Dash") == Skill::DASH); // Original retains old reverse entries.
    std::locale::global(before);
    Entity::entityTypeToString[static_cast<Entity::EntityType>(255)] = "fixture-extension";
    Entity::initEntityStringIdentifiers();
    assert(Entity::stringToEntityType.at("fixture-extension") == static_cast<Entity::EntityType>(255));
    ItemAttribute::initAttributeStrings();
    assert(ItemAttribute::attributeTypeToString.size() == 101);
    assert(_randEngine == rngBefore);
    std::cout << "Strings: 123 entities (124 assignments), 101 attributes, 27 skills; reverse mappings, sentinels, localization and RNG passed\n";
}
