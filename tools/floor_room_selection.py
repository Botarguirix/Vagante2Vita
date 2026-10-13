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
