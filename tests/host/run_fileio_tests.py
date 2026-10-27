#!/usr/bin/env python3
"""Isolated WriteFile tests, including termination without fclose/atexit."""
import os
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
CC=Path(r'C:\msys64\ucrt64\bin\gcc.exe')
ENV=dict(os.environ,PATH=str(CC.parent)+os.pathsep+os.environ.get('PATH',''))
OUT=ROOT/'artifacts/fileio-tests';OUT.mkdir(parents=True,exist_ok=True)
source=ROOT/'recomp/host/fileio.c'
if not source.exists():source=ROOT/'artifacts/fileio-module.c'
exe=OUT/'fileio_test.exe'
cmd=[str(CC),'-std=gnu11','-O1','-Wall','-Wextra','-I'+str(ROOT/'recomp/host'),'-I'+str(ROOT/'recomp/runtime'),
     str(ROOT/'tests/host/fileio_test.c'),str(source),'-Wl,--wrap=fflush','-lpthread','-o',str(exe)]
subprocess.run(cmd,env=ENV,check=True)
subprocess.run([str(exe)],cwd=OUT,env=ENV,check=True)
expected=b'last line survives forced exit\n'
for flag,name in (('--child-buffered','child-before.log'),('--child-flush','child-after.log')):
    directory=Path(tempfile.mkdtemp(prefix='kill-',dir=OUT))
    child=subprocess.Popen([str(exe),flag],cwd=directory,env=ENV,stdin=subprocess.PIPE,stdout=subprocess.PIPE)
    with ThreadPoolExecutor(max_workers=1) as pool:
        ready=pool.submit(child.stdout.readline)
        try:assert ready.result(timeout=10).strip()==b'READY'
        finally:child.kill();child.wait(timeout=10)
    child.stdin.close();child.stdout.close()
    data=(directory/name).read_bytes()
    if flag=='--child-flush':assert data==expected
    else:assert data!=expected
print('forced TerminateProcess: baseline loses buffered tail; flush preserves complete record')
