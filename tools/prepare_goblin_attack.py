"""Encode the user's original goblin-stab script for the Vita runtime."""
import json
import math
import struct
from pathlib import Path


def encode(path):
    data = json.loads(Path(path).read_bytes())
    scripts = {item['name']: item for item in data['attack-scripts']}
    script = scripts['goblin-stab']
    if (script['weilder-sprite-name'] != 'monster/goblin/goblin_attack.png'
            or (script['weilder-sprite-width'], script['weilder-sprite-height']) != (32, 17)
            or (script['weilder-anchor-x'], script['weilder-anchor-y']) != (0.5, 1)):
        raise ValueError('Unexpected Goblin weapon sprite geometry')
    frames = script['keyframes']
    if len(frames) != 5 or [frame['frames'] for frame in frames] != [9, 1, 1, 6, 1]:
        raise ValueError('Unexpected goblin-stab keyframe timing')
    packet = bytearray(struct.pack('<4sHHHHHH', b'VGA1', 1, len(frames), 16, 40, 5, 9))
    for frame in frames:
        shapes = frame['shapes']
        enabled_shapes = [shape for shape in shapes if shape['enabled']]
        if len(shapes) != 1 or len(enabled_shapes) != 1:
            raise ValueError('Unexpected Goblin hit shape layout')
        shape = shapes[0]
        vertices = shape['vertices']
        if not 3 <= len(vertices) <= 16:
            raise ValueError('Invalid Goblin hit polygon')
        values = [frame[key] for key in ('origin-x', 'origin-y', 'origin-degrees')]
        values += [shape['damage-min'], shape['damage-max']]
        if not all(math.isfinite(value) for value in values):
            raise ValueError('Non-finite Goblin attack geometry')
        packet.extend(struct.pack('<HBB5f', frame['frames'],
                                  int(frame['enabled'] and shape['enabled']),
                                  len(vertices), *values))
        for vertex in vertices:
            packet.extend(struct.pack('<ff', vertex['x'], vertex['y']))
    return bytes(packet)
