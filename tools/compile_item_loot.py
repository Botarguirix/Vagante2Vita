#!/usr/bin/env python3
"""Compile verified Ghidra loot tables and nested weights to the Vita VLT1 format."""
from __future__ import annotations

import argparse
import json
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.extract_item_pools import _pool_names


MAGIC = b"VLT1"
VERSION = 2
HEADER = struct.Struct("<4sHHHHH")
DESCRIPTOR = struct.Struct("<BBHH")
RARITY = struct.Struct("<IIHHBBH")
EDGE = struct.Struct("<BBI")
ATLAS_IDS = {
    "items/items.png": 0, "items/items-weapons.png": 1,
    "items/items-armor.png": 2, "items/potions.png": 3,
    "items/scrolls.png": 4, "items/spell_books.png": 5,
    "items/items-explosives.png": 6,
}
ATLAS_CELLS = {0: 16, 1: 20, 2: 20, 3: 16, 4: 20, 5: 16, 6: 20}


def validate_sprite_metadata(sprites: dict, atlas_sizes: dict[int, tuple[int, int]] | None = None) -> dict:
    """Ensure every explicit item sprite fits its private source atlas."""
    if sprites.get("format") != "vagante-item-sprite-indexes" or not sprites.get("verified"):
        raise ValueError("item type and uniqueness metadata are not verified")
    by_type = {}
    reverse_atlas = {name: atlas_id for name, atlas_id in ATLAS_IDS.items()}
    for item in sprites.get("items", []):
        item_type = item.get("item_type")
        atlas_name = item.get("atlas")
        sprite = item.get("sprite_index")
        if not isinstance(item_type, int) or item_type in by_type:
            raise ValueError(f"invalid or duplicate item type: {item_type}")
        if atlas_name not in reverse_atlas or not isinstance(sprite, int) or sprite < -1:
            raise ValueError(f"invalid sprite metadata for item type {item_type}")
        atlas_id = reverse_atlas[atlas_name]
        if sprite >= 0 and atlas_sizes is not None:
            if atlas_id not in atlas_sizes:
                raise ValueError(f"missing dimensions for atlas {atlas_name}")
            width, height = atlas_sizes[atlas_id]
            cell = ATLAS_CELLS[atlas_id]
            if width < cell or height < cell or width % cell or height % cell \
                    or sprite >= (width // cell) * (height // cell):
                raise ValueError(f"sprite index {sprite} is outside atlas for item type {item_type}")
        if not isinstance(item.get("unique"), bool):
            raise ValueError(f"missing unique flag for item type {item_type}")
        by_type[item_type] = item
    return by_type


def compile_tables(rarity: dict, graph: dict, sprites: dict,
                   pool_names: dict[int, str],
                   atlas_sizes: dict[int, tuple[int, int]] | None = None) -> bytes:
    if rarity.get("format") != "vagante-item-rarity-tables" or not rarity.get("verified"):
        raise ValueError("rarity tables are not verified")
    if graph.get("format") != "vagante-chest-pool-graph" \
            or not graph.get("verified_structure") or not graph.get("verified_weights"):
        raise ValueError("nested pool graph is not verified")
    item_meta = validate_sprite_metadata(sprites, atlas_sizes)
    ids_by_name = {name: pool_id for pool_id, name in pool_names.items()}
    direct = []
    rows = []
    for name, entries in rarity["pools"].items():
        if name not in ids_by_name or not entries:
            raise ValueError(f"unknown or empty rarity pool: {name}")
        start = len(rows)
        for entry in entries:
            minimum, maximum = entry["min_level"], entry["max_level"]
            weight, item_type = entry["weight"], entry["item_type"]
            if not (0 <= minimum <= maximum <= 99999 and 0 < weight <= 65535
                    and 0 <= item_type <= 65535 and item_type in item_meta):
                raise ValueError(f"invalid rarity record in {name}: {entry}")
            meta = item_meta[item_type]
            atlas = meta["atlas"]
            sprite = meta["sprite_index"]
            if atlas not in ATLAS_IDS or not -1 <= sprite <= 65534:
                raise ValueError(f"invalid sprite metadata for item type {item_type}")
            rows.append((minimum, maximum, weight, item_type,
                         1 if meta["unique"] else 0, ATLAS_IDS[atlas],
                         0xffff if sprite < 0 else sprite))
        direct.append((ids_by_name[name], start, len(entries)))

    nested = []
    edges = []
    leaf_names = set(rarity["pools"])
    nested_names = {pool["pool"] for pool in graph["nested_pools"]}
    for pool in graph["nested_pools"]:
        name = pool["pool"]
        if name not in ids_by_name:
            raise ValueError(f"unknown parent pool: {name}")
        start = len(edges)
        for entry in pool["entries"]:
            child = entry["pool"]
            if child not in leaf_names and child not in nested_names:
                raise ValueError(f"unresolved nested pool reference: {name}->{child}")
            weight = float(entry["weight"])
            scaled = round(weight * 1000)
            if not math.isfinite(weight) or weight <= 0 or abs(scaled / 1000 - weight) > 1e-5:
                raise ValueError(f"weight cannot be represented in VLT1: {name}->{child}={weight}")
            edges.append((entry["pool_id"], scaled))
        nested.append((ids_by_name[name], start, len(pool["entries"])))

    rows_by_pool = rarity["pools"]
    children_by_pool = {pool["pool"]: [entry["pool"] for entry in pool["entries"]]
                        for pool in graph["nested_pools"]}

    def can_yield(pool_name: str, level: int, ancestors=()) -> bool:
        if pool_name in ancestors:
            raise ValueError(f"nested pool cycle: {' -> '.join(ancestors + (pool_name,))}")
        if any(row["min_level"] <= level <= row["max_level"] and row["weight"] > 0
               for row in rows_by_pool.get(pool_name, [])):
            return True
        return any(can_yield(child, level, ancestors + (pool_name,))
                   for child in children_by_pool.get(pool_name, []))

    for level in range(11):
        if not can_yield("POOL_CHEST", level):
            raise ValueError(f"POOL_CHEST has no level-eligible item at level {level}")

    if len(rows) != rarity["rarity_rows"] or len(rows) > 65535 \
            or len(direct) > 65535 or len(nested) > 65535 or len(edges) > 65535:
        raise ValueError("loot table counts do not fit VLT1")
    output = bytearray(HEADER.pack(MAGIC, VERSION, len(direct), len(rows),
                                   len(nested), len(edges)))
    for pool_id, start, count in direct:
        output.extend(DESCRIPTOR.pack(pool_id, 0, start, count))
    for minimum, maximum, weight, item_type, flags, atlas, sprite in rows:
        output.extend(RARITY.pack(minimum, maximum, weight, item_type, flags,
                                  atlas, sprite))
    for pool_id, start, count in nested:
        output.extend(DESCRIPTOR.pack(pool_id, 0, start, count))
    for child_id, weight_milli in edges:
        output.extend(EDGE.pack(child_id, 0, weight_milli))
    return bytes(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rarity", type=Path, required=True)
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--sprites", type=Path, required=True,
                        help="verified item type and unique flag metadata")
    parser.add_argument("--assets", type=Path,
                        help="private assets root; validates every explicit sprite index against PNG dimensions")
    parser.add_argument("--enums", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    pools, _items = _pool_names(args.enums)
    sprite_doc = json.loads(args.sprites.read_text(encoding="utf-8"))
    sizes = None
    if args.assets:
        try:
            from PIL import Image
        except ImportError as exc:
            raise SystemExit("Pillow is required to validate sprite bounds with --assets") from exc
        sizes = {}
        for atlas_name, atlas_id in ATLAS_IDS.items():
            with Image.open(args.assets / "gfx" / atlas_name) as image:
                sizes[atlas_id] = image.size
    binary = compile_tables(json.loads(args.rarity.read_text(encoding="utf-8")),
                            json.loads(args.graph.read_text(encoding="utf-8")),
                            sprite_doc, pools, sizes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(binary)
    magic, version, direct_count, row_count, nested_count, edge_count = HEADER.unpack_from(binary)
    print(json.dumps({"format": magic.decode("ascii"), "version": version,
                      "direct_pools": direct_count, "rarity_rows": row_count,
                      "nested_pools": nested_count, "nested_edges": edge_count,
                      "bytes": len(binary), "output": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
