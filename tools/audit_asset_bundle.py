"""Compare a private runtime bundle with locally extracted Vagante assets.

The report contains file names, sizes, and hashes only. It never copies source
assets, the VRA archive, or proprietary data into the repository or VPK.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from prepare_world_bundle import validate_manifest


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def original_candidates(name: str, assets: Path) -> list[Path]:
    """Map packaged image names back to the exact source paths in the preparer."""
    path = Path(name)
    if name.startswith("enemies/"):
        if len(path.parts) != 3:
            return []
        _, family, filename = name.split("/")
        state = Path(filename).stem
        filenames = {
            "goblin": {"stand": "goblin_stand.png", "move": "goblin_walk.png", "air": "goblin_jump.png", "hurt": "goblin_hurt.png", "attack": "goblin_attack.png"},
            "bat": {"stand": "bat_hang.png", "move": "bat.png", "air": "bat.png", "hurt": "bat_hurt.png", "attack": "bat.png"},
            "lurker": {"stand": "lurker_retract.png", "move": "lurker_attack.png", "air": "lurker_retract.png", "hurt": "lurker_retract.png", "attack": "lurker_attack.png"},
            "slime": {"stand": "stand.png", "move": "walk.png", "air": "jump.png", "hurt": "jump.png", "attack": "walk.png"},
        }
        filename = filenames[family][state]
        return [assets / "gfx/monster" / family / filename]
    if name.startswith(("knight/", "rogue/", "mage/")):
        if len(path.parts) != 2:
            return []
        cls, animation = path.parts
        suffix = {"stand": "stand.png", "move": "walk.png", "air": "jump.png"}
        filename = suffix.get(animation, path.name)
        return [assets / "gfx/classes" / cls / "1" / filename]
    if name == "items-weapons.png" or name == "items/items-weapons.png":
        return [assets / "gfx/items/items-weapons.png"]
    if name.startswith("items/"):
        return [assets / "gfx/items" / path.name]
    if name.startswith("entities/"):
        if len(path.parts) == 3 and path.parts[1] == "doodads":
            return [assets / "gfx/doodads" / path.name]
        if len(path.parts) != 2:
            return []
        return [assets / "gfx/doodads" / path.name, assets / "gfx/traps" / path.name,
                assets / "gfx/projectiles" / path.name,
                assets / "gfx/scenes/intro/entities" / path.name,
                assets / "gfx/chest_small.png"] if path.name == "chest.png" else [
                assets / folder / path.name for folder in ("gfx/doodads", "gfx/traps", "gfx/projectiles")]
    return []


def audit(assets: Path, bundle: Path, output: Path, source_archive: Path | None = None) -> dict:
    assets, bundle, output = assets.resolve(), bundle.resolve(), output.resolve()
    manifest = validate_manifest(bundle)
    if not assets.is_dir():
        raise FileNotFoundError(f"Extracted asset directory not found: {assets}")
    records = []
    generated = []
    for name, expected in sorted(manifest["files"].items()):
        if name.endswith("-terrain.png") or name == "intro-terrain.png":
            generated.append(name)  # Terrain is rendered from tiles/layout data, not copied from one source image.
            continue
        if not name.lower().endswith((".png", ".jpg", ".bmp")):
            continue  # Runtime packets and generated metadata are validated by their own format tests.
        candidates = original_candidates(name, assets)
        matches = []
        for candidate in candidates:
            if candidate.is_file():
                matches.append({"path": candidate.relative_to(assets).as_posix(),
                                "bytes": candidate.stat().st_size,
                                "sha256": sha256(candidate),
                                "matches_bundle": candidate.stat().st_size == expected["bytes"]
                                and sha256(candidate) == expected["sha256"]})
        records.append({"bundle_path": name, "bundle_bytes": expected["bytes"],
                        "bundle_sha256": expected["sha256"],
                        "source_candidates": matches,
                        "source_match": any(item["matches_bundle"] for item in matches)})
    source = None
    if source_archive is not None:
        source_archive = source_archive.resolve()
        source = {"path": str(source_archive), "bytes": source_archive.stat().st_size,
                  "sha256": sha256(source_archive), "included": False}
    report = {"format": "vagante-asset-bundle-audit", "version": 1,
              "bundle": str(bundle), "bundle_manifest_sha256": sha256(bundle / "manifest.json"),
              "extracted_assets": str(assets), "source_archive": source,
              "source_archive_verified": source is not None,
              "bundle_files": len(records),
              "generated_images": generated,
              "source_matches": sum(item["source_match"] for item in records),
              "source_missing_or_mismatched": [item["bundle_path"] for item in records if not item["source_match"]],
              "files": records}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("assets", type=Path, help="Private extracted data.vra directory")
    parser.add_argument("bundle", type=Path, help="Private prepared runtime bundle")
    parser.add_argument("--source-archive", type=Path, help="Optional original data.vra; only its hash is recorded")
    parser.add_argument("--output", type=Path, default=Path("_local/analysis/asset-bundle-audit.json"))
    args = parser.parse_args()
    report = audit(args.assets, args.bundle, args.output, args.source_archive)
    print(json.dumps({key: report[key] for key in ("bundle_files", "source_matches", "source_missing_or_mismatched", "source_archive_verified")}, indent=2))


if __name__ == "__main__":
    main()
