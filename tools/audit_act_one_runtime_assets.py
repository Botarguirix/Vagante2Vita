"""Preflight ACT_ONE packets and visuals at the exact Vita runtime paths."""
from __future__ import annotations

import argparse
import collections
import json
import struct
import zlib
from pathlib import Path

PNG = b"\x89PNG\r\n\x1a\n"
DOODADS = {
    1: "counter.png", 2: "shop_sign.png", 3: "torch.png",
    4: "decorative_helmet.png", 5: "decorative_shield.png",
    13: "flower_1.png", 14: "spellbook_altar.png",
    19: "waterfall_1_2.png", 20: "waterfall_1_3.png",
    21: "waterfall_1_4.png",
}
ENEMIES = {9: "goblin", 10: "bat", 11: "lurker", 12: "slime", 14: "spider"}
ENEMY_STATES = ("stand", "move", "air", "hurt", "attack")
ITEM_ATLASES = {
    "items.png": 16, "items-weapons.png": 20, "items-armor.png": 20,
    "potions.png": 16, "scrolls.png": 20, "spell_books.png": 16,
    "items-explosives.png": 20,
}


def png_dimensions(path: Path) -> tuple[int, int]:
    """Check PNG chunk bounds, CRCs and compressed image data, then return size."""
    data = path.read_bytes()
    if len(data) < 33 or data[:8] != PNG:
        raise ValueError(f"{path}: invalid or truncated PNG")
    offset = 8
    width = height = 0
    saw_end = saw_data = False
    compressed = bytearray()
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        end = offset + 12 + length
        if end > len(data):
            raise ValueError(f"{path}: truncated PNG chunk")
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        crc = struct.unpack_from(">I", data, offset + 8 + length)[0]
        if zlib.crc32(kind + payload) & 0xFFFFFFFF != crc:
            raise ValueError(f"{path}: PNG chunk CRC mismatch")
        if offset == 8:
            if kind != b"IHDR" or length != 13:
                raise ValueError(f"{path}: missing first IHDR chunk")
            width, height = struct.unpack_from(">II", payload)
            if not width or not height:
                raise ValueError(f"{path}: empty PNG dimensions")
        elif kind == b"IDAT":
            saw_data = True
            compressed.extend(payload)
        elif kind == b"IEND":
            if length:
                raise ValueError(f"{path}: invalid IEND chunk")
            saw_end = True
            offset = end
            break
        offset = end
    if not saw_data or not saw_end or offset != len(data):
        raise ValueError(f"{path}: incomplete PNG stream")
    try:
        decoder = zlib.decompressobj()
        decoder.decompress(compressed)
        decoder.flush()
        if not decoder.eof:
            raise ValueError("incomplete zlib stream")
    except zlib.error as exc:
        raise ValueError(f"{path}: invalid PNG image data") from exc
    return width, height


def read_packet(path: Path, magic: bytes, record_size: int) -> tuple[bytes, list[bytes]]:
    data = path.read_bytes()
    if len(data) < 8 or data[:4] != magic:
        raise ValueError(f"{path}: expected {magic.decode()} packet")
    count, declared_size = struct.unpack_from("<HH", data, 4)
    if declared_size != record_size or len(data) != 8 + count * record_size:
        raise ValueError(f"{path}: invalid record size or count")
    return data, [data[8 + i * record_size:8 + (i + 1) * record_size]
                  for i in range(count)]


def require_png(root: Path, relative: str, checks: list[str],
                dimensions: tuple[int, int] | None = None,
                multiple: int | None = None,
                minimum: tuple[int, int] | None = None) -> tuple[int, int]:
    path = root / relative
    if not path.is_file():
        raise ValueError(f"Missing runtime image: {relative}")
    width, height = png_dimensions(path)
    if dimensions and (width, height) != dimensions:
        raise ValueError(f"{relative}: expected {dimensions[0]}x{dimensions[1]}, got {width}x{height}")
    if multiple and (width % multiple or height % multiple):
        raise ValueError(f"{relative}: dimensions must be multiples of {multiple}, got {width}x{height}")
    if minimum and (width < minimum[0] or height < minimum[1]):
        raise ValueError(f"{relative}: image is smaller than {minimum[0]}x{minimum[1]}")
    checks.append(relative)
    return width, height


