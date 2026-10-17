"""Check native VPK execution logs; this never certifies physical Vita FPS."""
import argparse,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('log',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--emulator-log',type=Path);p.add_argument('--process-exit-code',type=int);p.add_argument('--expected-iteration',type=str);p.add_argument('--route-sources',type=str);p.add_argument('--allow-harness-stop',action='store_true');p.add_argument('--require-act-one',action='store_true');p.add_argument('--require-act-one-exit',action='store_true');p.add_argument('--require-act-one-campaign',action='store_true');a=p.parse_args()
s=a.log.read_text(encoding='utf-8',errors='replace');lines=s.splitlines()
checks={
 'external_replay_enabled':'AUTOTEST enabled=1' in s,
 'archive_loaded':'LOAD complete=1' in s,
 'sequence_consumed':'AUTOTEST sequence_consumed=1' in s,
 'normal_world_exit':'EXIT normal=1 screen=WORLD display_error=0' in s,
 'world_data_loaded':s.count('WORLD ready=1')>=3,
 'three_classes':all(f'WORLD_START complete=1 class={c} ' in s for c in (0,1,2)),
 'base_hp':all(any(f'WORLD_START complete=1 class={c} ' in line and f'hp={hp}' in line for line in lines) for c,hp in ((0,100),(1,80),(2,60))),
 'movement':bool(re.search(r'PLAYER_EVENT .*motion=WALK .*input=',s)),
 'jump':bool(re.search(r'PLAYER_EVENT .*motion=RISE ',s)),
 'sheet_open_close':'CHARACTER_PANEL open=1' in s and 'CHARACTER_PANEL open=0' in s,
 'sheet_three_pages':all(f'page={n} ' in s for n in (0,1,2)),
 'pause_resume':'PAUSE active=1' in s and 'PAUSE active=0' in s,
 'rogue_arrow':bool(re.search(r'BOW_EVENT .*fired=1',s)),
 'no_load_failure':not any(x in s for x in ('WORLD_START complete=0','ARCHIVE error=','WORLD error=','MELEE_BLOCKED','BOW_BLOCKED','ERROR display'))
}
if a.require_act_one_exit and a.require_act_one_campaign:
 p.error('choose only one ACT_ONE smoke mode')
if a.require_act_one_exit:
 checks={
  'external_replay_enabled':'AUTOTEST enabled=1' in s,
  'archive_loaded':'LOAD complete=1' in s,
  'act_one_world_selected':'WORLD_SOURCE directory=ux0:data/Vagante2Vita/world/act-one ready=1' in s,
  'generated_act_one_extent':'WORLD ready=1 width=80 height=40' in s,
  'act_one_entities_loaded':bool(re.search(r'WORLD_ENTITIES ready=1 format=VWE2 count=[1-9]\d* boulders=\d+ behavior_pending=0',s)),
  'provisional_enemies_loaded':bool(re.search(r'WORLD_ENEMIES ready=1 count=[1-9]\d* parity=provisional',s)),
  'world_started':'WORLD_START complete=1 class=0 ' in s,
  'player_moved':bool(re.search(r'PLAYER_EVENT .*motion=WALK .*input=',s)),
  'player_jumped':bool(re.search(r'PLAYER_EVENT .*motion=RISE ',s)),
  'goal_reached':bool(re.search(r'WORLD_EXIT reached=1 tick=\d+ .*goal=\d+,\d+ next_floor_pending=1',s)),
  'next_floor_loaded':bool(re.search(r'WORLD_FLOOR_READY floor_index=1 source=.*floors/floor-01\.vwi',s)),
  'next_floor_transition':'WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=1' in s,
  'test_stopped_on_exit':'AUTOTEST act_one_exit_sequence_consumed=1' in s,
  'clean_runtime_exit':'EXIT normal=1 screen=WORLD display_error=0' in s,
  'no_load_failure':not any(x in s for x in ('WORLD_START complete=0','ARCHIVE error=','WORLD error=','ERROR display'))
 }
if a.require_act_one_campaign:
 checks={
  'external_replay_enabled':'AUTOTEST enabled=1' in s,
  'archive_loaded':'LOAD complete=1' in s,
  'act_one_world_selected':'WORLD_SOURCE directory=ux0:data/Vagante2Vita/world/act-one ready=1' in s,
  'all_three_floor_worlds':s.count('WORLD ready=1 width=80 height=40')>=3,
  'campaign_entities_loaded':len(re.findall(r'WORLD_ENTITIES ready=1 format=VWE2 count=[1-9]\d* boulders=\d+ behavior_pending=0',s))>=3,
  'campaign_enemies_loaded':len(re.findall(r'WORLD_ENEMIES ready=1 count=[1-9]\d* parity=provisional',s))>=3,
  'floor_one_loaded':bool(re.search(r'WORLD_FLOOR_READY floor_index=1 source=.*floors/floor-01\.vwi',s)),
  'floor_two_loaded':bool(re.search(r'WORLD_FLOOR_READY floor_index=2 source=.*floors/floor-02\.vwi',s)),
  'floor_one_transition':'WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=1' in s,
  'floor_two_transition':'WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=2' in s,
  'dungeon_cleared':'ACT_ONE_DUNGEON cleared=1 floors=3 boss_transition_pending=1' in s,
  'campaign_replay_consumed':'AUTOTEST act_one_campaign_sequence_consumed=1' in s,
  'clean_runtime_exit':'EXIT normal=1 screen=CLASS_PREVIEW display_error=0' in s,
  'no_load_failure':not any(x in s for x in ('WORLD_START complete=0','ARCHIVE error=','WORLD error=','ERROR display'))
 }
cls=None;attacks=set()
if not a.require_act_one_exit and not a.require_act_one_campaign:
 for line in lines:
  m=re.search(r'WORLD_START complete=1 class=(\d)',line)
  if m:cls=int(m[1])
  if line.startswith('MELEE_EVENT ') and 'serial=1 ' in line and 'shape_active=1' in line:attacks.add(cls)
 checks['three_class_attack_geometry']=attacks=={0,1,2}
if a.require_act_one and not a.require_act_one_exit and not a.require_act_one_campaign:
 checks['generated_act_one_selected']=s.count('WORLD_SOURCE directory=ux0:data/Vagante2Vita/world/act-one ready=1')>=3
 checks['generated_act_one_extent']=s.count('WORLD ready=1 width=80 height=40')>=3
emulator_clean=None
emulator_version_matches=None
if a.emulator_log:
 emu=a.emulator_log.read_text(encoding='utf-8',errors='replace')
 shutdown_recorded=any(marker in emu for marker in ('The app exited with code 0','Game closed: Vagante2Vita'))
 emulator_crashed=any(x in emu for x in ('|C|','EXCEPTION_ACCESS_VIOLATION','Exception EXCEPTION'))
 emulator_clean=(shutdown_recorded and not emulator_crashed) if shutdown_recorded or emulator_crashed else None
 if a.expected_iteration:
  expected_title_version=f"0.{a.expected_iteration[1:]}"
  emulator_version_matches=bool(re.search(rf'(?m)^.*Version: {re.escape(expected_title_version)}\s*$',emu))
  checks['emulator_loaded_expected_title_version']=emulator_version_matches
if a.expected_iteration:
 expected_native_version=f"00.{a.expected_iteration[1:]}"
 checks['native_log_matches_expected_iteration']=bool(re.search(rf'(?m)^BOOT version={re.escape(expected_native_version)} iteration={re.escape(a.expected_iteration)}\b',s))
process_ok=a.process_exit_code in (None,0) or a.allow_harness_stop
report=dict(passed=all(checks.values()) and emulator_clean is not False and process_ok,emulator_process_exit_code=a.process_exit_code,native_game_checks_passed=all(checks.values()),emulator_shutdown_clean=emulator_clean,harness_stopped_after_replay=a.allow_harness_stop,emulator_title_version_matches=emulator_version_matches,expected_iteration=a.expected_iteration,environment='Vita3K native VPK',input_source='external replay',route_sources=a.route_sources,physical_controller_verified=False,physical_vita_performance_verified=False,act_one_exit_smoke=a.require_act_one_exit,act_one_campaign_smoke=a.require_act_one_campaign,checks=checks)
a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,indent=2));raise SystemExit(0 if report['passed'] else 1)
