"""Start a private, log-only 280 MiB profiling session. Never captures the desktop."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
SHA='9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--reference',type=Path,default=ROOT.parent/'vagante_ref')
 p.add_argument('--isolated-exe',action='store_true',help='use a private vagante_level2_probe.exe copy to avoid other build/process-name cleanups')
 p.add_argument('--save-from',type=Path,help='copy an existing unmodified savedata.sav into this private session')
 p.add_argument('--mode',choices=['1','2'],default='2')
 a=p.parse_args()
 base=ROOT/'artifacts/recomp'/SHA
 exe=base/'host-280mib/vagante_host.exe'
 if not exe.exists():raise SystemExit('Build with recomp/build_host.py --window-mib 280 first')
 original=a.reference/'vagante.exe';data=a.reference/'data.vra'
 if digest(original)!=SHA:raise SystemExit('Reference executable hash mismatch')
 out=ROOT/'artifacts/heap-profile';out.mkdir(parents=True,exist_ok=True)
 run=Path(tempfile.mkdtemp(prefix='session-',dir=out))
 os.link(data,run/'data.vra')
 for name in ('config','custom'):
  source=base/'run'/name
  if source.exists():shutil.copytree(source,run/name)
 env=dict(os.environ,RECOMP_HEAP_PROFILE=a.mode,ALSOFT_DRIVERS='null')
 for name in ('RECOMP_NO_PALETTES','RECOMP_EAGER_IMAGES'):env.pop(name,None)
 if a.save_from:
  if not a.save_from.is_file():raise SystemExit('savedata source not found')
  shutil.copyfile(a.save_from,run/'savedata.sav')
 if a.isolated_exe:
  private=run/'host-280mib/vagante_level2_probe.exe';private.parent.mkdir()
  shutil.copy2(exe,private);exe=private
 command=[str(exe),str(original),str(run)]
 with (run/'stdout.txt').open('w') as stdout,(run/'stderr.txt').open('w') as stderr:
  child=subprocess.Popen(command,cwd=run,env=env,stdout=stdout,stderr=stderr)
 record={'pid':child.pid,'command':command,'cwd':str(run),'window_mib':280,
  'mode':a.mode,'openal_driver':'null','exe_sha256':digest(exe),'guest_sha256':SHA,
  'data_sha256':digest(data),'routing':json.loads((base/'host-280mib/profile-routing.json').read_text())}
 if a.save_from:record['savedata_source']={'path':str(a.save_from),'sha256':digest(a.save_from)}
 (run/'session.json').write_text(json.dumps(record,indent=2)+'\n')
 (out/'latest-session.txt').write_text(str(run))
 print(json.dumps(record,indent=2),flush=True)
if __name__=='__main__':main()
