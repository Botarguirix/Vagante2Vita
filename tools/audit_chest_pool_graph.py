#!/usr/bin/env python3
"""Audit the Ghidra-recovered nested POOL_CHEST graph without guessing weights."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.extract_item_pools import _listing_instructions, _pe_layout, _pool_names, _read_words


CHEST_ROOT = "ItemPoolRarity::ItemPoolRarity(&local_c0,POOL_WEAPONS"
POOL_CALL = re.compile(r"ItemPoolRarity::ItemPoolRarity\s*\((.*?)\);", re.DOTALL)
POOL_KEY = re.compile(r"local_f78\s*=.*?\*\)0x([0-9a-fA-F]+);", re.DOTALL)
POOL_NAME = re.compile(r"\b(POOL_[A-Z0-9_]+)\b")
CALL_POOL_RARITY = re.compile(r"CALL 0x0063cd10$", re.IGNORECASE)
CALL_VECTOR_INIT = re.compile(r"CALL 0x00413b70$", re.IGNORECASE)
POOL_MAP_KEY = re.compile(r"MOV dword ptr \[EBP \+ 0xfffff08c\],0x([0-9a-f]+)$", re.IGNORECASE)
POOL_PUSH = re.compile(r"PUSH 0x([0-9a-f]+)$", re.IGNORECASE)
WEIGHT_LOAD = re.compile(r"MOVSS XMM2,dword ptr \[(0x[0-9a-f]+)\]$", re.IGNORECASE)


def extract_graph(pseudocode: str) -> dict:
    """Recover chest category edges and preserve unresolved weight expressions."""
    marker = pseudocode.find(CHEST_ROOT)
    if marker < 0:
        raise ValueError("could not find POOL_CHEST initializer in Ghidra pseudocode")
    end = pseudocode.find("ItemPool::operator=(pIVar80,pIVar78);", marker)
    if end < 0:
        raise ValueError("could not find POOL_CHEST map assignment")
    block = pseudocode[marker:end]
    keys = list(POOL_KEY.finditer(block))
    if not keys or int(keys[-1].group(1), 16) != 23:
        raise ValueError("chest category initializer is not assigned to enum 23")

    categories = []
    for call in POOL_CALL.finditer(block):
        args = re.sub(r"\s+", " ", call.group(1)).strip()
        name = POOL_NAME.search(args)
        if not name:
            raise ValueError(f"missing ItemPoolType in initializer: {args}")
        tail = args.rsplit(",", 1)[-1].strip()
        weight = re.sub(r"^\(float\)\s*", "", tail)
        categories.append({"pool": name.group(1), "weight_expression": weight,
                           "weight_resolved": bool(re.fullmatch(r"\d+(?:\.\d+)?", weight))})
    if [item["pool"] for item in categories] != [
            "POOL_WEAPONS", "POOL_ARMOR", "POOL_OTHER", "POOL_WANDS"]:
        raise ValueError("unexpected or incomplete POOL_CHEST category graph")
    return {
        "format": "vagante-chest-pool-graph", "version": 1,
        "verified_structure": True,
        "verified_weights": all(item["weight_resolved"] for item in categories),
        "root": {"pool": "POOL_CHEST", "enum": 23},
        "categories": categories,
    }


def chest_weight_references(listing: str) -> list[dict]:
    """Resolve the four POOL_CHEST ItemPoolRarity arguments from machine code."""
    instructions = _listing_instructions(listing)
    map_indexes = [i for i, (_address, text) in enumerate(instructions)
                   if POOL_MAP_KEY.search(text)]
    for map_index in map_indexes:
        next_window = instructions[map_index + 1:map_index + 8]
        if not any(re.search(r"CALL 0x00645d90", text, re.IGNORECASE)
                   for _address, text in next_window):
            continue
        vector_indexes = [i for i in range(map_index)
                          if CALL_VECTOR_INIT.search(instructions[i][1])]
        if not vector_indexes:
            continue
        vector_index = vector_indexes[-1]
        call_indexes = [i for i in range(max(0, vector_index - 30), vector_index)
                        if CALL_POOL_RARITY.search(instructions[i][1])]
        if len(call_indexes) != 4:
            continue
        refs = []
        for call_index in call_indexes:
            previous_call = max((i for i in call_indexes if i < call_index), default=call_index - 8)
            segment = [text for _address, text in instructions[max(0, previous_call + 1):call_index]]
            pushes = [POOL_PUSH.search(text) for text in segment]
            loads = [WEIGHT_LOAD.search(text) for text in segment]
            pushes = [match for match in pushes if match]
            loads = [match for match in loads if match]
            if not pushes or not loads:
                raise ValueError("POOL_CHEST category is missing its pool id or static weight address")
            refs.append({"pool_id": int(pushes[-1].group(1), 16),
                         "weight_va": int(loads[-1].group(1), 16)})
        if [item["pool_id"] for item in refs] != [15, 6, 22, 11]:
            raise ValueError("unexpected POOL_CHEST category ids in constructor listing")
        return refs
    raise ValueError("could not associate POOL_CHEST vector with its constructor listing")


def resolve_chest_weights(exe: bytes, listing: str, enums_path: Path) -> list[dict]:
    """Read chest category weights from the user's PE and verify PDB names."""
    pool_names, _item_names = _pool_names(enums_path)
    refs = chest_weight_references(listing)
    expected = ["POOL_WEAPONS", "POOL_ARMOR", "POOL_OTHER", "POOL_WANDS"]
    base, sections = _pe_layout(exe)
    weights = []
    for ref, expected_name in zip(refs, expected):
        pool_id = ref["pool_id"]
        if pool_names.get(pool_id) != expected_name:
            raise ValueError(f"PDB enum mismatch for pool {pool_id}: {pool_names.get(pool_id)}")
        word = _read_words(exe, base, sections, ref["weight_va"])[0]
        weight = struct.unpack("<f", struct.pack("<I", word))[0]
        if not math.isfinite(weight) or weight <= 0:
            raise ValueError(f"invalid weight for {expected_name}: {weight}")
        weights.append({"pool": expected_name, "pool_id": pool_id,
                        "weight_va": f"0x{ref['weight_va']:08x}", "weight": weight})
    if not math.isclose(sum(item["weight"] for item in weights), 100.0, abs_tol=1e-6):
        raise ValueError("POOL_CHEST category weights do not sum to 100")
    return weights


