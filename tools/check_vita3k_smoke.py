"""Check native VPK execution logs; this never certifies physical Vita FPS."""
import argparse,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('log',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--emulator-log',type=Path);p.add_argument('--process-exit-code',type=int);a=p.parse_args()
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
cls=None;attacks=set()
for line in lines:
 m=re.search(r'WORLD_START complete=1 class=(\d)',line)
 if m:cls=int(m[1])
 if line.startswith('MELEE_EVENT ') and 'serial=1 ' in line and 'shape_active=1' in line:attacks.add(cls)
checks['three_class_attack_geometry']=attacks=={0,1,2}
emulator_clean=None
if a.emulator_log:
 emu=a.emulator_log.read_text(encoding='utf-8',errors='replace')
 emulator_clean=('The app exited with code 0' in emu and not any(x in emu for x in ('|C|','EXCEPTION_ACCESS_VIOLATION','Exception EXCEPTION')))
report=dict(passed=all(checks.values()) and emulator_clean is not False and a.process_exit_code in (None,0),emulator_process_exit_code=a.process_exit_code, native_game_checks_passed=all(checks.values()),emulator_shutdown_clean=emulator_clean,environment='Vita3K native VPK',input_source='external replay',physical_controller_verified=False,physical_vita_performance_verified=False,checks=checks)
a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,indent=2));raise SystemExit(0 if report['passed'] else 1)
