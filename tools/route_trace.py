"""Binary input traces recorded while the native player physics probe runs."""
from __future__ import annotations

import struct
from pathlib import Path

HEADER = struct.Struct('<4sI')
FRAME = struct.Struct('<H')
MAGIC = b'VTR1'
MAX_FRAMES = 36000
ALLOWED_BUTTONS = 0x0000F3F9


def read_trace(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) < HEADER.size:
        raise ValueError(f'{path}: truncated VTR1 header')
    magic, count = HEADER.unpack_from(data)
    if magic != MAGIC or not count or count > MAX_FRAMES:
        raise ValueError(f'{path}: invalid VTR1 header or frame count')
    if len(data) != HEADER.size + count * FRAME.size:
        raise ValueError(f'{path}: VTR1 size does not match frame count')
    masks = [FRAME.unpack_from(data, HEADER.size + i * FRAME.size)[0]
             for i in range(count)]
    if any(mask & ~ALLOWED_BUTTONS for mask in masks):
        raise ValueError(f'{path}: VTR1 contains unsupported Vita controller buttons')
    return masks