def resolve_nested_pools(exe: bytes, listing: str, enums_path: Path) -> list[dict]:
    """Recover every weighted ItemPoolRarity vector in the constructor."""
    pool_names, _item_names = _pool_names(enums_path)
    instructions = _listing_instructions(listing)
    base, sections = _pe_layout(exe)
    graphs = []
    for map_index, (_address, text) in enumerate(instructions):
        key = POOL_MAP_KEY.search(text)
        if not key or not any(re.search(r"CALL 0x00645d90", later, re.IGNORECASE)
                              for _va, later in instructions[map_index + 1:map_index + 8]):
            continue
        parent_id = int(key.group(1), 16)
        parent_name = pool_names.get(parent_id)
        if not parent_name:
            raise ValueError(f"PDB enum missing parent pool {parent_id}")
        vector_indexes = [i for i in range(map_index)
                          if CALL_VECTOR_INIT.search(instructions[i][1])]
        if not vector_indexes:
            continue
        vector_index = vector_indexes[-1]
        start = max(0, vector_index - 35)
        calls = [i for i in range(start, vector_index)
                 if CALL_POOL_RARITY.search(instructions[i][1])]
        if not calls:
            continue
        entries = []
        for n, call_index in enumerate(calls):
            lower = calls[n - 1] + 1 if n else max(start, call_index - 8)
            segment = [line for _va, line in instructions[lower:call_index]]
            pushes = [POOL_PUSH.search(line) for line in segment]
            loads = [WEIGHT_LOAD.search(line) for line in segment]
            pushes = [match for match in pushes if match]
            loads = [match for match in loads if match]
            if not pushes or not loads:
                raise ValueError(f"missing static weight for nested pool {parent_name}")
            child_id = int(pushes[-1].group(1), 16)
            child_name = pool_names.get(child_id)
            if not child_name:
                raise ValueError(f"PDB enum missing child pool {child_id}")
            weight_va = int(loads[-1].group(1), 16)
            word = _read_words(exe, base, sections, weight_va)[0]
            weight = struct.unpack("<f", struct.pack("<I", word))[0]
            if not math.isfinite(weight) or weight <= 0:
                raise ValueError(f"invalid {parent_name}->{child_name} weight: {weight}")
            entries.append({"pool": child_name, "pool_id": child_id,
                            "weight_va": f"0x{weight_va:08x}", "weight": weight})
        graphs.append({"pool": parent_name, "pool_id": parent_id, "entries": entries})
    if not graphs:
        raise ValueError("no nested ItemPoolRarity vectors found")
    by_name = {graph["pool"]: graph for graph in graphs}
    expected = {
        "POOL_CHEST": {"POOL_WEAPONS", "POOL_ARMOR", "POOL_OTHER", "POOL_WANDS"},
        "POOL_POTIONS": {"POOL_POTIONS_GOOD", "POOL_POTIONS_BAD"},
        "POOL_WEAPONS": {"POOL_SWORDS", "POOL_DAGGERS", "POOL_CLUBS", "POOL_BOWS",
                          "POOL_FIST", "POOL_AXES", "POOL_RODS"},
        "POOL_ARMOR": {"POOL_AMULETS", "POOL_BOOTS", "POOL_CHEST_ARMOR", "POOL_GLOVES",
                        "POOL_HELMETS", "POOL_RINGS"},
    }
    for parent, child_names in expected.items():
        if parent not in by_name or {entry["pool"] for entry in by_name[parent]["entries"]} != child_names:
            raise ValueError(f"unexpected nested pool graph for {parent}")
    return graphs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pseudo", type=Path, required=True,
                        help="user-local ItemGenerator constructor pseudocode")
    parser.add_argument("--listing", type=Path, help="constructor machine-code listing")
    parser.add_argument("--exe", type=Path, help="user-owned executable analyzed in Ghidra")
    parser.add_argument("--enums", type=Path, help="user-local Ghidra enum export")
    parser.add_argument("--output", type=Path, required=True,
                        help="write the audited graph to a local file")
    args = parser.parse_args()
    source = args.pseudo.read_bytes()
    result = extract_graph(source.decode("utf-8", errors="replace"))
    result["source_sha256"] = hashlib.sha256(source).hexdigest()
    if any((args.listing, args.exe, args.enums)) and not all((args.listing, args.exe, args.enums)):
        parser.error("--listing, --exe and --enums must be provided together")
    if args.listing:
        listing = args.listing.read_text(encoding="utf-8", errors="replace")
        executable = args.exe.read_bytes()
        weights = resolve_chest_weights(args.exe.read_bytes(),
            listing, args.enums)
        nested_pools = resolve_nested_pools(executable, listing, args.enums)
        result["weights"] = weights
        result["nested_pools"] = nested_pools
        for category, weight in zip(result["categories"], weights):
            category["weight"] = weight["weight"]
            category["weight_va"] = weight["weight_va"]
            category["weight_resolved"] = True
        result["verified_weights"] = True
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verified_structure": result["verified_structure"],
                      "verified_weights": result["verified_weights"],
                      "weights": result.get("weights"),
                      "nested_pools": [{"pool": pool["pool"],
                                        "entries": len(pool["entries"])}
                                       for pool in result.get("nested_pools", [])],
                      "output": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
