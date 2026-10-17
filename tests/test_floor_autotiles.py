import unittest
from tools.floor_autotiles import auto_tiles, update_display_indices, tile_bank, remainder


def blank():
    return [[[0, -1, -1, -1, -1, -1] for x in range(5)] for y in range(5)]


class AutotileTests(unittest.TestCase):
    def test_negative_midground_remainder_matches_native_solid_branch(self):
        self.assertEqual(remainder(-1, 32), -1)
        self.assertEqual(tile_bank(-1), -1)
        tiles = blank()
        tiles[2][2][0] = 1
        auto_tiles(tiles)
        self.assertEqual(tiles[2][2][3], 38)  # -1 + all four edges + 24.

    def test_regular_family_faces_and_vegetation_overlap_order(self):
        tiles = blank()
        tiles[2][2][1] = 17
        tiles[2][2][2] = 784
        tiles[2][1][2] = 720
        tiles[2][3][2] = 848
        auto_tiles(tiles)
        self.assertEqual(tiles[2][2][1], 55)
        self.assertEqual(tiles[2][2][2], 822)  # Only left lower family suppressed.

    def test_local_refresh_pads_one_tile_and_preserves_types(self):
        tiles = blank()
        for row in tiles:
            for tile in row: tile[1] = 17
        tiles[2][2][0] = 11
        tiles[2][2][4] = 104
        update_display_indices(tiles, 2, 2, 2, 2)
        self.assertEqual(tiles[2][2][4], -1)
        self.assertEqual(tiles[2][2][0], 11)
        self.assertEqual(tiles[0][0][1], 17)
        self.assertEqual(tiles[1][1][1], 16)


if __name__ == '__main__':
    unittest.main()
