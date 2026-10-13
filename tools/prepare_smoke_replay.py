"""Make an explicit one-shot controller sequence; no game resources included."""
import argparse,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('output',type=Path);args=p.parse_args()
CROSS=0x4000;SQUARE=0x8000;CIRCLE=0x2000;TRIANGLE=0x1000
RIGHT=0x20;R=0x200;SELECT=1;START=8
steps=[]
def hold(buttons,ticks):steps.append((buttons,ticks))
def tap(buttons,rest=12):hold(buttons,6);hold(0,rest)
hold(0,90);tap(CROSS,110);tap(CROSS,30)
# Knight: traversal, jump, melee, sheet pages, pause and return.
hold(RIGHT,60);hold(RIGHT|CROSS,6);hold(RIGHT,30);hold(0,30);tap(SQUARE,80)
tap(SELECT);tap(RIGHT);tap(RIGHT);tap(CIRCLE);tap(START,45);tap(START);tap(TRIANGLE,30)
# Rogue: base melee and charged projectile.
tap(RIGHT);tap(CROSS,30);hold(R,90);hold(0,50);tap(SQUARE,50)
tap(SELECT);tap(RIGHT);tap(CIRCLE);tap(TRIANGLE,30)
# Mage: charged Rod sequence, consult sheet and finish normally.
tap(RIGHT);tap(CROSS,30);hold(SQUARE,90);hold(0,80);tap(SELECT);tap(CIRCLE);hold(0,90)
assert len(steps)<=128
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_bytes(struct.pack('<4sHH',b'VTS1',len(steps),0)+b''.join(struct.pack('<IHH',b,t,0) for b,t in steps))
print(f'REPLAY steps={len(steps)} nominal_ticks={sum(t for _,t in steps)} assertions_on_host=1')
