import unittest

from tools.floor_tile_path import orthogonal_tile_path


def tiles(width, height, tile_type=0):
    return [[[tile_type, -1, -1, -1, -1, -1] for _ in range(width)]
            for _ in range(height)]


class OrthogonalTilePathTests(unittest.TestCase):
    def test_matches_original_ordered_set_tie_break(self):
        result = orthogonal_tile_path(tiles(3, 3), [0, 0], [2, 2])
        self.assertTrue(result['reachable'])
        self.assertEqual(result['path'], [[0, 0], [0, 1], [0, 2], [1, 2], [2, 2]])
        self.assertFalse(result['models_player_physics'])

    def test_solids_block_but_platforms_are_walkable_to_native_astar(self):
        grid = tiles(5, 3)
        for y in range(3):
            grid[y][2][0] = 7
        grid[1][2][0] = 6
        result = orthogonal_tile_path(grid, [0, 1], [4, 1])
        self.assertTrue(result['reachable'])
        self.assertEqual(result['path'], [[0, 1], [1, 1], [2, 1], [3, 1], [4, 1]])
        grid[1][2][0] = 1
        self.assertFalse(orthogonal_tile_path(grid, [0, 1], [4, 1])['reachable'])

    def test_disconnected_goal_and_native_visit_cap(self):
        grid = tiles(3, 3)
        for y in range(3):
            grid[y][1][0] = 1
        self.assertFalse(orthogonal_tile_path(grid, [0, 1], [2, 1])['reachable'])
        limited = orthogonal_tile_path(tiles(10, 10), [0, 0], [9, 9], visit_limit=1)
        self.assertFalse(limited['reachable'])
        self.assertTrue(limited['limit_reached'])
        self.assertEqual(limited['visited_tiles'], 1)

    def test_clamps_endpoints_and_allows_original_solid_seed_semantics(self):
        grid = tiles(2, 1)
        grid[0][0][0] = 1
        result = orthogonal_tile_path(grid, [-4, 0], [1, 0])
        self.assertEqual(result['source'], [0, 0])
        self.assertTrue(result['reachable'])
        self.assertEqual(result['path'], [[0, 0], [1, 0]])

    def test_rejects_malformed_inputs(self):
        with self.assertRaises(ValueError):
            orthogonal_tile_path([], [0, 0], [0, 0])
        with self.assertRaises(ValueError):
            orthogonal_tile_path(tiles(1, 1), [0.0, 0], [0, 0])
        with self.assertRaises(ValueError):
            orthogonal_tile_path(tiles(1, 1), [0, 0], [0, 0], visit_limit=0)


if __name__ == '__main__':
    unittest.main()
