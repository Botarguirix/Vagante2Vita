#!/usr/bin/env python3
"""Run function/switch discovery on vagante.exe and write the local discovery report.

Outputs (local, not versioned) under artifacts/recomp/<sha>/:
  discovery.json   summary, per-function records and switch tables
  unexplained.tsv  executable byte ranges that are neither code, tables nor padding
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from recomp.analysis.discover import Discovery  # noqa: E402
from recomp.analysis.image import Image  # noqa: E402


def ghidra_comparison(path: Path, functions: dict) -> dict:
    if not path.exists():
        return {}
    ghidra = {}
    for line in path.read_text(encoding="utf-8").splitlines()[1:]:
        cols = line.split("\t")
        ghidra[int(cols[0], 16)] = int(cols[2])
    ours = set(functions)
    theirs = set(ghidra)
    return {
        "ghidra_functions": len(theirs),
        "both": len(ours & theirs),
        "only_ours": len(ours - theirs),
        "only_ghidra": len(theirs - ours),
        "only_ghidra_sample": [f"0x{va:08x}" for va in sorted(theirs - ours)[:40]],
        "ghidra_function_bytes": sum(ghidra.values()),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path(r"F:\SteamLibrary\steamapps\common\vagante\vagante.exe"))
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()

    started = time.monotonic()
    image = Image.from_pe(args.exe)
    out = args.output or ROOT / "artifacts/recomp" / image.sha256
    out.mkdir(parents=True, exist_ok=True)
    d = Discovery(image)
    d.run()
    summary = d.summary()
    summary["sha256"] = image.sha256
    summary["seconds"] = round(time.monotonic() - started, 1)
    summary["ghidra"] = ghidra_comparison(
        ROOT / "artifacts/pe-inventory" / image.sha256 / "functions.tsv", d.functions)

    functions = []
    for va in sorted(d.functions):
        fn = d.functions[va]
        functions.append({
            "entry": f"0x{va:08x}",
            "source": fn.source,
            "bytes": sum(d.cache[i].size for i in fn.insns),
            "instructions": len(fn.insns),
            "noreturn": va in d.noreturn,
            "calls": [f"0x{t:08x}" for t in sorted(fn.calls)],
            "tailcalls": [f"0x{t:08x}" for t in sorted(fn.tailcalls)],
            "imports": sorted(fn.import_calls),
            "switches": [{
                "site": f"0x{sw.site:08x}", "table": f"0x{sw.table:08x}", "entries": sw.entries,
                "index_table": f"0x{sw.index_table:08x}" if sw.index_table is not None else None,
                "index_entries": sw.index_entries, "bound": sw.bound,
            } for sw in fn.switches],
            "unresolved_jumps": [f"0x{t:08x}" for t in fn.unresolved_jumps],
            "invalid": [f"0x{t:08x}" for t in fn.invalid],
        })
    (out / "discovery.json").write_text(
        json.dumps({"summary": summary, "functions": functions}, indent=1), encoding="utf-8")
    rows = ["start\tsize\tfirst_bytes"]
    for start, size in sorted(d.unexplained_ranges(), key=lambda r: -r[1]):
        rows.append(f"0x{start:08x}\t{size}\t{image.read(start, min(size, 16)).hex(' ')}")
    (out / "unexplained.tsv").write_text("\n".join(rows) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=1))
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
