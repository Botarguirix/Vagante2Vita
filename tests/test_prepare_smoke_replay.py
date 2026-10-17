import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import prepare_smoke_replay
from tools.record_act_one_route import record

SCRIPT = Path(__file__).resolve().parents[1] / 'tools' / 'prepare_smoke_replay.py'


class PrepareActOneCampaignReplayTests(unittest.TestCase):
    def make_campaign(self, root):
        campaign = root / 'campaign'
        floors = campaign / 'floors'
        floors.mkdir(parents=True)
        route = {'entrance': [1, 2], 'goal': [3, 2],
                 'native_tile_path': {'route_tiles': [[1, 2], [2, 2], [3, 2]]}}
        (campaign / 'act-one-layout.json').write_text(json.dumps(route), encoding='utf-8')
        (floors / 'floor-01-layout.json').write_text(json.dumps(route), encoding='utf-8')
        (floors / 'floor-02-layout.json').write_text(json.dumps(route), encoding='utf-8')
        return campaign

    def test_emits_bounded_replay_for_all_three_routes(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            output = root / 'campaign.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            data = output.read_bytes()
            magic, count, reserved = struct.unpack_from('<4sHH', data)
            self.assertEqual((magic, reserved), (b'VTS1', 0))
            self.assertEqual(len(data), 8 + count * 8)
            self.assertLessEqual(count, 1024)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8) for i in range(count)]
            self.assertTrue(all(1 <= ticks <= 600 and padding == 0
                                for _, ticks, padding in steps))
            self.assertLessEqual(sum(ticks for _, ticks, _ in steps), 36000)
            self.assertEqual(steps.count((0, 60, 0)), 3)
            self.assertIn('floors=3', process.stdout)

    def make_physical_traces(self, campaign, root, masks_by_floor):
        route_proofs = campaign / 'route-proofs'
        route_proofs.mkdir(parents=True, exist_ok=True)
        floor_files = {
            0: (campaign / 'act-one-world.vwi', campaign / 'act-one-entities.vwe'),
            1: (campaign / 'floors/floor-01.vwi', campaign / 'floors/floor-01-entities.vwe'),
            2: (campaign / 'floors/floor-02.vwi', campaign / 'floors/floor-02-entities.vwe'),
        }
        for index, (world, entities) in floor_files.items():
            world.parent.mkdir(parents=True, exist_ok=True)
            world.write_bytes(f'VWI2 world {index}'.encode())
            entities.write_bytes(f'VWE2 entities {index}'.encode())
            masks = masks_by_floor[index]
            trace_source = root / f'floor-{index:02d}-input.vtr'
            trace_source.write_bytes(struct.pack('<4sI', b'VTR1', len(masks))
                                     + b''.join(struct.pack('<H', mask) for mask in masks))
            log = root / f'floor-{index:02d}.log'
            log.write_text(f'ACT_ONE_ROUTE seed=12345 tiles=3 waypoint=2 ticks={len(masks)} '
                           'jumps=0 reached_exit=1', encoding='utf-8')
            record(log, world, entities, route_proofs / f'floor-{index:02d}.json',
                   12345, trace_path=trace_source)
        return route_proofs

    def test_uses_hash_bound_player_step_traces_for_each_floor(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            traces = {index: [0x80, 0x80, 0x20, 0x20] for index in (0, 1, 2)}
            self.make_physical_traces(campaign, root, traces)
            output = root / 'campaign.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            self.assertIn('route_sources=player_step:0,player_step:1,player_step:2', process.stdout)
            data = output.read_bytes()
            _, count, _ = struct.unpack_from('<4sHH', data)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8) for i in range(count)]
            timeline = [button for button, ticks, _ in steps for _ in range(ticks)]
            needle = [0x80, 0x80, 0x20, 0x20]
            self.assertEqual(sum(timeline[i:i + len(needle)] == needle
                                 for i in range(len(timeline) - len(needle) + 1)), 3)

    def test_accepts_physical_campaign_replay_with_more_than_128_steps(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            masks = [0x80 if index % 2 else 0x20 for index in range(300)]
            self.make_physical_traces(campaign, root, {0: masks, 1: masks, 2: masks})
            output = root / 'campaign.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            _, count, _ = struct.unpack_from('<4sHH', output.read_bytes())
            self.assertGreater(count, 512)
            self.assertLessEqual(count, 1024)

    def test_rejects_trace_tampered_after_route_proof(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            route_proofs = self.make_physical_traces(
                campaign, root, {index: [0x80, 0x20] for index in (0, 1, 2)})
            trace = route_proofs / 'floor-01.vtr'
            trace.write_bytes(trace.read_bytes()[:-1])
            process = subprocess.run([sys.executable, str(SCRIPT), str(root / 'campaign.vts'),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertIn('VTR1', process.stderr)

    def test_route_buttons_match_vita_dpad_masks(self):
        steps = []
        prepare_smoke_replay.append_route(steps, [[2, 2], [1, 2], [1, 3], [1, 2]])
        self.assertEqual(steps, [(0x80, 10), (0x4040, 4), (0, 10), (0x10, 10)])

    def test_rejects_missing_campaign_floor(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            (campaign / 'floors' / 'floor-02-layout.json').unlink()
            process = subprocess.run([sys.executable, str(SCRIPT), str(root / 'bad.vts'),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertFalse((root / 'bad.vts').exists())

    def test_rejects_disconnected_route(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            route = {'entrance': [1, 2], 'goal': [3, 2],
                     'native_tile_path': {'route_tiles': [[1, 2], [3, 2]]}}
            (campaign / 'act-one-layout.json').write_text(json.dumps(route), encoding='utf-8')
            process = subprocess.run([sys.executable, str(SCRIPT), str(root / 'bad.vts'),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertFalse((root / 'bad.vts').exists())


if __name__ == '__main__':
    unittest.main()


