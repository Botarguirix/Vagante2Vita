"""Secret terrain primitives; wall actors and display updates remain separate."""
from collections import deque

try:
    from tools.floor_layout import solid
    from tools.floor_autotiles import auto_tiles, update_display_indices
except ModuleNotFoundError:
    from floor_layout import solid
    from floor_autotiles import auto_tiles, update_display_indices


def nearest_solid(tiles, x, y, max_distance=0xffffffff, excluded=()):
    """conditionalTileBFS: geometric distance, not a walkable-path search."""
    if type(max_distance) is not int or not 0 <= max_distance <= 0xffffffff:
        raise ValueError('Distance must be uint32')
    height, width = len(tiles), len(tiles[0])
    x, y = max(0, min(width - 1, x)), max(0, min(height - 1, y))
    queue, visited = deque([(x, y, 0)]), {(x, y)}
    while queue:
        px, py, distance = queue.popleft()
        if solid(tiles, px, py) and (px, py) not in excluded:
            return [px, py]
        if distance >= max_distance:
            continue
        # Original nested row/column deltas produce UP, LEFT, RIGHT, DOWN.
        for nx, ny in ((px, py - 1), (px - 1, py), (px + 1, py), (px, py + 1)):
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in visited:
                visited.add((nx, ny))
                queue.append((nx, ny, distance + 1))
    return None


def remove_mushrooms(tiles, x, y):
    height, width = len(tiles), len(tiles[0])
    if x <= 0 or y <= 0:
        return []
    if x >= width or y >= height:
        raise ValueError('Mushroom removal point outside layout')
    removed = []
    foreground = tiles[y][x][4]
    if foreground in (100, 101, 102):
        first = x - (foreground - 100)
        fg_targets = [(nx, y) for nx in range(first, first + 3)]
    else:
        fg_targets = [(x, y)] if foreground in (103, 104, 105) else []
    closeground = tiles[y][x][5]
    cg_groups = {0x102: ((0, 0), (-1, 1), (0, 1), (1, 1)),
                 0x121: ((1, -1), (0, 0), (1, 0), (2, 0)),
                 0x122: ((0, -1), (-1, 0), (0, 0), (1, 0)),
                 0x123: ((-1, -1), (-2, 0), (-1, 0), (0, 0))}
    if closeground in cg_groups:
        cg_targets = [(x + dx, y + dy) for dx, dy in cg_groups[closeground]]
    else:
        cg_targets = [(x, y)] if closeground in (0x124, 0x125, 0x126) else []
    for field, targets in ((4, fg_targets), (5, cg_targets)):
        for nx, ny in targets:
            if not 0 <= nx < width or not 0 <= ny < height:
                raise ValueError('Mushroom graphic extends outside layout')
            tiles[ny][nx][field] = -1
            removed.append(dict(x=nx, y=ny, field=field))
    return removed


def seal_geometry(tiles, types, spec, secrets, rng=None, refresh_graphics=False):
    """Terrain in secret-list, column, row order; optional wall-registration RNG."""
    rw, rh = spec['width'], spec['height']
    height, width = len(tiles), len(tiles[0])
    if refresh_graphics:
        auto_tiles(tiles)
    walls, missing_donors = [], []
    for secret in secrets:
        rx, ry = secret['x'], secret['y']
        if not 0 <= ry < len(types) or not 0 <= rx < len(types[0]):
            raise ValueError('Secret coordinate outside room grid')
        links = types[ry][rx] & 15
        ox, oy = rx * rw, ry * rh
        for x in range(ox, ox + rw):
            for y in range(oy, oy + rh):
                strip = ((links & 1 and x <= ox + 1)
                         or (links & 4 and x >= ox + rw - 2)
                         or (links & 2 and y <= oy + 1)
                         or (links & 8 and y >= oy + rh - 2))
                if not strip:
                    continue
                if not 0 <= x < width or not 0 <= y < height:
                    raise ValueError('Secret room extends outside layout')
                if solid(tiles, x, y) or tiles[y][x][0] == 6:
                    continue
                before = tiles[y][x][:]
                donor = nearest_solid(tiles, x, y)
                if donor is not None:
                    dx, dy = donor
                    tiles[y][x][0] = tiles[dy][dx][0]
                    tiles[y][x][3] = tiles[dy][dx][3]
                else:
                    missing_donors.append([x, y])
                if refresh_graphics:
                    update_display_indices(tiles, x, y, x, y)
                rotation = rng.game_int() % 4 * 90 if rng is not None else None
                removed = remove_mushrooms(tiles, x, y)
                if refresh_graphics and removed:
                    update_display_indices(tiles, x - 4, y - 1, x + 4, y + 1)
                walls.append(dict(room=[rx, ry], tile=[x, y], donor_tile=donor,
                                  before=before, after=tiles[y][x][:],
                                  removed_mushrooms=removed, rotation_degrees=rotation,
                                  instantiated=False))
    return dict(walls=walls, missing_donors=missing_donors,
                terrain_geometry_implemented=True, wall_actors_instantiated=False,
                autotile_updates_implemented=refresh_graphics, display_vertices_implemented=False,
                wall_add_rng_consumed=rng is not None,
                midground_indices_pre_display=not refresh_graphics, floor_complete=False)
