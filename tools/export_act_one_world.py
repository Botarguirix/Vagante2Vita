"""Export a private generated ACT_ONE layout to the runtime's VWI2 format."""
import argparse
from collections import Counter
import hashlib
import json
import math
import struct
from pathlib import Path

from PIL import Image


LAYERS = ('background', 'farground', 'midground', 'foreground', 'closeground')
LIMITS = (1024, 1024, 704, 512, 512)
ENTITY_CODES = {'Chest': 16, 'Spikes': 17, 'DartTrap': 18, 'Doodad': 19,
                'BoulderTrap': 20, 'BreakableWall': 21, 'Boulder': 22,
                'BoulderBlocker': 23}
ENTITY_REQUIRED_FIELDS = {
    'Chest': ('x', 'y', 'type', 'chestType'),
    'Spikes': ('x', 'y', 'type'),
    'DartTrap': ('x', 'y', 'type', 'canReload'),
    'Doodad': ('x', 'y', 'type', 'doodadType'),
    'BoulderTrap': ('x', 'y', 'type'),
    'BreakableWall': ('x', 'y', 'type'),
    'Boulder': ('x', 'y', 'type'),
    'BoulderBlocker': ('x', 'y', 'type', 'facingLeft'),
}


def audit_act_one_entity_definitions(database):
    """Reject unsupported or incomplete entity definitions in ACT_ONE rooms."""
    try:
        act = database['acts'][0]
        rooms = act['rooms']
    except (KeyError, IndexError, TypeError) as error:
        raise ValueError('Room database is missing ACT_ONE rooms') from error
    counts = Counter()
    for room_type, room_pool in rooms.items():
        if not isinstance(room_pool, list):
            raise ValueError(f'ACT_ONE room pool {room_type} must be a list')
        for room in room_pool:
            for entity in room.get('entities', []):
                if not isinstance(entity, dict):
                    raise ValueError('ACT_ONE room entity must be an object')
                name = entity.get('type')
                required = ENTITY_REQUIRED_FIELDS.get(name)
                if required is None:
                    raise ValueError(f'Unsupported ACT_ONE entity definition: {name}')
                missing = [field for field in required if field not in entity]
                if missing:
                    raise ValueError(f'{name} definition missing fields: {", ".join(missing)}')
                counts[name] += 1
    return dict(sorted(counts.items()))


def encode_entities(instances, width, height, tiles=None):
    if len(instances) > 256:
        raise ValueError('ACT_ONE exceeds runtime entity capacity')
    records = []
    counts = Counter()
    for index, instance in enumerate(instances):
        definition = instance.get('definition', {})
        name = definition.get('type')
        if name not in ENTITY_CODES:
            raise ValueError(f'Unsupported ACT_ONE entity type: {name}')
        variant, flags = 0, 0
        if name == 'Chest':
            variant = definition.get('chestType', 0)
            if type(variant) is not int or variant not in (0, 1):
                raise ValueError('Unsupported chest rarity')
        elif name == 'Doodad':
            variant = definition.get('doodadType')
            if type(variant) is not int or not 1 <= variant <= 21:
                raise ValueError('Unsupported ACT_ONE doodad type')
        elif name == 'DartTrap':
            if type(definition.get('canReload', False)) is not bool:
                raise ValueError('DartTrap canReload must be boolean')
            flags |= int(definition.get('canReload', False)) << 2
            if tiles is None:
                if type(definition.get('facingLeft', False)) is not bool:
                    raise ValueError('DartTrap facingLeft must be boolean')
        elif name == 'BoulderBlocker':
            if type(definition.get('facingLeft', False)) is not bool:
                raise ValueError('BoulderBlocker facingLeft must be boolean')
            flags |= int(definition.get('facingLeft', False)) << 1
        point = instance.get('world_pixel')
        if not isinstance(point, (list, tuple)) or len(point) != 2:
            raise ValueError('ACT_ONE entity needs a world pixel coordinate')
        x, y = point
        if (type(x) not in (int, float) or type(y) not in (int, float)
                or not math.isfinite(x) or not math.isfinite(y)
                or not 0 <= x < width * 16 or not 0 <= y < height * 16):
            raise ValueError('ACT_ONE entity coordinate is outside world bounds')
        if name == 'DartTrap':
            if tiles is None:
                facing_left = definition.get('facingLeft', False)
            else:
                # DartTrap::setLevel derives the firing direction from the
                # solidity of the tile immediately to its right. Room JSON
                # has no authoritative facingLeft property.
                # DartTrap's constructor uses the center of its 16x16 body:
                # tile = floor((position + 8) / 16).
                tile_x, tile_y = math.floor((x + 8) / 16), math.floor((y + 8) / 16)
                right_x = tile_x + 1
                facing_left = (True if right_x >= width
                               else tiles[tile_y][right_x][0] in (1, 7))
            flags |= int(facing_left) << 1
        x_q8, y_q8 = round(x * 256), round(y * 256)
        records.append(struct.pack('<BBHiiI', ENTITY_CODES[name], flags, variant,
                                   x_q8, y_q8, index))
        counts[name] += 1
    return struct.pack('<4sHH', b'VWE2', len(records), 16) + b''.join(records), dict(counts)


