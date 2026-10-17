import unittest
from tools.floor_secret_geometry import nearest_solid, remove_mushrooms, seal_geometry
from test_generation_random import RawStream


def blank(width=9, height=6):
    return [[[0, -1, -1, -1, -1, -1] for x in range(width)] for y in range(height)]


class SecretGeometryTests(unittest.TestCase):
    def test_bfs_cardinal_tie_order_metal_and_distance_cap(self):
        tiles = blank()
        for x, y in ((4, 1), (3, 2), (5, 2), (4, 3)):
            tiles[y][x][0] = 1
        self.assertEqual(nearest_solid(tiles, 4, 2), [4, 1])
        self.assertEqual(nearest_solid(tiles, 4, 2, excluded={(4, 1)}), [3, 2])
        self.assertIsNone(nearest_solid(tiles, 4, 2, max_distance=0))
        tiles[1][4][0] = 7
        self.assertEqual(nearest_solid(tiles, 4, 2), [4, 1])
        with self.assertRaises(ValueError): nearest_solid(tiles, 4, 2, max_distance=-1)

    def test_bfs_crosses_platform_and_clamps_start(self):
        tiles = blank()
        tiles[2][4][0] = 6
        tiles[2][5][0] = 7
        self.assertEqual(nearest_solid(tiles, 4, 2), [5, 2])
        tiles[0][0][0] = 1
        self.assertEqual(nearest_solid(tiles, -1, -1), [0, 0])
        self.assertIsNone(nearest_solid(blank(), 4, 2))

    def test_seal_two_column_strip_skips_solid_platform_and_preserves_other_layers(self):
        tiles = blank()
        for y in range(6):
            tiles[y][5] = [1, 10, 20, 30, 40, 50]
        tiles[1][7][0] = 6
        tiles[2][7] = [1, -1, -1, 30, -1, -1]
        tiles[3][6] = [9, 11, 21, -1, 41, 51]
        r = seal_geometry(tiles, [[0, 0, 1]], {'width': 3, 'height': 6}, [{'x': 2, 'y': 0}])
        self.assertEqual([w['tile'] for w in r['walls']],
                         [[6, y] for y in range(6)] + [[7, y] for y in (0, 3, 4, 5)])
        self.assertEqual(tiles[3][6], [1, 11, 21, 30, 41, 51])
        self.assertEqual(tiles[1][7][0], 6)
        self.assertEqual(tiles[2][7][0], 1)
        self.assertEqual(tiles[3][8][0], 0)
        self.assertFalse(r['wall_actors_instantiated'])
        self.assertFalse(r['wall_add_rng_consumed'])

    def test_seal_vertical_strip_and_overlapping_sides_only_add_one_wall(self):
        tiles = blank(6, 6)
        for y in range(6):
            tiles[y][0][0] = 1
        r = seal_geometry(tiles, [[3]], {'width': 6, 'height': 6}, [{'x': 0, 'y': 0}])
        coords = [tuple(w['tile']) for w in r['walls']]
        self.assertEqual(len(coords), len(set(coords)))
        self.assertIn((5, 0), coords)
        self.assertIn((1, 5), coords)
        self.assertNotIn((5, 5), coords)

    def test_mushroom_groups_both_layers_and_single_graphics(self):
        tiles = blank()
        tiles[2][3][4] = 101
        tiles[2][3][5] = 0x126
        r = remove_mushrooms(tiles, 3, 2)
        self.assertEqual(len(r), 4)
        self.assertEqual([tiles[2][x][4] for x in (2, 3, 4)], [-1, -1, -1])
        self.assertEqual(tiles[2][3][5], -1)
        tiles[0][3][4] = 100
        self.assertEqual(remove_mushrooms(tiles, 3, 0), [])

    def test_closeground_mushroom_shapes_differ_from_foreground(self):
        cases = {0x102: {(3, 2), (2, 3), (3, 3), (4, 3)},
                 0x121: {(4, 1), (3, 2), (4, 2), (5, 2)},
                 0x122: {(3, 1), (2, 2), (3, 2), (4, 2)},
                 0x123: {(2, 1), (1, 2), (2, 2), (3, 2)}}
        for value, expected in cases.items():
            tiles = blank()
            tiles[2][3][5] = value
            removed = remove_mushrooms(tiles, 3, 2)
            self.assertEqual({(r['x'], r['y']) for r in removed}, expected)
            self.assertTrue(all(r['field'] == 5 for r in removed))
        tiles = blank()
        tiles[2][3][5] = 105
        self.assertEqual(remove_mushrooms(tiles, 3, 2), [])

    def test_registration_rng_is_one_game_integer_per_wall(self):
        tiles = blank(3, 3)
        for y in range(3): tiles[y][0][0] = 1
        rng = RawStream(0, 1, 2, 3, 4, 0xffffffff)
        r = seal_geometry(tiles, [[4]], {'width': 3, 'height': 3}, [{'x': 0, 'y': 0}], rng)
        self.assertEqual(rng.draws, 6)
        self.assertEqual([w['rotation_degrees'] for w in r['walls']], [0, 90, 180, 270, 0, 270])
        self.assertTrue(r['wall_add_rng_consumed'])

    def test_seal_no_links_and_missing_solid_donor_remain_explicit(self):
        tiles = blank(3, 3)
        self.assertEqual(seal_geometry(tiles, [[0]], {'width': 3, 'height': 3},
                                       [{'x': 0, 'y': 0}])['walls'], [])
        r = seal_geometry(tiles, [[4]], {'width': 3, 'height': 3}, [{'x': 0, 'y': 0}])
        self.assertEqual(len(r['missing_donors']), 6)
        self.assertTrue(all(tiles[y][x][0] == 0 for x, y in r['missing_donors']))


if __name__ == '__main__':
    unittest.main()
