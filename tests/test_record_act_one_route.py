import json
import hashlib
import struct
from pathlib import Path
import tempfile
import unittest

from tools.record_act_one_route import record
from tools.route_probe_fingerprint import SOURCE_FILES
from tools.prepare_act_one_campaign import load_route_proof


class RecordActOneRouteTests(unittest.TestCase):
    def test_accepts_physical_exit_touch_before_all_guide_waypoints(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, enemies, log, output, trace = (root / name for name in
                ('world.vwi', 'entities.vwe', 'enemies.vwe', 'route.log', 'floor-00.json', 'input.vtr'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            enemies.write_bytes(b'VWE3 boss')
            ticks = 1268
            log.write_text(
                'ACT_ONE_BOSS result defeated=1 hp=0.0 player_hp=10 ticks=1268 waypoint=3/6\n'
                'ACT_ONE_ROUTE seed=1 tiles=6 waypoint=3 ticks=1268 jumps=19 reached_exit=1',
                encoding='utf-8')
            trace.write_bytes(b'VTR1' + struct.pack('<I', ticks) + bytes(2 * ticks))
            proof = record(log, world, entities, output, 1, trace_path=trace,
                           require_boss_defeat=True, enemies=enemies)
            self.assertTrue(proof['reached_exit'])
            self.assertEqual(proof['final_waypoint'], 3)
            self.assertEqual(proof['route_tiles'], 6)
            self.assertEqual(proof['boss']['player_hp'], 10)
            self.assertEqual(proof['controller_trace_frames'], ticks)

    def test_records_hash_bound_controller_trace_with_successful_route(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, log, output, trace = (root / name for name in
                ('world.vwi', 'entities.vwe', 'route.log', 'floor-01.json', 'input.vtr'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            log.write_text('ACT_ONE_ROUTE seed=12345 tiles=3 waypoint=2 ticks=4 jumps=0 reached_exit=1',
                           encoding='utf-8')
            trace.write_bytes(struct.pack('<4sI4H', b'VTR1', 4, 0x80, 0x80, 0x20, 0x20))
            proof = record(log, world, entities, output, 12345, trace_path=trace)
            self.assertEqual(proof['version'], 3)
            self.assertEqual(proof['controller_trace_frames'], 4)
            self.assertEqual(proof['controller_trace_file'], 'floor-01.vtr')
            self.assertEqual((root / 'floor-01.vtr').read_bytes(), trace.read_bytes())
            self.assertEqual(load_route_proof(output, world.read_bytes(), entities.read_bytes())['version'], 3)

    def test_accepts_probe_trace_already_at_proof_sidecar_path(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, log, output = (root / name for name in
                ('world.vwi', 'entities.vwe', 'route.log', 'floor-02.json'))
            trace = root / 'floor-02.vtr'
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            log.write_text('ACT_ONE_ROUTE seed=12345 tiles=3 waypoint=2 ticks=2 jumps=0 reached_exit=1',
                           encoding='utf-8')
            trace.write_bytes(struct.pack('<4sI2H', b'VTR1', 2, 0x10, 0x20))
            proof = record(log, world, entities, output, 12345, trace_path=trace)
            self.assertEqual(proof['controller_trace_file'], trace.name)
            self.assertEqual(load_route_proof(output, world.read_bytes(), entities.read_bytes())['version'], 3)

    def test_rejects_controller_trace_with_wrong_frame_count(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, log, output, trace = (root / name for name in
                ('world.vwi', 'entities.vwe', 'route.log', 'proof.json', 'input.vtr'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            log.write_text('ACT_ONE_ROUTE seed=12345 tiles=3 waypoint=2 ticks=4 jumps=0 reached_exit=1',
                           encoding='utf-8')
            trace.write_bytes(struct.pack('<4sI3H', b'VTR1', 3, 0x80, 0x20, 0x80))
            with self.assertRaisesRegex(ValueError, 'frame count'):
                record(log, world, entities, output, 12345, trace_path=trace)
            self.assertFalse(output.exists())

    def test_successful_route_log_is_hash_bound_to_exact_world_and_entities(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, log, output = (root / name for name in
                                             ('world.vwi', 'entities.vwe', 'route.log', 'proof.json'))
            world.write_bytes(b'VWI2 test world')
            entities.write_bytes(b'VWE2 test entities')
            log.write_text('WORLD ready=1\nACT_ONE_ROUTE seed=12345 tiles=59 waypoint=58 '
                           'ticks=664 jumps=3 reached_exit=1 x=0 y=0\n', encoding='utf-8')
            proof = record(log, world, entities, output, 12345)
            self.assertTrue(proof['reached_exit'])
            self.assertEqual(proof['version'], 2)
            self.assertIn('vita/src/player.c', proof['simulator']['files'])
            self.assertEqual(json.loads(output.read_text('utf-8')), proof)
            world.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError, 'different floor data'):
                load_route_proof(output, world.read_bytes(), entities.read_bytes())

    def test_route_proof_is_invalidated_when_simulator_sources_change(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source_root = root / 'source'
            for relative in SOURCE_FILES:
                path = source_root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(relative, encoding='utf-8')
            (source_root / 'vita/src/world.h').write_text('header', encoding='utf-8')
            world, entities, log, output = (root / name for name in
                                             ('world.vwi', 'entities.vwe', 'route.log', 'proof.json'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            log.write_text('ACT_ONE_ROUTE seed=77 tiles=5 waypoint=4 ticks=80 jumps=1 reached_exit=1',
                           encoding='utf-8')
            proof = record(log, world, entities, output, 77, source_root)
            self.assertTrue(load_route_proof(output, world.read_bytes(), entities.read_bytes(), source_root))
            self.assertEqual(proof['simulator']['sha256'],
                             load_route_proof(output, world.read_bytes(), entities.read_bytes(), source_root)['simulator']['sha256'])
            (source_root / 'vita/src/player.c').write_text('changed physics', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'different simulator sources'):
                load_route_proof(output, world.read_bytes(), entities.read_bytes(), source_root)

    def test_failed_or_wrong_seed_route_is_not_recorded(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, log, output = (root / name for name in
                                             ('world.vwi', 'entities.vwe', 'route.log', 'proof.json'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            log.write_text('ACT_ONE_ROUTE seed=1 tiles=10 waypoint=9 ticks=50 jumps=2 reached_exit=0',
                           encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'did not reach'):
                record(log, world, entities, output, 12345)
            self.assertFalse(output.exists())

    def test_boss_route_proof_requires_defeat_and_surviving_player(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, enemies, log, output = (root / name for name in
                ('world.vwi', 'entities.vwe', 'enemies.vwe', 'route.log', 'proof.json'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            enemies.write_bytes(b'VWE3 exact test boss packet')
            log.write_text(
                'ACT_ONE_PLAYER class=1 hp=80/80 bow=1 arrows=30\n'
                'ACT_ONE_BOSS result defeated=1 hp=0.0 player_hp=41 ticks=2161 waypoint=89/90\n'
                'ACT_ONE_ROUTE seed=4 tiles=90 waypoint=89 ticks=2161 jumps=3 reached_exit=1',
                encoding='utf-8')
            proof = record(log, world, entities, output, 4, require_boss_defeat=True,
                           enemies=enemies)
            self.assertEqual(proof['version'], 5)
            self.assertEqual(proof['player_class'], 1)
            self.assertEqual(proof['start_state'], dict(hp=80, max_hp=80, arrows=30))
            self.assertEqual(proof['enemies_sha256'], hashlib.sha256(
                enemies.read_bytes()).hexdigest())
            self.assertEqual(proof['boss'], dict(defeated=True, hp=0.0, player_hp=41,
                ticks=2161, final_waypoint=89, route_tiles=90))
            log.write_text(
                'ACT_ONE_BOSS result defeated=0 hp=44.0 player_hp=70 ticks=900 waypoint=36/90\n'
                'ACT_ONE_ROUTE seed=4 tiles=90 waypoint=89 ticks=2161 jumps=3 reached_exit=1',
                encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'did not defeat'):
                record(log, world, entities, output, 4, require_boss_defeat=True,
                       enemies=enemies)

    def test_boss_route_proof_is_bound_to_the_exact_enemy_packet(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, enemies, log, proof_path, trace = (root / name for name in
                ('world.vwi', 'entities.vwe', 'enemies.vwe', 'route.log', 'floor-01.json', 'input.vtr'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            enemies.write_bytes(b'VWE3 boss A')
            log.write_text(
                'ACT_ONE_BOSS result defeated=1 hp=0.0 player_hp=21 ticks=40 waypoint=2/3\n'
                'ACT_ONE_ROUTE seed=49 tiles=3 waypoint=2 ticks=40 jumps=0 reached_exit=1',
                encoding='utf-8')
            trace.write_bytes(struct.pack('<4sI40H', b'VTR1', 40, *([0x20] * 40)))
            record(log, world, entities, proof_path, 49, trace_path=trace,
                   require_boss_defeat=True, enemies=enemies)
            self.assertTrue(load_route_proof(proof_path, world.read_bytes(),
                entities.read_bytes(), enemies=enemies.read_bytes()))
            with self.assertRaisesRegex(ValueError, 'boss proof is not bound'):
                load_route_proof(proof_path, world.read_bytes(), entities.read_bytes(),
                                 enemies=b'VWE3 boss B')

    def test_class_tagged_proof_rejects_invalid_player_class(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            world, entities, log, output = (root / name for name in
                ('world.vwi', 'entities.vwe', 'route.log', 'proof.json'))
            world.write_bytes(b'world')
            entities.write_bytes(b'entities')
            log.write_text('ACT_ONE_PLAYER class=3 hp=80/80 bow=1 arrows=30\n'
                           'ACT_ONE_ROUTE seed=9 tiles=2 waypoint=1 ticks=2 jumps=0 reached_exit=1',
                           encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'unsupported player class'):
                record(log, world, entities, output, 9)
            self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
