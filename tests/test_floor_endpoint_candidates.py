from copy import deepcopy
import unittest

from tools.floor_endpoint_candidates import reachable_above_ground_candidates


def tiles_from_types(rows):
    return [[[kind, -1, -1, -1, -1, -1] for kind in row] for row in rows]


def search(rows, spec=None, room=(0, 0)):
    tiles = tiles_from_types(rows)
    if spec is None:
        spec = dict(width=len(rows[0]), height=len(rows))
    return reachable_above_ground_candidates(tiles, spec, room)


class EndpointCandidateTests(unittest.TestCase):
    def test_seed_scan_bottom_to_top_then_left_and_lexicographic_results(self):
        result = search([[0, 0, 0, 0], [0, 0, 0, 0], [1, 6, 7, 1]])
        self.assertEqual(result['seed_tile'], [0, 1])
        self.assertEqual(result['candidates'], [[0, 1], [1, 1], [2, 1], [3, 1]])
        self.assertFalse(result['shuffled'])
        self.assertFalse(result['water_checked'])
        self.assertFalse(result['character_movement_reachability_verified'])

    def test_platform_current_tile_can_be_candidate(self):
        result = search([[0], [6], [1]])
        self.assertEqual(result['seed_tile'], [0, 1])
        self.assertEqual(result['candidates'], [[0, 0], [0, 1]])

    def test_search_can_descend_through_platform_tiles(self):
        rows = [[1, 1, 1], [1, 1, 1], [0, 0, 1],
                [0, 0, 6], [0, 1, 0], [1, 1, 1]]
        result = search(rows)
        self.assertEqual(result['seed_tile'], [0, 4])
        # The low right cell is reached by dropping through the platform.
        self.assertIn([2, 4], result['candidates'])
        rows[3][2] = 7
        self.assertNotIn([2, 4], search(rows)['candidates'])

    def test_solid_footing_can_reach_lateral_at_three_tiles_above(self):
        rows = [[1, 0, 0], [1, 0, 1], [1, 0, 0], [1, 0, 0], [1, 1, 1]]
        result = search(rows)
        self.assertEqual(result['seed_tile'], [1, 3])
        self.assertIn([2, 0], result['candidates'])
        # The same last-layer neighbor is rejected with a platform underneath.
        rows[1][2] = 6
        result = search(rows)
        self.assertNotIn([2, 0], result['candidates'])

    def test_ladder_below_allows_ascent_after_leaving_solid_footing(self):
        rows = [[1, 0, 0, 0], [1, 0, 1, 1], [0, 0, 1, 1],
                [0, 3, 1, 1], [1, 0, 1, 1]]
        result = search(rows)
        self.assertEqual(result['seed_tile'], [0, 3])
        self.assertIn([2, 0], result['candidates'])
        rows[3][1] = 0
        self.assertNotIn([2, 0], search(rows)['candidates'])

    def test_last_ascent_layer_cannot_enqueue_the_center_tile(self):
        result = search([[1, 0], [1, 6], [1, 0], [0, 3], [1, 1]])
        self.assertEqual(result['seed_tile'], [0, 3])
        self.assertNotIn([1, 0], result['candidates'])
        self.assertEqual(result['candidates'], [[0, 3], [1, 3]])

    def test_solid_center_ceiling_stops_ascent_even_when_lateral_is_empty(self):
        result = search([[0, 0, 0], [0, 1, 0], [1, 0, 1], [1, 1, 1]])
        self.assertEqual(result['seed_tile'], [1, 2])
        self.assertEqual(result['candidates'], [[1, 2]])

    def test_search_is_confined_to_room_in_larger_level(self):
        result = search([[0] * 6, [0] * 6, [1] * 6],
                        dict(width=3, height=3), room=(1, 0))
        self.assertEqual(result['room_bounds'], [3, 0, 5, 2])
        self.assertEqual(result['candidates'], [[3, 1], [4, 1], [5, 1]])

    def test_outside_level_is_not_footing_and_no_seed_means_no_candidates(self):
        self.assertEqual(search([[0] * 3] * 3)['candidates'], [])
        self.assertIsNone(search([[1] * 3] * 3)['seed_tile'])
        self.assertEqual(search([[1] * 3] * 3)['visited_tiles'], 0)

    def test_source_tiles_preserved_and_malformed_inputs_rejected(self):
        tiles = tiles_from_types([[0, 0], [1, 1]])
        original = deepcopy(tiles)
        reachable_above_ground_candidates(tiles, dict(width=2, height=2), (0, 0))
        self.assertEqual(tiles, original)
        for invalid in ([], [[]], [tiles[0], []], [[[0]]]):
            with self.assertRaises(ValueError):
                reachable_above_ground_candidates(invalid, dict(width=2, height=2), (0, 0))
        for spec, room in ((dict(width=0, height=2), (0, 0)),
                           (dict(width=2, height=2), (1, 0)),
                           (dict(width=2, height=2), (-1, 0)),
                           (dict(width=2, height=2), (True, 0))):
            with self.assertRaises(ValueError):
                reachable_above_ground_candidates(tiles, spec, room)


if __name__ == '__main__':
    unittest.main()
