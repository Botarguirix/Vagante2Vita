"""Recovered ACT_ONE orthogonal tile-path audit; it is not player physics."""


def orthogonal_tile_path(tiles, start, goal, visit_limit=5000):
    """Mirror the original g+h search over non-solid tiles.

    Pending ties keep row-major coordinate order (x first, then y), matching
    the original ordered set and full pending scan. Types 1 and 7 are solid;
    platforms remain traversable. Endpoints are clamped before searching.
    """
    if (not isinstance(tiles, list) or not tiles or not isinstance(tiles[0], list)
            or not tiles[0]):
        raise ValueError('A nonempty row-major tile matrix is required')
    height, width = len(tiles), len(tiles[0])
    if any(not isinstance(row, list) or len(row) != width for row in tiles):
        raise ValueError('Tile rows must have equal widths')
    if any(not isinstance(tile, (list, tuple)) or not tile
           or type(tile[0]) is not int for row in tiles for tile in row):
        raise ValueError('Every tile needs an integer type')
    if type(visit_limit) is not int or visit_limit <= 0:
        raise ValueError('visit_limit must be a positive integer')
    for point in (start, goal):
        if (not isinstance(point, (list, tuple)) or len(point) != 2
                or any(type(value) is not int for value in point)):
            raise ValueError('Endpoints must be integer coordinate pairs')

    def clamp(point):
        return (max(0, min(width - 1, point[0])),
                max(0, min(height - 1, point[1])))

    source, target = clamp(start), clamp(goal)
    pending = {source}
    visited = set()
    cost = {source: 0}
    parent = {}
    reached = False
    while pending:
        if len(visited) >= visit_limit:
            break
        current = min(pending, key=lambda point: (
            cost[point] + abs(target[0] - point[0]) + abs(target[1] - point[1]),
            point[0], point[1]))
        pending.remove(current)
        if current in visited:
            continue
        if current == target:
            reached = True
            break
        visited.add(current)
        x, y = current
        for neighbor in ((x - 1, y), (x, y - 1), (x + 1, y), (x, y + 1)):
            nx, ny = neighbor
            if not (0 <= nx < width and 0 <= ny < height):
                continue
            if tiles[ny][nx][0] in (1, 7) or neighbor in visited:
                continue
            new_cost = cost[current] + 1
            if new_cost < cost.get(neighbor, 1 << 60):
                cost[neighbor] = new_cost
                parent[neighbor] = current
                pending.add(neighbor)

    path = []
    if reached:
        point = target
        while point != source:
            path.append(point)
            point = parent[point]
        path.append(source)
        path.reverse()
    return dict(reachable=reached, path=[list(point) for point in path],
                visited_tiles=len(visited), source=list(source), target=list(target),
                visit_limit=visit_limit, limit_reached=bool(pending) and not reached,
                models_player_physics=False)
