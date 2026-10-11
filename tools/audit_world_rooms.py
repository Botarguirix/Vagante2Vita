"""Validate privately extracted room records without copying game data to the repo."""
import argparse
from collections import Counter
import html
import hashlib
import json
from pathlib import Path


def audit(path):
    raw = path.read_bytes()
    data = json.loads(raw)
    spec = data["act_specifications"][9]
    width, height = spec["width"], spec["height"]
    if type(width) is not int or type(height) is not int or not (0 < width <= 1024 and 0 < height <= 1024):
        raise ValueError("Invalid intermission dimensions")
    groups = data["acts"][9]["rooms"]
    report = {"source_sha256": hashlib.sha256(raw).hexdigest(), "act_index": 9,
              "width": width, "height": height, "tile_fields": 6, "groups": {},
              "runtime_world_implemented": False,
              "intro_group_mapping_verified": False}
    for group, rooms in groups.items():
        if not isinstance(rooms, list) or not rooms:
            raise ValueError(f"Empty/invalid room group {group}")
        entities = 0
        tile_types = Counter()
        for room in rooms:
            rows = room["tiles"]
            if len(rows) != height or any(len(row) != width * 6 for row in rows):
                raise ValueError(f"Unexpected tile layout in group {group}")
            if any(type(value) is not int for row in rows for value in row):
                raise ValueError(f"Non-integer tile record in group {group}")
            if not isinstance(room.get("entities"), list):
                raise ValueError(f"Invalid entity list in group {group}")
            entities += len(room["entities"])
            tile_types.update(value for row in rows for value in row[::6])
        report["groups"][group] = {"rooms": len(rooms), "tiles_per_room": width * height,
                                   "entities_total": entities, "tile_types": dict(sorted(tile_types.items())),
                                   "base_solid_tiles": sum(tile_types[t] for t in (1, 7)),
                                   "base_platform_tiles": tile_types[6]}
    return report


def previews(path, output):
    """Private diagnostic geometry, not the original graphics or a game renderer."""
    data = json.loads(path.read_bytes())
    width = data["act_specifications"][9]["width"]
    height = data["act_specifications"][9]["height"]
    output.mkdir(parents=True, exist_ok=False)
    pages = []
    for index, (group, rooms) in enumerate(data["acts"][9]["rooms"].items()):
        for variant, room in enumerate(rooms):
            name = f"group-{index:02d}-room-{variant:02d}.svg"
            cells = []
            for y, row in enumerate(room["tiles"]):
                for x in range(width):
                    kind = row[x * 6]
                    color = {0: "#101820", 1: "#9babb5", 7: "#9babb5", 6: "#e8ac4b"}.get(kind, "#b25cce")
                    cells.append(f'<rect x="{x*12}" y="{y*12}" width="12" height="12" fill="{color}"/>')
            svg = f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width*12} {height*12}">' + ''.join(cells) + '</svg>'
            (output / name).write_text(svg, encoding="utf-8")
            pages.append(f'<h2>Group {html.escape(group)} / {variant}</h2><img src="{name}" width="480">')
    page = '<!doctype html><meta charset="utf-8"><title>Private room geometry audit</title><style>body{background:#16212b;color:white;font-family:sans-serif}img{image-rendering:pixelated}</style><h1>INTERMISSION: diagnostic geometry</h1><p>Grey: base solid (1/7). Orange: platform (6). Purple: other types. INTRO group unresolved. No player physics or original tile graphics.</p>' + ''.join(pages)
    (output / "index.html").write_text(page, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rooms", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--preview-dir", type=Path, help="New private directory for all room geometry previews")
    args = parser.parse_args()
    text = json.dumps(audit(args.rooms), indent=2) + "\n"
    if args.preview_dir:
        previews(args.rooms, args.preview_dir)
    if args.output:
        # Preserve prior analysis instead of silently replacing it.
        with args.output.open("x", encoding="utf-8") as file:
            file.write(text)
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
