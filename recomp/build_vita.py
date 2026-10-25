#!/usr/bin/env python3
"""Build all lifted Vagante functions for Vita, then package code-only VAGA00002.

python -I recomp/build_vita.py [--jobs 8] [--opt=-Os] [--host-only]
The local PC executable/data.vra are NEVER packaged. Runtime: ux0:data/Vagante/.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import time
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SHA = '9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c'
TITLE_ID, TITLE = 'VAGA00002', 'Vagante rama C'
SDK = Path(os.environ.get('VAGANTE_VITASDK', r'C:\vitasdk-pm'))
RUNTIME, HOST, VITA = ROOT/'recomp/runtime', ROOT/'recomp/host', ROOT/'recomp/vita'
OUT = ROOT/'artifacts/recomp'/SHA/'vita'

def run(command):
    result = subprocess.run([str(x) for x in command], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f'{command[0]} failed ({result.returncode}):\n{result.stdout}\n{result.stderr}')
    return result.stdout

def compile_one(src, obj, flags, deps, cc):
    command = [str(cc), *flags, '-c', str(src), '-o', str(obj)]
    signature = hashlib.sha256(json.dumps([command, cc.stat().st_mtime_ns]).encode()).hexdigest()
    stamp = obj.with_suffix('.cmd')
    newest = max(p.stat().st_mtime_ns for p in [src, *deps])
    if obj.exists() and obj.stat().st_mtime_ns >= newest and stamp.exists() and stamp.read_text() == signature:
        return False
    # Failed/interrupted compiles must never poison the incremental object cache.
    temporary = obj.with_name(obj.name + f'.{os.getpid()}.tmp')
    try:
        run([*command[:-1], str(temporary)])
        os.replace(temporary, obj)
    finally:
        temporary.unlink(missing_ok=True)
    stamp.write_text(signature)
    return True

def compile_group(tasks, jobs, cc, label):
    rebuilt, finished = 0, 0
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(compile_one, *task, cc): task[0] for task in tasks}
        errors = []
        for future in as_completed(futures):
            finished += 1
            try:
                rebuilt += future.result()
            except Exception as exc:
                errors.append(f'{futures[future].name}: {exc}')
            if finished % 8 == 0 or finished == len(tasks):
                print(f'{label}: {finished}/{len(tasks)} ({rebuilt} rebuilt)', flush=True)
        if errors:
            raise RuntimeError('\n'.join(errors))
    return rebuilt

def snapshot_generated(source, destination, timeout=600):
    """Freeze a coherent generated set without editing/locking the lifter's files."""
    deadline = time.monotonic() + timeout
    announced = False
    while True:
        paths = sorted(source.glob('unit_*.c')) + [source/'table.c', source/'lifted.h']
        try:
            if len(paths) <= 2:
                raise FileNotFoundError('no generated units')
            before = {p.name: (p.stat().st_size, p.stat().st_mtime_ns) for p in paths}
            content = {p.name: p.read_bytes() for p in paths}
            # Detect an in-progress regeneration, including deletion/replacement.
            time.sleep(1)
            after_paths = sorted(source.glob('unit_*.c')) + [source/'table.c', source/'lifted.h']
            after = {p.name: (p.stat().st_size, p.stat().st_mtime_ns) for p in after_paths}
            if before != after or any(len(content[n]) != before[n][0] for n in content):
                raise FileNotFoundError('generated inputs changed while snapshotting')
            destination.mkdir(parents=True, exist_ok=True)
            digest = hashlib.sha256()
            for name, data in sorted(content.items()):
                digest.update(name.encode()); digest.update(data)
                path = destination/name
                # Preserve mtimes for unchanged files so incremental objects remain valid.
                if not path.exists() or path.read_bytes() != data:
                    path.write_bytes(data)
            for old in destination.glob('unit_*.c'):
                if old.name not in content: old.unlink()
            (destination/'snapshot.json').write_text(json.dumps({'source':str(source),'sha256':digest.hexdigest(),'units':len(paths)-2},indent=2))
            print(f'Generated snapshot: {len(paths)-2} units, SHA-256 {digest.hexdigest()}',flush=True)
            return
        except FileNotFoundError:
            if time.monotonic() >= deadline:
                raise RuntimeError('generated files are still being regenerated; finish lift_all.py first')
            if not announced:
                print('Waiting for a stable generated C set (lift_all.py may be running)...',flush=True)
                announced = True
            time.sleep(2)

