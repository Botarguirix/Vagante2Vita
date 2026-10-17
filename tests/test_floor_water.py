from copy import deepcopy
import unittest

from tools.floor_main_path import MT19937
from tools.floor_water import WaterField


class RawStream:
    def __init__(self, *words):
        self.words = iter(words)
        self.draws = 0

    def raw(self):
        self.draws += 1
        return next(self.words)

    def game_int(self):
        return self.raw() & 0x7fffffff


class FloorWaterTests(unittest.TestCase):
    def test_seed_tile_expands_to_four_subcells_without_changing_tiles(self):
        tiles = [[[9, 11, 12, 13, 14, 15], [0, -1, -1, -1, -1, -1]],
                 [[1, -1, -1, -1, -1, -1], [7, -1, -1, -1, -1, -1]]]
        before = deepcopy(tiles)
        field = WaterField(tiles)
        self.assertEqual((field.width, field.height), (4, 4))
        self.assertEqual(list(field.fluid), [100, 100, 0, 0] * 2 + [0] * 8)
        self.assertEqual(sum(field.fluid), 400)
        self.assertEqual(tiles, before)
        self.assertEqual(field.solid, [[False, False], [True, True]])

    def test_in_place_horizontal_sweep_balances_same_pair_twice(self):
        field = WaterField([[0, 0]], [0] * 4 + [100, 0, 0, 0])
        rng = RawStream(1, 1, 1, 1)
        audit = field.step(rng)
        self.assertEqual(list(field.fluid), [0] * 4 + [80, 10, 5, 5])
        self.assertEqual(audit['integer_draws'], 4)
        self.assertEqual(rng.draws, 4)

    def test_gravity_is_capped_and_new_lower_mass_waits_for_next_tick(self):
        field = WaterField([[0], [0]], [0, 0, 100, 0, 0, 0, 0, 0])
        rng = RawStream(1)
        audit = field.step(rng)
        self.assertEqual(list(field.fluid), [0, 0, 80, 0, 10, 10, 0, 0])
        self.assertEqual(audit['integer_draws'], 1)
        self.assertEqual(sum(field.fluid), 100)

    def test_drained_cell_consumes_no_draw_and_row_zero_is_not_swept(self):
        field = WaterField([[0], [0]], [7, 8, 5, 0, 0, 0, 0, 0])
        audit = field.step(RawStream())
        self.assertEqual(list(field.fluid), [7, 8, 0, 0, 5, 0, 0, 0])
        self.assertEqual(audit['total_draws'], 0)

    def test_full_lower_cell_accepts_compression_and_pressure_can_rise(self):
        field = WaterField([[0], [0]], [0, 0, 0, 0, 100, 100, 100, 100])
        field.step(RawStream(1, 1, 1, 1))
        self.assertEqual(list(field.fluid), [0, 0, 0, 0, 99, 99, 102, 100])
        pressure = WaterField([[0]], [0, 0, 120, 120])
        pressure.step(RawStream(1, 1))
        self.assertEqual(list(pressure.fluid), [30, 30, 100, 80])

    def test_rng_for_bubble_position_and_two_particle_init_words(self):
        # Both bottom subcells are full, supported, and remain unchanged.
        field = WaterField([[0]], [0, 0, 100, 100])
        rng = RawStream(0, 0x80000000, 3, 4, 1)
        audit = field.step(rng)
        self.assertEqual(list(field.fluid), [0, 0, 100, 100])
        self.assertEqual(audit['bubble_candidates'], 1)
        self.assertEqual(audit['particles_spawned'], 1)
        self.assertEqual(audit['float_draws'], 1)
        self.assertEqual(audit['integer_draws'], 4)
        self.assertEqual(audit['pool_available_end'], 1023)
        self.assertEqual(audit['total_draws'], rng.draws)

    def test_float_draw_survives_disabled_particles_or_exhausted_pool(self):
        for enabled, capacity in ((False, 1024), (True, 0)):
            field = WaterField([[0]], [0, 0, 100, 100])
            rng = RawStream(0, 0xffffffff, 1)
            audit = field.step(rng, particles_enabled=enabled, pool_available=capacity)
            self.assertEqual(audit['particles_spawned'], 0)
            self.assertEqual(audit['bubble_candidates'], 1)
            self.assertEqual(rng.draws, 3)
            self.assertEqual(audit['pool_available_end'], capacity)

    def test_pool_slots_are_not_recycled_between_generation_ticks(self):
        field = WaterField([[0]], [0, 0, 100, 100])
        # First tick has one allocated bubble, next has a rejected allocation.
        rng = RawStream(0, 0, 1, 1, 1, 0, 0, 1)
        audit = field.simulate(2, rng, pool_available=1)
        self.assertEqual(audit['bubble_candidates'], 2)
        self.assertEqual(audit['particles_spawned'], 1)
        self.assertEqual(audit['pool_available_end'], 0)
        self.assertEqual(rng.draws, 8)

    def test_point_requires_supported_water_and_strict_float32_surface(self):
        field = WaterField([[0], [0]], [0, 0, 100, 0, 49, 0, 25, 0])
        self.assertFalse(field.is_point_in_water(1, 9))
        field.fluid[4] = 50
        self.assertTrue(field.is_point_in_water(1, 9))
        self.assertFalse(field.is_point_in_water(1, 8))
        self.assertFalse(field.is_point_in_water(1, 30))  # 75 == 100 - 25.
        self.assertFalse(field.is_point_in_water(1, 30.00000001))  # Rounds to 30.
        self.assertTrue(field.is_point_in_water(1, 31))
        field.fluid[6] = 101
        self.assertTrue(field.is_point_in_water(1, 24))
        self.assertFalse(field.is_point_in_water(16, 25))
        self.assertFalse(field.is_point_in_water(1, 32))
        # The executable bounds the truncated cell, not the raw pixel value.
        self.assertTrue(field.is_point_in_water(-1, 25))
        self.assertFalse(field.is_point_in_water(-8, 25))
        self.assertFalse(field.is_point_in_water(float('nan'), 25))

    def test_solid_support_does_not_need_lower_liquid(self):
        field = WaterField([[0], [7]], [0, 0, 100, 0, 0, 0, 0, 0])
        self.assertTrue(field.is_point_in_water(1, 9))
        field.step(RawStream(1, 1))
        self.assertEqual(list(field.fluid), [0, 0, 80, 20, 0, 0, 0, 0])

    def test_sixty_ticks_keep_mass_and_geometry_independent_from_rng(self):
        tiles = [[1, 1, 1, 1], [1, 9, 0, 1], [1, 0, 0, 1], [1, 1, 1, 1]]
        deterministic = WaterField(tiles)
        randomized = WaterField(tiles)
        initial_mass = sum(deterministic.fluid)
        audit_geometry = deterministic.simulate()
        rng = MT19937(12345)
        audit_rng = randomized.simulate(rng=rng)
        self.assertEqual(deterministic.fluid, randomized.fluid)
        self.assertEqual(sum(deterministic.fluid), initial_mass)
        self.assertEqual(randomized.frame, 60)
        self.assertFalse(audit_geometry['rng_consumption_reconstructed'])
        self.assertEqual(audit_rng['total_draws'], rng.draws)
        self.assertTrue(audit_rng['rng_consumption_reconstructed'])
        self.assertFalse(audit_rng['pc_runtime_parity_verified'])

    def test_invalid_inputs_and_nonpositive_tick_behavior(self):
        for tiles in ([], [[]], [[0], [0, 0]], [[True]], [[128]]):
            with self.assertRaises(ValueError): WaterField(tiles)
        with self.assertRaises(ValueError): WaterField([[0]], [0])
        field = WaterField([[9]])
        self.assertEqual(field.simulate(-1)['ticks'], 0)
        self.assertEqual(field.frame, 0)
        for args in ({'ticks': True}, {'pool_available': -1},
                     {'particles_enabled': 1}, {'rng': object()}):
            with self.assertRaises(ValueError): field.simulate(**args)


if __name__ == '__main__':
    unittest.main()
