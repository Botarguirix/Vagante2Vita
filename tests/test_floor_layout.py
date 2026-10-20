from copy import deepcopy
import unittest
from tools.floor_layout import (assemble_tiles, close_act_one_border, solid,
                                tile_paths_to_room_edge, above_ground_candidates,
                                repair_provisional_route_support,
                                act_one_room_monster_budget, act_one_enemy_type_for_roll,
                                act_one_water_enemy_type_for_roll,
                                act_one_water_enemy_spawn_weight,
                                act_one_solid_above_type_for_roll,
                                act_one_lurker_geometry_supported,
                                act_one_shuffled_room_tiles,
                                goal_candidate_rank,
                                sort_goal_candidates_msvc_path,
                                provisional_enemies,
                                consume_spellbook_rng,
                                consume_placed_entity_rng,
                                blocked_endpoint_tiles)
from test_floor_room_selection import raw


SPEC = {'width': 3, 'height': 4}


def joined_rooms():
    left = raw('own-left', [[0, 0, 0], [1, 0, 0], [1, 1, 1], [1, 1, 1]])
    right = raw('own-right', [[0, 0, 0], [0, 0, 1], [1, 1, 1], [1, 1, 1]])
    act = {'rooms': {'4': [left], '1': [right]}}
    selection = {'placements': [dict(x=0, y=0, room_type=4, uid='own-left'),
                                dict(x=1, y=0, room_type=1, uid='own-right')]}
    layout = assemble_tiles(selection, act, SPEC, 2, 1)
    close_act_one_border(layout['tiles_row_major'])
    return layout, act, selection


