"""Assemble private ACT_ONE room tiles; not a complete generated or playable floor."""
import argparse
from collections import deque
from copy import deepcopy
import json
import math
from pathlib import Path

try:
    from tools.floor_main_path import MT19937
    from tools.floor_room_selection import select_room_grid
    from tools.floor_special_rooms import special_graph
    from tools.room_connectivity import decode_room
    from tools.generation_random import float32
except ModuleNotFoundError:
    from floor_main_path import MT19937
    from floor_room_selection import select_room_grid
    from floor_special_rooms import special_graph
    from room_connectivity import decode_room
    from generation_random import float32

TILE_SIZE = 16


def assemble_tiles(selection, act, spec, grid_width, grid_height):
    """initRoom's six serialized fields, copied without aliasing source rooms."""
    rw, rh = spec['width'], spec['height']
    if any(type(v) is not int or v <= 0 for v in (rw, rh, grid_width, grid_height)):
        raise ValueError('Positive integer dimensions required')
    width, height = rw * grid_width, rh * grid_height
    if width > 128 or height > 128 or width * height > 4096:
        raise ValueError('Layout exceeds current Vita runtime capacity')
    placements = selection['placements']
    if len(placements) != grid_width * grid_height:
        raise ValueError('Every grid cell needs one room placement')
    rooms = {}
    tiles = [[None] * width for _ in range(height)]
    for placement in placements:
        x, y, kind = placement['x'], placement['y'], placement['room_type']
        if (type(x) is not int or type(y) is not int or type(kind) is not int
                or not 0 <= x < grid_width or not 0 <= y < grid_height or kind < 0):
            raise ValueError('Invalid room placement')
        if kind & 0x3000:
            raise ValueError('Double rooms are not implemented by this assembler')
        if (x, y) in rooms:
            raise ValueError('Duplicate room placement')
        candidates = [r for r in act['rooms'].get(str(kind), [])
                      if r['uid'] == placement['uid']]
        if len(candidates) != 1:
            raise ValueError('Selected room UID must resolve uniquely in its pool')
        raw = candidates[0]
        decode_room(raw, kind, rw, rh)
        rooms[x, y] = raw
        for ly, row in enumerate(raw['tiles']):
            for lx in range(rw):
                tiles[y * rh + ly][x * rw + lx] = row[lx * 6:lx * 6 + 6]
    # createRoomEntities traverses rows first, unlike room selection.
    locations = []
    for y in range(grid_height):
        for x in range(grid_width):
            raw = rooms[x, y]
            definitions = raw.get('entities', [])
            if not isinstance(definitions, list):
                raise ValueError('Room entities must be a list')
            for index, definition in enumerate(definitions):
                if not isinstance(definition, dict):
                    raise ValueError('Entity definition must be an object')
                ex, ey = definition.get('x'), definition.get('y')
                if (type(ex) not in (int, float) or type(ey) not in (int, float)
                        or not math.isfinite(ex) or not math.isfinite(ey)
                        or not 0 <= ex < rw * TILE_SIZE or not 0 <= ey < rh * TILE_SIZE
                        or not isinstance(definition.get('type'), str) or not definition['type']):
                    raise ValueError('Invalid original entity location')
                locations.append(dict(room=[x, y], source_uid=raw['uid'],
                                      source_entity_index=index,
                                      world_pixel=[float32(float32(x * rw * TILE_SIZE) + float32(ex)),
                                                   float32(float32(y * rh * TILE_SIZE) + float32(ey))],
                                      definition=deepcopy(definition), instantiated=False))
    return dict(width=width, height=height, tile_size=TILE_SIZE,
                fields=['type', 'backgroundIndex', 'fargroundIndex', 'midgroundIndex',
                        'foregroundIndex', 'closegroundIndex'],
                tiles_row_major=tiles, entity_locations=locations,
                entities_instantiated=False, lighting_implemented=False,
                display_indices_implemented=False, secret_sealing_implemented=False,
                water_simulated=False, entrance_exit_placed=False, floor_complete=False)


