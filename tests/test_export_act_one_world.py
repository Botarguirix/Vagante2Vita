import struct
import tempfile
import unittest
from pathlib import Path

from PIL import Image

from tools.export_act_one_world import (audit_act_one_entity_definitions,
                                        encode_entities, encode_enemies, export, validate_spawn_geometry)


def valid_spawn_report():
    return {'body_pixels': [8, 12], 'horizontal_inset_pixels': 4,
            'native_spawn_pixel_offset': [4, 4], 'body_bounds_pixels': [4, 4, 11, 15],
            'body_tiles': [[0, 0, 0]], 'support_tiles': [[0, 1, 1]],
            'clear': True, 'supported': True, 'verified': True}


class ExportActOneWorldTests(unittest.TestCase):
    def test_encodes_only_explicitly_provisional_act_one_enemies(self):
        packet, counts = encode_enemies([{'subtype': 9, 'world_pixel': [16, 32],
                                          'native_parity': False}], 8, 8)
        self.assertEqual(struct.unpack_from('<4sHH', packet), (b'VWE3', 1, 16))
        self.assertEqual(struct.unpack_from('<BBHiiI', packet, 8), (24, 9, 0, 4096, 8192, 0))
        self.assertEqual(counts, {9: 1})

    def test_encodes_native_act_one_spider_as_provisional_enemy(self):
        packet, counts = encode_enemies([{'subtype': 14, 'world_pixel': [16, 32],
                                          'native_parity': False}], 8, 8)
        self.assertEqual(packet[:4], b'VWE3')
        self.assertEqual(packet[9], 14)
        self.assertEqual(counts, {14: 1})
        with self.assertRaisesRegex(ValueError, 'Invalid provisional'):
            encode_enemies([{'subtype': 49, 'world_pixel': [0, 0], 'native_parity': False}], 8, 8)

    def test_act_one_room_entity_catalog_is_supported_and_complete(self):
        import json
        database_path = Path(__file__).parents[1] / '_local/assets/rooms.json'
        if not database_path.is_file():
            self.skipTest('Private local Vagante room database is not available')
        database = json.loads(database_path.read_text('utf-8'))
        counts = audit_act_one_entity_definitions(database)
        self.assertEqual(counts, {'Boulder': 15, 'BoulderBlocker': 1,
                                  'BoulderTrap': 33, 'BreakableWall': 16,
                                  'Chest': 289, 'DartTrap': 50, 'Doodad': 39,
                                  'Spikes': 258})

    def test_act_one_entity_audit_rejects_unknown_and_missing_parameters(self):
        unknown = {'acts': [{'rooms': {'1': [{'entities': [{'type': 'Monster'}]}]}}]}
        missing = {'acts': [{'rooms': {'1': [{'entities': [{'type': 'Chest'}]}]}}]}
        with self.assertRaisesRegex(ValueError, 'Unsupported ACT_ONE entity'):
            audit_act_one_entity_definitions(unknown)
        with self.assertRaisesRegex(ValueError, 'Chest definition missing fields'):
            audit_act_one_entity_definitions(missing)

    def test_encodes_act_one_entity_types_and_fractional_positions(self):
        instances = [
            {'world_pixel': [10.5, 20.25], 'definition': {'type': 'Chest', 'chestType': 1}},
            {'world_pixel': [32, 48], 'definition': {'type': 'Spikes'}},
            {'world_pixel': [64, 80], 'definition': {'type': 'DartTrap', 'canReload': True, 'facingLeft': True}},
            {'world_pixel': [96, 112], 'definition': {'type': 'Doodad', 'doodadType': 19}},
            {'world_pixel': [128, 144], 'definition': {'type': 'BoulderTrap'}},
            {'world_pixel': [160, 176], 'definition': {'type': 'BreakableWall'}},
            {'world_pixel': [192, 208], 'definition': {'type': 'Boulder'}},
            {'world_pixel': [224, 240], 'definition': {'type': 'BoulderBlocker', 'facingLeft': True}},
        ]
        packet, counts = encode_entities(instances, 16, 16)
        self.assertEqual(struct.unpack_from('<4sHH', packet), (b'VWE2', 8, 16))
        first = struct.unpack_from('<BBHiiI', packet, 8)
        self.assertEqual(first, (16, 0, 1, 10 * 256 + 128, 20 * 256 + 64, 0))
        trap = struct.unpack_from('<BBHiiI', packet, 8 + 2 * 16)
        self.assertEqual((trap[0], trap[1]), (18, 6))
        blocker = struct.unpack_from('<BBHiiI', packet, 8 + 7 * 16)
        self.assertEqual((blocker[0], blocker[1]), (23, 2))
        boulder = struct.unpack_from('<BBHiiI', packet, 8 + 6 * 16)
        self.assertEqual((boulder[0], boulder[1], boulder[2]), (22, 0, 0))
        self.assertEqual(sum(counts.values()), 8)

    def test_dart_trap_defaults_orientation_and_rejects_invalid_orientation(self):
        packet, _ = encode_entities([{'world_pixel': [0, 0], 'definition': {
            'type': 'DartTrap', 'canReload': False}}], 1, 1)
        self.assertEqual(struct.unpack_from('<BBHiiI', packet, 8)[1], 0)
        with self.assertRaisesRegex(ValueError, 'DartTrap facingLeft must be boolean'):
            encode_entities([{'world_pixel': [0, 0], 'definition': {
                'type': 'DartTrap', 'canReload': False, 'facingLeft': 1}}], 1, 1)

    def test_dart_trap_orientation_matches_set_level_wall_probe(self):
        tiles = [[[0, -1, -1, -1, -1, -1] for _ in range(3)] for _ in range(2)]
        tiles[0][2][0] = 1
        instances = [
            {'world_pixel': [9, 0], 'definition': {
                'type': 'DartTrap', 'canReload': True, 'facingLeft': False}},
            {'world_pixel': [0, 0], 'definition': {
                'type': 'DartTrap', 'canReload': False, 'facingLeft': True}},
        ]
        packet, _ = encode_entities(instances, 3, 2, tiles)
        right_wall = struct.unpack_from('<BBHiiI', packet, 8)
        open_right = struct.unpack_from('<BBHiiI', packet, 24)
        self.assertEqual(right_wall[1], 6)
        self.assertEqual(open_right[1], 0)

    def test_rejects_unknown_or_out_of_bounds_entities(self):
        with self.assertRaisesRegex(ValueError, 'Unsupported ACT_ONE entity'):
            encode_entities([{'world_pixel': [0, 0], 'definition': {'type': 'Unknown'}}], 2, 2)
        with self.assertRaisesRegex(ValueError, 'outside world bounds'):
            encode_entities([{'world_pixel': [32, 0], 'definition': {'type': 'Spikes'}}], 2, 2)

    def test_emits_runtime_vwi2_and_composite_for_endpoint_layout(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            assets = root / 'assets'
            gfx = assets / 'gfx'
            gfx.mkdir(parents=True)
            for name in ('background', 'farground', 'midground', 'foreground', 'closeground'):
                Image.new('RGBA', (16, 16), (90, 120, 150, 255)).save(gfx / f'tiles-{name}.png')
            report = {
                'stage': 'terrain_secret_water_and_provisional_endpoints',
                'tile_layout': {
                    'width': 1, 'height': 2,
                    'tiles_row_major': [[[0, -1, -1, -1, -1, -1]],
                                        [[1, -1, -1, -1, -1, -1]]],
                    'endpoints': {'entrance': [0, 0], 'goal': [0, 0], 'mutations_applied': True,
                                  'native_tile_path_verified': True,
                                  'native_player_spawn_geometry': valid_spawn_report()},
                    'native_tile_path': {'reachable': True, 'path': [[0, 0]],
                                         'visited_tiles': 0, 'models_player_physics': False},
                    'water': {},
                },
            }
            output = root / 'output'
            summary = export(report, output, assets)
            packet = (output / 'act-one-world.vwi').read_bytes()
            magic, width, height, flags, count = struct.unpack_from('<4sHHII', packet)
            self.assertEqual((magic, width, height, flags, count), (b'VWI2', 1, 2, 0, 2))
            self.assertEqual(struct.unpack_from('<4H', packet, 16), (0, 0, 0, 0))
            self.assertEqual(len(packet), 32 + 24)
            self.assertEqual(struct.unpack('<4sHH', (output / 'act-one-entities.vwe').read_bytes()),
                             (b'VWE2', 0, 16))
            self.assertEqual(summary['exact_generation_parity'], False)
            self.assertTrue(summary['native_player_spawn_geometry']['verified'])
            self.assertEqual(summary['native_tile_path']['route_tiles'], [[0, 0]])
            with Image.open(output / 'intro-terrain.png') as image:
                self.assertEqual(image.size, (16, 32))

    def test_rejects_endpoint_missing_or_invalid_dimensions(self):
        report = {'tile_layout': {'width': 1, 'height': 1,
                                  'tiles_row_major': [[[0, -1, -1, -1, -1, -1]]]}}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            gfx = root / 'assets/gfx'
            gfx.mkdir(parents=True)
            for name in ('background', 'farground', 'midground', 'foreground', 'closeground'):
                Image.new('RGBA', (16, 16), (0, 0, 0, 0)).save(gfx / f'tiles-{name}.png')
            with self.assertRaises(ValueError):
                export(report, root / 'out', root / 'assets')

    def test_rejects_spawn_geometry_that_is_not_verified(self):
        report = {'tile_layout': {
            'width': 1, 'height': 2,
            'tiles_row_major': [[[7, -1, -1, -1, -1, -1]],
                                [[1, -1, -1, -1, -1, -1]]],
            'endpoints': {'entrance': [0, 0], 'goal': [0, 0], 'mutations_applied': True,
                          'native_player_spawn_geometry': {
                              **valid_spawn_report(), 'body_tiles': [[0, 0, 7]], 'clear': False}},
        }}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            gfx = root / 'assets/gfx'
            gfx.mkdir(parents=True)
            for name in ('background', 'farground', 'midground', 'foreground', 'closeground'):
                Image.new('RGBA', (16, 16), (0, 0, 0, 0)).save(gfx / f'tiles-{name}.png')
            with self.assertRaisesRegex(ValueError, 'spawn geometry'):
                export(report, root / 'out', root / 'assets')

    def test_rejects_disconnected_native_tile_path(self):
        report = {'tile_layout': {
            'width': 1, 'height': 2,
            'tiles_row_major': [[[0, -1, -1, -1, -1, -1]],
                                [[1, -1, -1, -1, -1, -1]]],
            'endpoints': {'entrance': [0, 0], 'goal': [0, 0], 'mutations_applied': True,
                          'native_tile_path_verified': True,
                          'native_player_spawn_geometry': valid_spawn_report()},
            'native_tile_path': {'reachable': False, 'path': [], 'visited_tiles': 1},
        }}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            gfx = root / 'assets/gfx'
            gfx.mkdir(parents=True)
            for name in ('background', 'farground', 'midground', 'foreground', 'closeground'):
                Image.new('RGBA', (16, 16), (0, 0, 0, 0)).save(gfx / f'tiles-{name}.png')
            with self.assertRaisesRegex(ValueError, 'tile-path validation'):
                export(report, root / 'out', root / 'assets')

    def test_rejects_complete_floor_claim_without_runtime_evidence(self):
        report = {'exact_generation_parity': False, 'tile_layout': {
            'width': 1, 'height': 2,
            'tiles_row_major': [[[0, -1, -1, -1, -1, -1]],
                                [[1, -1, -1, -1, -1, -1]]],
            'floor_complete': True,
        }}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(ValueError, 'complete ACT_ONE floor requires'):
                export(report, root / 'out', root / 'assets')

    def test_spawn_validator_rejects_stale_or_fabricated_clearance(self):
        tiles = [[[0, -1, -1, -1, -1, -1]], [[1, -1, -1, -1, -1, -1]]]
        report = valid_spawn_report()
        tiles[0][0][0] = 7
        with self.assertRaisesRegex(ValueError, 'does not match exported tiles'):
            validate_spawn_geometry(tiles, [0, 0], report)


if __name__ == '__main__':
    unittest.main()
