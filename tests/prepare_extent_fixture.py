"""Host fixture requiring Pillow and the user's private sprite bundle."""
from pathlib import Path
from PIL import Image
import shutil,struct
import argparse
p=argparse.ArgumentParser(description='Generate synthetic terrain for native camera tests; never a game floor')
p.add_argument('source_bundle',type=Path);p.add_argument('output',type=Path);args=p.parse_args()
src=args.source_bundle;out=args.output
shutil.copytree(src,out)
w,h=80,40
packet=bytearray(struct.pack('<4sHHII4HII',b'VWI2',w,h,0,w*h,70,38,75,38,0,0))
for y in range(h):
 for x in range(w):packet.extend(struct.pack('<BB5h',1 if y==39 or x in (0,79) else 0,0,-1,-1,-1,-1,-1))
(out/'intro-world.vwi').write_bytes(packet)
im=Image.new('RGBA',(1280,640));px=im.load()
for y in range(640):
 for x in range(1280):px[x,y]=(x//16*3,y//16*5,64,255)
im.save(out/'intro-terrain.png')
