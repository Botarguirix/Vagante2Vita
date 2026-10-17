import json
import struct
from pathlib import Path
import tempfile
import unittest

from tools.record_act_one_route import record
from tools.route_probe_fingerprint import SOURCE_FILES
from tools.prepare_act_one_campaign import load_route_proof


class RecordActOneRouteTests(unittest.TestCase):
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


if __name__ == '__main__':
    unittest.main()
