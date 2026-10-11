"""Validate privately extracted room records without copying game data to the repo."""
import argparse
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
        for room in rooms:
            rows = room["tiles"]
            if len(rows) != height or any(len(row) != width * 6 for row in rows):
                raise ValueError(f"Unexpected tile layout in group {group}")
            if any(type(value) is not int for row in rows for value in row):
                raise ValueError(f"Non-integer tile record in group {group}")
            if not isinstance(room.get("entities"), list):
                raise ValueError(f"Invalid entity list in group {group}")
            entities += len(room["entities"])
        report["groups"][group] = {"rooms": len(rooms), "tiles_per_room": width * height,
                                   "entities_total": entities}
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rooms", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    text = json.dumps(audit(args.rooms), indent=2) + "\n"
    if args.output:
        # Preserve prior analysis instead of silently replacing it.
        with args.output.open("x", encoding="utf-8") as file:
            file.write(text)
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
