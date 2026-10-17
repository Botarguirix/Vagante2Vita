#!/usr/bin/env python3
"""Extract item sprite indices from Ghidra pseudocode and PDB enums."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


CASE = re.compile(r"^  case (ITEMTYPE_[A-Z0-9_]+):$")
ANY_CASE = re.compile(r"^\s*case (ITEMTYPE_[A-Z0-9_]+):$")
SPRITE = re.compile(r"this->spriteIndex\s*=\s*(0x[0-9a-fA-F]+|[0-9]+);\s*$")


def parse_sprite_indices(pseudocode: str) -> tuple[dict[str, int], list[str]]:
    """Map every pending enum case to its spriteIndex assignment."""
    result: dict[str, int] = {}
    pending: list[str] = []
    unresolved: list[str] = []
    for line in pseudocode.splitlines():
        case = CASE.match(line)
        if case:
            pending.append(case.group(1))
            continue
        assignment = SPRITE.search(line.strip())
        if assignment and pending:
            index = int(assignment.group(1), 0)
            for name in pending:
                previous = result.get(name)
                if previous is not None and previous != index:
                    raise ValueError(f"conflicting spriteIndex for {name}: {previous} vs {index}")
                result[name] = index
            pending.clear()
        elif line.strip() == "break;" and pending:
            unresolved.extend(pending)
            pending.clear()
    unresolved.extend(pending)
    return result, unresolved


def parse_unique_types(pseudocode: str) -> set[str]:
    """Collect switch cases that set Item::unique before their break."""
    pending: list[str] = []
    unique: set[str] = set()
    for line in pseudocode.splitlines():
        case = ANY_CASE.match(line)
        if case:
            pending.append(case.group(1))
        if "this->unique = true;" in line:
            unique.update(pending)
        if line.strip() == "break;":
            pending.clear()
    return unique


def atlas_for(name: str, item_id: int) -> str:
    """Resolve the atlas from the observed Item predicates and item type."""
    if 45 <= item_id <= 90 or 93 <= item_id <= 96 or name in {
        "ITEMTYPE_BOOMERANG", "ITEMTYPE_ENCHANTED_BOOMERANG"
    }:
        return "items/items-weapons.png"
    if name.startswith(("ITEMTYPE_HELMET", "ITEMTYPE_SHIRT", "ITEMTYPE_GLOVES",
                       "ITEMTYPE_BOOTS", "ITEMTYPE_RING", "ITEMTYPE_AMULET")):
        return "items/items-armor.png"
    if name.startswith("ITEMTYPE_POTION_"):
        return "items/potions.png"
    if name.startswith("ITEMTYPE_SCROLL_"):
        return "items/scrolls.png"
    if name == "ITEMTYPE_SPELL_BOOK":
        return "items/spell_books.png"
    if name in {"ITEMTYPE_BOMB", "ITEMTYPE_BOOMERANG", "ITEMTYPE_ENCHANTED_BOOMERANG",
                "ITEMTYPE_DYNAMITE", "ITEMTYPE_GRENADE"}:
        return "items/items-explosives.png"
    return "items/items.png"


def read_enums(path: Path) -> dict[str, int]:
    result = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split("\t")
        if len(fields) == 3 and fields[0] == "/vagante.pdb/Item/ItemType" \
                and fields[1].startswith("ITEMTYPE_"):
            result.setdefault(fields[1], int(fields[2]))
    return result


def extract(pseudo_path: Path, enums_path: Path, rarity_path: Path | None = None):
    pseudocode = pseudo_path.read_text(encoding="utf-8", errors="replace")
    enums = read_enums(enums_path)
    sprites, unresolved = parse_sprite_indices(pseudocode)
    unique_types = parse_unique_types(pseudocode)
    missing_enums = sorted(set(sprites) - enums.keys())
    if missing_enums:
        raise ValueError(f"sprite cases missing PDB enum values: {missing_enums[:5]}")
    required_names: set[str] = set()
    if rarity_path:
        tables = json.loads(rarity_path.read_text(encoding="utf-8"))
        for entries in tables["pools"].values():
            required_names.update(entry.get("name") for entry in entries if entry.get("name"))
    case_names = set(sprites) | set(unresolved)
    missing_required = sorted(required_names - case_names)
    return {
        "format": "vagante-item-sprite-indexes", "version": 1,
        "verified": not missing_required,
        "case_count": len(sprites), "unresolved_case_count": len(unresolved),
        "unique_type_count": len(unique_types),
        "required_pool_types": len(required_names), "missing_pool_types": missing_required,
        "sources": {
            "pseudocode_sha256": hashlib.sha256(pseudo_path.read_bytes()).hexdigest(),
            "enums_sha256": hashlib.sha256(enums_path.read_bytes()).hexdigest(),
        },
        "items": [
            {"name": name, "item_type": enums[name], "unique": name in unique_types,
             "sprite_index": sprites.get(name, -1),
             "index_source": "case-assignment" if name in sprites else "function-default-minus-one",
             "atlas": atlas_for(name, enums[name])}
            for name in sorted(required_names, key=lambda item: enums[item])
        ],
        "uses_default_sprite_index": sorted(set(unresolved)),
        "unresolved": missing_required,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pseudo", type=Path, required=True)
    parser.add_argument("--enums", type=Path, required=True)
    parser.add_argument("--rarity-tables", type=Path)
    parser.add_argument("--output", type=Path, required=True,
                        help="keep extracted metadata in a local directory")
    args = parser.parse_args()
    result = extract(args.pseudo, args.enums, args.rarity_tables)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verified": result["verified"], "case_count": result["case_count"],
                      "default_index_case_count": result["unresolved_case_count"],
                      "required_pool_types": result["required_pool_types"],
                      "missing_pool_types": result["missing_pool_types"],
                      "output": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
