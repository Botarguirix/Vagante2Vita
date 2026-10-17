import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools.prepare_melee import WEAPONS, encode


class PrepareMeleeTests(unittest.TestCase):
    def fixture(self, root):
        script_names = sorted({script for _item, _weapon, script in WEAPONS})
        attack = {
            "attack-scripts": [{
                "name": name, "weilder-sprite-width": 24,
                "weilder-sprite-height": 24,
                "keyframes": [{
                    "weilder-sprite-frame": 1, "frames": 2, "enabled": True,
                    "isChargeFrame": False, "origin-x": 0.0, "origin-y": 0.0,
                    "origin-degrees": 0.0, "weapon-duration-factor": 1.0,
                    "weapon-gfx-scale-x": 1.0, "weapon-gfx-scale-y": 1.0,
                }],
            } for name in script_names],
            "weapon-properties": [{
                "name": name, "sprite-width": 20, "sprite-height": 20,
                "sprite-frame": item, "per-frame-duration": 4,
                "type-flags": 0, "sprite-anchor-x": 0.5,
                "sprite-anchor-y": 0.5, "shapes": [{
                    "vertices": [{"x": 0.0, "y": 0.0},
                                 {"x": 2.0, "y": 0.0},
                                 {"x": 0.0, "y": 2.0}],
                    "enabled": True, "damage-min": 1.0,
                    "damage-max": 2.0, "knockback": 0.5,
                }],
            } for item, name, _script in WEAPONS],
        }
        path = root / "attacks.json"
        path.write_text(json.dumps(attack), encoding="utf-8")
        return path

    def test_encodes_every_confirmed_stab_weapon_and_preserves_item_ids(self):
        with tempfile.TemporaryDirectory() as td:
            packet = encode(self.fixture(Path(td)))
        self.assertEqual(struct.unpack_from("<4sHHII", packet),
                         (b"VWA1", 1, len(WEAPONS), 0, 0))
        actual = []
        offset = 16
        for _ in WEAPONS:
            item, _sprite, _duration, _flags, _ax, _ay, frames, shapes = \
                struct.unpack_from("<HHHHffHH", packet, offset)
            actual.append(item)
            offset += 20 + frames * 32
            for _ in range(shapes):
                vertices = struct.unpack_from("<H", packet, offset)[0]
                offset += 16 + vertices * 8
        self.assertEqual(actual, [item for item, _name, _script in WEAPONS])
        self.assertEqual(offset, len(packet))

    def test_rejects_wrong_character_sprite_geometry(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.fixture(Path(td))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["attack-scripts"][0]["weilder-sprite-width"] = 32
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "attack geometry"):
                encode(path)


if __name__ == "__main__":
    unittest.main()
