"""Assemble private ACT_ONE room tiles; not a complete generated or playable floor."""
import argparse
from collections import deque
from copy import deepcopy
import hashlib
import json
import math
from pathlib import Path

try:
    from tools.floor_main_path import MT19937
    from tools.floor_room_selection import select_room_grid
    from tools.floor_special_rooms import special_graph
    from tools.room_connectivity import decode_room
    from tools.generation_random import float32, shuffle
except ModuleNotFoundError:
    from floor_main_path import MT19937
    from floor_room_selection import select_room_grid
    from floor_special_rooms import special_graph
    from room_connectivity import decode_room
    from generation_random import float32, shuffle

TILE_SIZE = 16
DOODAD_CRATE = 16  # Doodad::DoodadType from the local PDB export.
DOODAD_TORCH = 3
DOODAD_BOOK_PEDESTAL = 14
RNG_FREE_DOODADS = {1, 2, 4, 5, 19, 21}

# Skill::spellTier values recovered from Skill::Skill (Ghidra 0x0078b150)
# for the sixteen skill IDs that generateNewRandomSpellBook can select.
BOOK_SKILL_SPELL_TIERS = (
    -1, 2, 2, -1, -1, -1, 3, 3, 1, 2, 0, 2, 1, 4, 2, 3,
)


def act_one_room_monster_budget(floor_index, new_game_plus=False):
    """Return Room::monstersAllowed's ACT_ONE budget from the local x86 listing."""
    if type(floor_index) is not int or floor_index not in (0, 1, 2):
        raise ValueError('ACT_ONE floor index must be 0, 1, or 2')
    if type(new_game_plus) is not bool:
        raise ValueError('new_game_plus must be a bool')
    # Room::monstersAllowed @ 0x0075af50 reads Room::act at +0x80.
    # The ACT_ONE branch computes (floorIndex >> 1) + 0.5 and adds
    # another 0.5 for newGamePlus. MONSTER_SPAWN_WEIGHT is 1.0 for
    # the ACT_ONE monster types, so ceil(budget) is the per-room cap.
    return float(floor_index >> 1) + 0.5 + (0.5 if new_game_plus else 0.0)


def act_one_enemy_type_for_roll(roll, *, lurker_geometry_supported=True):
    """Map ACT_ONE's recovered 0..99 roll to an entity type for a valid floor candidate."""
    if type(roll) is not int or not 0 <= roll < 100:
        raise ValueError('ACT_ONE enemy roll must be an integer from 0 through 99')
    if type(lurker_geometry_supported) is not bool:
        raise ValueError('lurker_geometry_supported must be a bool')
    if roll < 40:
        return 9   # TYPE_GOBLIN (40%)
    if roll < 85:
        return 12  # TYPE_SLIME (45%)
    return 11 if lurker_geometry_supported else None  # TYPE_LURKER (15%)


def act_one_solid_above_type_for_roll(roll, floor_index):
    """Mirror ACT_ONE's native roll when the tile above the candidate is solid."""
    if type(roll) is not int or not 0 <= roll < 125:
        raise ValueError('ACT_ONE supported-surface roll must be 0 through 124')
    if type(floor_index) is not int or floor_index not in (0, 1, 2):
        raise ValueError('ACT_ONE floor index must be 0, 1, or 2')
    if roll < 50:
        return 10  # TYPE_BAT
    if roll < 70 and floor_index > 0:
        return 14  # TYPE_SPIDER
    return None


def act_one_lurker_geometry_supported(tiles, x, y):
    """Mirror addRandomEnemies' three Level::isSolid checks for a Lurker roll."""
    return not solid(tiles, x, y - 1) and not (
        solid(tiles, x - 1, y) and solid(tiles, x + 1, y))


def act_one_shuffled_room_tiles(room, spec, rng):
    """Build x-major room coordinates and apply the native Windows shuffle."""
    rw, rh = spec['width'], spec['height']
    rx, ry = room['x'], room['y']
    candidates = [(rx * rw + local_x, ry * rh + local_y)
                  for local_x in range(rw) for local_y in range(rh)]
    shuffle(rng, candidates)
    return candidates


