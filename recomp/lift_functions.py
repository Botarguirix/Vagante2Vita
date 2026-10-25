#!/usr/bin/env python3
"""Lift selected vagante.exe functions to C (local output, never versioned).

Function boundaries come from the discovery report (artifacts/recomp/<sha>/
discovery.json, created by recomp/discover.py), so jumps into other functions
are emitted as tail calls exactly as discovered.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from recomp.analysis.discover import Discovery  # noqa: E402
from recomp.analysis.image import Image  # noqa: E402
from recomp.lift.lifter import Lifter  # noqa: E402


def known_entries(image: Image, discovery: Discovery) -> set[int]:
    report = ROOT / "artifacts/recomp" / image.sha256 / "discovery.json"
    if report.exists():
        data = json.loads(report.read_text(encoding="utf-8"))
        return {int(f["entry"], 16) for f in data["functions"]}
    discovery.run()
    return set(discovery.functions)


def lift_to(exe: Path, entries: list[int], output: Path) -> dict:
    image = Image.from_pe(exe)
    discovery = Discovery(image)
    lifter = Lifter(discovery, known_entries(image, discovery) | set(entries))
    result = lifter.lift(entries)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(result.source, encoding="utf-8")
    return {"functions": len(result.functions),
            "faults": {f"0x{va:08x}": text for va, text in sorted(result.faults.items())}}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("entries", nargs="+", help="function entry addresses (hex)")
    parser.add_argument("--exe", type=Path, default=Path(r"F:\SteamLibrary\steamapps\common\vagante\vagante.exe"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    summary = lift_to(args.exe, [int(e, 16) for e in args.entries], args.output)
    print(json.dumps(summary, indent=1))


if __name__ == "__main__":
    main()
