"""Encode original, user-owned melee attack data for the Vita runtime."""
import json
import math
import struct
from pathlib import Path


# Item::getWeapon in Ghidra maps these IDs to weapon property names and
# player-stab. Other sequences use different sprite-cell geometry and remain
# excluded until the Vita renderer can represent them without cropping.
WEAPONS = (
    (45, 'dagger', 'player-stab'), (46, 'dagger2', 'player-stab'),
    (47, 'dagger3', 'player-stab'),
    (49, 'sword', 'player-stab'), (50, 'sword2', 'player-stab'),
    (51, 'sword3', 'player-stab'), (52, 'sword4', 'player-stab'),
    (53, 'sword5', 'player-stab'),
    (79, 'rod-magic', 'player-stab'),
    (85, 'cqc-caestus', 'player-stab'), (86, 'cqc-claw', 'player-stab'),
    (87, 'cqc-qatar', 'player-stab'),
)


def encode(path):
    data = json.loads(Path(path).read_bytes())
    scripts = {script['name']: script for script in data['attack-scripts']}
    properties = {weapon['name']: weapon for weapon in data['weapon-properties']}
    packet = bytearray(struct.pack('<4sHHII', b'VWA1', 1, len(WEAPONS), 0, 0))
    for item_type, name, script_name in WEAPONS:
        script = scripts[script_name]
        weapon = properties[name]
        if (script['weilder-sprite-width'], script['weilder-sprite-height']) != (24, 24):
            raise ValueError(f'Unexpected player attack geometry: {script_name}')
        if (weapon['sprite-width'], weapon['sprite-height']) != (20, 20):
            raise ValueError(f'Unexpected melee weapon geometry: {name}')
        frames, shapes = script['keyframes'], weapon['shapes']
        if not 1 <= len(frames) <= 16 or len(shapes) > 4:
            raise ValueError(f'Unexpected melee sequence size: {name}')
        packet.extend(struct.pack('<HHHHffHH', item_type, weapon['sprite-frame'],
                                  weapon['per-frame-duration'], weapon['type-flags'],
                                  weapon['sprite-anchor-x'], weapon['sprite-anchor-y'],
                                  len(frames), len(shapes)))
        for frame in frames:
            values = [frame[key] for key in ('origin-x', 'origin-y', 'origin-degrees',
                     'weapon-duration-factor', 'weapon-gfx-scale-x', 'weapon-gfx-scale-y')]
            if not all(math.isfinite(value) for value in values):
                raise ValueError(f'Non-finite melee frame: {name}')
            packet.extend(struct.pack('<HHBBH6f', frame['weilder-sprite-frame'],
                                      frame['frames'], frame['enabled'],
                                      frame['isChargeFrame'], 0, *values))
        for shape in shapes:
            vertices = shape['vertices']
            if not 3 <= len(vertices) <= 16:
                raise ValueError(f'Invalid melee hit shape: {name}')
            packet.extend(struct.pack('<HHfff', len(vertices), shape['enabled'],
                                      shape['damage-min'], shape['damage-max'],
                                      shape['knockback']))
            for vertex in vertices:
                packet.extend(struct.pack('<ff', vertex['x'], vertex['y']))
    return bytes(packet)
