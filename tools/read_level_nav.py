"""Read-only GDB game-state log for navigation when screenshots are forbidden."""
import argparse,json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pid',type=int,required=True);p.add_argument('--engine',default='0x003045c8');p.add_argument('--out',type=Path,required=True)
 a=p.parse_args();gdb=Path(r'C:\msys64\ucrt64\bin\gdb.exe');script=(ROOT/'tools/level_nav_gdb.py').as_posix()
 command=[str(gdb),'-nx','-p',str(a.pid),'-batch','-ex','set pagination off','-ex',f'set $profile_engine={int(a.engine,0):#x}','-ex',f'python exec(open({script!r},encoding="utf-8").read())','-ex','detach']
 t=time.monotonic();r=subprocess.run(command,env=dict(os.environ,PATH=str(gdb.parent)+os.pathsep+os.environ.get('PATH','')),capture_output=True,text=True,errors='replace',timeout=45)
 a.out.parent.mkdir(parents=True,exist_ok=True);a.out.with_suffix('.txt').write_text(r.stdout+'\n'+r.stderr,encoding='utf-8')
 lines=[l[15:] for l in r.stdout.splitlines() if l.startswith('NAV_STATE_JSON ')]
 if not lines:raise SystemExit(r.stderr+'\nNo NAV_STATE_JSON produced')
 state=json.loads(lines[0]);state['gdb_elapsed_s']=time.monotonic()-t;a.out.with_suffix('.json').write_text(json.dumps(state),encoding='utf-8')
 print(json.dumps({k:v for k,v in state.items() if k not in ('entities','tiles')},indent=2));print('chickens:',[e for e in state['entities'] if e['type']==106])
if __name__=='__main__':main()