def close_act_one_border(tiles):
    height, width = len(tiles), len(tiles[0])
    changed = 0
    for y, row in enumerate(tiles):
        for x, tile in enumerate(row):
            if x in (0, width - 1) or y in (0, height - 1):
                changed += tile[0] != 1
                tile[0] = 1  # Original changes the type, preserving graphic layers.
    return changed


def solid(tiles, x, y):
    if not 0 <= y < len(tiles) or not 0 <= x < len(tiles[0]):
        return True  # Closed ACT_ONE level edges.
    return tiles[y][x][0] in (1, 7)


def tile_paths_to_room_edge(tiles, types, spec, x, y):
    """Recovered tile flood, not a simulation of jumping or character collision."""
    width, height = len(tiles[0]), len(tiles)
    if x < 1 or y < 1 or x >= width - 1 or y >= height - 1 or solid(tiles, x, y):
        return False
    rw, rh = spec['width'], spec['height']
    rx, ry = x // rw, y // rh
    links = types[ry][rx] & 255
    ox, oy = rx * rw, ry * rh
    queue = deque([(x, y)])
    visited = {(x, y)}
    while queue:
        px, py = queue.popleft()
        if ((px == ox - 1 and links & 0x11) or (px == ox + rw and links & 0x44)
                or (py == oy - 1 and links & 0x22) or (py == oy + rh and links & 0x88)):
            return True
        for nx, ny in ((px, py - 1), (px, py + 1), (px - 1, py), (px + 1, py)):
            if (ox - 1 <= nx <= ox + rw and oy - 1 <= ny <= oy + rh
                    and (nx, ny) not in visited and not solid(tiles, nx, ny)):
                visited.add((nx, ny))
                queue.append((nx, ny))
    return False


def above_ground_candidates(tiles, types, spec, room):
    """Unshuffled entrance predicate; later generator stages must precede its RNG."""
    rw, rh = spec['width'], spec['height']
    rx, ry = room
    preferred, near_water = [], []
    for lx in range(rw):
        for ly in range(rh):
            x, y = rx * rw + lx, ry * rh + ly
            if (not solid(tiles, x, y) and tile_paths_to_room_edge(tiles, types, spec, x, y)
                    and solid(tiles, x, y + 1)):
                water = any(0 <= nx < len(tiles[0]) and tiles[y][nx][0] == 9
                            for nx in (x, x - 1, x + 1))
                (near_water if water else preferred).append([x, y])
    return dict(preferred=preferred, near_water=near_water,
                effective_group='preferred' if preferred else 'near_water',
                shuffled=False, analysis_only_before_secret_sealing=True)


def floor_layout(seed, floor_index, database):
    act, spec = database['acts'][0], database['act_specifications'][0]
    report = special_graph(seed, floor_index, act)
    rng = MT19937(seed)
    for _ in range(report['rng_draws']):
        rng.raw()
    if rng.fingerprint() != report['rng_state_sha256']:
        raise RuntimeError('Special-stage RNG resume mismatch')
    report['pre_selection_rng_draws'] = rng.draws
    report['room_selection'] = select_room_grid(report['room_types_row_major'], act, spec, rng)
    report['rng_draws'], report['rng_state_sha256'] = rng.draws, rng.fingerprint()
    layout = assemble_tiles(report['room_selection'], act, spec, *report['room_grid'])
    report['tile_layout'] = layout
    layout['border_tiles_changed'] = close_act_one_border(layout['tiles_row_major'])
    report['entrance_candidates_pre_sealing'] = above_ground_candidates(
        layout['tiles_row_major'], report['room_types_row_major'], spec, report['start'])
    report['stage'] = 'tiles_and_border_only'
    report['floor_complete'] = False
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rooms', type=Path, help='Private original rooms.json')
    parser.add_argument('--seed', required=True, type=int)
    parser.add_argument('--floor', default=0, type=int)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    report = floor_layout(args.seed, args.floor, json.loads(args.rooms.read_text('utf-8')))
    if args.output.exists():
        raise FileExistsError(args.output)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    layout = report['tile_layout']
    print(f"TILE_LAYOUT width={layout['width']} height={layout['height']} "
          f"entity_locations={len(layout['entity_locations'])} floor_complete=0")


if __name__ == '__main__':
    main()
