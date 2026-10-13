from copy import deepcopy
import unittest
from tools.floor_layout import (assemble_tiles, close_act_one_border, solid,
                                tile_paths_to_room_edge, above_ground_candidates)
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


if __name__ == '__main__':
    unittest.main()
