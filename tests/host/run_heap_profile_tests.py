"""Independent heap-profile invariants using the real TLSF, no game files."""
from pathlib import Path
import os, subprocess, re
ROOT=Path(__file__).resolve().parents[2]
cc=Path(r'C:\msys64\ucrt64\bin\gcc.exe')
out=ROOT/'artifacts/heap-profile-tests';out.mkdir(parents=True,exist_ok=True)
exe=out/'profile.exe'
env=dict(os.environ,PATH=str(cc.parent)+os.pathsep+os.environ.get('PATH',''),RECOMP_HEAP_PROFILE='1')
subprocess.run([str(cc),'-std=gnu11','-O2','-Wall','-Wextra','-Wno-unused-parameter','-DGUEST_WINDOW=0x11800000u',
 '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'recomp/runtime'),'-I'+str(ROOT/'recomp/host'),
 str(ROOT/'tests/host/heap_profile_test.c'),str(ROOT/'recomp/host/support.c'),'-Wl,--gc-sections','-lpthread','-o',str(exe)],env=env,check=True)
subprocess.run([str(exe)],cwd=out,env=env,check=True,timeout=30)
s=(out/'recomp-trace.log').read_text()
parts=re.findall(r'# HEAP_PROFILE BEGIN (.*?)# HEAP_PROFILE END[^\n]*',s,re.S)
assert len(parts)==5,len(parts)
assert 'addr=11111111 bytes=8096 requested=8096 blocks=2' in parts[0]
assert 'addr=22222222 bytes=1114112 requested=1114112 blocks=2' in parts[0]
for i,size in enumerate((4000,4096,65536,1048576)):
 assert f'bucket={i} bytes={size} blocks=1' in parts[0]
assert 'addr=33333333 bytes=2000000 requested=2000000 blocks=1' in parts[1]
assert 'live=0' in parts[2] and 'groups=' not in parts[2]
assert 'reason=allocation-failed' in parts[3]
for i in range(4):
 assert f'addr={0x44440000+i:08x} bytes={64*(128+16*i)} requested={64*(128+16*i)} blocks=64' in parts[4]
for p in parts:
 vals={k:int(v) for k,v in re.findall(r'(arena|live|free|headers|complete|lost)=(\d+)',p)}
 assert vals['live']+vals['free']+vals['headers']==vals['arena'],vals
 assert vals['complete']==1 and vals['lost']==0,vals
print('profile accounting: capacity + free + headers = arena; four buckets and all origins exact')

image_exe=out/'images.exe'
subprocess.run([str(cc),'-std=gnu11','-O2','-Wall','-Wextra','-Wno-unused-parameter','-DGUEST_WINDOW=0x11800000u',
 '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'recomp/runtime'),'-I'+str(ROOT/'recomp/host'),
 str(ROOT/'tests/host/heap_profile_image_test.c'),str(ROOT/'recomp/host/support.c'),str(ROOT/'recomp/host/heap_profile.c'),
 '-Wl,--gc-sections','-lpthread','-o',str(image_exe)],env=env,check=True)
subprocess.run([str(image_exe)],cwd=out,env=env,check=True,timeout=30)