def consume_spellbook_rng(rng, floor_index):
    """Consume the recovered Item/Skill draws for one empty book pedestal."""
    draws_before = rng.draws
    rand_index = rng.game_int()  # Item::Item initializes randIndex.
    skill_type = None
    skill_tier = None
    selected_skills = []
    threshold = 1 + floor_index // 3  # 1 - int(floor * -1/3), truncates toward zero.

    for _ in range(5):
        if skill_type is not None and not threshold < skill_tier:
            # The same Skill remains installed, so the remaining guards fail too.
            break
        selector = rng.game_int() & 0x8000000f
        if selector & 0x80000000:
            selector = ((selector - 1) | 0xfffffff0) + 1
        skill_type = selector
        skill_tier = BOOK_SKILL_SPELL_TIERS[skill_type]
        selected_skills.append(skill_type)
        if (rand_index & 0x0f) != skill_type:
            raw = rng.game_int()
            rand_index = ((skill_type & 0xff) - (raw & 0x8000000f)) + raw

    return dict(draws=rng.draws - draws_before,
                skill_types=selected_skills,
                final_rand_index=rand_index,
                final_skill_tier=skill_tier,
                tier_threshold=threshold)


def consume_placed_entity_rng(rng, entity_locations):
    """Consume RNG visible in ACT_ONE Room::createEntity/setLevel paths."""
    start = rng.draws
    draws_by_reason = {}
    per_entity = []
    rng_free_entities = []
    deferred_entities = []
    unclassified_entities = []
    for entity in entity_locations:
        definition = entity.get('definition', {})
        kind = definition.get('type')
        doodad_type = definition.get('doodadType')
        reasons = []
        if kind == 'Chest':
            reasons.append('chest_constructor_facing')
        if kind == 'Spikes':
            reasons.append('spike_variant_roll')
        if kind == 'BreakableWall':
            reasons.append('breakable_wall_rotation')
        if kind == 'Doodad' and doodad_type == DOODAD_TORCH:
            # Doodad::setTypeVars selects the initial torch frame and timer.
            reasons.extend(('torch_initial_frame', 'torch_initial_timer'))
        if (kind == 'Doodad' and doodad_type == DOODAD_CRATE
                and definition.get('crateType', -1) == -1):
            reasons.append('random_crate_type')
        for reason in reasons:
            rng.game_int()
            draws_by_reason[reason] = draws_by_reason.get(reason, 0) + 1
        if reasons:
            per_entity.append(dict(room=entity.get('room'),
                                   source_uid=entity.get('source_uid'),
                                   source_entity_index=entity.get('source_entity_index'),
                                   kind=kind, doodad_type=doodad_type,
                                   draws=len(reasons), reasons=reasons))
        elif kind == 'Doodad' and doodad_type == DOODAD_BOOK_PEDESTAL \
                and not definition.get('heldItem'):
            deferred_entities.append(dict(
                room=entity.get('room'), source_uid=entity.get('source_uid'),
                source_entity_index=entity.get('source_entity_index'),
                kind=kind, doodad_type=doodad_type,
                reason='empty_book_skill_generation_after_room_entities'))
        elif (kind == 'Doodad' and doodad_type in RNG_FREE_DOODADS) \
                or (kind == 'Doodad' and doodad_type == DOODAD_BOOK_PEDESTAL) \
                or (kind == 'Doodad' and doodad_type == DOODAD_CRATE
                    and definition.get('crateType', -1) != -1) \
                or kind in ('BoulderTrap', 'DartTrap'):
            # Ghidra shows no RNG in these ACT_ONE setup paths. Empty books are
            # accounted later, after Room::createRoomEntities finishes.
            rng_free_entities.append(dict(
                room=entity.get('room'), source_uid=entity.get('source_uid'),
                source_entity_index=entity.get('source_entity_index'),
                kind=kind, doodad_type=doodad_type))
        else:
            unclassified_entities.append(dict(
                room=entity.get('room'), source_uid=entity.get('source_uid'),
                source_entity_index=entity.get('source_entity_index'),
                kind=kind, doodad_type=doodad_type))
    return dict(draws=rng.draws - start, draws_by_reason=draws_by_reason,
                per_entity=per_entity,
                rng_free_entity_count=len(rng_free_entities),
                rng_free_entities=rng_free_entities,
                deferred_entity_count=len(deferred_entities),
                deferred_entities=deferred_entities,
                unclassified_entity_count=len(unclassified_entities),
                unclassified_entities=unclassified_entities)


