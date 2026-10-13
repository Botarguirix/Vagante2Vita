"""Recovered ACT_ONE side links and shop stage; tiles/treasure remain pending."""
import argparse,json
from pathlib import Path
try:
    from tools.floor_main_path import MT19937,main_path
except ModuleNotFoundError:
    from floor_main_path import MT19937,main_path

# Side-stage direction order differs from the main-path order.
SIDE_DELTAS=((-1,0),(1,0),(0,-1),(0,1))
SIDE_BITS=(1,4,2,8)
OPPOSITE_BITS=(4,1,8,2)


def attempt_side_link(links,region,count,max_rooms,rng):
    x,y=rng.game_int()%8,rng.game_int()%5
    details=dict(source_draws=0,cycle_draws=0,direction=None,new_room=False)
    if not links[y][x]:return count,details
    d=rng.game_int()%4;details['source_draws']=1;details['direction']=d
    ox,oy,width,height=region;dx,dy=SIDE_DELTAS[d];nx,ny=x+dx,y+dy
    if not (ox<=nx<ox+width and oy<=ny<oy+height):return count,details
    if links[ny][nx]:
        details['cycle_draws']=1
        if rng.game_int()%20>6:return count,details
    else:
        if count>=max_rooms:return count,details
        count+=1;details['new_room']=True
    links[y][x]|=SIDE_BITS[d];links[ny][nx]|=OPPOSITE_BITS[d]
    return count,details


def expand_sides(links,region,rng):
    count=sum(bool(mask) for row in links for mask in row)
    max_rooms=region[2]*region[3];limit=max_rooms*4
    attempts=source_draws=cycle_draws=0;direction_attempts=[0]*4
    while not (attempts>=limit and (count>8 or count>=max_rooms)):
        if attempts>=100000:raise RuntimeError('Analysis safeguard: side expansion incomplete')
        attempts+=1
        count,detail=attempt_side_link(links,region,count,max_rooms,rng)
        source_draws+=detail['source_draws'];cycle_draws+=detail['cycle_draws']
        if detail['direction'] is not None:direction_attempts[detail['direction']]+=1
    return dict(rooms=count,max_rooms=max_rooms,attempt_limit=limit,attempts=attempts,
                source_draws=source_draws,cycle_draws=cycle_draws,
                direction_order=['left','right','up','down'],direction_attempts=direction_attempts)


def place_shop(links,rng):
    if all(mask for row in links for mask in row):return None
    for attempt in range(1,100001):
        x,y=rng.game_int()%8,rng.game_int()%5
        if links[y][x]:continue
        # The shop stage precedes treasures: no treasure exclusions apply yet.
        if x<7 and links[y][x+1]:
            links[y][x]=4;links[y][x+1]|=1
            return dict(x=x,y=y,room_type=260,neighbor=[x+1,y],attempts=attempt)
        if x>0 and links[y][x-1]:
            links[y][x]=1;links[y][x-1]|=4
            return dict(x=x,y=y,room_type=257,neighbor=[x-1,y],attempts=attempt)
    raise RuntimeError('Analysis safeguard: shop placement incomplete')


def room_graph(seed,floor_index=0):
    report=main_path(seed,floor_index)
    rng=MT19937(seed)
    for _ in range(report['rng_draws']):rng.game_int()
    if rng.fingerprint()!=report['rng_state_sha256']:raise RuntimeError('Main-stage RNG state mismatch')
    report['main_rng_draws']=report['rng_draws']
    report['main_links_row_major']=[row[:] for row in report['links_row_major']]
    links=report['links_row_major']
    report['side_expansion']=expand_sides(links,report['active_region'],rng)
    report['shop']=place_shop(links,rng)
    types=[row[:] for row in links]
    if report['shop']:
        shop=report['shop'];types[shop['y']][shop['x']]|=256
    report['room_types_row_major']=types
    report['rng_draws']=rng.draws;report['rng_state_sha256']=rng.fingerprint()
    report['stage']='side_links_and_shop_only'
    report['treasure_stage_implemented']=False
    report['floor_complete']=False
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--seed',type=int,required=True);p.add_argument('--floor',type=int,default=0)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    report=room_graph(a.seed,a.floor);a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f"ROOM_GRAPH seed={a.seed} floor={a.floor} side_rooms={report['side_expansion']['rooms']} shop={report['shop']} draws={report['rng_draws']} floor_complete=0")

if __name__=='__main__':main()
