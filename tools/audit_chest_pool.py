#!/usr/bin/env python3
"""Verify chest categories and selected item pools from a local Ghidra listing and EXE."""
from __future__ import annotations

import argparse
import json
import math
import re
import struct
from pathlib import Path


POOL_NAMES = {
    0: "HELMETS", 1: "CHEST_ARMOR", 2: "GLOVES", 3: "BOOTS",
    4: "RINGS", 5: "AMULETS", 6: "ARMOR", 7: "DAGGERS",
    8: "SWORDS", 9: "CLUBS", 10: "BOWS", 11: "WANDS",
    12: "RODS", 13: "FIST", 14: "AXES", 15: "WEAPONS",
    22: "OTHER",
}
ITEM_NAMES = {
    45: "DAGGER", 46: "DAGGER2", 47: "DAGGER3",
    49: "SWORD", 50: "SWORD2", 51: "SWORD3", 52: "SWORD4", 53: "SWORD5",
    55: "CLUB", 56: "CLUB2", 57: "CLUB3", 58: "CLUB4", 59: "CLUB5",
    60: "CLUB6", 61: "CLUB7", 62: "FLAIL",
    64: "AXE", 65: "AXE2", 85: "FIST_WEAPON", 86: "FIST_WEAPON2",
    87: "QATAR", 93: "BOW", 94: "LONG_BOW", 95: "CROSS_BOW",
}
INSTRUCTION = re.compile(r"^([0-9a-fA-F]{8})\s+(.*?)\s*$")
FLOAT_LOAD = re.compile(r"MOVSS XMM2,dword ptr \[(0x[0-9a-fA-F]+)\]")
PUSH_IMMEDIATE = re.compile(r"PUSH (0x[0-9a-fA-F]+)$")
CALL = re.compile(r"CALL 0x0063cd10$", re.IGNORECASE)


