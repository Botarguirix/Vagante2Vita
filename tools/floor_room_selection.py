"""Original single-room selection mechanics; not the complete floor assembler."""
try:
    from tools.floor_main_path import choose_least_used
    from tools.room_connectivity import decode_room,fits_vertically
except ModuleNotFoundError:
    from floor_main_path import choose_least_used
    from room_connectivity import decode_room,fits_vertically


def fits_horizontally(left,right):
    if left.height!=right.height:return False
    return any(left.rows[y][-1]!=1 and right.rows[y][0]!=1 for y in range(left.height))


def select_room_grid(types,act,spec,rng):
    height=len(types);width=len(types[0]) if height else 0
    if not width or any(len(row)!=width for row in types):raise ValueError('Rectangular grid required')
    selected=[[None]*width for _ in range(height)];placements=[];frequencies={}
    pools={};geometry={}
    for row in types:
        for kind in row:
            if kind&0x3000:raise ValueError('Double rooms are not implemented by this selector')
            if kind in pools:continue
            pool=act['rooms'].get(str(kind),[])
            if not pool:raise ValueError(f'Original room pool missing: {kind}')
            pools[kind]=[r['uid'] for r in pool]
            for raw in pool:
                key=(kind,raw['uid'])
                geometry[key]=decode_room(raw,kind,spec['width'],spec['height'])
    # The original traverses columns, then rows; retained candidates influence
    # the next cells and rejected choices still increment UID frequencies.
    for x in range(width):
        for y in range(height):
            kind=types[y][x];compatible=False;chosen=None
            for attempt in range(1,22):
                uid=choose_least_used(pools[kind],frequencies,rng)
                chosen=geometry[(kind,uid)]
                horizontal=not(x>0 and kind&1) or fits_horizontally(selected[y][x-1],chosen)
                vertical=not(y>0 and kind&2) or fits_vertically(selected[y-1][x],chosen)
                compatible=horizontal and vertical
                if compatible:break
            selected[y][x]=chosen
            placements.append(dict(x=x,y=y,room_type=kind,uid=chosen.uid,attempts=attempt,
                                   geometry_compatible=compatible,retained_after_retry_limit=not compatible))
    return dict(traversal='column_then_row',placements=placements,frequencies=frequencies,
                all_geometry_compatible=all(p['geometry_compatible'] for p in placements),
                single_rooms_only=True,floor_complete=False)


def choose_boss_room(placements, act, start, goal, secret_rooms=()):
    """Recover generateLevel's max-open-tile boss room selection (0x0067c4c0)."""
    if not isinstance(placements, list) or not placements:
        raise ValueError('At least one selected room is required')
    if any(not isinstance(point, (list, tuple)) or len(point) != 2
           or any(type(value) is not int for value in point)
           for point in (start, goal)):
        raise ValueError('Start and goal must be integer room coordinates')
    secrets = {tuple(point) for point in secret_rooms}
    pools = act.get('rooms', {})
    best = None
    best_open = 0
    candidates = []
    for room in placements:  # select_room_grid preserves native x-major order.
        x, y, kind, uid = (room.get(key) for key in ('x', 'y', 'room_type', 'uid'))
        if any(type(value) is not int for value in (x, y, kind)) or not isinstance(uid, str):
            raise ValueError('Invalid selected room record')
        if kind & (0x100 | 0x200 | 0x400) or (x, y) in secrets:
            continue  # shop, treasure, bright-sky or a secret room.
        if abs(x - start[0]) + abs(y - start[1]) <= 2:
            continue
        if abs(x - goal[0]) + abs(y - goal[1]) <= 2:
            continue
        matches = [raw for raw in pools.get(str(kind), []) if raw.get('uid') == uid]
        if len(matches) != 1:
            raise ValueError('Selected boss-room candidate must resolve uniquely')
        raw_tiles = matches[0].get('tiles')
        if not isinstance(raw_tiles, list):
            raise ValueError('Boss-room candidate has no tile grid')
        open_tiles = sum(1 for row in raw_tiles for index in range(0, len(row), 6)
                         if row[index] != 1)
        candidates.append(dict(x=x, y=y, uid=uid, open_tiles=open_tiles))
        if open_tiles > best_open:
            best, best_open = candidates[-1], open_tiles
    if best is None:
        # Native locals retain the goal room coordinates when no room beats 0.
        best = dict(x=goal[0], y=goal[1], uid=None, open_tiles=0)
    return dict(room=best, candidate_count=len(candidates), candidates=candidates,
                selection='maximum_non_type_1_tiles_first_x_major',
                minimum_manhattan_distance=3, floor_complete=False)
