"""Audit original room geometry before floor assembly; never generates a floor."""
import argparse
import hashlib
import json
from dataclasses import dataclass
from pathlib import Path

@dataclass(frozen=True)
class RoomGeometry:
    uid: str
    room_type: int
    rows: tuple

    @property
    def width(self): return len(self.rows[0])
    @property
    def height(self): return len(self.rows)


def decode_room(raw, room_type, single_width, single_height):
    rows = raw.get("tiles")
    uid = raw.get("uid")
    if not isinstance(uid, str) or not uid or not isinstance(rows, list) or not rows:
        raise ValueError("Room requires UID and nonempty tiles")
    width = single_width * (2 if room_type & 0x2000 else 1)
    height = single_height * (2 if room_type & 0x1000 else 1)
    if len(rows) != height or any(not isinstance(r, list) or len(r) != width * 6 for r in rows):
        raise ValueError(f"Room {uid}: dimensions disagree with type flags")
    for row in rows:
        for i, value in enumerate(row):
            if type(value) is not int or not (0 <= value <= 12 if i % 6 == 0 else -32768 <= value <= 32767):
                raise ValueError(f"Room {uid}: invalid tile field")
    return RoomGeometry(uid, room_type, tuple(tuple(r[::6]) for r in rows))


def fits_vertically(upper, lower):
    """Recovered edge predicate at 0x0075eb90; type 1 is its solid test.

    This is directional and checks clearance/support, not just open edges.
    It does not prove that a player can traverse the whole resulting floor.
    """
    if upper.width != lower.width or upper.height < 3 or lower.height < 2:
        return False
    width, height = upper.width, upper.height
    a, b = upper.rows, lower.rows
    for x in range(width):
        last, first = a[height-1][x], b[0][x]
        if last == 1 or first == 1:
            continue
        can_descend = last in (3, 6)
        for nx in (x-1, x+1):
            if not 0 <= nx < width:
                continue
            step = (a[height-2][nx] == 1 and a[height-3][nx] != 1
                    and a[height-2][x] != 1 and a[height-3][x] != 1)
            side = ((a[height-1][nx] == 1 or b[0][nx] == 1)
                    and a[height-2][x] != 1 and a[height-2][nx] != 1)
            can_descend |= step or side
        if not can_descend:
            continue
        support = b[1][x] in (1, 6) or first in (3, 6)
        for nx in (x-1, x+1):
            if 0 <= nx < width and b[0][nx] != 1 and b[1][nx] in (1, 6):
                support = True
        if support:
            return True
    return False


def audit(database):
    specs, acts = database["act_specifications"], database["acts"]
    if len(specs) != len(acts):
        raise ValueError("Act specifications and databases disagree")
    result = []
    for index, (spec, act) in enumerate(zip(specs, acts)):
        w, h = spec["width"], spec["height"]
        if type(w) is not int or type(h) is not int or not 1 <= w <= 256 or not 1 <= h <= 256:
            raise ValueError("Invalid single-room dimensions")
        rooms = []
        seen = set()
        for key, group in act["rooms"].items():
            kind = int(key)
            if not 0 <= kind <= 0xffffffff:
                raise ValueError("Invalid room type")
            for raw in group:
                room = decode_room(raw, kind, w, h)
                if room.uid in seen:
                    raise ValueError("Duplicate room UID within act")
                seen.add(room.uid)
                rooms.append(room)
        # Single rooms are the first dependency; double-height links use eight bits.
        singles = [r for r in rooms if not r.room_type & 0x3000]
        upper = [r for r in singles if r.room_type & 8]
        lower = [r for r in singles if r.room_type & 2]
        pairs = [(a.uid, b.uid) for a in upper for b in lower if fits_vertically(a, b)]
        result.append(dict(act=index, name=act.get("_comment", str(index)),
                           single_tile_width=w, single_tile_height=h, rooms=len(rooms),
                           single_rooms=len(singles), upper_candidates=len(upper),
                           lower_candidates=len(lower), vertical_pairs_checked=len(upper)*len(lower),
                           vertical_pairs_fit=len(pairs), compatible_pairs=pairs))
    return dict(floor_generation_implemented=False, vertical_predicate="0x0075eb90",
                coverage="single-room vertical adjacency; no RNG or whole-floor reachability", acts=result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rooms", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    raw = args.rooms.read_bytes()
    report = audit(json.loads(raw))
    report["source_sha256"] = hashlib.sha256(raw).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for act in report["acts"]:
        print(f"ROOM_CONNECTIVITY act={act['act']} rooms={act['rooms']} checked={act['vertical_pairs_checked']} fit={act['vertical_pairs_fit']}")

if __name__ == "__main__": main()
