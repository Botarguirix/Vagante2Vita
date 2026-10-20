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
ENEMIES = {9: "goblin", 10: "bat", 11: "lurker", 12: "slime", 14: "spider", 84: "thief"}
WORLD_KIND_ENEMY = 24
WORLD_KIND_BOSS = 26
WORLD_BOSS_GOBLIN_KING = 49
WORLD_BOSS_DRAGON = 50
WORLD_BOSS_WORM = 51
ENEMY_STATES = ("stand", "move", "air", "hurt", "attack")
ITEM_ATLASES = {
    "items.png": 16, "items-weapons.png": 20, "items-armor.png": 20,
    "potions.png": 16, "scrolls.png": 20, "spell_books.png": 16,
    "items-explosives.png": 20,
}
BOSS_ATLASES = {
    "goblin_king": ("stand.png", "prep_hop.png", "prep_leap.png", "walk.png",
                    "jump.png", "charge_attack.png", "charge_axe.png", "dead.png",
                    "roar.png", "leap.png", "throw.png", "hurt.png"),
    "dragon": ("baby_dragon_flying.png", "dead.png", "dive.png", "fall.png",
               "flip.png", "hurt.png", "launch.png", "launching_fireballs.png",
               "unconscious.png"),
    "worm": ("body.png", "head.png"),
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


def validate_enemy_record(record: bytes, path: str, width: int, height: int) -> str:
    """Validate one VWE3 record against the Vita loader's kind/subtype rules."""
    if len(record) != 16:
        raise ValueError(f"{path}: invalid enemy record size")
    kind, subtype, reserved, x_q8, y_q8, _ = struct.unpack("<BBHiiI", record)
    if kind == WORLD_KIND_ENEMY and subtype in ENEMIES and reserved == 0:
        family = ENEMIES[subtype]
    elif kind == WORLD_KIND_BOSS and reserved == 0 and subtype in (
            WORLD_BOSS_GOBLIN_KING, WORLD_BOSS_DRAGON, WORLD_BOSS_WORM):
        family = {WORLD_BOSS_GOBLIN_KING: "goblin_king",
                  WORLD_BOSS_DRAGON: "dragon", WORLD_BOSS_WORM: "worm"}[subtype]
    else:
        raise ValueError(f"{path}: unsupported enemy kind/subtype {kind}/{subtype}")
    if not (0 <= x_q8 < width * 16 * 256 and 0 <= y_q8 < height * 16 * 256):
        raise ValueError(f"{path}: enemy coordinate outside {width}x{height} world")
    return family


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
    enemy_families: set[str] = {"spider", "thief"}
    boss_families: set[str] = set()
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
            family = validate_enemy_record(record, enemies_name, width, height)
            if family in ("goblin_king", "dragon", "worm"):
                boss_families.add(family)
            else:
                enemy_families.add(family)
        checked.extend((world_name, entities_name, enemies_name))

    for animation in ("stand", "walk", "jump", "fall", "duck", "climb", "lookup"):
        require_png(root, f"knight/{animation}.png", checked, multiple=24)
    for family in sorted(enemy_families):
        for state in ENEMY_STATES:
            image = f"enemies/{family}/{state}.png"
            width, height = require_png(root, image, checked)
            cell = 18 if family == "thief" else 16
            if width % cell or (height != 17 if family == "goblin" else height % cell):
                expectation = ("width multiple of 16 and height 17" if family == "goblin"
                               else f"dimensions multiple of {cell}")
                raise ValueError(f"{image}: expected {expectation}, got {width}x{height}")
    for kind, name in ((16, "chest.png"), (17, "spikes.png"),
                       (18, "arrow_trap.png"), (20, "trap_trigger.png"),
                       (21, "breakable_wall.png"), (22, "shake_boulder.png"),
                       (23, "boulder_blocker.png")):
        if entity_kinds[kind]:
            require_png(root, f"entities/{name}", checked)
    worm_frame_counts = {}
    if "worm" in boss_families:
        require_png(root, "bosses/worm/body.png", checked, dimensions=(512, 512))
        require_png(root, "bosses/worm/head.png", checked, dimensions=(512, 512))
        require_png(root, "bosses/worm/spitball.png", checked, dimensions=(64, 16))
        metadata_path = root / "bosses" / "worm" / "sprites.json"
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        evidence = metadata.get("evidence", {})
        if (metadata.get("format") != "vagante-vita-boss-sprites"
                or metadata.get("version") != 1 or metadata.get("boss") != "worm"
                or metadata.get("entity_type") != WORLD_BOSS_WORM
                or evidence.get("constructor_address") != "0x005301e0"
                or evidence.get("set_level_address") != "0x005307a0"
                or evidence.get("animation_address") != "0x00537280"
                or evidence.get("segment_count") != 20
                or evidence.get("body_cell") != [32, 32]
                or evidence.get("head_cell") != [32, 32]
                or evidence.get("sprite_speed_argument") != 5
                or evidence.get("frame_advance_ticks") != 6
                or evidence.get("segment_reference_interval_pixels") != 24
                or evidence.get("native_parity") is not False):
            raise ValueError("Worm sprite metadata lacks the verified Ghidra structure")
        for name, cell in (("body.png", (32, 32)), ("head.png", (32, 32)),
                           ("spitball.png", (16, 16))):
            width, height = png_dimensions(root / "bosses" / "worm" / name)
            if width % cell[0] or height % cell[1]:
                raise ValueError(f"bosses/worm/{name}: atlas is not divisible by {cell}")
            worm_frame_counts[name] = (width // cell[0]) * (height // cell[1])
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

    # Older bundles predate boss staging. When the new namespace is present,
    # validate every ACT_ONE boss sheet so a partial copy cannot silently
    # enter a candidate campaign package.
    boss_root = root / "bosses"
    boss_assets_checked = 0
    if boss_families and not boss_root.is_dir():
        raise ValueError("ACT_ONE boss record requires the private bosses/ sprite bundle")
    if boss_root.exists():
        for boss, filenames in BOSS_ATLASES.items():
            for name in filenames:
                require_png(root, f"bosses/{boss}/{name}", checked)
                boss_assets_checked += 1
        measured_frames = {}
        measured_intervals = {}
        if "goblin_king" in boss_families:
            metadata_path = root / "bosses" / "goblin_king" / "sprites.json"
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
            if (metadata.get("format") != "vagante-vita-boss-sprites"
                    or metadata.get("version") != 1 or metadata.get("boss") != "goblin_king"
                    or metadata.get("entity_type") != 49):
                raise ValueError("Invalid Goblin King sprite metadata header")
            if metadata.get("evidence", {}).get("address") != "0x00515c70":
                raise ValueError("Goblin King sprite metadata lacks its Ghidra evidence address")
            evidence = metadata["evidence"]
            if (evidence.get("frame_step_address") != "0x007b9720"
                    or evidence.get("speed_semantics") !=
                    "timer_increments_once_per_step_and_advances_when_timer_exceeds_speed"):
                raise ValueError("Goblin King sprite metadata lacks verified Sprite::step timing semantics")
            sprites = metadata.get("sprites")
            if not isinstance(sprites, dict) or len(sprites) != 13:
                raise ValueError("Goblin King sprite metadata must define 13 sprite roles")
            for role, sprite in sprites.items():
                if (not isinstance(sprite, dict) or sprite.get("path", "").startswith("/")
                        or ".." in Path(sprite.get("path", "")).parts):
                    raise ValueError(f"Invalid Goblin King sprite metadata for {role}")
                cell = sprite.get("cell")
                if (not isinstance(cell, list) or len(cell) != 2
                        or any(type(value) is not int or value <= 0 for value in cell)
                        or type(sprite.get("speed_arg")) is not int
                        or not isinstance(sprite.get("offset"), list)
                        or len(sprite["offset"]) != 2):
                    raise ValueError(f"Invalid Goblin King sprite parameters for {role}")
                width, height = png_dimensions(root / sprite["path"])
                if width % cell[0] or height % cell[1]:
                    raise ValueError(f"{sprite['path']}: atlas is not divisible by native cell size {cell}")
                measured_frames[role] = (width // cell[0]) * (height // cell[1])
                measured_intervals[role] = sprite["speed_arg"] + 1
        if "dragon" in boss_families:
            # BossDragon::setLevel passes 48 as Sprite's horizontal frame size;
            # these exact atlas dimensions were measured from the staged PNGs.
            dragon_frames = {}
            for role, filename in (("flying", "baby_dragon_flying.png"),
                                   ("dive", "dive.png"), ("fall", "fall.png"),
                                   ("launch", "launch.png")):
                width, height = png_dimensions(root / "bosses" / "dragon" / filename)
                if height != 48 or width % 48:
                    raise ValueError(f"bosses/dragon/{filename}: expected 48px cells")
                dragon_frames[role] = width // 48

    attack = (root / "act-one-goblin-attack.vga").read_bytes()
    if len(attack) < 16 or attack[:4] != b"VGA1":
        raise ValueError("act-one-goblin-attack.vga: invalid VGA1 packet")
    loot = (root / "item-loot.vlt").read_bytes()
    if len(loot) < 14 or loot[:4] != b"VLT1" or struct.unpack_from("<H", loot, 4)[0] != 2:
        raise ValueError("item-loot.vlt: expected VLT1 v2 packet")
    for name, cell in ITEM_ATLASES.items():
        require_png(root, f"items/{name}", checked, multiple=cell)
    return {"valid": True, "floors": len(floors), "enemy_families": sorted(enemy_families),
            "boss_families": sorted(boss_families),
            "entity_kinds": dict(sorted(entity_kinds.items())),
            "doodad_variants": sorted(doodad_variants), "images_checked": len(checked),
            "boss_reference_atlases_checked": boss_assets_checked,
            "goblin_king_sprite_roles_checked": len(measured_frames)
            if boss_root.exists() else 0,
            "goblin_king_frame_counts": measured_frames if boss_root.exists() else {},
            "goblin_king_frame_advance_ticks": measured_intervals
            if "goblin_king" in boss_families else {},
            "dragon_frame_counts": dragon_frames if "dragon" in boss_families else {},
            "worm_frame_counts": worm_frame_counts}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime_directory", type=Path,
                        help="Staged ux0:data/.../world/act-one directory")
    args = parser.parse_args()
    print(json.dumps(audit(args.runtime_directory), indent=2))


if __name__ == "__main__":
    main()
