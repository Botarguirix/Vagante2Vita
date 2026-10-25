#!/usr/bin/env python3
"""Stress the guest heap, then benchmark 1,000,000 alloc/free pairs per workload.

python -I tests/host/run_heap_tests.py
python -I tests/host/run_heap_tests.py --source artifacts/heap-before.c --label before --bench-only
Same GCC flags and fixture for both implementations; raw samples saved in artifacts/.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import statistics
import subprocess

ROOT=Path(__file__).resolve().parents[2]
CC=Path(r'C:\msys64\ucrt64\bin\gcc.exe')
ENV=dict(os.environ,PATH=str(CC.parent)+os.pathsep+os.environ.get('PATH',''))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'recomp/host/support.c')
    parser.add_argument('--label',default='tlsf')
    parser.add_argument('--bench-only',action='store_true')
    parser.add_argument('--stress-only',action='store_true')
    parser.add_argument('--repeat',type=int,default=3)
    args=parser.parse_args()
    if not re.fullmatch(r'[a-zA-Z0-9_-]+',args.label) or args.repeat<1: parser.error('invalid label/repeat')
    if args.bench_only and args.stress_only: parser.error('choose bench-only or stress-only')
    source=args.source.resolve()
    out=ROOT/'artifacts/heap-tests';out.mkdir(parents=True,exist_ok=True)
    exe=out/(args.label+'.exe')
    flags=['-std=gnu11','-O2','-Wall','-Wextra','-Wno-unused-parameter','-DGUEST_WINDOW=0x10000000u',
           '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'recomp/runtime'),'-I'+str(ROOT/'recomp/host')]
    cmd=[str(CC),*flags,str(ROOT/'tests/host/heap_test.c'),str(source),'-Wl,--gc-sections','-lpthread','-o',str(exe)]
    result=subprocess.run(cmd,env=ENV,capture_output=True,text=True)
    if result.returncode: raise SystemExit(result.stderr)
    if result.stderr: print(result.stderr)
    if not args.bench_only: subprocess.run([str(exe)],cwd=out,env=ENV,check=True,timeout=180)
    if args.stress_only: return
    record={'source':str(source),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'compiler':subprocess.check_output([str(CC),'--version'],env=ENV,text=True).splitlines()[0],
            'flags':flags,'pairs':1000000,'samples_us':{},'median_us':{}}
    for mode in ('simple','fragmented','threads','threads_fragmented'):
        samples=[]
        for _ in range(args.repeat):
            r=subprocess.run([str(exe),'--bench',mode],cwd=out,env=ENV,check=True,capture_output=True,text=True,timeout=180)
            print(r.stdout.strip(),flush=True)
            match=re.search(r'pairs=1000000 us=(\d+)',r.stdout);assert match
            samples.append(int(match[1]))
        record['samples_us'][mode]=samples
        record['median_us'][mode]=statistics.median(samples)
    path=out/(args.label+'.json');path.write_text(json.dumps(record,indent=2)+'\n')
    print('saved',path,flush=True)

if __name__=='__main__': main()
