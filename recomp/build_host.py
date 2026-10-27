#!/usr/bin/env python3
"""Build and optionally run the PC host of the recompiled game (F2).

Links recomp/host/*.c and the runtime with the generated units from
recomp/lift_all.py (artifacts/recomp/<sha>/gen). Generated units are recompiled
only when their object is older than the source or the runtime header.

  python recomp/build_host.py            # build
  python recomp/build_host.py --run      # build, then run in artifacts/recomp/<sha>/run
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GCC = Path(r"C:\msys64\ucrt64\bin\gcc.exe")
SHA = "9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c"  # vagante.exe Steam 1.10.1
REFERENCE = Path(r"C:\Users\Andy PC\Documents\Chatgpt\vagante_ref")  # vagante.exe + data.vra
RUNTIME = ROOT / "recomp/runtime"
HOST = ROOT / "recomp/host"


def env():
    return dict(os.environ, PATH=str(GCC.parent) + os.pathsep + os.environ.get("PATH", ""))


def compile_one(src: Path, obj: Path, flags: list[str], deps: list[Path]) -> str | None:
    newest = max([src.stat().st_mtime] + [d.stat().st_mtime for d in deps])
    if obj.exists() and obj.stat().st_mtime >= newest:
        return None
    run = subprocess.run([str(GCC), *flags, "-c", str(src), "-o", str(obj)], env=env(),
                         capture_output=True, text=True)
    return f"{src.name}:\n{run.stderr}" if run.returncode else None


def build(jobs: int, packed: bool = False) -> Path:
    gen = ROOT / "artifacts/recomp" / SHA / "gen"
    # --packed-textures: the Vita's deflated texture storage, to test it on PC (own objects)
    out = ROOT / "artifacts/recomp" / SHA / ("host-packed" if packed else "host")
    out.mkdir(parents=True, exist_ok=True)
    if not (gen / "table.c").exists():
        raise SystemExit("run recomp/lift_all.py first")
    if not any(gen.glob("unit_*.c")):
        raise SystemExit("generated units missing (lift_all.py may be running); wait for lifting to finish")
    header = [RUNTIME / "guest.h"]
    gen_flags = ["-std=gnu11", "-O1", "-frounding-math", "-fno-strict-aliasing", "-w",
                 "-DT123_TRACE_CALLS", f"-I{RUNTIME}", f"-I{gen}"]
    host_flags = ["-std=gnu11", "-O1", "-g", "-frounding-math", "-Wall", "-Wextra", "-DT123_TRACE_CALLS",
                  "-Wno-unused-parameter", "-DAL_LIBTYPE_STATIC", f"-I{RUNTIME}", f"-I{HOST}"]
    if packed:
        host_flags.append("-DT123_PACKED_TEXTURES")
    tasks = [(src, gen / (src.stem + ".o"), gen_flags, header + [gen / "lifted.h"])
             for src in sorted(gen.glob("unit_*.c")) + [gen / "table.c"]]
    tasks += [(src, out / (src.stem + ".o"), host_flags, header + [HOST / "win32.h", Path(__file__)])
              for src in sorted(HOST.glob("*.c")) + [RUNTIME / "guest.c"]]
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        errors = [e for e in pool.map(lambda t: compile_one(*t), tasks) if e]
    if errors:
        raise SystemExit("\n".join(errors))
    exe = out / "vagante_host.exe"
    link = subprocess.run([str(GCC), *[str(t[1]) for t in tasks], "-static", "-lSDL2", "-lopenal",
                           "-lopengl32", "-lsetupapi", "-limm32", "-lversion", "-lwinmm", "-lole32",
                           "-loleaut32", "-lgdi32", "-luuid", "-luser32", "-ladvapi32", "-lshell32",
                           "-lcfgmgr32", "-lavrt", "-lstdc++", "-lz", "-lm", "-lpthread", "-o", str(exe)],
                          env=env(), capture_output=True, text=True)
    if link.returncode:
        raise SystemExit(link.stderr)
    return exe


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--packed-textures", action="store_true", help="store textures deflated, as on the Vita")
    args = parser.parse_args()
    exe = build(args.jobs, args.packed_textures)
    print(f"built {exe}")
    if args.run:
        workdir = ROOT / "artifacts/recomp" / SHA / "run"
        workdir.mkdir(parents=True, exist_ok=True)
        data = REFERENCE / "data.vra"
        if not (workdir / "data.vra").exists():
            import shutil
            shutil.copyfile(data, workdir / "data.vra")
        run = subprocess.run([str(exe), str(REFERENCE / "vagante.exe"), str(workdir)],
                             cwd=workdir, capture_output=True, text=True, timeout=600)
        print(run.stdout.strip())
        if run.stderr.strip():
            print(run.stderr.strip()[-3000:])
        print(f"trace: {workdir / 'host-trace.log'}")


if __name__ == "__main__":
    main()
