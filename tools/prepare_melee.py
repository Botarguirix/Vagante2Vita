"""Encode the user's original base stab scripts, never publish extracted data."""
import json,math,struct
from pathlib import Path

def encode(path):
    data=json.loads(Path(path).read_bytes())
    script=next(s for s in data['attack-scripts'] if s['name']=='player-stab')
    if (script['weilder-sprite-width'],script['weilder-sprite-height'])!=(24,24):raise ValueError('Unexpected wielder geometry')
    frames=script['keyframes']
    if not 1<=len(frames)<=16:raise ValueError('Too many frames')
    packet=bytearray(struct.pack('<4sHHII',b'VWA1',1,3,0,0))
    for item,name in [(49,'sword'),(45,'dagger'),(79,'rod-magic')]:
        weapon=next(w for w in data['weapon-properties'] if w['name']==name)
        if (weapon['sprite-width'],weapon['sprite-height'])!=(20,20):raise ValueError('Unexpected weapon geometry')
        shapes=weapon['shapes']
        if len(shapes)>4:raise ValueError('Too many shapes')
        packet.extend(struct.pack('<HHHHffHH',item,weapon['sprite-frame'],weapon['per-frame-duration'],weapon['type-flags'],weapon['sprite-anchor-x'],weapon['sprite-anchor-y'],len(frames),len(shapes)))
        for f in frames:
            values=[f[k] for k in ['origin-x','origin-y','origin-degrees','weapon-duration-factor','weapon-gfx-scale-x','weapon-gfx-scale-y']]
            if not all(math.isfinite(v) for v in values):raise ValueError('Non-finite frame')
            packet.extend(struct.pack('<HHBBH6f',f['weilder-sprite-frame'],f['frames'],f['enabled'],f['isChargeFrame'],0,*values))
        for shape in shapes:
            vertices=shape['vertices']
            if not 3<=len(vertices)<=16:raise ValueError('Invalid polygon size')
            packet.extend(struct.pack('<HHfff',len(vertices),shape['enabled'],shape['damage-min'],shape['damage-max'],shape['knockback']))
            for vertex in vertices:packet.extend(struct.pack('<ff',vertex['x'],vertex['y']))
    return bytes(packet)
