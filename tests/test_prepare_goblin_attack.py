import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools.prepare_goblin_attack import encode


class GoblinAttackPackageTests(unittest.TestCase):
    def attack_script(self):
        return {
            'name': 'goblin-stab',
            'weilder-sprite-name': 'monster/goblin/goblin_attack.png',
            'weilder-sprite-width': 32,
            'weilder-sprite-height': 17,
            'weilder-anchor-x': 0.5,
            'weilder-anchor-y': 1,
            'keyframes': [
                {'frames': duration, 'enabled': enabled, 'origin-x': float(index),
                 'origin-y': -6.0, 'origin-degrees': 0.0,
                 'shapes': [{'enabled': True, 'damage-min': 0.0, 'damage-max': 1.0,
                             'vertices': [{'x': 0.0, 'y': 0.0}, {'x': 10.0, 'y': 0.0},
                                          {'x': 9.0, 'y': 3.0}, {'x': 0.0, 'y': 3.0}]}]}
                for index, (duration, enabled) in enumerate(
                    zip((9, 1, 1, 6, 1), (False, True, True, True, False)))
            ],
        }

    def test_encodes_ghidra_timing_damage_and_original_attack_geometry(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'attacks.json'
            path.write_text(json.dumps({'attack-scripts': [self.attack_script()]}), encoding='utf-8')
            packet = encode(path)
        magic, version, count, windup, cooldown, minimum, maximum = struct.unpack_from('<4sHHHHHH', packet)
        self.assertEqual((magic, version, count, windup, cooldown, minimum, maximum),
                         (b'VGA1', 1, 5, 16, 40, 5, 9))
        offset = 16
        active = []
        for frame_index in range(count):
            duration, enabled, vertices, ox, oy, degrees, damage_min, damage_max = \
                struct.unpack_from('<HBB5f', packet, offset)
            offset += struct.calcsize('<HBB5f')
            points = [struct.unpack_from('<ff', packet, offset + i * 8) for i in range(vertices)]
            offset += vertices * 8
            if enabled:
                active.append(frame_index)
                self.assertEqual((ox, oy, degrees, damage_min, damage_max),
                                 (float(frame_index), -6.0, 0.0, 0.0, 1.0))
                self.assertEqual(points[1], (10.0, 0.0))
        self.assertEqual(active, [1, 2, 3])
        self.assertEqual(offset, len(packet))

    def test_rejects_changed_native_keyframe_timing(self):
        script = self.attack_script()
        script['keyframes'][2]['frames'] = 2
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'attacks.json'
            path.write_text(json.dumps({'attack-scripts': [script]}), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'keyframe timing'):
                encode(path)


if __name__ == '__main__':
    unittest.main()
