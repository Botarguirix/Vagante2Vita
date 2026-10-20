import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

CHECKER = Path(__file__).resolve().parents[1] / 'tools' / 'check_vita3k_smoke.py'


class Vita3KActOneSmokeTests(unittest.TestCase):
    def check_log(self, log_text, emulator_text=None, harness_stop=False, campaign=False, route_sources=None,
                  iteration='032', act_one_class=0):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            log, report = root / 'runtime.log', root / 'result.json'
            log.write_text(log_text, encoding='utf-8')
            command = [
                sys.executable, str(CHECKER), str(log),
                '--output', str(report),
                '--expected-iteration', iteration
            ]
            command.append('--require-act-one-campaign' if campaign else '--require-act-one-exit')
            command.extend(['--expected-act-one-class', str(act_one_class)])
            if route_sources:
                command.extend(['--route-sources', route_sources])
            command.extend([
            ])
            if emulator_text is not None:
                emulator_log = root / 'vita3k.log'
                emulator_log.write_text(emulator_text, encoding='utf-8')
                command.extend(['--emulator-log', str(emulator_log)])
            if harness_stop:
                command.extend(['--allow-harness-stop', '--process-exit-code', '-1'])
            process = subprocess.run(command, capture_output=True, text=True)
            return process.returncode, json.loads(report.read_text(encoding='utf-8'))

    def sample_log(self):
        return '''BOOT version=00.32 iteration=032
AUTOTEST enabled=1
LOAD complete=1
WORLD_SOURCE directory=ux0:data/Vagante2Vita/world/act-one ready=1
WORLD ready=1 width=80 height=40
WORLD_ENTITIES ready=1 format=VWE2 count=44 boulders=1 behavior_pending=0
WORLD_ENEMIES ready=1 count=8 bosses=1 boss_behavior=provisional_combat_model parity=provisional
WORLD_START complete=1 class=0 hp=100
PLAYER_EVENT tick=4 motion=WALK input=1
PLAYER_EVENT tick=8 motion=RISE input=2
WORLD_BOSS_DEFEAT count=1 defeated_mask=2 reward_chests=1 reward_model=provisional_floor_plus_3 native_parity=0
WORLD_EXIT reached=1 tick=582 x=588.0 y=452.0 goal=36,28 next_floor_pending=1
WORLD_FLOOR_READY floor_index=1 source=ux0:data/Vagante2Vita/world/act-one/floors/floor-01.vwi entities=29 hp=100
WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=1
AUTOTEST act_one_exit_sequence_consumed=1
EXIT normal=1 screen=WORLD display_error=0
'''

    def test_intro_entry_smoke_requires_cave_transition_and_first_floor_exit(self):
        log = '''BOOT version=01.61 iteration=161
AUTOTEST enabled=1
AUTOTEST playback=fixed_step catch_up=disabled start_intro=1
LOAD complete=1
WORLD_SOURCE directory=ux0:data/Vagante2Vita/world ready=1
WORLD ready=1 width=40 height=24
WORLD_START complete=1 class=1 hp=80
WORLD_EXIT reached=1 tick=38 x=281.5 y=324.0 goal=18,20 next_floor_pending=1
WORLD ready=1 width=80 height=40
WORLD_FLOOR_READY floor_index=0 source=ux0:data/Vagante2Vita/world/act-one/act-one-world.vwi entities=44 hp=100
WORLD_TRANSITION complete=1 source=intro target=act-one mode=first-floor floor_index=0
WORLD_BOSS_DEFEAT count=1 defeated_mask=2 reward_chests=1 reward_model=provisional_floor_plus_3 native_parity=0
WORLD_EXIT reached=1 tick=462 x=588.0 y=452.0 goal=36,28 next_floor_pending=1
WORLD_FLOOR_READY floor_index=1 source=ux0:data/Vagante2Vita/world/act-one/floors/floor-01.vwi entities=29 hp=100
WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=1
AUTOTEST intro_entry_sequence_consumed=1
AUTOTEST sequence_consumed=1 final_screen=WORLD assertions_pending_on_host=1
EXIT normal=1 screen=WORLD display_error=0
'''
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            runtime, report = root / 'runtime.log', root / 'result.json'
            runtime.write_text(log, encoding='utf-8')
            process = subprocess.run([sys.executable, str(CHECKER), str(runtime),
                                      '--output', str(report), '--expected-iteration', '161',
                                      '--require-intro-entry','--expected-intro-entry-class','1'], capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stdout + process.stderr)
            result = json.loads(report.read_text(encoding='utf-8'))
            self.assertTrue(result['passed'])
            self.assertTrue(result['checks']['first_floor_transition_completed'])
            self.assertTrue(result['checks']['first_floor_boss_defeated'])


    def campaign_log(self):
        return self.sample_log().replace(
            'WORLD ready=1 width=80 height=40\nWORLD_ENTITIES',
            'WORLD ready=1 width=80 height=40\nWORLD_ENTITIES', 1
        ).replace(
            'AUTOTEST act_one_exit_sequence_consumed=1\n',
            'WORLD ready=1 width=80 height=40\n'
            'WORLD_ENTITIES ready=1 format=VWE2 count=25 boulders=1 behavior_pending=0\n'
            'WORLD_ENEMIES ready=1 count=8 parity=provisional\n'
            'WORLD_FLOOR_READY floor_index=2 source=ux0:data/Vagante2Vita/world/act-one/floors/floor-02.vwi entities=25 hp=100\n'
            'WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=2\n'
            'WORLD_BOSS_DEFEAT count=1 defeated_mask=7 reward_chests=1 reward_model=provisional_floor_plus_3 native_parity=0\n'
            'ACT_ONE_FLOORS complete=1 count=3 boss_defeated=1 defeated_mask=7 campaign_complete=1\n'
            'AUTOTEST act_one_exit_sequence_consumed=1\n'
            'AUTOTEST act_one_campaign_sequence_consumed=1\n'
        ).replace(
            'WORLD_FLOOR_READY floor_index=1',
            'WORLD ready=1 width=80 height=40\n'
            'WORLD_ENTITIES ready=1 format=VWE2 count=29 boulders=0 behavior_pending=0\n'
            'WORLD_ENEMIES ready=1 count=7 parity=provisional\n'
            'WORLD_FLOOR_READY floor_index=1'
        ).replace(
            'WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=1\n',
            'WORLD_BOSS_DEFEAT count=1 defeated_mask=2 reward_chests=0 reward_model=provisional_floor_plus_3 native_parity=0\n'
            'WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=1\n'
        ).replace(
            'EXIT normal=1 screen=WORLD display_error=0',
            'EXIT normal=1 screen=ACT_ONE_COMPLETE display_error=0'
        )

    def test_campaign_smoke_requires_both_transitions_and_clear(self):
        code, report = self.check_log(self.campaign_log(), campaign=True)
        self.assertEqual(code, 0, report)
        self.assertTrue(report['checks']['floor_one_transition'])
        self.assertTrue(report['checks']['intro_dragon_defeated_before_floor_one'])
        self.assertTrue(report['checks']['floor_two_transition'])
        self.assertTrue(report['checks']['goblin_king_defeated_before_campaign_completion'])
        self.assertTrue(report['checks']['three_floors_and_boss_roster_completed'])
        self.assertTrue(report['checks']['campaign_entities_loaded'])
        self.assertTrue(report['checks']['campaign_enemies_loaded'])

    def test_campaign_report_preserves_per_floor_route_source(self):
        code, report = self.check_log(self.campaign_log(), campaign=True,
                                      route_sources='player_step:0,tile-estimate:1,player_step:2')
        self.assertEqual(code, 0, report)
        self.assertEqual(report['route_sources'],
                         'player_step:0,tile-estimate:1,player_step:2')

    def test_campaign_smoke_rejects_missing_floor_two_enemy_packet(self):
        log = self.campaign_log().replace(
            'WORLD_ENEMIES ready=1 count=8 parity=provisional\n'
            'WORLD_FLOOR_READY floor_index=2',
            'WORLD_FLOOR_READY floor_index=2')
        code, report = self.check_log(log, campaign=True)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['campaign_enemies_loaded'])

    def test_campaign_smoke_rejects_missing_second_floor_clear(self):
        log = self.campaign_log().replace(
            'ACT_ONE_FLOORS complete=1 count=3 boss_defeated=1 defeated_mask=7 campaign_complete=1\n', '')
        code, report = self.check_log(log, campaign=True)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['three_floors_and_boss_roster_completed'])

    def test_campaign_smoke_rejects_completion_without_boss_defeat(self):
        log = self.campaign_log().replace(
            'WORLD_BOSS_DEFEAT count=1 defeated_mask=7 reward_chests=1 reward_model=provisional_floor_plus_3 native_parity=0\n', '')
        code, report = self.check_log(log, campaign=True)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['goblin_king_defeated_before_campaign_completion'])

    def test_campaign_smoke_rejects_incomplete_boss_roster(self):
        log = self.campaign_log().replace('defeated_mask=7', 'defeated_mask=1')
        code, report = self.check_log(log, campaign=True)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['goblin_king_defeated_before_campaign_completion'])
        self.assertFalse(report['checks']['three_floors_and_boss_roster_completed'])

    def test_requires_entity_load_and_exit_contact(self):
        code, report = self.check_log(self.sample_log())
        self.assertEqual(code, 0)
        self.assertTrue(report['checks']['act_one_entities_loaded'])
        self.assertTrue(report['checks']['goal_reached'])
        self.assertTrue(report['checks']['floor_zero_boss_defeated'])
        self.assertTrue(report['checks']['provisional_enemies_loaded'])
        self.assertTrue(report['checks']['next_floor_loaded'])
        self.assertTrue(report['checks']['next_floor_transition'])
        self.assertNotIn('three_class_attack_geometry', report['checks'])

    def test_act_one_exit_accepts_selected_rogue_class(self):
        log = self.sample_log().replace('WORLD_START complete=1 class=0 hp=100',
                                        'WORLD_START complete=1 class=1 hp=80')
        code, report = self.check_log(log, act_one_class=1)
        self.assertEqual(code, 0, report)
        self.assertTrue(report['checks']['world_started'])

    def test_rejects_first_floor_exit_without_dragon_defeat(self):
        log = self.sample_log().replace(
            'WORLD_BOSS_DEFEAT count=1 defeated_mask=2 reward_chests=1 reward_model=provisional_floor_plus_3 native_parity=0\n', '')
        code, report = self.check_log(log)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['floor_zero_boss_defeated'])

    def test_rejects_missing_act_one_entity_packet(self):
        log = self.sample_log().replace(
            'WORLD_ENTITIES ready=1 format=VWE2 count=44 boulders=1 behavior_pending=0\n', '')
        code, report = self.check_log(log)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['act_one_entities_loaded'])

    def test_rejects_replay_log_from_a_different_iteration(self):
        log = self.sample_log().replace('BOOT version=00.32 iteration=032',
                                        'BOOT version=00.31 iteration=031')
        code, report = self.check_log(log)
        self.assertEqual(code, 1)
        self.assertFalse(report['checks']['native_log_matches_expected_iteration'])

    def test_rejects_emulator_log_that_loaded_a_stale_title_version(self):
        emulator = '''[I] Booting game 'VAGV00006'
[I] Version: 0.25
[I] The app exited with code 0
'''
        code, report = self.check_log(self.sample_log(), emulator)
        self.assertEqual(code, 1)
        self.assertTrue(report['checks']['native_log_matches_expected_iteration'])
        self.assertFalse(report['checks']['emulator_loaded_expected_title_version'])

    def test_accepts_only_matching_native_and_emulator_title_versions(self):
        emulator = '''[I] Booting game 'VAGV00006'
[I] Version: 0.32
[I] The app exited with code 0
'''
        code, report = self.check_log(self.sample_log(), emulator)
        self.assertEqual(code, 0)
        self.assertTrue(report['checks']['native_log_matches_expected_iteration'])
        self.assertTrue(report['checks']['emulator_loaded_expected_title_version'])

    def test_accepts_valid_replay_then_controlled_emulator_close(self):
        emulator = '''[I] Booting game 'VAGV00006'
[I] Version: 0.32
[I] Game closed: Vagante2Vita
'''
        code, report = self.check_log(self.sample_log(), emulator, harness_stop=True)
        self.assertEqual(code, 0)
        self.assertTrue(report['harness_stopped_after_replay'])
        self.assertTrue(report['emulator_shutdown_clean'])

    def test_uses_one_major_version_for_iterations_after_099(self):
        log = self.sample_log().replace('BOOT version=00.32 iteration=032',
                                        'BOOT version=01.09 iteration=109')
        emulator = '''[I] Booting game 'VAGV00006'
[I] Version: 1.09
[I] Game closed: Vagante2Vita
'''
        code, report = self.check_log(log, emulator, iteration='109')
        self.assertEqual(code, 0, report)
        self.assertTrue(report['checks']['native_log_matches_expected_iteration'])
        self.assertTrue(report['checks']['emulator_loaded_expected_title_version'])


if __name__ == '__main__':
    unittest.main()
