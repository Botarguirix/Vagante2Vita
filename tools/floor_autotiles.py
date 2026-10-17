"""Tile-index transforms shared with the native reconstruction; no vertex output."""


def remainder(value, divisor):
    # C/C++ truncates toward zero; Python's negative modulo differs.
    return value - int(value / divisor) * divisor


def tile_bank(value):
    if value < 0 or value % 32 < 16:
        return -1
    return value // 64 * 64 + value % 32 - value % 8


def tile_variant(bank, edges):
    return bank + edges + (24 if edges >= 8 else 0)


def auto_tiles(tiles, region=None):
    """In-place column/row traversal; an optional rectangle is inclusive."""
    height, width = len(tiles), len(tiles[0])
    if region is None:
        region = (0, 0, width - 1, height - 1)
    left, top, right, bottom = region
    left, top = max(0, left), max(0, top)
    right, bottom = min(width - 1, right), min(height - 1, bottom)
    sides = ((-1, 0, 1), (0, -1, 2), (1, 0, 4), (0, 1, 8))
    for x in range(left, right + 1):
        for y in range(top, bottom + 1):
            tile = tiles[y][x]
            for field in (1, 2, 4, 5):
                bank = tile_bank(tile[field])
                if bank < 0:
                    continue
                edges = 0
                for dx, dy, bit in sides:
                    nx, ny = x + dx, y + dy
                    if not 0 <= nx < width or not 0 <= ny < height:
                        continue
                    other_bank = tile_bank(tiles[ny][nx][field])
                    if other_bank == bank:
                        continue
                    if (field == 2 and bank in (720, 784, 848)
                            and other_bank in (720, 784, 848) and bank >= other_bank):
                        continue
                    edges |= bit
                tile[field] = tile_variant(bank, edges)
            value, edges = tile[3], 0
            bank = tile_bank(value)
            if tile[0] == 1 and remainder(value, 32) < 24:
                for dx, dy, bit in sides:
                    nx, ny = x + dx, y + dy
                    if (0 <= nx < width and 0 <= ny < height
                            and (tiles[ny][nx][0] != 1 or remainder(tiles[ny][nx][3], 32) >= 24)):
                        edges |= bit
                tile[3] = tile_variant(bank, edges)
            elif bank >= 0:
                for dx, dy, bit in sides:
                    nx, ny = x + dx, y + dy
                    if (0 <= nx < width and 0 <= ny < height
                            and tile_bank(tiles[ny][nx][3]) != bank):
                        edges |= bit
                tile[3] = tile_variant(bank, edges)
            if tile[0] in (10, 11):
                tile[4] = -1


def update_display_indices(tiles, x1, y1, x2, y2):
    # Native rectangle updates include one neighbor on each side.
    auto_tiles(tiles, (x1 - 1, y1 - 1, x2 + 1, y2 + 1))