def blocked_endpoint_tiles(entity_locations, tiles):
    """Recover the G/H entity-coordinate map built before endpoint selection."""
    height, width = len(tiles), len(tiles[0])
    blocked = set()
    records = []

    def tile_at_pixel(point):
        # Room::createEntity rounds its world-pixel arguments before creating
        # the entity; addStartAndGoal rounds those stored coordinates again,
        # then shifts by four (16px tiles).
        x, y = (math.floor(float(value) + 0.5) for value in point)
        return x // TILE_SIZE, y // TILE_SIZE

    def record(point, kind, source, source_uid, source_entity_index):
        x, y = point
        if 0 <= x < width and 0 <= y < height:
            blocked.add((x, y))
            records.append(dict(tile=[x, y], native_type=kind, source=source,
                                source_uid=source_uid,
                                source_entity_index=source_entity_index))

    for entity in entity_locations:
        definition = entity.get('definition', {})
        kind = definition.get('type')
        if kind not in ('BoulderTrap', 'Boulder'):
            continue
        origin = tile_at_pixel(entity['world_pixel'])
        native_type = 'G' if kind == 'BoulderTrap' else 'H'
        record(origin, native_type, 'placed_' + kind,
               entity.get('source_uid'), entity.get('source_entity_index'))
        if kind != 'BoulderTrap':
            continue

        # BoulderTrap::setLevel checks the tile under the trap center and,
        # unless it is an exit or solid, creates a linked Boulder in the first
        # solid tile above it. This linked Entity has native type H and is also
        # present when addStartAndGoal builds its blocker set.
        pixel = entity['world_pixel']
        trap_center = tile_at_pixel([float(pixel[0]) + 8.0,
                                     float(pixel[1]) + 8.0])
        x, y = trap_center
        if not (0 <= x < width and 0 <= y < height):
            continue
        if tiles[y][x][0] == 11 or solid(tiles, x, y):
            continue
        while y >= 0 and not solid(tiles, x, y):
            y -= 1
        if y >= 0:
            record((x, y), 'H', 'boulder_trap_link',
                   entity.get('source_uid'), entity.get('source_entity_index'))

    return dict(tiles=blocked, records=records,
                native_type_mapping={'BoulderTrap': 'G', 'Boulder': 'H'},
                coordinate_rounding='roundf_world_pixel_then_shift_4',
                linked_boulder_scan='trap_center_column_to_first_solid_tile')


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


def repair_provisional_route_support(tiles, route):
    """Add explicitly provisional footing/ladders along an orthogonal route.

    This is a playability repair for non-parity exports, not recovered ACT_ONE
    generation. Solid room geometry is never overwritten.
    """
    if not route:
        raise ValueError('A nonempty route is required')
    height, width = len(tiles), len(tiles[0])
    platforms, ladder_tiles = set(), set()
    for index, point in enumerate(route):
        if (not isinstance(point, (list, tuple)) or len(point) != 2
                or any(type(value) is not int for value in point)):
            raise ValueError('Route points must be integer coordinate pairs')
        x, y = point
        if not (0 < x < width - 1 and 0 < y < height - 1):
            raise ValueError('Route point must be inside the closed border')
        if tiles[y][x][0] in (1, 7):
            raise ValueError('Route cannot cross a solid tile')
        if index and route[index - 1][0] == x and route[index - 1][1] == y + 1:
            ladder_tiles.add((x, y))
            ladder_tiles.add((x, y + 1))
        below = tiles[y + 1][x][0]
        if below not in (1, 7, 6):
            platforms.add((x, y + 1))
    for x, y in ladder_tiles:
        if tiles[y][x][0] not in (1, 7, 10, 11):
            tiles[y][x][0] = 3
    for x, y in platforms:
        if tiles[y][x][0] not in (1, 7, 3, 10, 11):
            tiles[y][x][0] = 6
            tiles[y][x][3] = 2
    return dict(applied=True, method='provisional_route_footing_and_ladders',
                platform_count=len(platforms), ladder_tile_count=len(ladder_tiles),
                native_generation_parity=False)


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


