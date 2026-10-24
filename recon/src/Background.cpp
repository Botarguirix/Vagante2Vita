// Reconstructed from Background (004f97b0-004fbf68), PDB layouts.
#include "recon_all.h"
#include "localization.h"

Background::Background(BackgroundType value) : type(value) { updateNameAndDescription(); }
Background::~Background() = default;

void Background::updateNameAndDescription() {
    positive.clear();
    negative.clear();
    // Original local lambdas 004fbe90 (positive), 004fbf00 (negative).
    const auto good = [this](std::string text) { positive.push_back(std::move(text)); };
    const auto bad = [this](std::string text) { negative.push_back(std::move(text)); };
    switch (type) {
    case BG_NONE:
        name = recon::translate("No Background");
        description = recon::translate("There's nothing special about you.");
        break;
    case BG_STRONG:
        name = recon::translate("Strong");
        description = recon::translate("You might not be the sharpest tool in the shed, but you're stronger than most.");
        good(recon::translate("Heavy throws"));
        good(recon::translate("+2 STR"));
        bad(recon::translate("-2 INT"));
        break;
    case BG_EXPERIENCED:
        name = recon::translate("Experienced");
        description = recon::translate("You're older and more experienced than the average adventurer, but you've used up most of your beginner's luck.");
        good(recon::translate("+1 Starting Level"));
        bad(recon::translate("-3 LUK"));
        break;
    case BG_BOOKWORM:
        name = recon::translate("Bookworm");
        description = recon::translate("Due to your sickly nature, you've spent most of your life indoors reading books.");
        good(recon::translate("+1 Spellbook Starting Level"));
        bad(recon::translate("-1 STR"));
        bad(recon::translate("-1 DEX"));
        break;
    case BG_SURVIVOR:
        name = recon::translate("Survivor");
        description = recon::translate("Years of living in the harsh wilderness have left you with impressive survival skills.");
        good(recon::translate("+5 Bonfire Healing"));
        break;
    case BG_ILLITERATE:
        name = recon::translate("Illiterate");
        description = recon::translate("Instead of going to school and learning how to read, you spent your time tinkering with explosives.");
        good(recon::translate("+5 Bombs"));
        good(recon::translate("+Charred Helmet"));
        bad(recon::translate("Cannot read scrolls or books"));
        break;
    case BG_MERCHANT:
        name = recon::translate("Raised by Merchant");
        description = recon::translate("Growing up as a merchant's child has taught you how to bargain effectively.");
        good(recon::translate("+25 Starting Gold"));
        good(recon::translate("Discount at Shops"));
        break;
    case BG_WANTED:
        name = recon::translate("Wanted");
        description = recon::translate("You were caught stealing something you shouldn't have. Now, you have a price on your head.");
        good(recon::translate("+Item: Stolen Ring"));
        bad(recon::translate("Shopkeepers attack on sight"));
        break;
    case BG_FARSIGHTED:
        name = recon::translate("Farsighted");
        description = recon::translate("You have exceptional long-ranged vision which lets you aim much better with ranged weapons.");
        good(recon::translate("+1 Light Radius"));
        good(recon::translate("Increased Ranged Damage"));
        break;
    case BG_ASCETIC:
        name = recon::translate("Ascetic");
        description = recon::translate("You've taken an ascetic vow. Wearing equipment will break your holy vow.");
        good(recon::translate("Increased stats for each floor beaten without equipment"));
        bad(recon::translate("Breaking holy vow is permanent"));
        break;
    case BG_SNAKEEATER:
        name = recon::translate("Snake Eater");
        description = recon::translate("Years of ingesting poisons to train your body has changed your metabolism.");
        good(recon::translate("+ Poison Potions"));
        good(recon::translate("+1 POISRES"));
        good(recon::translate("Poison Potions heal"));
        bad(recon::translate("Healing Potion effect reduced"));
        break;
    case BG_NIMBLE:
        name = recon::translate("Nimble");
        description = recon::translate("You're generally quicker and nimbler than normal.");
        good(recon::translate("+2 SPD"));
        good(recon::translate("Reduced fall damage"));
        break;
    case BG_WISPCURSE:
        name = recon::translate("Wisp Curse");
        description = recon::translate("They've followed you for most of your life, but you've learned to deal with it.");
        break;
    case BG_SYNCHRONIZED:
        name = recon::translate("Synchronized");
        description = recon::translate("The world moves in step with you.");
        break;
    case BG_DEATH:
        name = recon::translate("Death's Acolyte");
        description = recon::translate("Two tattoos of demonic dragons etched into your skin fill your body with deadly strength, at great cost. The souls of mortals tremble from your presence.");
        good(recon::translate("Death is easier to encounter"));
        bad(recon::translate("Death is easier to encounter"));
        break;
    case BG_MAGE_HEIRLOOM:
        name = recon::translate("Heirloom");
        description = recon::translate("You're not sure how old this ring is, but it's been passed down within the family for generations.");
        good(recon::translate("+Item: Heirloom Ring"));
        break;
    case BG_ROGUE_CROSSBOW:
        name = recon::translate("Crossbow");
        description = recon::translate("Instead of spending your hard-earned money on a regular bow, you decided to invest in an enchanted crossbow.");
        good(recon::translate("+Item: Enchanted Crossbow"));
        bad(recon::translate("-Item: Short Bow"));
        break;
    case BG_KNIGHT_ARMORED:
        name = recon::translate("Armored");
        description = recon::translate("You used your inheritance to purchase a full plate of the finest armor you could buy.");
        good(recon::translate("+Item: Full Plate Armor"));
        break;
    case BG_WILDLING_WEAPON:
        name = recon::translate("Will of the Elements");
        description = recon::translate("Start with a caestus blessed by the elements.");
        good(recon::translate("+Item: Elemental Caestus"));
        break;
    case BG_FLAIL:
        name = recon::translate("Flail Wielder");
        description = recon::translate("You're not a fan of conventional weapons.");
        good(recon::translate("+Item: Heavy Flail"));
        bad(recon::translate("-Item: Wooden Club"));
        break;
    case BG_POTION_ADDICT:
        name = recon::translate("Potion Addict");
        description = recon::translate("You *really* love drinking potions.");
        good(recon::translate("Heal +4 when drinking any potion"));
        break;
    case BG_STOIC:
        name = recon::translate("Stoic");
        description = recon::translate("With your well-disciplined body, you can shrug off damage quicker than others.");
        good(recon::translate("Status Effect Duration -33%"));
        good(recon::translate("Reduced received knockback"));
        break;
    case BG_GRAPPLING_HOOK:
        name = recon::translate("Tinkerer");
        description = recon::translate("You've compensated for your bad leg by crafting something to help out your mobility.");
        good(recon::translate("+Item: Grappling Hook"));
        bad(recon::translate("-1 SPD"));
        break;
    case BG_BOOMERANG:
        name = recon::translate("Boomerang");
        description = recon::translate("You decided to use your spare time on the journey here carve a pair of throwing sticks.");
        good(recon::translate("+Item: Boomerangs"));
        break;
    case BG_MIMIC:
        name = recon::translate("Mimic Whisperer");
        description = recon::translate("You've learned the secret behind the origin of mimics.");
        break;
    case BG_GOD_BLOOD:
        name = recon::translate("Bloody Adherent");
        description = recon::translate("Followers of The God of Blood and War are most holy when in the thick of combat.");
        good(recon::translate("Heal in combat when low on health"));
        break;
    case BG_GOD_SMITH:
        name = recon::translate("Disciple of the Smith");
        description = recon::translate("The Smith looks favorably upon devotees that take care of their equipment.");
        good(recon::translate("Bonfires have a chance to grant stacking bonus to equipped items until unequipped"));
        break;
    case BG_GOD_SPELL:
        name = recon::translate("Disciple of the Scholar");
        description = recon::translate("Followers of The Scholar strive to comprehend the language of magic.");
        good(recon::translate("Sacrifices to the Scholar are rewarded more."));
        break;
    case BG_GOD_SHACKLED:
        name = recon::translate("Cursed Adherent");
        description = recon::translate("Adherents to the Shackled are blessed with more opportunities to embrace cursedness.");
        good(recon::translate("Greatly increased chance of cursed items from chests"));
        break;
    case BG_GOD_LIGHT:
        name = recon::translate("Acolyte of Light");
        description = recon::translate("Acolytes of Light can heal their allies through the use of holy light.");
        good(recon::translate("Ability: Circle of Light"));
        break;
    case BG_GOD_CHAOS:
        name = recon::translate("Acolyte of Chaos");
        description = recon::translate("The Deranged finds you interesting.");
        good("+???");
        bad("+???");
        break;
    case BG_FAITHLESS:
        name = recon::translate("Faithless Heathen");
        description = recon::translate("You reject the gods and they reject you.");
        good(recon::translate("Gain stats from destroying shrines"));
        bad(recon::translate("Very Unlucky"));
        break;
    default:
        name = "Undefined Background";
        description = "Undefined Description";
    }
}
