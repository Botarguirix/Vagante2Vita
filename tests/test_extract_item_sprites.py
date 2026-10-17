import unittest

from tools.extract_item_sprites import (atlas_for, parse_sprite_indices,
                                        parse_unique_types)


class ExtractItemSpritesTests(unittest.TestCase):
    def test_maps_shared_case_and_reports_default_index_cases(self):
        pseudo = "\n".join([
            "  case ITEMTYPE_SWORD:",
            "  case ITEMTYPE_SWORD2:",
            "    this->spriteIndex = 0x0c;",
            "    break;",
            "  case ITEMTYPE_POTION_HEAL:",
            "    break;",
        ])
        indexes, defaults = parse_sprite_indices(pseudo)
        self.assertEqual(indexes, {"ITEMTYPE_SWORD": 12, "ITEMTYPE_SWORD2": 12})
        self.assertEqual(defaults, ["ITEMTYPE_POTION_HEAL"])

    def test_atlas_uses_weapon_predicate_and_category_assets(self):
        self.assertEqual(atlas_for("ITEMTYPE_FLAIL", 62), "items/items-weapons.png")
        self.assertEqual(atlas_for("ITEMTYPE_AMULET", 101), "items/items-armor.png")
        self.assertEqual(atlas_for("ITEMTYPE_POTION_HEAL", 6), "items/potions.png")
        self.assertEqual(atlas_for("ITEMTYPE_SCROLL_REVEAL", 34), "items/scrolls.png")
        self.assertEqual(atlas_for("ITEMTYPE_SPELL_BOOK", 216), "items/spell_books.png")

    def test_unique_types_are_scoped_to_their_switch_case(self):
        pseudo = "\n".join([
            "case ITEMTYPE_RING_ZYGOX:",
            "    this->unique = true;",
            "    break;",
            "case ITEMTYPE_POTION_HEAL:",
            "    this->unique = false;",
            "    break;",
        ])
        self.assertEqual(parse_unique_types(pseudo), {"ITEMTYPE_RING_ZYGOX"})


if __name__ == "__main__":
    unittest.main()
