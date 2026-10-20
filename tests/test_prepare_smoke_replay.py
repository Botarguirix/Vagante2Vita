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
            self.assertLessEqual(count, prepare_smoke_replay.REPLAY_LIMIT)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8) for i in range(count)]
            self.assertTrue(all(1 <= ticks <= 600 and padding == 0
                                for _, ticks, padding in steps))
            self.assertLessEqual(sum(ticks for _, ticks, _ in steps), 36000)
            self.assertEqual(steps.count((0, 60, 0)), 0)
            self.assertIn('floors=3', process.stdout)

    def make_physical_traces(self, campaign, root, masks_by_floor, player_class=None):
        route_proofs = campaign / 'route-proofs'
        route_proofs.mkdir(parents=True, exist_ok=True)
        floor_files = {
            0: (campaign / 'act-one-world.vwi', campaign / 'act-one-entities.vwe',
                campaign / 'act-one-enemies.vwe'),
            1: (campaign / 'floors/floor-01.vwi', campaign / 'floors/floor-01-entities.vwe',
                campaign / 'floors/floor-01-enemies.vwe'),
            2: (campaign / 'floors/floor-02.vwi', campaign / 'floors/floor-02-entities.vwe',
                campaign / 'floors/floor-02-enemies.vwe'),
        }
        for index, (world, entities, enemies) in floor_files.items():
            world.parent.mkdir(parents=True, exist_ok=True)
            world.write_bytes(f'VWI2 world {index}'.encode())
            entities.write_bytes(f'VWE2 entities {index}'.encode())
            enemies.write_bytes(f'VWE3 enemies {index}'.encode())
            masks = masks_by_floor[index]
            trace_source = root / f'floor-{index:02d}-input.vtr'
            trace_source.write_bytes(struct.pack('<4sI', b'VTR1', len(masks))
                                     + b''.join(struct.pack('<H', mask) for mask in masks))
            log = root / f'floor-{index:02d}.log'
            class_line = f'ACT_ONE_PLAYER class={player_class} hp=80/80 bow=1 arrows=30\n' if player_class is not None else ''
            log.write_text(class_line + f'ACT_ONE_ROUTE seed=12345 tiles=3 waypoint=2 ticks={len(masks)} '
                           'jumps=0 reached_exit=1', encoding='utf-8')
            record(log, world, entities, route_proofs / f'floor-{index:02d}.json',
                   12345, trace_path=trace_source, enemies=enemies)
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

            rogue_output = root / 'campaign-rogue.vts'
            rogue_process = subprocess.run(
                [sys.executable, str(SCRIPT), str(rogue_output),
                 '--act-one-campaign-exit', '--campaign', str(campaign),
                 '--player-class', '1'], capture_output=True, text=True)
            self.assertEqual(rogue_process.returncode, 0, rogue_process.stderr)
            rogue_data = rogue_output.read_bytes()
            _, rogue_count, _ = struct.unpack_from('<4sHH', rogue_data)
            rogue_steps = [struct.unpack_from('<IHH', rogue_data, 8 + i * 8)
                           for i in range(rogue_count)]
            self.assertEqual(rogue_steps[2], (0x20, 6, 0))
            self.assertEqual(rogue_steps[4], (0x4000, 6, 0))
            # Class acceptance leads directly into the recorded world trace;
            # there are no neutral world ticks to shift the physics timeline.
            self.assertEqual(rogue_steps[4][1], 6)
            self.assertEqual(rogue_steps[5], (0x80, 2, 0))

    def test_intro_cave_entry_replay_forces_intro_then_uses_floor_zero_trace(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            traces = {index: [0x80, 0x80, 0x20, 0x20] for index in (0, 1, 2)}
            self.make_physical_traces(campaign, root, traces)
            intro_trace = root / 'intro.vtr'
            intro_trace.write_bytes(struct.pack('<4sI4H', b'VTR1', 4, 0x20, 0x20, 0x20, 0))
            output = root / 'intro-entry.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--intro-cave-entry', '--campaign', str(campaign),
                                      '--intro-trace', str(intro_trace)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            data = output.read_bytes()
            magic, count, flags = struct.unpack_from('<4sHH', data)
            self.assertEqual((magic, flags), (b'VTS1', 1))
            self.assertIn('route_sources=intro:player_step,act-one:player_step', process.stdout)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8) for i in range(count)]
            timeline = [button for button, ticks, _ in steps for _ in range(ticks)]
            self.assertEqual(timeline[:6], [0x4000] * 6)
            self.assertEqual(timeline[6:116], [0] * 110)
            self.assertEqual(timeline[116:122], [0x4000] * 6)
            self.assertEqual(timeline[122:152], [0] * 30)
            self.assertEqual(timeline[152:156], [0x20, 0x20, 0x20, 0])
            self.assertEqual(timeline[-4:], traces[0])

    def test_fast_forward_sets_an_independent_replay_flag(self):
        with tempfile.TemporaryDirectory() as td:
            output = Path(td) / 'fast-forward.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--fast-forward'], capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            magic, _, flags = struct.unpack_from('<4sHH', output.read_bytes())
            self.assertEqual((magic, flags), (b'VTS1', 4))

    def test_intro_entry_replays_class_accept_jump_before_floor_route(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            traces = {index: [0x80, 0x20] for index in (0, 1, 2)}
            self.make_physical_traces(campaign, root, traces)
            intro_trace = root / 'intro.vtr'
            intro_trace.write_bytes(struct.pack('<4sI2H', b'VTR1', 2, 0x20, 0x20))
            output = root / 'intro-jump.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--intro-cave-entry', '--campaign', str(campaign),
                                      '--intro-trace', str(intro_trace)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            data = output.read_bytes()
            _, count, _ = struct.unpack_from('<4sHH', data)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8) for i in range(count)]
            timeline = [button for button, ticks, _ in steps for _ in range(ticks)]
            self.assertEqual(timeline[-8:], [0x4000] * 6 + traces[0])
            self.assertIn('act_one_entry_jump_ticks=6', process.stdout)
            # The redundant final cave RIGHT is removed before the jump phase.
            self.assertEqual(timeline[-9:-8], [0x20])

    def test_intro_entry_selects_the_class_bound_to_its_floor_trace(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            traces = {index: [0x80, 0x20] for index in (0, 1, 2)}
            self.make_physical_traces(campaign, root, traces, player_class=1)
            intro_trace = root / 'intro.vtr'
            intro_trace.write_bytes(struct.pack('<4sI2H', b'VTR1', 2, 0x20, 0))
            output = root / 'intro-rogue.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--intro-cave-entry', '--campaign', str(campaign),
                                      '--intro-trace', str(intro_trace), '--player-class', '1'],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            data = output.read_bytes()
            _, count, _ = struct.unpack_from('<4sHH', data)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8) for i in range(count)]
            timeline = [button for button, ticks, _ in steps for _ in range(ticks)]
            self.assertEqual(timeline[116:122], [0x20] * 6)
            self.assertEqual(timeline[122:134], [0] * 12)
            self.assertEqual(timeline[134:140], [0x4000] * 6)

    def test_intro_entry_rejects_a_trace_recorded_for_a_different_class(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            traces = {index: [0x80, 0x20] for index in (0, 1, 2)}
            self.make_physical_traces(campaign, root, traces, player_class=1)
            intro_trace = root / 'intro.vtr'
            intro_trace.write_bytes(struct.pack('<4sI2H', b'VTR1', 2, 0x20, 0))
            process = subprocess.run([sys.executable, str(SCRIPT), str(root / 'wrong-class.vts'),
                                      '--intro-cave-entry', '--campaign', str(campaign),
                                      '--intro-trace', str(intro_trace), '--player-class', '0'],
                                     capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertIn('not 0', process.stderr)

    def test_intro_entry_accepts_a_verified_knight_floor_trace_override(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            traces = {index: [0x80, 0x20] for index in (0, 1, 2)}
            self.make_physical_traces(campaign, root, traces)
            intro_trace = root / 'intro.vtr'
            intro_trace.write_bytes(struct.pack('<4sI2H', b'VTR1', 2, 0x20, 0))
            floor_trace = root / 'knight-floor-00.vtr'
            knight_masks = [0x4000, 0x8020, 0x80]
            floor_trace.write_bytes(struct.pack('<4sI', b'VTR1', len(knight_masks))
                                     + b''.join(struct.pack('<H', mask)
                                                for mask in knight_masks))
            output = root / 'intro-knight.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--intro-cave-entry', '--campaign', str(campaign),
                                      '--intro-trace', str(intro_trace),
                                      '--route-trace', str(floor_trace)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            self.assertIn('act_one_floor0_ticks=3', process.stdout)
            data = output.read_bytes()
            _, count, flags = struct.unpack_from('<4sHH', data)
            self.assertEqual(flags, 1)
            steps = [struct.unpack_from('<IHH', data, 8 + i * 8)
                     for i in range(count)]
            timeline = [button for button, ticks, _ in steps for _ in range(ticks)]
            self.assertEqual(timeline[-len(knight_masks):], knight_masks)

    def test_accepts_physical_campaign_replay_with_more_than_128_steps(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            masks = [0x80 if index % 2 else 0x20 for index in range(500)]
            self.make_physical_traces(campaign, root, {0: masks, 1: masks, 2: masks})
            output = root / 'campaign.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--act-one-campaign-exit', '--campaign', str(campaign)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            _, count, _ = struct.unpack_from('<4sHH', output.read_bytes())
            self.assertGreater(count, 512)
            self.assertGreater(count, 1024)
            self.assertLessEqual(count, prepare_smoke_replay.REPLAY_LIMIT)

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

    def test_rejects_v4_route_trace_when_enemy_packet_changes(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            self.make_physical_traces(
                campaign, root, {index: [0x80, 0x20] for index in (0, 1, 2)})
            enemy_packet = campaign / 'floors/floor-01-enemies.vwe'
            enemy_packet.write_bytes(enemy_packet.read_bytes() + b' changed')
            with self.assertRaisesRegex(ValueError, 'different floor enemy data'):
                prepare_smoke_replay.campaign_route_traces(campaign)

    def test_route_buttons_match_vita_dpad_masks(self):
        steps = []
        prepare_smoke_replay.append_route(steps, [[2, 2], [1, 2], [1, 3], [1, 2]])
        self.assertEqual(steps, [(0x80, 10), (0x4040, 4), (0, 10), (0x10, 10)])

    def test_single_act_one_floor_replay_uses_native_trace_and_stops_after_transition(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            campaign = self.make_campaign(root)
            trace = root / 'floor-00.vtr'
            masks = [0x20, 0x4000, 0x8000, 0]
            trace.write_bytes(struct.pack('<4sI', b'VTR1', len(masks))
                              + b''.join(struct.pack('<H', mask) for mask in masks))
            output = root / 'floor-00.vts'
            process = subprocess.run([sys.executable, str(SCRIPT), str(output),
                                      '--act-one-exit', '--layout',
                                      str(campaign / 'act-one-layout.json'),
                                      '--route-trace', str(trace)],
                                     capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            self.assertIn('route_sources=player_step:0', process.stdout)
            magic, count, flags = struct.unpack_from('<4sHH', output.read_bytes())
            self.assertEqual((magic, flags), (b'VTS1', 2))
            steps = [struct.unpack_from('<IHH', output.read_bytes(), 8 + i * 8)
                     for i in range(count)]
            timeline = [button for button, ticks, _ in steps for _ in range(ticks)]
            self.assertEqual(timeline[-len(masks):], masks)
            self.assertLessEqual(count, prepare_smoke_replay.REPLAY_LIMIT)

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


