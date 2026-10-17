"""ACT_ONE endpoint candidates, before native shuffle and endpoint placement."""

try:
    from tools.floor_layout import solid
except ModuleNotFoundError:
    from floor_layout import solid


def reachable_above_ground_candidates(tiles, spec, room):
    """Recover a room-local candidate search, not playable-player reachability.

    The original shuffles the returned coordinates later. This primitive keeps
    the set's lexicographic order and deliberately accepts no random engine.
    Secret sealing, fluid simulation and entity creation must precede final
    endpoint selection; their effects and random consumption remain external.
    """
    if (not isinstance(tiles, list) or not tiles or not isinstance(tiles[0], list)
            or not tiles[0]):
        raise ValueError('A nonempty row-major tile matrix is required')
    height, width = len(tiles), len(tiles[0])
    if any(not isinstance(row, list) or len(row) != width for row in tiles):
        raise ValueError('Tile rows must have equal widths')
    if any(not isinstance(tile, (list, tuple)) or len(tile) != 6
           or type(tile[0]) is not int for row in tiles for tile in row):
        raise ValueError('Every tile needs six fields and an integer type')
    rw, rh = spec['width'], spec['height']
    if any(type(v) is not int or v <= 0 for v in (rw, rh)):
        raise ValueError('Positive single-room dimensions are required')
    if not isinstance(room, (list, tuple)) or len(room) != 2:
        raise ValueError('A room coordinate pair is required')
    rx, ry = room
    if any(type(v) is not int or v < 0 for v in room):
        raise ValueError('Nonnegative integer room coordinates are required')
    left, top = rx * rw, ry * rh
    right, bottom = left + rw - 1, top + rh - 1
    if right >= width or bottom >= height:
        raise ValueError('The complete room must fit in the tile matrix')

    def in_level(x, y):
        return 0 <= x < width and 0 <= y < height

    def platform(x, y):
        return in_level(x, y) and tiles[y][x][0] == 6

    def footing(x, y):
        return in_level(x, y + 1) and (solid(tiles, x, y + 1) or platform(x, y + 1))

    seed = next(((x, y) for y in range(bottom, top - 1, -1)
                 for x in range(left, right + 1)
                 if footing(x, y) and not solid(tiles, x, y)), None)
    pending = {seed} if seed is not None else set()
    visited, candidates = set(), set()

    def enqueue(x, y):
        coordinate = (x, y)
        if (left <= x <= right and top <= y <= bottom
                and coordinate not in visited and coordinate not in pending):
            pending.add(coordinate)

    while pending:
        x, y = min(pending)
        pending.remove((x, y))
        visited.add((x, y))
        supported = footing(x, y)
        if supported and not solid(tiles, x, y):
            candidates.add((x, y))
        # Platforms are not solid: the original exploration can drop through.
        if not solid(tiles, x, y + 1):
            enqueue(x, y + 1)
        ladder_below = in_level(x, y + 1) and tiles[y + 1][x][0] == 3
        if not supported and not ladder_below:
            continue
        layers = 4 if supported else 3
        for layer in range(layers):
            level_y = y - layer
            for neighbor_x in (x - 1, x + 1):
                if (not solid(tiles, neighbor_x, level_y)
                        and (layer != layers - 1 or not platform(neighbor_x, level_y + 1))):
                    enqueue(neighbor_x, level_y)
            if 0 < layer < layers - 1:
                enqueue(x, level_y)
            if solid(tiles, x, level_y - 1):
                break

    return dict(candidates=[list(position) for position in sorted(candidates)],
                seed_tile=list(seed) if seed is not None else None,
                visited_tiles=len(visited), room_bounds=[left, top, right, bottom],
                shuffled=False, water_checked=False,
                upstream_rng_consumption_recovered=False,
                character_movement_reachability_verified=False,
                analysis_only=True)
