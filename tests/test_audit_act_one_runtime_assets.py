import struct
import tempfile
import unittest
import zlib
from pathlib import Path

from tools.audit_act_one_runtime_assets import (
    png_dimensions, read_packet, validate_enemy_record,
)


def png_chunk(kind, payload):
    return (struct.pack(">I", len(payload)) + kind + payload
            + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))


def tiny_png():
    header = struct.pack(">IIBBBBB", 1, 1, 8, 6, 0, 0, 0)
    image = zlib.compress(b"\x00\xff\x00\x00\xff")
    return (b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header)
            + png_chunk(b"IDAT", image) + png_chunk(b"IEND", b""))


class RuntimeAssetAuditTests(unittest.TestCase):
    def test_reads_png_dimensions_after_crc_and_deflate_validation(self):
        with tempfile.TemporaryDirectory() as temp:
            image = Path(temp) / "valid.png"
            image.write_bytes(tiny_png())
            self.assertEqual(png_dimensions(image), (1, 1))

    def test_rejects_png_with_corrupt_chunk_crc(self):
        with tempfile.TemporaryDirectory() as temp:
            image = Path(temp) / "broken.png"
            data = bytearray(tiny_png())
            data[-5] ^= 1
            image.write_bytes(data)
            with self.assertRaisesRegex(ValueError, "CRC mismatch"):
                png_dimensions(image)

    def test_reads_fixed_record_packets_and_rejects_truncation(self):
        with tempfile.TemporaryDirectory() as temp:
            packet = Path(temp) / "entities.vwe"
            packet.write_bytes(struct.pack("<4sHH", b"VWE2", 1, 16) + bytes(16))
            _, records = read_packet(packet, b"VWE2", 16)
            self.assertEqual(records, [bytes(16)])
            packet.write_bytes(packet.read_bytes()[:-1])
            with self.assertRaisesRegex(ValueError, "invalid record size"):
                read_packet(packet, b"VWE2", 16)

    def test_accepts_only_supported_enemy_and_provisional_boss_records(self):
        normal = struct.pack("<BBHiiI", 24, 9, 0, 128, 256, 0)
        boss = struct.pack("<BBHiiI", 26, 49, 0, 128, 256, 1)
        dragon = struct.pack("<BBHiiI", 26, 50, 0, 128, 256, 2)
        worm = struct.pack("<BBHiiI", 26, 51, 0, 128, 256, 3)
        self.assertEqual(validate_enemy_record(normal, "floor.vwe", 80, 40), "goblin")
        self.assertEqual(validate_enemy_record(boss, "floor.vwe", 80, 40), "goblin_king")
        self.assertEqual(validate_enemy_record(dragon, "floor.vwe", 80, 40), "dragon")
        self.assertEqual(validate_enemy_record(worm, "floor.vwe", 80, 40), "worm")
        invalid_records = (
            struct.pack("<BBHiiI", 24, 49, 0, 128, 256, 1),
            struct.pack("<BBHiiI", 26, 52, 0, 128, 256, 1),
            struct.pack("<BBHiiI", 26, 49, 1, 128, 256, 1),
            struct.pack("<BBHiiI", 26, 49, 0, -1, 256, 1),
        )
        for record in invalid_records:
            with self.subTest(record=record):
                with self.assertRaises(ValueError):
                    validate_enemy_record(record, "floor.vwe", 80, 40)


if __name__ == "__main__":
    unittest.main()
