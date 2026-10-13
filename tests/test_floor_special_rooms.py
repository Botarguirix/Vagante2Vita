import unittest
from tools.floor_special_rooms import (secret_neighbors, place_secrets, secret_stage,
                                       book_room_bases, place_pedestals, special_graph, BOOK)
from test_generation_random import RawStream


class SpecialRoomTests(unittest.TestCase):
    def test_secret_excludes_specials_direction_and_second_act_bright_rooms(self):
        self.assertEqual(secret_neighbors([[2, 0, 2]], 1, 0), [(0, 0), (2, 0)])
        self.assertEqual(secret_neighbors([[4, 0, 1]], 1, 0), [])
        for flag in (0x100, 0x200, BOOK):
            self.assertEqual(secret_neighbors([[2 | flag, 0, 2 | flag]], 1, 0), [])
        self.assertEqual(secret_neighbors([[0x402, 0]], 1, 0, act_index=0), [(0, 0)])
        self.assertEqual(secret_neighbors([[0x402, 0]], 1, 0, act_index=1), [])
        self.assertEqual(secret_neighbors([[2, 1, 2]], 1, 0), [])

    def test_zero_secret_branch_still_consumes_integer_and_float(self):
        rng = RawStream(1, 0)
        result = secret_stage([[2, 0]], rng)
        self.assertEqual(result['requested_before_chance'], 2)
        self.assertEqual(result['requested'], 0)
        self.assertEqual(result['placements'], [])
        self.assertEqual(rng.draws, 2)
        rng = RawStream()
        self.assertEqual(place_secrets([[2, 0]], 0, rng)['candidates_initial'], 0)
        self.assertEqual(rng.draws, 0)

    def test_chance_threshold_uses_float32_boundary_and_double_comparison(self):
        for raw, count in ((1417339200, 0), (1417339201, 1)):
            rng = RawStream(0, raw)
            self.assertEqual(secret_stage([[2, 0]], rng)['requested'], count)
            self.assertEqual(rng.draws, 2)

    def test_cached_candidates_recompute_neighbors_after_first_secret(self):
        types = [[2, 0, 0, 2]]
        rng = RawStream(1, 1)
        result = place_secrets(types, 2, rng)
        self.assertEqual(result['candidates_initial'], 2)
        self.assertEqual([p['neighbor'] for p in result['placements']], [[0, 0], [1, 0]])
        self.assertEqual(types, [[6, 5, 1, 2]])
        self.assertEqual(rng.draws, 2)

    def test_empty_secret_catalog_and_shortage_use_available_candidates(self):
        rng = RawStream()
        self.assertEqual(place_secrets([[0, 0]], 2, rng)['placements'], [])
        self.assertEqual(len(place_secrets([[2, 0]], 2, rng)['placements']), 1)
        self.assertEqual(rng.draws, 0)

    def test_pedestal_catalog_mask_and_empty_catalog_no_draw(self):
        self.assertEqual(book_room_bases({'rooms': {'16386': [], '4': [], '20485': []}}), {2, 4101})
        rng = RawStream()
        self.assertEqual(place_pedestals([[0, 2]], set(), (0, 0), (1, 0), rng)['placements'], [])
        self.assertEqual(rng.draws, 0)

    def test_pedestal_existing_room_skips_start_goal_and_shop(self):
        types = [[1, 0x103, 4, 2]]
        rng = RawStream(1, 2, 3)
        r = place_pedestals(types, {1, 0x103, 4, 2}, (0, 0), (3, 0), rng)
        self.assertEqual(r['placements'][0]['x'], 2)
        self.assertEqual(r['placements'][0]['mode'], 'existing_room')
        self.assertEqual(types[0][2], BOOK | 4)
        self.assertEqual(r['cells_examined'], 3)
        self.assertEqual(rng.draws, 3)

    def test_pedestal_literal_catalog_tests_for_new_left_and_right_rooms(self):
        types = [[0, 2]]
        r = place_pedestals(types, {2}, (-1, -1), (1, 0), RawStream(1))
        self.assertEqual(r['placements'][0]['neighbor'], [1, 0])
        self.assertEqual(types, [[BOOK | 4, 3]])
        r = place_pedestals([[0, 2]], {4}, (-1, -1), (1, 0), RawStream(1))
        self.assertEqual(r['placements'], [])
        types = [[2, 0]]
        r = place_pedestals(types, {0}, (0, 0), (-1, -1), RawStream(1))
        self.assertEqual(types, [[6, BOOK | 1]])
        self.assertEqual(r['placements'][0]['neighbor'], [0, 0])
        r = place_pedestals([[2, 0]], {1}, (0, 0), (-1, -1), RawStream(1))
        self.assertEqual(r['placements'], [])

    def test_nonempty_pedestal_catalog_shuffles_even_when_request_zero(self):
        rng = RawStream(1, 2)
        r = place_pedestals([[0, 0, 2]], {2}, (0, 0), (2, 0), rng, requested=0)
        self.assertEqual(rng.draws, 2)
        self.assertEqual(r['cells_examined'], 0)

    def test_complete_special_graph_keeps_reciprocal_links_and_rng_accounting(self):
        act = {'rooms': {str(BOOK | b): [] for b in (1, 2, 3, 4, 5, 12)}}
        for floor in range(3):
            for seed in range(30):
                r = special_graph(seed, floor, act)
                self.assertEqual(r['rng_draws'] - r['pre_pedestal_rng_draws'], 39)
                self.assertEqual(len(r['pedestals']['placements']), 1)
                self.assertFalse(r['floor_complete'])
                types = r['room_types_row_major']
                for y, row in enumerate(types):
                    for x, kind in enumerate(row):
                        for dx, dy, bit, opposite in ((-1, 0, 1, 4), (1, 0, 4, 1),
                                                       (0, -1, 2, 8), (0, 1, 8, 2)):
                            if kind & bit:
                                nx, ny = x + dx, y + dy
                                self.assertTrue(0 <= nx < 8 and 0 <= ny < 5)
                                self.assertTrue(types[ny][nx] & opposite)


if __name__ == '__main__':
    unittest.main()