def section_sizes(output):
    sizes = {}
    for line in output.splitlines():
        words = line.split()
        if len(words) >= 3 and words[0].startswith('.'):
            sizes[words[0]] = int(words[1])
    if '.text' not in sizes:
        raise RuntimeError('size output contains no .text section')
    return sizes

def assets(out):
    """Own diagnostic graphics/font; no game resources or commercial executable."""
    from PIL import Image, ImageDraw, ImageFont
    files = {}
    for name, size in [('icon0.png',(128,128)), ('livearea/contents/bg0.png',(840,500)),
                       ('livearea/contents/startup.png',(280,158))]:
        path = out/'sce_sys'/name
        path.parent.mkdir(parents=True, exist_ok=True)
        image = Image.new('RGB', size, (8, 26, 35))
        draw = ImageDraw.Draw(image)
        draw.text((12, size[1]//3), 'Vagante\nRAMA C\nBuild tecnico', fill=(100,220,200))
        image.quantize(colors=128).save(path)
        files['sce_sys/'+name] = path
    template = out/'sce_sys/livearea/contents/template.xml'
    template.write_text('<?xml version="1.0" encoding="utf-8"?>\n<livearea style="psmobile" format-ver="01.00" content-rev="1"><livearea-background><image>bg0.png</image></livearea-background><gate><startup-image>startup.png</startup-image></gate></livearea>\n', encoding='utf-8')
    files['sce_sys/livearea/contents/template.xml'] = template
    font = Image.new('RGBA',(192,128))
    draw = ImageDraw.Draw(font)
    for ch in range(32,127):
        draw.text(((ch%16)*12,(ch//16)*16),chr(ch), font=ImageFont.load_default(), fill='white')
    path = out/'font.rgba'
    path.write_bytes(struct.pack('<II',192,128)+font.tobytes())
    files['assets/font.rgba'] = path
    return files

def package(out, files):
    manifest = {'eboot.bin':out/'eboot.bin', 'sce_sys/param.sfo':out/'param.sfo', **files}
    vpk = out/'Vagante-ramaC.vpk'
    with zipfile.ZipFile(vpk,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name, path in manifest.items():
            archive.write(path,name)
    with zipfile.ZipFile(vpk) as archive:
        if set(archive.namelist()) != set(manifest) or archive.testzip():
            raise RuntimeError('VPK manifest/CRC validation failed')
    return vpk

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs',type=int,default=min(os.cpu_count() or 4,8))
    parser.add_argument('--opt',choices=['-Os','-O1','-O2'],default='-Os')
    parser.add_argument('--trace-calls',action='store_true')
    parser.add_argument('--host-only',action='store_true',help='compile ARM platform/host first, without linking')
    args=parser.parse_args()
    if args.jobs < 1: parser.error('--jobs must be positive')
    cc=SDK/'bin/arm-vita-eabi-gcc.exe'
    source_gen=ROOT/'artifacts/recomp'/SHA/'gen'
    gen=OUT/'gen-snapshot'
    snapshot_generated(source_gen,gen)
    units=sorted(gen.glob('unit_*.c'))
    if not units or not (gen/'table.c').exists():
        raise RuntimeError('missing generated C: finish recomp/lift_all.py first')
    objdir=OUT/('obj'+args.opt+('-trace' if args.trace_calls else ''))
    objdir.mkdir(parents=True,exist_ok=True)
    print(f'SDK: {SDK}\n{run([cc,"--version"]).splitlines()[0]}\nGuest window: 256 MiB; {len(units)} generated units; {args.opt}',flush=True)
    common=['-std=gnu11',args.opt,'-mthumb','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard',
            '-ffp-contract=off','-frounding-math','-fno-strict-aliasing','-ffunction-sections','-fdata-sections',
            '-DGUEST_WINDOW=0x10000000u',f'-I{RUNTIME}',f'-I{VITA/"include"}']
    if args.trace_calls: common.append('-DT123_TRACE_CALLS')
    headers=sorted(RUNTIME.glob('*.h'))+sorted(HOST.glob('*.h'))+sorted(HOST.glob('*.inc'))+sorted(VITA.rglob('*.h'))+[SDK/'version_info.txt']
    host_flags=common+['-Wall','-Wno-unused-parameter','-DAL_LIBTYPE_STATIC',f'-I{HOST}']
    sources=[s for s in sorted(HOST.glob('*.c')) if s.name != 'platform_pc.c']+sorted(VITA.glob('*.c'))+[RUNTIME/'guest.c']
    host_tasks=[(s,objdir/(s.parent.name+'_'+s.stem+'.o'),host_flags,headers) for s in sources]
    started=time.monotonic()
    rebuilt=compile_group(host_tasks,args.jobs,cc,'host ARM')
    if args.host_only:
        print('OK: host ARM compiled (no ELF/VPK requested)'); return
    gen_tasks=[(s,objdir/(s.stem+'.o'),common+['-w',f'-I{gen}'],[RUNTIME/'guest.h',gen/'lifted.h',SDK/'version_info.txt']) for s in units+[gen/'table.c']]
    rebuilt+=compile_group(gen_tasks,args.jobs,cc,'lifted ARM')
    objects=[t[1] for t in gen_tasks+host_tasks]
    elf, velf, eboot=OUT/'vagante.elf',OUT/'vagante.velf',OUT/'eboot.bin'
    # Group archives to resolve SDL2/vitaGL/OpenAL and pthread mutual dependencies.
    libs=['SDL2','openal','vitaGL','vitashark','SceShaccCgExt','mathneon','taihen_stub','SceShaccCg_stub',
          'SceKernelDmacMgr_stub','SceGxm_stub','SceCommonDialog_stub','SceAppUtil_stub','SceSysmodule_stub',
          'SceIme_stub','SceAppMgr_stub','SceDisplay_stub','SceCtrl_stub','SceSysmem_stub','SceLibKernel_stub',
          'SceKernelThreadMgr_stub','SceProcessmgr_stub','SceAudio_stub','SceAudioIn_stub','SceIofilemgr_stub',
          'SceTouch_stub','SceHid_stub','SceMotion_stub','ScePower_stub','z','stdc++','pthread','m','gcc']
    link=['-mthumb','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-Wl,-q','-Wl,-z,nocopyreloc',
          '-Wl,--gc-sections',f'-Wl,-Map,{(OUT/"vagante.map").as_posix()}',
          *[p.as_posix() for p in objects],'-Wl,--start-group','-Wl,--whole-archive','-lpthread','-Wl,--no-whole-archive',*['-l'+s for s in libs],'-Wl,--end-group','-o',elf.as_posix()]
    rsp=OUT/'link.rsp'
    rsp.write_text('\n'.join('"'+str(x).replace('\\','/').replace('"','\\"')+'"' for x in link))
    print('Linking ARM ELF...',flush=True)
    run([cc,'@'+str(rsp)])
    sizes=section_sizes(run([SDK/'bin/arm-vita-eabi-size.exe','-A',elf]))
    print(f'ARM .text = {sizes[".text"]} bytes ({sizes[".text"]/1048576:.2f} MiB)',flush=True)
    run([SDK/'bin/vita-elf-create.exe',elf,velf])
    run([SDK/'bin/vita-make-fself.exe','-c',velf,eboot])
    run([SDK/'bin/vita-mksfoex.exe','-s','APP_VER=00.01','-s','TITLE_ID='+TITLE_ID,'-d','ATTRIBUTE2=12',TITLE,OUT/'param.sfo'])
    vpk=package(OUT,assets(OUT))
    metrics={'sdk':str(SDK),'optimization':args.opt,'guest_window':0x10000000,'title_id':TITLE_ID,
             'units':len(units),'objects':len(objects),'rebuilt':rebuilt,'seconds':round(time.monotonic()-started,1),
             'sections':sizes,'eboot_bytes':eboot.stat().st_size,'vpk_bytes':vpk.stat().st_size,
             'vpk_sha256':hashlib.sha256(vpk.read_bytes()).hexdigest(),
             'generated_snapshot':json.loads((gen/'snapshot.json').read_text())}
    (OUT/'build-metrics.json').write_text(json.dumps(metrics,indent=2)+'\n')
    print(f'eboot.bin = {eboot.stat().st_size} bytes ({eboot.stat().st_size/1048576:.2f} MiB)')
    print(f'built {vpk}\nSHA-256 {metrics["vpk_sha256"]}\n{rebuilt}/{len(objects)} objects rebuilt in {metrics["seconds"]} s',flush=True)

if __name__=='__main__':
    main()
