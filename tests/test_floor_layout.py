from copy import deepcopy
import unittest
from tools.floor_layout import (assemble_tiles, close_act_one_border, solid,
                                tile_paths_to_room_edge, above_ground_candidates,
                                repair_provisional_route_support,
                                act_one_room_monster_budget, act_one_enemy_type_for_roll,
                                act_one_lurker_geometry_supported,
                                act_one_shuffled_room_tiles,
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
                ('Doodad', 16, {'crateType': 2}), ('DartTrap', None, {}))]
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
        self.assertEqual(report['rng_free_entity_count'], 2)
        self.assertEqual([(entity['source_entity_index'], entity['kind'], entity['doodad_type'])
                          for entity in report['rng_free_entities']],
                         [(6, 'Doodad', 16), (7, 'DartTrap', None)])
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