def encode_enemies(enemies, width, height):
    """Encode provisional ACT_ONE enemies in a separate private VWE3 packet."""
    if len(enemies) > 64:
        raise ValueError('ACT_ONE exceeds provisional enemy capacity')
    records = []
    counts = Counter()
    for index, enemy in enumerate(enemies):
        subtype = enemy.get('subtype')
        point = enemy.get('world_pixel')
        if (subtype not in (9, 10, 11, 12, 14) or enemy.get('native_parity') is not False
                or not isinstance(point, (list, tuple)) or len(point) != 2):
            raise ValueError('Invalid provisional ACT_ONE enemy')
        x, y = point
        if (type(x) not in (int, float) or type(y) not in (int, float)
                or not math.isfinite(x) or not math.isfinite(y)
                or not 0 <= x < width * 16 or not 0 <= y < height * 16):
            raise ValueError('Enemy coordinate is outside world bounds')
        records.append(struct.pack('<BBHiiI', 24, subtype, 0, round(x * 256),
                                   round(y * 256), index))
        counts[subtype] += 1
    return struct.pack('<4sHH', b'VWE3', len(records), 16) + b''.join(records), dict(counts)


def validate_spawn_geometry(tiles, entrance, reported):
    """Recompute player_spawn collision cells from the exact exported tiles."""
    if not isinstance(entrance, (list, tuple)) or len(entrance) != 2:
        raise ValueError('ACT_ONE entrance is not a coordinate pair')
    ex, ey = entrance
    if any(type(value) is not int for value in (ex, ey)):
        raise ValueError('ACT_ONE entrance coordinates must be integers')
    height, width = len(tiles), len(tiles[0])
    if not (0 <= ex < width and 0 <= ey < height):
        raise ValueError('ACT_ONE entrance is outside the tile matrix')

    # Mirrors player_spawn: x=tile_x*16+4, y=tile_y*16+16-12.
    left, top, player_width, player_height = ex * 16 + 4, ey * 16 + 4, 8, 12
    right, bottom = left + player_width - 1, top + player_height - 1
    body_coords = sorted({(px // 16, py // 16)
                          for px in range(left, right + 1)
                          for py in range(top, bottom + 1)}, key=lambda p: (p[1], p[0]))
    body = [[x, y, tiles[y][x][0]] for x, y in body_coords
            if 0 <= x < width and 0 <= y < height]
    clear = len(body) == len(body_coords) and all(kind not in (1, 7) for _, _, kind in body)
    support_y = (bottom + 1) // 16
    support_xs = sorted({left // 16, right // 16})
    support = [[x, support_y, tiles[support_y][x][0]] for x in support_xs
               if 0 <= x < width and 0 <= support_y < height]
    supported = (len(support) == len(support_xs)
                 and all(kind in (1, 7, 6) for _, _, kind in support))
    expected = dict(body_pixels=[8, 12], horizontal_inset_pixels=4,
                    native_spawn_pixel_offset=[4, 4],
                    body_bounds_pixels=[left, top, right, bottom],
                    body_tiles=body, support_tiles=support,
                    clear=clear, supported=supported, verified=True)
    if not isinstance(reported, dict) or any(reported.get(key) != value
                                              for key, value in expected.items()):
        raise ValueError('ACT_ONE spawn geometry report does not match exported tiles')
    if not clear or not supported:
        raise ValueError('ACT_ONE entrance does not pass native player spawn geometry')
    return expected


def export(report, output, assets):
    layout = report['tile_layout']
    required = (
        report.get('exact_generation_parity') is True,
        layout.get('entities_instantiated') is True,
        layout.get('native_player_route_verified') is True,
        layout.get('native_exit_transition_verified') is True,
    )
    if layout.get('floor_complete') is True:
        if not all(required):
            raise ValueError('A complete ACT_ONE floor requires generation parity, instantiated entities, a verified player route, and a verified exit transition')
    width, height = layout['width'], layout['height']
    tiles = layout['tiles_row_major']
    if len(tiles) != height or any(len(row) != width for row in tiles):
        raise ValueError('Layout dimensions do not match its tile matrix')
    if width * height > 4096 or not 0 < width <= 128 or not 0 < height <= 128:
        raise ValueError('World exceeds the Vita runtime limits')
    endpoints = layout.get('endpoints')
    if not endpoints or not endpoints.get('mutations_applied'):
        raise ValueError('ACT_ONE endpoints have not been applied')
    entrance, goal = endpoints['entrance'], endpoints['goal']
    spawn = validate_spawn_geometry(tiles, entrance,
                                    endpoints.get('native_player_spawn_geometry', {}))
    path = layout.get('native_tile_path', {})
    if (not endpoints.get('native_tile_path_verified') or not path.get('reachable')
            or not path.get('path')):
        raise ValueError('ACT_ONE entrance and goal fail native tile-path validation')
    packet = bytearray(struct.pack('<4sHHII4HII', b'VWI2', width, height,
                                   0, width * height, *entrance, *goal, 0, 0))
    for row in tiles:
        for tile in row:
            if len(tile) != 6 or type(tile[0]) is not int or not 0 <= tile[0] <= 12:
                raise ValueError('Unsupported tile record')
            values = tile[1:]
            if any(type(value) is not int or not -32768 <= value <= 32767
                   for value in values):
                raise ValueError('Tile layers must be signed 16-bit integers')
            if any(value >= limit for value, limit in zip(values, LIMITS)):
                raise ValueError('Tile layer index exceeds runtime limits')
            packet.extend(struct.pack('<bB5h', tile[0], 0, *values))

    entity_packet, entity_counts = encode_entities(
        layout.get('entity_locations', []), width, height, tiles)
    enemy_packet, enemy_counts = encode_enemies(
        layout.get('provisional_enemies', []), width, height)

    image = Image.new('RGBA', (width * 16, height * 16), (32, 24, 16, 255))
    for layer in LAYERS:
        atlas = Image.open(assets / 'gfx' / f'tiles-{layer}.png').convert('RGBA')
        columns, rows = atlas.width // 16, atlas.height // 16
        limit = columns * rows
        for y, row in enumerate(tiles):
            for x, tile in enumerate(row):
                index = tile[1 + LAYERS.index(layer)]
                if index < 0:
                    continue
                if index >= limit:
                    raise ValueError(f'{layer} tile {index} exceeds its atlas')
                sx, sy = index % columns * 16, index // columns * 16
                image.alpha_composite(atlas.crop((sx, sy, sx + 16, sy + 16)),
                                      (x * 16, y * 16))

    water = layout.get('water', {})
    fluid = water.get('fluid_cells', [])
    fluid_width, fluid_height = water.get('fluid_width', 0), water.get('fluid_height', 0)
    if fluid and len(fluid) == fluid_width * fluid_height and fluid_width == width * 2 and fluid_height == height * 2:
        overlay = Image.new('RGBA', image.size, (0, 0, 0, 0))
        pixels = overlay.load()
        for fy in range(fluid_height):
            for fx in range(fluid_width):
                mass = fluid[fy * fluid_width + fx]
                if mass:
                    color = (53, 151, 224, min(176, 54 + mass))
                    for py in range(fy * 8, fy * 8 + 8):
                        for px in range(fx * 8, fx * 8 + 8):
                            pixels[px, py] = color
        image.alpha_composite(overlay)

    output.mkdir(parents=True, exist_ok=False)
    (output / 'act-one-world.vwi').write_bytes(packet)
    (output / 'act-one-entities.vwe').write_bytes(entity_packet)
    (output / 'act-one-enemies.vwe').write_bytes(enemy_packet)
    layout_path = output / 'act-one-layout.json'
    layout_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    image.save(output / 'intro-terrain.png')
    summary = dict(
        format='VWI2', width=width, height=height, entrance=entrance, goal=goal,
        packet_bytes=len(packet), packet_sha256=hashlib.sha256(packet).hexdigest(),
        entity_packet_bytes=len(entity_packet),
        entity_packet_sha256=hashlib.sha256(entity_packet).hexdigest(),
        enemy_packet_bytes=len(enemy_packet),
        enemy_packet_sha256=hashlib.sha256(enemy_packet).hexdigest(),
        provisional_enemy_count=sum(enemy_counts.values()),
        provisional_enemy_counts=enemy_counts,
        layout_file='act-one-layout.json',
        entity_count=sum(entity_counts.values()), entity_counts=entity_counts,
        terrain_sha256=hashlib.sha256((output / 'intro-terrain.png').read_bytes()).hexdigest(),
        stage=report.get('stage'), exact_generation_parity=report.get('exact_generation_parity') is True,
        generation_rng_complete_through=report.get('generation_rng_complete_through'),
        floor_complete=layout.get('floor_complete') is True and all(required),
        entities_instantiated=required[1],
        native_player_route_verified=required[2],
        native_exit_transition_verified=required[3],
        original_assets_kept_private=True,
        native_player_spawn_geometry=spawn,
        native_tile_path=dict(reachable=path['reachable'],
                              visited_tiles=path['visited_tiles'],
                              path_steps=len(path['path']) - 1,
                              models_player_physics=path['models_player_physics'],
                              route_tiles=path['path']),
        caveat=('The 60-tick water warmup reconstructs particle RNG using the recovered fresh 1024-slot pool. Empty book pedestals consume the recovered Item constructor and state-conditional skill-loop draws. Windows std::shuffle, G/H endpoint blockers (including BoulderTrap-linked Boulders), and the goal comparator are modeled, including the fewer-than-eight fallback. Other room-entity RNG and Python sort tie ordering remain unresolved, so endpoint results are not seed-exact. Chest, spikes, dart-trap, doodad, and boulder-trap behavior is approximated. BreakableWall has melee damage, connected-group destruction, and tile collision updates, but native damage filters, rotation, indestructible-tile metadata, visuals after tile destruction, and effects are incomplete. Standalone Boulder has an initial shake, fall, collision, and impact model but lacks subtype confirmation, exact shake, audio/particles, and full collision parity. Provisional ACT_ONE enemies are separate and non-parity.'))
    (output / 'act-one-export.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('layout', type=Path)
    parser.add_argument('assets', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    print(json.dumps(export(json.loads(args.layout.read_text('utf-8')), args.output, args.assets), indent=2))


if __name__ == '__main__':
    main()
