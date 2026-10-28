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
from concurrent.futures import ThreadPoolExecutor, as_completed
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


def build(jobs: int, packed: bool = False, window_mib: int | None = None) -> Path:
    gen = ROOT / "artifacts/recomp" / SHA / "gen"
    # --packed-textures: the Vita's deflated texture storage, to test it on PC (own objects)
    out = ROOT / "artifacts/recomp" / SHA / ("host-packed" if packed else "host")
    if window_mib is not None:
        out = out.with_name(out.name + f"-{window_mib}mib")
    out.mkdir(parents=True, exist_ok=True)
    if window_mib is not None:
        # Freeze inputs while another local task regenerates lift output.
        import importlib.util
        spec = importlib.util.spec_from_file_location("vita_snapshot", ROOT / "recomp/build_vita.py")
        snapshot = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(snapshot)
        source_gen = gen
        raw_gen = out / "gen-source-snapshot"
        snapshot.snapshot_generated(source_gen, raw_gen)
        gen = out / "gen-snapshot"
        gen.mkdir(parents=True, exist_ok=True)
        # An in-progress lift may have read the previous natives list. Route only
        # this private snapshot's direct native calls through the normal dispatcher.
        # Never edit gen/ or lift/runtime; source definitions and table stay intact.
        import re, hashlib, json
        natives = set(re.findall(r"^([0-9a-fA-F]{8})\s", (HOST / "natives.txt").read_text(), re.M))
        # Measurement-only copy constructors may be inlined at the sf::Image level.
        # Keep these private routes out of the shared natives/lifter files.
        natives.update(re.findall(r"^IMAGE_HOOK\(([0-9a-fA-F]{8})\)", (HOST / "heap_profile.c").read_text(encoding="utf-8"), re.M))
        routes = 0
        paths = sorted(raw_gen.glob("unit_*.c")) + [raw_gen / "table.c", raw_gen / "lifted.h"]
        for unit in paths:
            before = unit.read_text()
            def route(match):
                nonlocal routes
                address = match[1]
                if address not in natives:
                    return match[0]
                routes += 1
                return f"guest_call(c, 0x{address}u)"
            after = re.sub(r"\bf_([0-9a-fA-F]{8})\(c\)", route, before)
            destination = gen / unit.name
            if not destination.exists() or destination.read_text() != after:
                destination.write_text(after)
        expected = {unit.name for unit in paths}
        for old in gen.glob("unit_*.c"):
            if old.name not in expected:
                old.unlink()
        (gen / "snapshot.json").write_bytes((raw_gen / "snapshot.json").read_bytes())
        digest = hashlib.sha256()
        for unit in sorted(gen.glob("unit_*.c")) + [gen / "table.c", gen / "lifted.h"]:
            digest.update(unit.name.encode()); digest.update(unit.read_bytes())
        (out / "profile-routing.json").write_text(json.dumps({"routes": routes, "sha256": digest.hexdigest(), "source": json.loads((raw_gen / "snapshot.json").read_text())}, indent=2))
        print(f"Private native routing: {routes} callsites; SHA-256 {digest.hexdigest()}", flush=True)
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
    if window_mib is not None:
        define = f"-DGUEST_WINDOW={window_mib << 20:#x}u"
        gen_flags.append(define)
        host_flags.append(define)
    gen_obj = out / "generated" if window_mib is not None else gen
    gen_obj.mkdir(parents=True, exist_ok=True)
    tasks = [(src, gen_obj / (src.stem + ".o"), gen_flags, header + [gen / "lifted.h"])
             for src in sorted(gen.glob("unit_*.c")) + [gen / "table.c"]]
    tasks += [(src, out / (src.stem + ".o"), host_flags, header + [HOST / "win32.h", Path(__file__), *HOST.glob("*.inc")])
              for src in sorted(HOST.glob("*.c")) + [RUNTIME / "guest.c"]]
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        pending = [pool.submit(compile_one, *task) for task in tasks]
        errors = []
        for finished, future in enumerate(as_completed(pending), 1):
            error = future.result()
            if error:
                errors.append(error)
            if window_mib is not None and (finished % 16 == 0 or finished == len(tasks)):
                print(f"PC {window_mib} MiB: {finished}/{len(tasks)} units checked", flush=True)
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
    parser.add_argument("--window-mib", type=int, help="isolated build with this guest window, e.g. 280")
    args = parser.parse_args()
    if args.window_mib is not None and not 40 <= args.window_mib <= 1024:
        parser.error("--window-mib must be between 40 and 1024")
    exe = build(args.jobs, args.packed_textures, args.window_mib)
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
