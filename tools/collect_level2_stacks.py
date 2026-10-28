"""Two read-only GDB thread snapshots, with the inferior resumed for the 10 s gap."""
import argparse,datetime,json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
GDB=Path(r'C:\msys64\ucrt64\bin\gdb.exe')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pid',type=int,required=True);p.add_argument('--out',type=Path,required=True)
 a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
 env=dict(os.environ,PATH=str(GDB.parent)+os.pathsep+os.environ.get('PATH',''))
 records=[]
 for n in (1,2):
  if n==2:time.sleep(10)
  command=[str(GDB),'-nx','-p',str(a.pid),'-batch','-ex','set pagination off','-ex','info threads','-ex','thread apply all bt 30','-ex','detach']
  start=time.monotonic();stamp=datetime.datetime.now(datetime.timezone.utc).isoformat()
  try:
   result=subprocess.run(command,env=env,capture_output=True,text=True,errors='replace',timeout=45)
   output=result.stdout+'\n'+result.stderr;code=result.returncode
  except subprocess.TimeoutExpired as error:
   output='GDB TIMEOUT\n'+str(error.stdout)+'\n'+str(error.stderr);code=-1
  raw=a.out/f'threads-{n}.txt';raw.write_text(output,encoding='utf-8')
  subprocess.run(['python','-I',str(ROOT/'tools/resolve_guest_bt.py'),str(raw),'--out',str(a.out/f'threads-{n}-resolved.txt')],env=env,check=True)
  records.append({'number':n,'utc':stamp,'monotonic_start':start,'monotonic_finish':time.monotonic(),'returncode':code,'command':command})
  print(f'snapshot {n} exit={code} saved {raw}',flush=True)
 (a.out/'capture.json').write_text(json.dumps({'pid':a.pid,'gap_after_first_s':records[1]['monotonic_start']-records[0]['monotonic_finish'],'samples':records},indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
