import struct
import unittest

from tools.compile_item_loot import (DESCRIPTOR, EDGE, HEADER, MAGIC, RARITY,
                                     VERSION, compile_tables)


class CompileItemLootTests(unittest.TestCase):
    def setUp(self):
        self.rarity = {
            "format": "vagante-item-rarity-tables", "version": 1,
            "verified": True, "rarity_rows": 1,
            "pools": {"POOL_WEAPONS": [{"min_level": 0, "max_level": 10,
                       "weight": 2, "item_type": 49}]},
        }
        self.graph = {
            "format": "vagante-chest-pool-graph", "version": 1,
            "verified_structure": True, "verified_weights": True,
            "nested_pools": [{"pool": "POOL_CHEST", "pool_id": 23,
                "entries": [{"pool": "POOL_WEAPONS", "pool_id": 15, "weight": 30.0}]}],
        }
        self.sprites = {"format": "vagante-item-sprite-indexes", "verified": True,
                        "items": [{"item_type": 49, "unique": True,
                                   "atlas": "items/items-weapons.png", "sprite_index": 12}]}
        self.pool_names = {15: "POOL_WEAPONS", 23: "POOL_CHEST"}
        self.atlas_sizes = {1: (240, 240)}

    def test_encodes_verified_direct_rows_and_nested_weight(self):
        binary = compile_tables(self.rarity, self.graph, self.sprites, self.pool_names,
                                self.atlas_sizes)
        header = HEADER.unpack_from(binary)
        self.assertEqual(header, (MAGIC, VERSION, 1, 1, 1, 1))
        cursor = HEADER.size
        self.assertEqual(DESCRIPTOR.unpack_from(binary, cursor), (15, 0, 0, 1))
        cursor += DESCRIPTOR.size
        self.assertEqual(RARITY.unpack_from(binary, cursor), (0, 10, 2, 49, 1, 1, 12))
        cursor += RARITY.size
        self.assertEqual(DESCRIPTOR.unpack_from(binary, cursor), (23, 0, 0, 1))
        cursor += DESCRIPTOR.size
        self.assertEqual(EDGE.unpack_from(binary, cursor), (15, 0, 30000))
        self.assertEqual(cursor + EDGE.size, len(binary))

    def test_rejects_missing_leaf_pool(self):
        self.graph["nested_pools"][0]["entries"][0]["pool"] = "POOL_MISSING"
        with self.assertRaisesRegex(ValueError, "unresolved nested pool"):
            compile_tables(self.rarity, self.graph, self.sprites, self.pool_names)

    def test_rejects_chest_graph_without_level_eligible_items(self):
        self.rarity["pools"]["POOL_WEAPONS"][0]["max_level"] = 3
        with self.assertRaisesRegex(ValueError, "no level-eligible item at level 4"):
            compile_tables(self.rarity, self.graph, self.sprites, self.pool_names)

    def test_validates_explicit_sprite_index_against_actual_atlas_size(self):
        with self.assertRaisesRegex(ValueError, "outside atlas"):
            self.sprites["items"][0]["sprite_index"] = 144
            compile_tables(self.rarity, self.graph, self.sprites, self.pool_names,
                           self.atlas_sizes)

    def test_validates_every_used_atlas_is_available(self):
        with self.assertRaisesRegex(ValueError, "missing dimensions"):
            compile_tables(self.rarity, self.graph, self.sprites, self.pool_names, {})

    def test_accepts_real_weapon_atlas_sprite(self):
        binary = compile_tables(self.rarity, self.graph, self.sprites,
                                self.pool_names, self.atlas_sizes)
        self.assertTrue(binary.startswith(b"VLT1"))


if __name__ == "__main__":
    unittest.main()
