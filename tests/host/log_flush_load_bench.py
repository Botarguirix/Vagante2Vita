#!/usr/bin/env python3
"""Compare actual ResourceManager loading with/without write-only fflush.

--freeze-baseline: build PC host, preserve its exact objects/headers and link probe.
--candidate: replace ONLY support.o and add fileio.o in the frozen object set.
--run: alternate probes to finishInitialization; private profiles; no UI input.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'artifacts/fileio-load'
spec=importlib.util.spec_from_file_location('host_build',ROOT/'recomp/build_host.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
CC=host.GCC
ENV=host.env()
LIBS=['-static','-lSDL2','-lopenal','-lopengl32','-lsetupapi','-limm32','-lversion','-lwinmm','-lole32','-loleaut32','-lgdi32','-luuid','-luser32','-ladvapi32','-lshell32','-lcfgmgr32','-lavrt','-lstdc++','-lz','-lm','-lpthread']

def invoke(cmd):
    r=subprocess.run([str(x) for x in cmd],cwd=ROOT,env=ENV,capture_output=True,text=True)
    if r.returncode: raise RuntimeError(r.stderr[-12000:])
    return r.stdout

def compile_source(source,obj,headers):
    flags=['-std=gnu11','-O1','-g','-frounding-math','-Wall','-Wextra','-Wno-unused-parameter','-DT123_TRACE_CALLS','-DAL_LIBTYPE_STATIC','-I'+str(headers)]
    invoke([CC,*flags,'-c',source,'-o',obj])

def link(objects,exe):
    rsp=exe.with_suffix('.rsp')
    cmd=[*map(str,objects),'-Wl,--wrap=guest_intercept',*LIBS,'-o',str(exe)]
    rsp.write_text('\n'.join('"'+x.replace('\\','/')+'"' for x in cmd))
    invoke([CC,'@'+str(rsp)])

def freeze():
    WORK.mkdir(parents=True,exist_ok=True)
    deadline=time.monotonic()+600
    gen=ROOT/'artifacts/recomp'/host.SHA/'gen'
    announced=False
    while True:
        units=sorted(gen.glob('unit_*.c'))
        if units:
            stamp=[(p.name,p.stat().st_size,p.stat().st_mtime_ns) for p in units]
            time.sleep(2)
            current=sorted(gen.glob('unit_*.c'))
            try:
                if stamp==[(p.name,p.stat().st_size,p.stat().st_mtime_ns) for p in current]:break
            except FileNotFoundError:pass
        if not announced:print('Waiting for concurrent lift_all.py to finish...',flush=True);announced=True
        if time.monotonic()>deadline:raise RuntimeError('generated C still unavailable')
        time.sleep(2)
    try:
        invoke([os.sys.executable,'-I',ROOT/'recomp/build_host.py','--jobs','4'])
    except RuntimeError as error:
        text=str(error)
        if 'cannot open output file' not in text or 'Permission denied' not in text:raise
        print('Existing host executable is in use; linking isolated baseline from the completed objects.',flush=True)
    folder=Path(tempfile.mkdtemp(prefix='frozen-',dir=WORK))
    gen=ROOT/'artifacts/recomp'/host.SHA/'gen';out=gen.parent/'host'
    objects=[]
    for source in sorted(gen.glob('unit_*.c'))+[gen/'table.c']:
        dest=folder/source.with_suffix('.o').name;shutil.copy2(source.with_suffix('.o'),dest);objects.append(str(dest))
    for source in sorted(host.HOST.glob('*.c'))+[host.RUNTIME/'guest.c']:
        if source.name=='fileio.c':continue
        dest=folder/('host_'+source.stem+'.o');shutil.copy2(out/(source.stem+'.o'),dest);objects.append(str(dest))
    for source in (host.RUNTIME/'guest.h',host.HOST/'win32.h',host.HOST/'support.c'):
        shutil.copy2(source,folder/source.name)
    # Reconstruct the legacy route even when rerun after this change is installed.
    legacy=(folder/'support.c').read_text(encoding='utf-8').replace('{win32_fileio, win32_threads, win32_kernel32,','{win32_threads, win32_kernel32,')
    (folder/'support.c').write_text(legacy,encoding='utf-8')
    compile_source(folder/'support.c',folder/'host_support.o',folder)
    probe=folder/'load_probe.c';shutil.copy2(ROOT/'tests/host/log_flush_load_probe.c',probe)
    compile_source(probe,folder/'probe.o',folder)
    objects.append(str(folder/'probe.o'))
    exe=folder/'before.exe';link(objects,exe)
    record={'folder':str(folder),'objects':objects,'before':str(exe),
            'before_sha256':hashlib.sha256(exe.read_bytes()).hexdigest()}
    (WORK/'manifest.json').write_text(json.dumps(record,indent=2))
    print('frozen baseline',exe,flush=True)

def candidate():
    record=json.loads((WORK/'manifest.json').read_text());folder=Path(record['folder'])
    headers=folder/'candidate';headers.mkdir(exist_ok=True)
    shutil.copy2(folder/'guest.h',headers/'guest.h')
    old_header=(folder/'win32.h').read_text(encoding='utf-8')
    if 'extern const ApiEntry win32_fileio[];' not in old_header:
        old_header=old_header.replace('extern const ApiEntry win32_kernel32[];','extern const ApiEntry win32_fileio[];\nextern const ApiEntry win32_kernel32[];')
    (headers/'win32.h').write_text(old_header,encoding='utf-8')
    old_source=(folder/'support.c').read_text(encoding='utf-8')
    (headers/'support.c').write_text(old_source.replace('{win32_threads, win32_kernel32,','{win32_fileio, win32_threads, win32_kernel32,'),encoding='utf-8')
    shutil.copy2(ROOT/'recomp/host/fileio.c',headers/'fileio.c')
    compile_source(headers/'support.c',headers/'support.o',headers)
    compile_source(headers/'fileio.c',headers/'fileio.o',headers)
    objects=[str(headers/'support.o') if Path(p).name=='host_support.o' else p for p in record['objects']]
    objects.append(str(headers/'fileio.o'))
    exe=folder/'after.exe';link(objects,exe)
    template=folder/'profile-template'
    if not template.exists():
        template.mkdir()
        original=ROOT/'artifacts/recomp'/host.SHA/'run'
        for name in ('config','custom'):
            if (original/name).is_dir():shutil.copytree(original/name,template/name)
    record['template']=str(template)
    record['after']=str(exe);record['after_sha256']=hashlib.sha256(exe.read_bytes()).hexdigest()
    record['fileio_sha256']=hashlib.sha256((headers/'fileio.c').read_bytes()).hexdigest()
    (WORK/'manifest.json').write_text(json.dumps(record,indent=2))
    print('candidate linked with identical original objects',exe,flush=True)

def run_one(exe,label,ref,data,template):
    profile=Path(tempfile.mkdtemp(prefix=label+'-',dir=WORK))
    os.link(data,profile/'data.vra')
    for name in ('config','custom'):
        if (template/name).is_dir():shutil.copytree(template/name,profile/name)
    (profile/'logs').mkdir(exist_ok=True)
    env=dict(ENV,ALSOFT_DRIVERS='null')
    env.pop('RECOMP_NO_PALETTES',None)
    stdout=profile/'stdout.txt';stderr=profile/'stderr.txt'
    with stdout.open('wb') as out,stderr.open('wb') as err:
        child=subprocess.Popen([str(exe),str(ref),str(profile)],cwd=profile,env=env,stdout=out,stderr=err)
        try: code=child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            child.kill();child.wait();raise RuntimeError(f'loading timed out: {profile}')
    text=stderr.read_text(errors='replace')
    match=re.search(r'FILEIO_LOAD_COMPLETE us=(\d+)',text)
    if code or not match:raise RuntimeError(f'load did not complete: code {code}, {profile}\n{text[-1500:]}')
    result={'label':label,'us':int(match[1]),'profile':str(profile),'log_bytes':(profile/'logs/log.log').stat().st_size if (profile/'logs/log.log').exists() else 0}
    print(result,flush=True);return result

def measure(pairs):
    record=json.loads((WORK/'manifest.json').read_text())
    template=Path(record['template'])
    ref=host.REFERENCE/'vagante.exe';data=host.REFERENCE/'data.vra'
    # One untimed warm-up per executable; both use all palettes and real resources.
    for label in ('before','after'):run_one(Path(record[label]),label+'-warmup',ref,data,template)
    samples=[]
    for i in range(pairs):
        for label in (('before','after') if i%2==0 else ('after','before')):
            samples.append(run_one(Path(record[label]),label,ref,data,template))
    med={label:statistics.median(s['us'] for s in samples if s['label']==label) for label in ('before','after')}
    record.update(samples=samples,median_us=med,change_percent=(med['after']/med['before']-1)*100)
    (WORK/'results.json').write_text(json.dumps(record,indent=2))
    print('median_us',med,'change_percent',record['change_percent'],flush=True)
    if record['change_percent']>2:raise RuntimeError('load median regression exceeds 2%; results preserved')

def check_result():
    record=json.loads((WORK/'results.json').read_text())
    frozen=Path(record['folder'])/'candidate/fileio.c'
    if frozen.read_text(encoding='utf-8')!=(ROOT/'recomp/host/fileio.c').read_text(encoding='utf-8'):
        raise RuntimeError('fileio.c changed since measurement; rerun candidate and comparison')
    if record['change_percent']>2:raise RuntimeError('load regression exceeds 2%')
    print(f"load gate OK: median change {record['change_percent']:+.3f}%, limit +2%",flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--freeze-baseline',action='store_true');p.add_argument('--candidate',action='store_true')
    p.add_argument('--run',action='store_true');p.add_argument('--pairs',type=int,default=7)
    p.add_argument('--check-result',action='store_true')
    args=p.parse_args()
    if args.pairs<1:p.error('--pairs must be positive')
    if args.freeze_baseline:freeze()
    if args.candidate:candidate()
    if args.run:measure(args.pairs)
    if args.check_result:check_result()
if __name__=='__main__':main()
