"""Recovered ACT_ONE main-path stage. Does not assemble a playable floor."""
import argparse,json,hashlib,struct
from dataclasses import dataclass
from pathlib import Path

MASK32=(1<<32)-1

class MT19937:
    def __init__(self, seed):
        if type(seed) is not int or not 0<=seed<=MASK32:
            raise ValueError("Seed must be uint32")
        self.state=[seed]
        for i in range(1,624):
            previous=self.state[-1]
            self.state.append((1812433253*(previous^(previous>>30))+i)&MASK32)
        self.index=624
        self.draws=0

    def raw(self):
        if self.index==624:
            for i in range(624):
                joined=(self.state[i]&0x80000000)|(self.state[(i+1)%624]&0x7fffffff)
                self.state[i]=self.state[(i+397)%624]^(joined>>1)^(0x9908b0df if joined&1 else 0)
            self.index=0
        value=self.state[self.index]
        self.index+=1
        self.draws+=1
        value^=value>>11
        value^=(value<<7)&0x9d2c5680
        value^=(value<<15)&0xefc60000
        value^=value>>18
        return value&MASK32

    def game_int(self):
        # 0x0041cce0 uses modulo 2**31, unlike scaled modern distributions.
        return self.raw()&0x7fffffff

    def fingerprint(self):
        return hashlib.sha256(struct.pack('<624I2I',*self.state,self.index,self.draws)).hexdigest()


@dataclass
class Node:
    x:int
    y:int
    tried:int


def initial_tried(x,y,ox,oy,width,height):
    return ((1 if x==ox else 0)|(2 if y==oy else 0)|
            (4 if x==ox+width-1 else 0)|(8 if y==oy+height-1 else 0))


def main_path(seed,floor_index=0):
    if type(floor_index) is not int or floor_index not in (0,1,2):
        raise ValueError("Only ACT_ONE floor indices 0..2 recovered here")
    rng=MT19937(seed)
    width,height=(3,4) if floor_index==0 else (5,3) if floor_index==1 else (3,5)
    ox,oy=(8-width)//2,(5-height)//2
    # Draw order follows generateLevel: start X, start Y, goal X, goal Y;
    # coordinates fixed for a particular floor consume no value.
    sx=1 if floor_index==1 else ox+rng.game_int()%width
    sy=(0 if floor_index==0 else 4) if floor_index!=1 else oy+rng.game_int()%height
    gx=5 if floor_index==1 else ox+rng.game_int()%width
    gy=(3 if floor_index==0 else 0) if floor_index!=1 else oy+rng.game_int()%height
    stack=[Node(sx,sy,initial_tried(sx,sy,ox,oy,width,height))]
    active={(sx,sy)}
    dx,dy=(-1,0,1,0),(0,-1,0,1)
    attempts=backtracks=0
    while stack:
        node=stack[-1]
        if (node.x,node.y)==(gx,gy):break
        rng.game_int()  # The original ACT_ONE branch discards this direction.
        direction=rng.game_int()%4
        attempts+=1
        if attempts>100000:
            raise RuntimeError("Analysis safeguard reached; no floor generated")
        for _ in range(4):
            if not node.tried&(1<<direction):break
            direction=(direction+1)%4
        else:
            active.remove((node.x,node.y));stack.pop();backtracks+=1
            continue
        node.tried|=1<<direction
        x,y=node.x+dx[direction],node.y+dy[direction]
        if (x,y) not in active:
            stack.append(Node(x,y,initial_tried(x,y,ox,oy,width,height)))
            active.add((x,y))
    if not stack:raise RuntimeError("Main path exhausted without goal")
    rng.game_int()  # Consumption immediately before materializing parent chain.
    path=[(n.x,n.y) for n in stack]
    links=[[0]*8 for _ in range(5)]
    for (x,y),(nx,ny) in zip(path,path[1:]):
        direction=next(i for i in range(4) if (x+dx[i],y+dy[i])==(nx,ny))
        links[y][x]|=1<<direction
        links[ny][nx]|=1<<((direction+2)%4)
    return dict(seed=seed,floor_index=floor_index,act='ACT_ONE',room_grid=[8,5],
                active_region=[ox,oy,width,height],start=[sx,sy],goal=[gx,gy],
                path=path,links_row_major=links,attempts=attempts,backtracks=backtracks,
                rng_draws=rng.draws,rng_state_sha256=rng.fingerprint(),
                stage='main_path_only',floor_complete=False,pc_runtime_parity_verified=False)


def choose_least_used(candidates,frequencies,rng):
    """0x0075f250: minimum use tier, preserve vector order, random modulo count.

    randomizeRooms counts each choice before geometry validation, including
    rejected attempts. The caller must not roll the frequency back on rejection.
    """
    if not candidates:return None
    if len(set(candidates))!=len(candidates):raise ValueError("Duplicate UID")
    counts=[frequencies.get(uid,0) for uid in candidates]
    if any(type(count) is not int or count<0 for count in counts):raise ValueError("Invalid frequency")
    tier=min(counts)
    eligible=[uid for uid,count in zip(candidates,counts) if count==tier]
    chosen=eligible[rng.game_int()%len(eligible)]
    frequencies[chosen]=frequencies.get(chosen,0)+1
    return chosen


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--seed',type=int,required=True);p.add_argument('--floor',type=int,default=0)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    report=main_path(a.seed,a.floor);a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f"MAIN_PATH seed={a.seed} floor={a.floor} rooms={len(report['path'])} draws={report['rng_draws']} floor_complete=0 pc_runtime_parity_verified=0")

if __name__=='__main__':main()
