"""ACT_ONE treasure stage; secret rooms and pedestal stage remain pending."""
import argparse,json
from pathlib import Path
try:
    from tools.floor_room_graph import room_graph
    from tools.floor_main_path import MT19937
except ModuleNotFoundError:
    from floor_room_graph import room_graph
    from floor_main_path import MT19937


def eligible_neighbor(types,x,y):
    if types[y][x]&15:return None
    if x<7 and types[y][x+1]&15 and not types[y][x+1]&256:return (x+1,y,4,1)
    if x>0 and types[y][x-1]&15 and not types[y][x-1]&256:return (x-1,y,1,4)
    return None


def place_treasure(types,goal,rng):
    candidates=sum(eligible_neighbor(types,x,y) is not None for y in range(5) for x in range(8))
    if not candidates:return None
    rx=ry=1;spanx=spany=2;counter=0;growths=0
    for attempt in range(1,100001):
        x=max(0,min(7,goal[0]+rng.game_int()%spanx-rx))
        y=max(0,min(4,goal[1]+rng.game_int()%spany-ry))
        # Original expands AFTER sampling this candidate, BEFORE validating it.
        if counter==rx*ry*2:
            counter=0
            if rx<8:rx+=1;spanx+=2
            if ry<5:ry+=1;spany+=2
            growths+=1
        else:counter+=1
        neighbor=eligible_neighbor(types,x,y)
        if neighbor is None:continue
        nx,ny,bit,opposite=neighbor
        types[y][x]=512|bit;types[ny][nx]|=opposite
        return dict(x=x,y=y,neighbor=[nx,ny],room_type=512|bit,attempts=attempt,
                    eligible_initial=candidates,radius_after_candidate=[rx,ry],growths=growths)
    raise RuntimeError('Analysis safeguard: treasure stage incomplete')


def treasure_graph(seed,floor_index=0):
    report=room_graph(seed,floor_index);rng=MT19937(seed)
    for _ in range(report['rng_draws']):rng.game_int()
    if rng.fingerprint()!=report['rng_state_sha256']:raise RuntimeError('RNG resume mismatch')
    report['pre_treasure_rng_draws']=report['rng_draws']
    report['treasure']=place_treasure(report['room_types_row_major'],report['goal'],rng)
    report['links_row_major']=[[v&15 for v in row] for row in report['room_types_row_major']]
    report['rng_draws']=rng.draws;report['rng_state_sha256']=rng.fingerprint()
    report['treasure_stage_implemented']=True
    report['secret_stage_implemented']=report['pedestal_stage_implemented']=False
    report['stage']='treasure_graph_only';report['floor_complete']=False
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--seed',type=int,required=True);p.add_argument('--floor',type=int,default=0)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    r=treasure_graph(a.seed,a.floor);a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print(f"TREASURE seed={a.seed} floor={a.floor} placement={r['treasure']} floor_complete=0")

if __name__=='__main__':main()