def audit(root: Path) -> dict:
    root = root.resolve()
    floors = [
        (0, "act-one-world.vwi", "act-one-entities.vwe", "act-one-enemies.vwe", "intro-terrain.png"),
        (1, "floors/floor-01.vwi", "floors/floor-01-entities.vwe", "floors/floor-01-enemies.vwe", "floors/floor-01-terrain.png"),
        (2, "floors/floor-02.vwi", "floors/floor-02-entities.vwe", "floors/floor-02-enemies.vwe", "floors/floor-02-terrain.png"),
    ]
    checked: list[str] = []
    # ACT_ONE can generate Spider even when the currently selected seed does
    # not; keep its source sprites ready for later floor transitions.
    enemy_families: set[str] = {"spider"}
    entity_kinds: collections.Counter[int] = collections.Counter()
    doodad_variants: set[int] = set()
    for index, world_name, entities_name, enemies_name, terrain_name in floors:
        world = (root / world_name).read_bytes()
        if len(world) < 16 or world[:4] != b"VWI2":
            raise ValueError(f"{world_name}: expected VWI2 world")
        width, height = struct.unpack_from("<HH", world, 4)
        require_png(root, terrain_name, checked, dimensions=(width * 16, height * 16))
        _, entity_records = read_packet(root / entities_name, b"VWE2", 16)
        _, enemy_records = read_packet(root / enemies_name, b"VWE3", 16)
        for record in entity_records:
            kind, _, variant = struct.unpack_from("<BBH", record)
            entity_kinds[kind] += 1
            if kind == 19:
                doodad_variants.add(variant)
        for record in enemy_records:
            _, subtype = struct.unpack_from("<BB", record)
            if subtype not in ENEMIES:
                raise ValueError(f"{enemies_name}: unsupported enemy subtype {subtype}")
            enemy_families.add(ENEMIES[subtype])
        checked.extend((world_name, entities_name, enemies_name))

    for animation in ("stand", "walk", "jump", "fall", "duck", "climb", "lookup"):
        require_png(root, f"knight/{animation}.png", checked, multiple=24)
    for family in sorted(enemy_families):
        for state in ENEMY_STATES:
            image = f"enemies/{family}/{state}.png"
            width, height = require_png(root, image, checked)
            if width % 16 or (height != 17 if family == "goblin" else height % 16):
                expectation = "width multiple of 16 and height 17" if family == "goblin" else "dimensions multiple of 16"
                raise ValueError(f"{image}: expected {expectation}, got {width}x{height}")
    for kind, name in ((16, "chest.png"), (17, "spikes.png"),
                       (18, "arrow_trap.png"), (20, "trap_trigger.png"),
                       (21, "breakable_wall.png"), (22, "shake_boulder.png"),
                       (23, "boulder_blocker.png")):
        if entity_kinds[kind]:
            require_png(root, f"entities/{name}", checked)
    if entity_kinds[20] or entity_kinds[22]:
        require_png(root, "entities/boulders.png", checked)
    for variant in sorted(doodad_variants):
        name = DOODADS.get(variant)
        if not name:
            raise ValueError(f"No runtime art mapping for ACT_ONE doodad variant {variant}")
        require_png(root, f"entities/doodads/{name}", checked)
    for name in ("torch.png", "flower_1.png", "little_firefly.png"):
        require_png(root, f"entities/{name}", checked,
                    minimum=(24, 24) if name == "torch.png" else (16, 16))

    attack = (root / "act-one-goblin-attack.vga").read_bytes()
    if len(attack) < 16 or attack[:4] != b"VGA1":
        raise ValueError("act-one-goblin-attack.vga: invalid VGA1 packet")
    loot = (root / "item-loot.vlt").read_bytes()
    if len(loot) < 14 or loot[:4] != b"VLT1" or struct.unpack_from("<H", loot, 4)[0] != 2:
        raise ValueError("item-loot.vlt: expected VLT1 v2 packet")
    for name, cell in ITEM_ATLASES.items():
        require_png(root, f"items/{name}", checked, multiple=cell)
    return {"valid": True, "floors": len(floors), "enemy_families": sorted(enemy_families),
            "entity_kinds": dict(sorted(entity_kinds.items())),
            "doodad_variants": sorted(doodad_variants), "images_checked": len(checked)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime_directory", type=Path,
                        help="Staged ux0:data/.../world/act-one directory")
    args = parser.parse_args()
    print(json.dumps(audit(args.runtime_directory), indent=2))


if __name__ == "__main__":
    main()