def _spawn_geometry(tiles, entrance, *, pixel_offset=(4, 4)):
    """Return collision and foot-support tiles for the native 8x12 player body."""
    ex, ey = entrance
    player_width, player_height, inset = 8, 12, 4
    offset_x, offset_y = pixel_offset
    body_left, body_right = ex * 16 + offset_x, ex * 16 + offset_x + player_width - 1
    body_top, body_bottom = ey * 16 + offset_y, ey * 16 + offset_y + player_height - 1
    body_cells = sorted({(px // 16, py // 16)
                         for px in range(body_left, body_right + 1)
                         for py in range(body_top, body_bottom + 1)}, key=lambda p: (p[1], p[0]))
    body_tiles = [[x, y, tiles[y][x][0]] for x, y in body_cells
                  if 0 <= y < len(tiles) and 0 <= x < len(tiles[0])]
    clear = (len(body_tiles) == len(body_cells)
             and all(kind not in (1, 7) for _, _, kind in body_tiles))
    support_y = (body_bottom + 1) // 16
    support_xs = sorted({body_left // 16, body_right // 16})
    support_tiles = [[x, support_y, tiles[support_y][x][0]] for x in support_xs
                     if support_y < len(tiles) and 0 <= x < len(tiles[0])]
    supported = (len(support_tiles) == len(support_xs)
                 and all(kind in (1, 7, 6) for _, _, kind in support_tiles))
    return dict(body_pixels=[player_width, player_height], horizontal_inset_pixels=inset,
                native_spawn_pixel_offset=[offset_x, offset_y],
                body_bounds_pixels=[body_left, body_top, body_right, body_bottom],
                body_tiles=body_tiles, support_tiles=support_tiles,
                clear=clear, supported=supported, verified=True)


def provisional_enemies(tiles, room_types, placements, spec, rng, entrance, goal,
                        floor_index=0, new_game_plus=False):
    """ACT_ONE test spawns; not original room-budget/RNG parity."""
    try:
        from tools.floor_tile_path import orthogonal_tile_path
    except ModuleNotFoundError:
        from floor_tile_path import orthogonal_tile_path
    rw, rh = spec['width'], spec['height']
    room_budget = act_one_room_monster_budget(floor_index, new_game_plus)
    enemies = []
    # addRandomEnemies starts with a 31-bit uniform draw, builds the ACT_ONE
    # room list in x-major order, then shuffles that list once before budgets.
    rng.game_int()
    room_order = sorted(placements, key=lambda room: (room['x'], room['y']))
    shuffle(rng, room_order)
    for room in room_order:
        room_type = room['room_type']
        if room_type & 0x3000 or room_type == 1:  # special/boss-room exclusion
            continue
        rx, ry = room['x'], room['y']
        if abs(rx - entrance[0] // rw) + abs(ry - entrance[1] // rh) <= 1:
            continue
        candidates = act_one_shuffled_room_tiles(room, spec, rng)
        # Coordinates are consumed in the shuffled vector order. These
        # eligibility gates still approximate the native tile/entity filters.
        room_weight = 0.0
        budget_slot = 0
        candidate_attempt = 0
        for x, y in candidates:
            if room_weight >= room_budget:
                break
            if solid(tiles, x, y):
                continue
            if not tile_paths_to_room_edge(tiles, room_types, spec, x, y):
                continue
            if any(abs(x - px) + abs(y - py) < 5 for px, py in (entrance, goal)):
                continue
            if solid(tiles, x, y - 1):
                roll = rng.game_int() % 125
                subtype = act_one_solid_above_type_for_roll(roll, floor_index)
            else:
                roll = rng.game_int() % 100
                lurker_supported = act_one_lurker_geometry_supported(tiles, x, y)
                subtype = act_one_enemy_type_for_roll(
                    roll, lurker_geometry_supported=lurker_supported)
            candidate_attempt += 1
            if subtype is None:
                continue
            enemies.append(dict(kind='enemy', subtype=subtype,
                                world_pixel=[x * 16 + 4, y * 16 + 4],
                                room=[rx, ry], spawn_model='provisional_act_one',
                                room_monster_budget=room_budget,
                                room_budget_slot=budget_slot,
                                candidate_attempt=candidate_attempt,
                                native_parity=False))
            room_weight += 1.0
            budget_slot += 1
    return enemies


def floor_layout(seed, floor_index, database):
    # These modules depend on solid() above, so import them after this module
    # has finished initializing to keep the shared terrain helpers acyclic.
    try:
        from tools.floor_autotiles import auto_tiles
        from tools.floor_secret_geometry import seal_geometry, remove_mushrooms
        from tools.floor_water import WaterField
        from tools.floor_endpoint_candidates import reachable_above_ground_candidates
        from tools.floor_tile_path import orthogonal_tile_path
    except ModuleNotFoundError:
        from floor_autotiles import auto_tiles
        from floor_secret_geometry import seal_geometry, remove_mushrooms
        from floor_water import WaterField
        from floor_endpoint_candidates import reachable_above_ground_candidates
        from floor_tile_path import orthogonal_tile_path

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
    tiles = layout['tiles_row_major']

    # Native display-index generation precedes secret walls. Vertex emission is
    # still runtime work, but preserving tile indices makes later stages usable.
    auto_tiles(tiles)
    layout['display_indices_implemented'] = True
    layout['display_vertices_implemented'] = False

    # Wall orientation consumes the reconstructed game RNG. The recovered
    # Level constructor initializes a fresh 1024-slot pool, and secret sealing
    # does not allocate particles, so water warmup can account for bubble RNG.
    sealing_rng_start = rng.draws
    layout['secret_geometry'] = seal_geometry(
        tiles, report['room_types_row_major'], spec,
        report['secrets']['placements'], rng=rng, refresh_graphics=True)
    layout['secret_geometry']['rng_draws'] = rng.draws - sealing_rng_start
    layout['rng_draws_after_secret_geometry'] = rng.draws
    layout['rng_state_after_secret_geometry'] = rng.fingerprint()

    water = WaterField(tiles)
    water_rng_start = rng.draws
    water_report = water.simulate(ticks=60, rng=rng, particles_enabled=True,
                                  pool_available=1024)
    water_report['rng_draws_before'] = water_rng_start
    water_report['rng_draws_after'] = rng.draws
    water_report['rng_state_after'] = rng.fingerprint()
    water_bytes = bytes(water.fluid)
    water_report.update(
        fluid_width=water.width, fluid_height=water.height,
        nonzero_cells=sum(value != 0 for value in water.fluid),
        total_mass=sum(water.fluid),
        fluid_sha256=hashlib.sha256(water_bytes).hexdigest(),
        particle_randomness_included=True,
        particle_pool_capacity_recovered=1024,
        particle_pool_initialization_consumes_rng=False,
        tiles_rendered_from_fluid=False)
    water_report['fluid_cells'] = list(water.fluid)
    layout['water'] = water_report
    layout['rng_draws_after_water'] = rng.draws
    layout['rng_state_after_water'] = rng.fingerprint()

    # Room::createEntity consumes RNG in Chest construction, Spikes factory
    # setup, BreakableWall::setLevel, and untyped Doodad crates. These calls
    # happen while Room::createEntities walks rooms and source definitions.
    placed_entity_rng = consume_placed_entity_rng(rng, layout['entity_locations'])

    # Level::createRoomEntities creates books only after every Room has created
    # its placed entities. Consume each Item and its interleaved five-iteration
    # Skill loop before moving to the next pedestal, as the native traversal does.
    book_pedestals = [entity for entity in layout['entity_locations']
                      if (entity.get('definition', {}).get('type') == 'Doodad'
                          and entity['definition'].get('doodadType') == 14
                          and not entity['definition'].get('heldItem'))]
    pedestal_rng_start = rng.draws
    spellbook_rng = [consume_spellbook_rng(rng, floor_index)
                     for _ in book_pedestals]
    layout['room_entity_rng'] = dict(
        unheld_book_pedestals=len(book_pedestals),
        placed_entity_draws=placed_entity_rng['draws'],
        placed_entity_draws_by_reason=placed_entity_rng['draws_by_reason'],
        placed_entity_rng=placed_entity_rng['per_entity'],
        rng_free_placed_entity_count=placed_entity_rng['rng_free_entity_count'],
        rng_free_placed_entities=placed_entity_rng['rng_free_entities'],
        deferred_placed_entity_count=placed_entity_rng['deferred_entity_count'],
        deferred_placed_entities=placed_entity_rng['deferred_entities'],
        unclassified_placed_entity_count=placed_entity_rng['unclassified_entity_count'],
        unclassified_placed_entities=placed_entity_rng['unclassified_entities'],
        known_item_constructor_draws=len(book_pedestals),
        spellbook_rng_draws=rng.draws - pedestal_rng_start,
        spellbook_rng=spellbook_rng,
        known_draws_consumed=True,
        spellbook_skill_draws_included=True,
        full_room_entity_rng_reconstructed=False,
        rng_state_after_spellbooks=rng.fingerprint())

    # addStartAndGoal first builds an ordered map of TYPE_BOULDER_TRAP ('G')
    # and TYPE_BOULDER ('H') entity positions. It then shuffles endpoints,
    # skips blocked positions, and checks goal candidates for water.
    endpoint_blockers = blocked_endpoint_tiles(layout['entity_locations'], tiles)
    start_candidates = above_ground_candidates(
        tiles, report['room_types_row_major'], spec, report['start'])
    entrance_group = (start_candidates['preferred'] if start_candidates['preferred']
                      else start_candidates['near_water'])
    entrance_rng_start = rng.draws
    shuffle(rng, entrance_group)
    unblocked_entrances = [point for point in entrance_group
                           if tuple(point) not in endpoint_blockers['tiles']]
    entrance = unblocked_entrances[0] if unblocked_entrances else None
    start_candidates.update(
        effective_group='preferred' if start_candidates['preferred'] else 'near_water',
        shuffled=bool(entrance_group),
        shuffle_rng_draws=rng.draws - entrance_rng_start,
        blocked_candidate_count=len(entrance_group) - len(unblocked_entrances))

    goal_search = reachable_above_ground_candidates(
        tiles, spec, report['goal'])
    goal_candidates = goal_search['candidates']
    goal_rng_start = rng.draws
    shuffle(rng, goal_candidates)
    goal_search.update(shuffled=bool(goal_candidates),
                       shuffle_rng_draws=rng.draws - goal_rng_start)
    goal_fallback = None
    if len(goal_candidates) < 8:
        fallback_candidates = above_ground_candidates(
            tiles, report['room_types_row_major'], spec, report['goal'])
        fallback_group = (fallback_candidates['preferred']
                          if fallback_candidates['preferred']
                          else fallback_candidates['near_water'])
        fallback_rng_start = rng.draws
        shuffle(rng, fallback_group)
        fallback_candidates.update(
            effective_group=('preferred' if fallback_candidates['preferred']
                             else 'near_water'),
            shuffled=bool(fallback_group),
            shuffle_rng_draws=rng.draws - fallback_rng_start)
        goal_candidates = fallback_group
        goal_fallback = fallback_candidates
        goal_search['fallback_shuffle_rng_draws'] = rng.draws - fallback_rng_start
    def goal_rank(point):
        x, y = point
        open_sides = sum(not solid(tiles, x + dx, y) for dx in (-1, 1))
        support_is_solid = solid(tiles, x, y + 1)
        is_not_platform_type = tiles[y][x][0] != 6
        return (-open_sides, 0 if support_is_solid else 1,
                0 if is_not_platform_type else 1, -y)
    goal_candidates.sort(key=goal_rank)
    blocked_goal_candidate_count = sum(
        tuple(point) in endpoint_blockers['tiles'] for point in goal_candidates)
    goal = next((point for point in goal_candidates
                 if tuple(point) not in endpoint_blockers['tiles']
                 and not water.is_point_in_water(point[0] * 16 + 8,
                                                 point[1] * 16 + 12)), None)

    def endpoint_or_center(point, room):
        if point is not None:
            return list(point), 'candidate'
        center = [room[0] * spec['width'] + spec['width'] // 2,
                  room[1] * spec['height'] + spec['height'] // 2]
        if not solid(tiles, *center):
            return center, 'native_center_fallback'
        # Keep the VWI2 runtime bootable when the native center is obstructed.
        free = next(([x, y] for y in range(room[1] * spec['height'],
                                            (room[1] + 1) * spec['height'])
                     for x in range(room[0] * spec['width'],
                                    (room[0] + 1) * spec['width'])
                     if not solid(tiles, x, y)), None)
        if free is None:
            raise RuntimeError('No non-solid endpoint tile in selected room')
        return free, 'runtime_safety_fallback'

    entrance, entrance_method = endpoint_or_center(entrance, report['start'])
    goal, goal_method = endpoint_or_center(goal, report['goal'])
    for point, kind in ((entrance, 10), (goal, 11)):
        x, y = point
        tiles[y][x][0] = kind
        if not solid(tiles, x, y + 1):
            tiles[y + 1][x][0] = 6
            tiles[y + 1][x][3] = 2
        remove_mushrooms(tiles, x, y)
    auto_tiles(tiles)
    layout['provisional_enemies'] = provisional_enemies(
        tiles, report['room_types_row_major'], report['room_selection']['placements'],
        spec, rng, entrance, goal, floor_index=floor_index)
    layout['act_one_room_monster_budget'] = dict(
        floor_index=floor_index,
        new_game_plus=False,
        budget_per_room=act_one_room_monster_budget(floor_index),
        monster_weight=1.0,
        maximum_spawns_per_room=math.ceil(act_one_room_monster_budget(floor_index)),
        recovered_function='Room::monstersAllowed@0x0075af50',
        spawn_selection_still_provisional=True)
    layout['provisional_enemies_instantiated'] = True
    layout['provisional_enemy_spawn_parity'] = False
    # Match player_spawn's native 8x12 pixel body and 4px inset. The
    # horizontal inset is 4px on each side and feet touch the row below.
    spawn_geometry = _spawn_geometry(tiles, entrance)
    spawn_clear, spawn_supported = spawn_geometry['clear'], spawn_geometry['supported']
    if not spawn_clear or not spawn_supported:
        raise RuntimeError('ACT_ONE entrance fails native player spawn geometry')
    native_path = orthogonal_tile_path(tiles, entrance, goal)
    layout['provisional_route_geometry_repair'] = repair_provisional_route_support(
        tiles, native_path['path'])
    auto_tiles(tiles)
    spawn_geometry = _spawn_geometry(tiles, entrance)
    if not spawn_geometry['clear'] or not spawn_geometry['supported']:
        raise RuntimeError('ACT_ONE entrance fails spawn geometry after route repair')
    layout['native_tile_path'] = native_path
    layout['endpoints'] = dict(
        entrance=entrance, goal=goal,
        entrance_method=entrance_method, goal_method=goal_method,
        candidate_shuffle_reconstructed=True,
        candidate_shuffle_rng_draws=dict(
            entrance=start_candidates['shuffle_rng_draws'],
            goal=goal_search['shuffle_rng_draws'],
            goal_fallback=(goal_fallback['shuffle_rng_draws']
                           if goal_fallback else 0)),
        goal_comparator_fields=['open_horizontal_neighbors_desc',
                                'solid_support_first', 'non_platform_first',
                                'y_desc'],
        blocked_room_entities_checked=True,
        blocked_room_entity_tile_count=len(endpoint_blockers['tiles']),
        blocked_room_entity_tiles=[list(point) for point in
                                   sorted(endpoint_blockers['tiles'])],
        blocked_room_entity_records=endpoint_blockers['records'],
        blocked_goal_candidate_count=blocked_goal_candidate_count,
        blocker_coordinate_model=endpoint_blockers['coordinate_rounding'],
        linked_boulder_model=endpoint_blockers['linked_boulder_scan'],
        water_checked_for_goal=True,
        native_player_spawn_geometry=spawn_geometry,
        native_tile_path_reachable=native_path['reachable'],
        native_tile_path_verified=True,
        mutations_applied=True,
        native_ghidra_order_followed=True,
        exact_generation_parity=False)
    layout['entrance_candidates_after_terrain'] = start_candidates
    layout['goal_candidates_after_terrain'] = goal_search
    layout['goal_fallback_candidates_after_terrain'] = goal_fallback
    layout['display_indices_implemented'] = True
    layout['secret_sealing_geometry_implemented'] = True
    layout['water_geometry_simulated'] = True
    layout['water_rendering_implemented'] = False
    layout['entities_instantiated'] = False
    layout['entrance_exit_placed'] = True
    layout['floor_complete'] = False
    report['generation_rng_complete_through'] = 'known_placed_entity_and_book_rng'
    report['generation_rng_incomplete_reason'] = (
        'other room-entity creation hooks and exact MSVC sort tie ordering '
        'remain unresolved; missing upstream draws keep endpoint shuffle '
        'results from being seed-exact')
    report['rng_draws'], report['rng_state_sha256'] = rng.draws, rng.fingerprint()
    report['stage'] = 'terrain_secret_water_and_provisional_endpoints'
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
