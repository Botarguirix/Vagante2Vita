#!/usr/bin/env python3
"""Lift every discovered function of vagante.exe and report translation coverage.

Writes (local only, never versioned) to artifacts/recomp/<sha>/gen/:
  lifted.h, unit_NNN.c, table.c   the whole program as C
  faults.json                      every instruction lifted as guest_fault
  coverage.json                    counts by mnemonic and by region
With --compile it also builds every unit with gcc and links them with the
runtime and a stub main, which proves the program is complete C.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from recomp.analysis import eh  # noqa: E402
from recomp.analysis.discover import Discovery  # noqa: E402
from recomp.analysis.image import Image  # noqa: E402
from recomp.lift.lifter import Lifter  # noqa: E402

GCC = Path(r"C:\msys64\ucrt64\bin\gcc.exe")
# Statically linked MSVC CRT and third-party libraries (zlib, libpng, vorbis) live
# at the end of .text; they are candidates for native replacement, so their faults
# are reported apart from the game's own code. Boundary from the CRT entry points
# identified in docs/X87_MODE.md and docs/PE_INVENTORY.md.
LIBRARY_START = 0x81F000

STUB_MAIN = """#include <stdio.h>
#include "lifted.h"
extern const GuestEntry t123_lifted[];
extern const unsigned t123_lifted_count;
int main(void) {
    guest_register(t123_lifted, t123_lifted_count);
    printf("%u lifted functions linked\\n", t123_lifted_count);
    return 0;
}
"""


def region(va: int) -> str:
    return "library" if va >= LIBRARY_START else "game"


def compile_all(gen: Path, jobs: int) -> dict:
    runtime = ROOT / "recomp/runtime"
    env = dict(os.environ, PATH=str(GCC.parent) + os.pathsep + os.environ.get("PATH", ""))
    flags = ["-std=gnu11", "-O1", "-frounding-math", "-fno-strict-aliasing", "-w",
             f"-I{runtime}", f"-I{gen}"]
    (gen / "stub_main.c").write_text(STUB_MAIN, encoding="utf-8")
    sources = sorted(gen.glob("unit_*.c")) + [gen / "table.c", gen / "stub_main.c",
                                              runtime / "guest.c"]
    errors = {}

    def build(src: Path):
        obj = gen / (src.stem + ".o")
        run = subprocess.run([str(GCC), *flags, "-c", str(src), "-o", str(obj)], env=env,
                             capture_output=True, text=True)
        if run.returncode:
            errors[src.name] = run.stderr[-2000:]
        return obj

    started = time.monotonic()
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        objects = list(pool.map(build, sources))
    compile_seconds = time.monotonic() - started
    if errors:
        return {"compiled": False, "errors": errors}
    exe = gen / "th123_lifted.exe"
    link = subprocess.run([str(GCC), *map(str, objects), "-lm", "-o", str(exe)], env=env,
                          capture_output=True, text=True)
    if link.returncode:
        return {"compiled": True, "linked": False, "errors": {"link": link.stderr[-4000:]}}
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    return {"compiled": True, "linked": True, "compile_seconds": round(compile_seconds, 1),
            "units": len(sources) - 3, "stub_output": run.stdout.strip(),
            "exe_bytes": exe.stat().st_size}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path(r"F:\SteamLibrary\steamapps\common\vagante\vagante.exe"))
    parser.add_argument("--compile", action="store_true")
    parser.add_argument("--compile-only", action="store_true",
                        help="build the existing gen/ output without lifting again")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args()

    image = Image.from_pe(args.exe)
    report = ROOT / "artifacts/recomp" / image.sha256 / "discovery.json"
    if not report.exists():
        raise SystemExit("run recomp/discover.py first")
    entries = {int(f["entry"], 16) for f in json.loads(report.read_text())["functions"]}
    # Functions known to the PDB but not reached by discovery (destructors and handlers that
    # only exception tables or vtables point to): artifacts/pdb_seeds.txt, one hex address per line.
    seeds = ROOT / "artifacts/pdb_seeds.txt"
    if seeds.exists():
        entries |= {int(line, 16) for line in seeds.read_text().split() if line}
    # C++ EH: the handler stubs (only pushed as fs:[0] handlers) and the table funclets.
    tables = eh.scan(image)
    entries |= tables.stubs | tables.funclets
    entries = sorted(entries)
    gen =ROOT / "artifacts/recomp" / image.sha256 / "gen"
    gen.mkdir(parents=True, exist_ok=True)
    if args.compile_only:
        coverage = json.loads((gen / "coverage.json").read_text(encoding="utf-8"))
        coverage["build"] = compile_all(gen, args.jobs)
        (gen / "coverage.json").write_text(json.dumps(coverage, indent=1), encoding="utf-8")
        print(json.dumps(coverage["build"], indent=1))
        return
    for old in gen.glob("unit_*.c"):
        old.unlink()

    started = time.monotonic()
    lifter = Lifter(Discovery(image), set(entries))
    lifter.resume_points = eh.continuations(lifter.d, tables.catch_funclets, set(entries))
    files, result = lifter.lift_units(
        entries, progress=lambda i, n: print(f"  lifting {i}/{n}", flush=True))
    for name, text in files.items():
        (gen / name).write_text(text, encoding="utf-8")
    lift_seconds = time.monotonic() - started

    by_mnemonic: dict[str, Counter] = {"game": Counter(), "library": Counter()}
    for va, text in result.faults.items():
        by_mnemonic[region(va)][text.split(" (")[0].split()[0]] += 1
    coverage = {
        "functions": len(entries),
        "units": sum(1 for n in files if n.startswith("unit_")),
        "c_bytes": sum(len(t) for t in files.values()),
        "lift_seconds": round(lift_seconds, 1),
        "traps": {"game": sum(1 for va in lifter.traps if region(va) == "game"),
                  "library": sum(1 for va in lifter.traps if region(va) == "library")},
        "faults_total": len(result.faults),
        "faults_game": sum(by_mnemonic["game"].values()),
        "faults_library": sum(by_mnemonic["library"].values()),
        "fault_mnemonics_game": dict(by_mnemonic["game"].most_common()),
        "fault_mnemonics_library": dict(by_mnemonic["library"].most_common()),
    }
    (gen / "faults.json").write_text(json.dumps(
        {f"0x{va:08x}": text for va, text in sorted(result.faults.items())}, indent=1),
        encoding="utf-8")
    if args.compile:
        coverage["build"] = compile_all(gen, args.jobs)
    (gen / "coverage.json").write_text(json.dumps(coverage, indent=1), encoding="utf-8")
    print(json.dumps(coverage, indent=1))


if __name__ == "__main__":
    main()
