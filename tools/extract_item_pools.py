#!/usr/bin/env python3
"""Extract ItemRarity tables from a private Ghidra listing and PE executable."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


INSTRUCTION = re.compile(r"^([0-9a-fA-F]{8})\s+(.*?)\s*$")
RARITY_CALL = re.compile(r"CALL 0x0063cc40$", re.IGNORECASE)
INIT_CALL = re.compile(r"CALL 0x00413b70$", re.IGNORECASE)
POOL_ASSIGNMENT = re.compile(
    r"(?:local_f78\s*=\s*\(map<ItemGenerator::ItemPoolType.*?\)|"
    r"local_1008\s*=\s*\(map<Item::ItemType.*?\))0x([0-9a-fA-F]+);",
    re.DOTALL,
)


def _pe_layout(data: bytes):
    if data[:2] != b"MZ" or len(data) < 0x40:
        raise ValueError("input is not a PE executable")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("invalid PE signature")
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("expected a PE32 executable")
    image_base = struct.unpack_from("<I", data, optional + 28)[0]
    section_table = optional + optional_size
    sections = []
    for i in range(count):
        offset = section_table + i * 40
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", data, offset + 8)
        sections.append((rva, min(virtual_size, raw_size), raw_offset))
    return image_base, sections


def _read_words(data: bytes, image_base: int, sections, va: int) -> list[int]:
    rva = va - image_base
    for section_rva, span, raw_offset in sections:
        if section_rva <= rva and rva + 16 <= section_rva + span:
            return list(struct.unpack_from("<IIII", data, raw_offset + rva - section_rva))
    raise ValueError(f"VA 0x{va:08x} is not backed by four PE words")


def _stack_offset(text: str):
    match = re.search(r"\[EBP \+ (0x[0-9a-f]+|-0x[0-9a-f]+)\]", text, re.IGNORECASE)
    if not match:
        return None
    value = int(match.group(1), 16) if not match.group(1).startswith("-") else -int(match.group(1)[1:], 16)
    return value - 0x100000000 if value > 0x7FFFFFFF else value


def _listing_instructions(text: str):
    result = []
    for line in text.splitlines():
        match = INSTRUCTION.match(line)
        if match:
            result.append((int(match.group(1), 16), match.group(2)))
    return result


def _rarity_rows(instructions, data: bytes, image_base: int, sections):
    calls = [i for i, (_address, text) in enumerate(instructions) if RARITY_CALL.search(text)]
    if len(calls) != 170:
        raise ValueError(f"expected 170 ItemRarity calls, found {len(calls)}")
    rows = []
    for call_number, call_index in enumerate(calls):
        previous = calls[call_number - 1] if call_number else -1
        helper = next((i for i in range(call_index - 1, previous, -1)
                       if INIT_CALL.search(instructions[i][1])), None)
        if helper is None:
            raise ValueError(f"missing initializer helper before 0x{instructions[call_index][0]:08x}")
        segment = instructions[previous + 1:helper + 1]
        bases = [_stack_offset(text) for _address, text in segment
                 if text.startswith("LEA EAX,") and _stack_offset(text) is not None]
        if not bases:
            raise ValueError(f"cannot find initializer stack base before 0x{instructions[call_index][0]:08x}")
        base = bases[-1]
        immediate = {}
        for _address, text in segment:
            match = re.search(
                r"MOV dword ptr \[EBP \+ (0x[0-9a-f]+|-0x[0-9a-f]+)\],0x([0-9a-f]+)$",
                text, re.IGNORECASE,
            )
            if match:
                offset = _stack_offset(text)
                immediate[offset] = int(match.group(2), 16)

        if all(base + offset in immediate for offset in (0, 4, 8)):
            if base + 12 in immediate:
                minimum, maximum, weight, item_type = [immediate[base + offset]
                                                        for offset in (0, 4, 8, 12)]
                record_size = 4
            else:
                minimum, weight, item_type = [immediate[base + offset]
                                               for offset in (0, 4, 8)]
                maximum, record_size = 99999, 3
        else:
            store = next((_stack_offset(text) for _address, text in segment
                          if "MOVUPS xmmword ptr" in text and _stack_offset(text) is not None), None)
            load = next((int(match.group(1), 16) for _address, text in segment
                         if (match := re.search(
                             r"MOVAPS XMM0,xmmword ptr \[(0x[0-9a-f]+)\]", text,
                             re.IGNORECASE,
                         ))), None)
            if store != base or load is None:
                raise ValueError(f"unrecognized ItemRarity initializer at 0x{instructions[call_index][0]:08x}")
            minimum, maximum, weight, item_type = _read_words(data, image_base, sections, load)
            record_size = 4

        if (minimum < 0 or minimum > maximum or maximum > 99999 or not weight
                or item_type > 0xFFFF):
            raise ValueError(f"invalid ItemRarity data at 0x{instructions[call_index][0]:08x}")
        rows.append({
            "instruction": f"0x{instructions[call_index][0]:08x}",
            "min_level": minimum, "max_level": maximum,
            "weight": weight, "item_type": item_type,
            "initializer_words": record_size,
        })
    return rows


def _pool_names(enums_path: Path | None):
    if enums_path is None:
        return {}
    pool_names = {}
    item_names = {}
    for line in enums_path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split("\t")
        if len(fields) != 3:
            continue
        path, name, value = fields
        if path == "/vagante.pdb/ItemGenerator/ItemPoolType" and name.startswith("POOL_"):
            pool_names[int(value)] = name
        elif path == "/vagante.pdb/Item/ItemType" and name.startswith("ITEMTYPE_"):
            item_names.setdefault(int(value), name)
    return pool_names, item_names


def extract(exe_path: Path, listing_path: Path, pseudo_path: Path,
            enums_path: Path | None = None):
    data = exe_path.read_bytes()
    image_base, sections = _pe_layout(data)
    instructions = _listing_instructions(listing_path.read_text(encoding="utf-8", errors="replace"))
    rows = _rarity_rows(instructions, data, image_base, sections)
    pseudo = pseudo_path.read_text(encoding="utf-8", errors="replace")
    row_calls = list(re.finditer(r"ItemRarity::ItemRarity\(", pseudo))
    if len(row_calls) != len(rows):
        raise ValueError(f"Ghidra pseudocode has {len(row_calls)} rows, listing has {len(rows)}")
    assignments = list(POOL_ASSIGNMENT.finditer(pseudo))
    if not assignments:
        raise ValueError("could not find pool assignments in Ghidra pseudocode")
    pools = {}
    cursor = 0
    previous_position = 0
    for match in assignments:
        row_count = sum(previous_position < call.start() < match.start() for call in row_calls)
        pool_id = int(match.group(1), 16)
        if cursor + row_count > len(rows):
            raise ValueError("pool assignments exceed recovered constructor rows")
        if row_count:
            if str(pool_id) in pools:
                raise ValueError(f"pool {pool_id} assigned more than once")
            pools[str(pool_id)] = rows[cursor:cursor + row_count]
            cursor += row_count
        previous_position = match.start()
    if cursor != len(rows):
        raise ValueError(f"pool assignments account for {cursor} of {len(rows)} rarity rows")
    pool_names, item_names = _pool_names(enums_path) if enums_path else ({}, {})
    named_pools = {}
    for pool_id, entries in pools.items():
        name = pool_names.get(int(pool_id), f"POOL_UNKNOWN_{pool_id}")
        named_pools[name] = [dict(entry, name=item_names.get(entry["item_type"])) for entry in entries]
    return {
        "format": "vagante-item-rarity-tables", "version": 1,
        "verified": True, "rarity_rows": len(rows),
        "sources": {
            "exe_sha256": hashlib.sha256(data).hexdigest(),
            "listing_sha256": hashlib.sha256(listing_path.read_bytes()).hexdigest(),
            "pseudocode_sha256": hashlib.sha256(pseudo_path.read_bytes()).hexdigest(),
        },
        "pools": named_pools,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="user-owned PE analyzed in Ghidra")
    parser.add_argument("--listing", type=Path, required=True, help="ItemGenerator constructor listing")
    parser.add_argument("--pseudo", type=Path, required=True, help="ItemGenerator constructor pseudocode")
    parser.add_argument("--enums", type=Path, help="optional user-local Ghidra enum export")
    parser.add_argument("--output", type=Path, required=True, help="keep extracted metadata in a local directory")
    args = parser.parse_args()
    result = extract(args.exe, args.listing, args.pseudo, args.enums)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verified": result["verified"], "rarity_rows": result["rarity_rows"],
                      "pools": {name: len(rows) for name, rows in result["pools"].items()},
                      "output": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