class FloorLayoutTests(unittest.TestCase):
    def test_act_one_water_enemy_float_threshold_matches_piranha_minnow_branch(self):
        self.assertEqual(act_one_water_enemy_type_for_roll(0.799999), 16)
        self.assertEqual(act_one_water_enemy_type_for_roll(0.8), 17)
        for invalid in (-0.1, 1.1, float('nan'), True):
            with self.assertRaises(ValueError):
                act_one_water_enemy_type_for_roll(invalid)

    def test_act_one_water_enemy_spawn_weights_match_native_map(self):
        self.assertEqual(act_one_water_enemy_spawn_weight(16), 0.5)
        self.assertAlmostEqual(act_one_water_enemy_spawn_weight(17), 1 / 3, places=7)
        with self.assertRaises(ValueError):
            act_one_water_enemy_spawn_weight(9)

    def test_enemy_water_probe_reads_top_left_subtile_used_by_native_expression(self):
        from tools.floor_water import WaterField

        tiles = [[0] * 3 for _ in range(2)]
        water = WaterField(tiles, fluid=[0] * (6 * 4))
        index = (2 * 1) * water.width + 2 * 2
        water.fluid[index] = 100
        self.assertEqual(water.native_enemy_probe_mass(2, 1), 100)
        self.assertEqual(water.native_enemy_probe_mass(-1, 1), 0)

    def test_provisional_enemy_trace_consumes_canonical_for_native_water_branch(self):
        from tools.floor_water import WaterField
        from tools.floor_main_path import MT19937

        tiles = [[[9, -1, -1, -1, -1, -1] for _ in range(16)] for _ in range(16)]
        water = WaterField(tiles, fluid=[100] * (32 * 32))
        room_types = [[0] * 4 for _ in range(4)]
        room_types[2][2] = 0xff
        placements = [dict(x=2, y=2, room_type=0, uid='water-room')]
        trace = {}
        enemies = provisional_enemies(
            tiles, room_types, placements, {'width': 4, 'height': 4}, MT19937(123),
            [0, 0], [15, 15], floor_index=0, rng_trace=trace, water_field=water)
        self.assertEqual(enemies, [])  # Fish packet/runtime support is still pending.
        self.assertEqual(len(trace['water_spawn_decisions']), 1)
        decision = trace['water_spawn_decisions'][0]
        self.assertIn(decision['subtype'], (16, 17))
        self.assertEqual(decision['probe_mass'], 100)
        self.assertTrue(decision['rng_branch_reconstructed'])
        self.assertFalse(decision['candidate_filter_parity'])
        room = trace['rooms'][0]
        self.assertEqual(room['water_enemy_float_draws'], 1)
        self.assertEqual(room['roll_draws'], 0)
        self.assertEqual(room['spawn_count'], 1)

    def test_goal_rank_matches_ghidra_native_comparator_predicates(self):
        # This directly transcribes comparator @ 0x00689c60. It tests the
        # ordering relation, not MSVC std::sort's permutation of exact ties.
        kinds = (0, 1, 6, 7, 9)
        tiles = [[[kinds[(x * 7 + y * 11) % len(kinds)], -1, -1, -1, -1, -1]
                  for x in range(7)] for y in range(7)]

        def native_precedes(a, b):
            ax, ay = a
            bx, by = b
            a_open = int(not solid(tiles, ax - 1, ay)) + int(not solid(tiles, ax + 1, ay))
            b_open = int(not solid(tiles, bx - 1, by)) + int(not solid(tiles, bx + 1, by))
            if a_open != b_open:
                return b_open < a_open
            a_support = solid(tiles, ax, ay + 1)
            b_support = solid(tiles, bx, by + 1)
            if a_support == b_support:
                a_non_platform = tiles[ay][ax][0] != 6
                b_non_platform = tiles[by][bx][0] != 6
                if a_non_platform == b_non_platform:
                    return by < ay
                return a_non_platform and not b_non_platform
            return a_support and not b_support

        points = [(x, y) for y in range(1, 5) for x in range(1, 6)]
        for a in points:
            for b in points:
                native = native_precedes(a, b)
                rank_a, rank_b = goal_candidate_rank(tiles, a), goal_candidate_rank(tiles, b)
                self.assertEqual(rank_a < rank_b, native, (a, b, rank_a, rank_b))

    def test_goal_sort_reconstructs_native_insertion_path_and_keeps_equal_order(self):
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(8)] for _ in range(8)]
        tiles[6][2][0] = tiles[6][3][0] = 1
        points = [[x, 4] for x in range(1, 7)] + [[3, 4], [4, 4]]
        expected = sorted(points, key=lambda point: goal_candidate_rank(tiles, point))
        actual = points[:]
        report = sort_goal_candidates_msvc_path(actual, tiles)
        self.assertEqual(actual, expected)
        self.assertEqual(report['algorithm'], 'msvc_insertion_sort')
        self.assertTrue(report['exact_tie_order'])

        large = [[1 + (x % 6), 4] for x in range(34)]
        report = sort_goal_candidates_msvc_path(large, tiles)
        self.assertEqual(report['algorithm'], 'python_stable_sort_fallback')
        self.assertFalse(report['exact_tie_order'])

    def test_act_one_room_monster_budget_matches_native_act_branch(self):
        self.assertEqual([act_one_room_monster_budget(i) for i in range(3)],
                         [0.5, 0.5, 1.5])
        self.assertEqual([act_one_room_monster_budget(i, True) for i in range(3)],
                         [1.0, 1.0, 2.0])
        for invalid in (-1, 3, True, 1.0):
            with self.assertRaises(ValueError):
                act_one_room_monster_budget(invalid)

    def test_act_one_enemy_roll_uses_native_entity_ids_and_boundaries(self):
        self.assertEqual([act_one_enemy_type_for_roll(i) for i in (0, 39, 40, 84, 85, 99)],
                         [9, 9, 12, 12, 11, 11])
        self.assertIsNone(act_one_enemy_type_for_roll(85, lurker_geometry_supported=False))
        for invalid in (-1, 100, True, 1.0):
            with self.assertRaises(ValueError):
                act_one_enemy_type_for_roll(invalid)

    def test_solid_above_roll_matches_act_one_bat_and_spider_branch(self):
        self.assertEqual([act_one_solid_above_type_for_roll(i, 0)
                          for i in (0, 49, 50, 69, 70, 124)],
                         [10, 10, None, None, None, None])
        self.assertEqual([act_one_solid_above_type_for_roll(i, 1)
                          for i in (0, 49, 50, 69, 70, 124)],
                         [10, 10, 14, 14, None, None])

    def test_act_one_lurker_roll_respects_native_neighbor_solidity_filter(self):
        tiles = [[[0] for _ in range(5)] for _ in range(5)]
        self.assertTrue(act_one_lurker_geometry_supported(tiles, 2, 2))

        tiles[1][2][0] = 1
        self.assertFalse(act_one_lurker_geometry_supported(tiles, 2, 2))
        tiles[1][2][0] = 0

        tiles[2][1][0] = 1
        tiles[2][3][0] = 7
        self.assertFalse(act_one_lurker_geometry_supported(tiles, 2, 2))
        tiles[2][3][0] = 0
        self.assertTrue(act_one_lurker_geometry_supported(tiles, 2, 2))

    def test_act_one_room_tiles_use_native_x_major_shuffle(self):
        from tools.floor_main_path import MT19937

        room = {'x': 2, 'y': 1}
        spec = {'width': 3, 'height': 2}
        actual_rng = MT19937(123)
        actual = act_one_shuffled_room_tiles(room, spec, actual_rng)
        self.assertEqual(actual, [(8, 2), (7, 2), (7, 3), (6, 2), (8, 3), (6, 3)])
        self.assertEqual(actual_rng.draws, 5)
        self.assertEqual(len(set(actual)), spec['width'] * spec['height'])

    def test_provisional_enemy_spawns_obey_recovered_room_weight_cap(self):
        from tools.floor_main_path import MT19937

        spec = {'width': 10, 'height': 8}
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(30)] for _ in range(24)]
        for x in range(10, 20):
            tiles[14][x][0] = 1
        room_types = [[0, 0, 0] for _ in range(3)]
        room_types[1][1] = 4
        placements = [dict(x=1, y=1, room_type=4, uid='spawn-budget-fixture')]

        counts = []
        for floor_index in range(3):
            enemies = provisional_enemies(
                tiles, room_types, placements, spec, MT19937(12345),
                (0, 0), (29, 23), floor_index=floor_index)
            counts.append(len(enemies))
            self.assertTrue(all(enemy['room_monster_budget'] ==
                                act_one_room_monster_budget(floor_index)
                                for enemy in enemies))
            self.assertEqual([enemy['room_budget_slot'] for enemy in enemies],
                             list(range(len(enemies))))
        self.assertEqual(counts, [1, 1, 2])

    def test_provisional_enemy_rng_trace_marks_native_boss_call_position(self):
        from tools.floor_main_path import MT19937

        spec = {'width': 1, 'height': 1}
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(3)] for _ in range(3)]
        placements = [dict(x=0, y=0, room_type=4),
                      dict(x=1, y=1, room_type=4),
                      dict(x=2, y=2, room_type=1)]
        trace = {}
        rng = MT19937(12345)
        provisional_enemies(tiles, [[0] * 3 for _ in range(3)], placements,
                            spec, rng, (0, 0), (2, 2), boss_room=(2, 2),
                            rng_trace=trace)

        boss_call = trace['boss_call']
        boss_room_trace = trace['rooms'][boss_call['order_index']]
        self.assertEqual(boss_room_trace['room'], [2, 2])
        self.assertEqual(boss_call['draw_index'], boss_room_trace['draw_start'])
        self.assertEqual(boss_room_trace['skip_reason'], 'boss_call_not_simulated')
        self.assertFalse(trace['native_parity'])
        self.assertEqual(trace['exit_draw'], rng.draws)
        self.assertEqual(trace['draws_consumed'], rng.draws - trace['entry_draw'])

    def test_act_one_enemy_room_vector_excludes_top_row_and_omits_boss_call(self):
        from tools.floor_main_path import MT19937

        spec = {'width': 1, 'height': 1}
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(3)] for _ in range(2)]
        placements = [dict(x=0, y=0, room_type=1),
                      dict(x=1, y=1, room_type=4)]
        trace = {}
        provisional_enemies(tiles, [[0, 0], [0, 0]], placements, spec,
                            MT19937(7), (0, 0), (1, 1), boss_room=(0, 0),
                            rng_trace=trace)

        self.assertEqual(trace['room_list_filter']['excluded_top_row_count'], 1)
        self.assertEqual(trace['room_order'], [[1, 1]])
        self.assertIsNone(trace['boss_call'])
        self.assertEqual(trace['boss_not_called']['reason'],
                         'native_ACT_ONE_room_vector_excludes_y_zero')

    def test_placed_entity_rng_counts_only_recovered_creation_paths_in_source_order(self):
        class CountingRng:
            draws = 0

            def game_int(self):
                self.draws += 1
                return 0

        definitions = [
            {'type': kind, 'doodadType': doodad_type, **extra}
            for kind, doodad_type, extra in (
                ('Chest', None, {}), ('Spikes', None, {}), ('BreakableWall', None, {}),
                ('Doodad', 3, {}), ('Doodad', 16, {}), ('Doodad', 14, {}),
                ('Doodad', 16, {'crateType': 2}), ('DartTrap', None, {}),
                ('Doodad', 20, {}), ('Boulder', None, {}),
                ('BoulderBlocker', None, {}))]
        locations = [dict(room=[0, 0], source_uid='fixture', source_entity_index=index,
                          definition=definition)
                     for index, definition in enumerate(definitions)]
        rng = CountingRng()
        report = consume_placed_entity_rng(rng, locations)
        self.assertEqual(report['draws'], 6)
        self.assertEqual(report['draws_by_reason'], {
            'chest_constructor_facing': 1,
            'spike_variant_roll': 1,
            'breakable_wall_rotation': 1,
            'torch_initial_frame': 1,
            'torch_initial_timer': 1,
            'random_crate_type': 1,
        })
        self.assertEqual([entity['source_entity_index'] for entity in report['per_entity']],
                         [0, 1, 2, 3, 4])
        self.assertEqual(report['rng_free_entity_count'], 5)
        self.assertEqual([(entity['source_entity_index'], entity['kind'], entity['doodad_type'])
                          for entity in report['rng_free_entities']],
                         [(6, 'Doodad', 16), (7, 'DartTrap', None),
                          (8, 'Doodad', 20), (9, 'Boulder', None),
                          (10, 'BoulderBlocker', None)])
        self.assertEqual(report['rng_free_entities'][3]['boulder_type'], 1)
        self.assertEqual(report['deferred_entity_count'], 1)
        self.assertEqual(report['deferred_entities'][0]['source_entity_index'], 5)
        self.assertEqual(report['unclassified_entity_count'], 0)
        self.assertEqual(rng.draws, 6)

    def test_endpoint_blockers_include_rounded_boulders_and_linked_trap_boulders(self):
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(8)] for _ in range(8)]
        tiles[1][2][0] = 1
        locations = [
            dict(world_pixel=[17.2, 32.7],
                 definition={'type': 'Boulder'}, source_uid='room-boulder',
                 source_entity_index=0),
            dict(world_pixel=[33.2, 65.2],
                 definition={'type': 'BoulderTrap'}, source_uid='room-trap',
                 source_entity_index=1),
        ]
        result = blocked_endpoint_tiles(locations, tiles)
        self.assertEqual(result['tiles'], {(1, 2), (2, 4), (2, 1)})
        self.assertEqual([(record['native_type'], record['source'])
                          for record in result['records']], [
                              ('H', 'placed_Boulder'),
                              ('G', 'placed_BoulderTrap'),
                              ('H', 'boulder_trap_link'),
                          ])

    def test_spellbook_rng_tracks_skill_tier_guards_and_rand_index_adjustment(self):
        class ScriptedRng:
            def __init__(self):
                self.values = iter((2, 1, 17, 1, 5, 7))
                self.draws = 0

            def game_int(self):
                self.draws += 1
                return next(self.values)

        rng = ScriptedRng()
        report = consume_spellbook_rng(rng, floor_index=0)
        self.assertEqual(report['skill_types'], [1, 1, 5])
        self.assertEqual(report['final_skill_tier'], -1)
        self.assertEqual(report['final_rand_index'] & 0xf, 5)
        self.assertEqual(report['draws'], 6)
        self.assertEqual(rng.draws, 6)

    def test_spawn_geometry_checks_every_pixel_occupied_body_and_footing_cell(self):
        # A 12px body at local y=12 crosses the boundary into the next tile.
        from tools.floor_layout import _spawn_geometry

        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(3)] for _ in range(3)]
        tiles[1][1][0] = 1
        geometry = _spawn_geometry(tiles, [1, 0], pixel_offset=(4, 12))
        self.assertEqual([tile[:2] for tile in geometry['body_tiles']], [[1, 0], [1, 1]])
        self.assertFalse(geometry['clear'])
        self.assertEqual(geometry['support_tiles'], [[1, 1, 1]])
        self.assertTrue(geometry['supported'])

    def test_layers_and_source_tiles_preserved_when_border_closes(self):
        room = raw('own', [[0, 0, 0]] * 4)
        room['tiles'][0][1:6] = [1, 2, 3, 4, 5]
        original = deepcopy(room)
        layout = assemble_tiles({'placements': [dict(x=0, y=0, room_type=0, uid='own')]},
                                {'rooms': {'0': [room]}}, SPEC, 1, 1)
        self.assertEqual(close_act_one_border(layout['tiles_row_major']), 10)
        self.assertEqual(layout['tiles_row_major'][0][0], [1, 1, 2, 3, 4, 5])
        self.assertEqual(layout['tiles_row_major'][1][1][0], 0)
        self.assertEqual(room, original)
        layout['tiles_row_major'][1][1][3] = 17
        self.assertEqual(room, original)

    def test_offsets_and_row_first_entity_locations_keep_parameters(self):
        room = raw('own', [[0, 0, 0]] * 4)
        room['entities'] = [dict(type='SyntheticTrap', x=7, y=9.5, canReload=True,
                                nested={'value': [1, 2]})]
        selection = {'placements': [dict(x=x, y=y, room_type=0, uid='own')
                                    for x in range(2) for y in range(2)]}
        layout = assemble_tiles(selection, {'rooms': {'0': [room]}}, SPEC, 2, 2)
        entities = layout['entity_locations']
        self.assertEqual([e['room'] for e in entities], [[0, 0], [1, 0], [0, 1], [1, 1]])
        self.assertEqual([e['world_pixel'] for e in entities], [[7, 9.5], [55, 9.5], [7, 73.5], [55, 73.5]])
        self.assertTrue(all(e['definition']['canReload'] for e in entities))
        entities[0]['definition']['nested']['value'][0] = 99
        self.assertEqual(room['entities'][0]['nested']['value'], [1, 2])
        self.assertFalse(layout['floor_complete'])
        self.assertFalse(layout['entities_instantiated'])

    def test_incomplete_duplicate_double_unknown_uid_and_invalid_entity_rejected(self):
        layout, act, selection = joined_rooms()
        bad = deepcopy(selection)
        bad['placements'].pop()
        with self.assertRaises(ValueError): assemble_tiles(bad, act, SPEC, 2, 1)
        bad = deepcopy(selection)
        bad['placements'][1]['x'] = 0
        with self.assertRaises(ValueError): assemble_tiles(bad, act, SPEC, 2, 1)
        bad = deepcopy(selection)
        bad['placements'][0]['room_type'] |= 4096
        with self.assertRaises(ValueError): assemble_tiles(bad, act, SPEC, 2, 1)
        bad = deepcopy(selection)
        bad['placements'][0]['uid'] = 'absent'
        with self.assertRaises(ValueError): assemble_tiles(bad, act, SPEC, 2, 1)
        for entity in ({'type': 'Synthetic', 'x': True, 'y': 0},
                       {'type': 'Synthetic', 'x': 48, 'y': 0},
                       {'type': 'Synthetic', 'x': 0, 'y': float('nan')}, 'not an object'):
            act['rooms']['4'][0]['entities'] = [entity]
            with self.assertRaises(ValueError): assemble_tiles(selection, act, SPEC, 2, 1)

    def test_tile_flood_requires_link_and_an_open_path_to_neighbor_cell(self):
        layout, act, selection = joined_rooms()
        tiles = layout['tiles_row_major']
        self.assertTrue(tile_paths_to_room_edge(tiles, [[4, 1]], SPEC, 1, 1))
        self.assertFalse(tile_paths_to_room_edge(tiles, [[0, 1]], SPEC, 1, 1))
        self.assertFalse(tile_paths_to_room_edge(tiles, [[4, 1]], SPEC, 0, 1))
        tiles[1][2][0] = 7
        self.assertTrue(solid(tiles, 2, 1))
        self.assertFalse(tile_paths_to_room_edge(tiles, [[4, 1]], SPEC, 1, 1))
        self.assertTrue(solid(tiles, -1, 1))

    def test_four_link_sides_and_an_enclosed_component(self):
        tiles = [[[0, -1, -1, -1, -1, -1] for x in range(9)] for y in range(9)]
        close_act_one_border(tiles)
        types = [[0] * 3 for _ in range(3)]
        spec = {'width': 3, 'height': 3}
        for bit in (1, 2, 4, 8, 16, 32, 64, 128):
            types[1][1] = bit
            self.assertTrue(tile_paths_to_room_edge(tiles, types, spec, 4, 4))
        types[1][1] = 0
        self.assertFalse(tile_paths_to_room_edge(tiles, types, spec, 4, 4))
        for i in range(2, 7):
            tiles[2][i][0] = tiles[6][i][0] = tiles[i][2][0] = tiles[i][6][0] = 1
        types[1][1] = 255
        self.assertFalse(tile_paths_to_room_edge(tiles, types, spec, 4, 4))

    def test_entrance_candidates_prefer_dry_tiles_and_platform_is_not_solid_support(self):
        layout, act, selection = joined_rooms()
        tiles = layout['tiles_row_major']
        candidates = above_ground_candidates(tiles, [[4, 1]], SPEC, [0, 0])
        self.assertEqual(candidates['preferred'], [[1, 1], [2, 1]])
        self.assertEqual(candidates['effective_group'], 'preferred')
        self.assertFalse(candidates['shuffled'])
        tiles[1][2][0] = 9
        candidates = above_ground_candidates(tiles, [[4, 1]], SPEC, [0, 0])
        self.assertEqual(candidates['preferred'], [])
        self.assertEqual(candidates['near_water'], [[1, 1], [2, 1]])
        self.assertEqual(candidates['effective_group'], 'near_water')
        tiles[2][1][0] = 6
        candidates = above_ground_candidates(tiles, [[4, 1]], SPEC, [0, 0])
        self.assertNotIn([1, 1], candidates['preferred'] + candidates['near_water'])

    def test_entrance_candidates_keep_native_x_major_order_and_water_grouping(self):
        # Ghidra 0x0065a9a0 iterates room x, then y, and stores candidates
        # adjacent to water in a separate vector before either vector is shuffled.
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(12)] for _ in range(12)]
        close_act_one_border(tiles)
        for x in range(4, 8):
            tiles[7][x][0] = 1
        tiles[6][4][0] = 9
        types = [[255] * 3 for _ in range(3)]
        candidates = above_ground_candidates(
            tiles, types, {'width': 4, 'height': 4}, [1, 1])
        self.assertEqual(candidates['preferred'], [[6, 6], [7, 6]])
        self.assertEqual(candidates['near_water'], [[4, 6], [5, 6]])
        self.assertEqual(candidates['effective_group'], 'preferred')
        self.assertFalse(candidates['shuffled'])
        self.assertFalse(candidates['candidate_set_seed_exact'])

    def test_provisional_route_repair_adds_platforms_and_ladders_without_overwriting_walls(self):
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(7)] for _ in range(7)]
        tiles[5][1][0] = 1
        route = [[1, 4], [2, 4], [2, 3], [3, 3]]
        report = repair_provisional_route_support(tiles, route)
        self.assertTrue(report['applied'])
        self.assertFalse(report['native_generation_parity'])
        self.assertEqual(tiles[4][2][0], 3)
        self.assertEqual(tiles[3][2][0], 3)
        self.assertEqual(tiles[5][2][0], 6)
        self.assertEqual(tiles[4][3][0], 6)
        self.assertEqual(tiles[5][1][0], 1)
        with self.assertRaisesRegex(ValueError, 'solid tile'):
            repair_provisional_route_support(tiles, [[1, 4], [1, 5]])


if __name__ == '__main__':
    unittest.main()