def pe_sections(data: bytes) -> tuple[int, list[tuple[str, int, int, int, int]]]:
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
    for index in range(count):
        offset = section_table + index * 40
        name = data[offset:offset + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", data, offset + 8)
        sections.append((name, rva, virtual_size, raw_offset, raw_size))
    return image_base, sections


def read_float(data: bytes, image_base: int, sections: list[tuple[str, int, int, int, int]], va: int) -> float:
    rva = va - image_base
    for _name, section_rva, virtual_size, raw_offset, raw_size in sections:
        span = min(virtual_size, raw_size)
        if section_rva <= rva and rva + 4 <= section_rva + span:
            value = struct.unpack_from("<f", data, raw_offset + rva - section_rva)[0]
            if not math.isfinite(value):
                break
            return value
    raise ValueError(f"VA 0x{va:08x} is not backed by finite PE data")


def read_words(data: bytes, image_base: int, sections: list[tuple[str, int, int, int, int]], va: int, count: int) -> list[int]:
    rva = va - image_base
    for _name, section_rva, virtual_size, raw_offset, raw_size in sections:
        span = min(virtual_size, raw_size)
        if section_rva <= rva and rva + count * 4 <= section_rva + span:
            return list(struct.unpack_from("<" + "I" * count, data, raw_offset + rva - section_rva))
    raise ValueError(f"VA 0x{va:08x} is not backed by {count * 4} bytes in the PE")


def verify_sword_pool(instructions: list[tuple[int, str]], data: bytes, image_base: int,
                      sections: list[tuple[str, int, int, int, int]]) -> list[dict[str, object]]:
    by_address = dict(instructions)
    first_record = read_words(data, image_base, sections, 0x00B20FB0, 4)
    if first_record != [0, 3, 1, 49]:
        raise SystemExit(f"Unexpected first sword rarity record: {first_record!r}")
    rows = [{"item_type": first_record[3], "name": ITEM_NAMES[first_record[3]],
             "min_level": first_record[0], "max_level": first_record[1], "weight": float(first_record[2])}]
    record_addresses = [(0x0063F7F4, 0x0063F7FF, 0x0063F80D),
                        (0x0063F82C, 0x0063F837, 0x0063F845),
                        (0x0063F864, 0x0063F86F, 0x0063F87D),
                        (0x0063F89C, 0x0063F8A7, 0x0063F8AE)]
    for minimum_address, weight_address, type_address in record_addresses:
        values = []
        for address in (minimum_address, weight_address, type_address):
            text = by_address.get(address, "")
            match = re.search(r"MOV dword ptr \[EBP \+ -0x[0-9a-f]+\],0x([0-9a-f]+)$", text, re.IGNORECASE)
            if not match:
                raise SystemExit(f"Cannot verify expected sword-pool instruction at 0x{address:08x}")
            values.append(int(match.group(1), 16))
        minimum, weight, item_type = values
        if item_type not in ITEM_NAMES or weight != 1:
            raise SystemExit(f"Unexpected sword rarity tuple at 0x{minimum_address:08x}: {values!r}")
        rows.append({"item_type": item_type, "name": ITEM_NAMES[item_type],
                     "min_level": minimum, "max_level": 99999, "weight": float(weight)})
    return rows


def verify_inline_pool(instructions: list[tuple[int, str]], pool_name: str,
                       expected: list[tuple[int, int, int, int]]) -> list[dict[str, object]]:
    """Verify (minimum, weight, type) initializer triples before Ghidra constructor calls."""
    by_address = dict(instructions)
    rows = []
    immediate = re.compile(r"MOV dword ptr \[[^]]+\],0x([0-9a-f]+)$", re.IGNORECASE)
    for call_address, minimum, weight, item_type in expected:
        text = by_address.get(call_address, "")
        if not re.search(r"CALL 0x0063cc40$", text, re.IGNORECASE):
            raise SystemExit(f"Cannot verify {pool_name} constructor call at 0x{call_address:08x}")
        call_index = next(i for i, (address, _text) in enumerate(instructions) if address == call_address)
        values = []
        for _address, previous in instructions[max(0, call_index - 15):call_index]:
            match = immediate.search(previous)
            if match:
                values.append(int(match.group(1), 16))
        actual = values[-3:]
        wanted = [minimum, weight, item_type]
        if actual != wanted:
            raise SystemExit(f"Unexpected {pool_name} rarity at 0x{call_address:08x}: {actual!r}, expected {wanted!r}")
        rows.append({"instruction": f"0x{call_address:08x}", "item_type": item_type,
                     "name": ITEM_NAMES.get(item_type, f"UNKNOWN_{item_type}"),
                     "min_level": minimum, "max_level": 99999, "weight": weight})
    return rows


def verify_axe_pool(instructions: list[tuple[int, str]], data: bytes, image_base: int,
                    sections: list[tuple[str, int, int, int, int]]) -> list[dict[str, object]]:
    """The first axe record is loaded as a four-word static initializer."""
    record = read_words(data, image_base, sections, 0x00B20FD0, 4)
    if record != [0, 4, 1, 64]:
        raise SystemExit(f"Unexpected first axe rarity record: {record!r}")
    rows = [{"item_type": 64, "name": ITEM_NAMES[64], "min_level": 0,
             "max_level": 4, "weight": 1}]
    rows.extend(verify_inline_pool(instructions, "axe", [(0x0063FAD9, 3, 1, 65)]))
    return rows


def parse_instructions(lines: list[str]) -> list[tuple[int, str]]:
    instructions = []
    for line in lines:
        match = INSTRUCTION.match(line)
        if match:
            instructions.append((int(match.group(1), 16), match.group(2)))
    return instructions


def pool_entries(lines: list[str], start: int, end: int, data: bytes,
                 image_base: int, sections: list[tuple[str, int, int, int, int]]) -> list[dict[str, object]]:
    instructions = parse_instructions(lines)
    result = []
    for index, (address, text) in enumerate(instructions):
        if not start <= address <= end:
            continue
        load = FLOAT_LOAD.search(text)
        if not load:
            continue
        pool_id = None
        for _next_address, following in instructions[index + 1:index + 5]:
            push = PUSH_IMMEDIATE.search(following)
            if push:
                pool_id = int(push.group(1), 16)
            if CALL.search(following):
                break
        else:
            continue
        if pool_id is None or not CALL.search(following):
            continue
        result.append({
            "instruction": f"0x{address:08x}",
            "pool_id": pool_id,
            "pool": POOL_NAMES.get(pool_id, f"UNKNOWN_{pool_id}"),
            "weight_va": load.group(1),
            "weight": read_float(data, image_base, sections, int(load.group(1), 16)),
        })
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="user-owned Vagante executable used for this Ghidra analysis")
    parser.add_argument("--listing", type=Path, required=True, help="DumpFunctionListing output for ItemGenerator::ItemGenerator")
    args = parser.parse_args()
    data = args.exe.read_bytes()
    image_base, sections = pe_sections(data)
    lines = args.listing.read_text(encoding="utf-8", errors="replace").splitlines()
    instructions = parse_instructions(lines)
    chest = pool_entries(lines, 0x00641B6A, 0x00641BB9, data, image_base, sections)
    weapons = pool_entries(lines, 0x00641CDE, 0x00641D6C, data, image_base, sections)
    armor = pool_entries(lines, 0x00641DF8, 0x00641E69, data, image_base, sections)
    swords = verify_sword_pool(instructions, data, image_base, sections)
    daggers = verify_inline_pool(instructions, "dagger", [
        (0x0063F6CD, 0, 2, 45), (0x0063F705, 1, 3, 46), (0x0063F73D, 2, 2, 47),
    ])
    fists = verify_inline_pool(instructions, "fist", [
        (0x0063F985, 1, 3, 85), (0x0063F9BD, 3, 2, 86), (0x0063F9F5, 5, 2, 87),
    ])
    bows = verify_inline_pool(instructions, "bow", [
        (0x0063FB8F, 0, 3, 93), (0x0063FBC7, 2, 2, 94), (0x0063FBFF, 1, 2, 95),
    ])
    clubs = verify_inline_pool(instructions, "club", [
        (0x0063FCB8, 0, 2, 55), (0x0063FCF0, 0, 2, 56), (0x0063FD28, 1, 2, 57),
        (0x0063FD60, 1, 2, 62), (0x0063FD98, 2, 2, 58), (0x0063FDD0, 3, 2, 59),
        (0x0063FE17, 3, 2, 60), (0x0063FE5E, 4, 2, 61),
    ])
    axes = verify_axe_pool(instructions, data, image_base, sections)
    expected_chest = [(15, 30.0), (6, 60.0), (22, 4.0), (11, 6.0)]
    expected_weapons = [(8, 1.0), (7, 1.0), (9, 1.0), (10, 1.0), (13, 1.0), (14, 1.0), (12, 1.0)]
    expected_armor = [(5, 0.7), (3, 1.0), (1, 1.1), (2, 1.0), (0, 1.0), (4, 0.75)]

    def matches(actual: list[dict[str, object]], expected: list[tuple[int, float]]) -> bool:
        return len(actual) == len(expected) and all(
            row["pool_id"] == pool_id and math.isclose(row["weight"], weight, rel_tol=0, abs_tol=1e-6)
            for row, (pool_id, weight) in zip(actual, expected)
        )

    if not matches(chest, expected_chest) or not matches(weapons, expected_weapons) or not matches(armor, expected_armor):
        raise SystemExit(f"Unexpected constructor data: chest={chest!r}, weapons={weapons!r}, armor={armor!r}")
    print(json.dumps({"verified": True, "chest_categories": chest, "weapon_categories": weapons,
                      "armor_categories": armor, "sword_items": swords, "dagger_items": daggers,
                      "fist_items": fists, "bow_items": bows, "club_items": clubs,
                      "axe_items": axes}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
