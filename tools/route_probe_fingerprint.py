"""Fingerprint the exact C sources and build script used by the route probe."""
from __future__ import annotations

import hashlib
from pathlib import Path


SOURCE_FILES = (
    "vita/src/act_one_route_probe.c",
    "vita/src/world.c",
    "vita/src/world_entities.c",
    "vita/src/world_combat.c",
    "vita/src/loot.c",
    "vita/src/player.c",
    "vita/src/player_stats.c",
    "vita/src/melee.c",
    "vita/src/bow.c",
    "vita/src/input_replay.c",
    "vita/src/campaign_flow.c",
    "vita/src/campaign_flow.h",
    "tools/run_act_one_route_probe_wsl.sh",
    "tools/run_act_one_campaign_routes_windows.ps1",
    "tools/route_probe_fingerprint.py",
    "tools/route_trace.py",
    "tools/record_act_one_route.py",
    "tools/prepare_act_one_campaign.py",
)


def source_fingerprint(source_root: Path) -> dict:
    """Return one stable SHA-256 covering probe sources, headers and build script."""
    source_root = source_root.resolve()
    files = set(SOURCE_FILES)
    files.update(path.relative_to(source_root).as_posix()
                 for path in (source_root / "vita/src").glob("*.h"))
    digest = hashlib.sha256()
    for relative in sorted(files):
        path = (source_root / relative).resolve()
        if not path.is_relative_to(source_root) or not path.is_file():
            raise FileNotFoundError(f"Route probe source is missing: {relative}")
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return {"sha256": digest.hexdigest(), "files": sorted(files)}
