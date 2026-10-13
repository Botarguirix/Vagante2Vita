"""Random adapters recovered from this Windows build, independent of its assets."""
import struct

UINT32_RANGE = 1 << 32


def float32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def rand_float(rng):
    # The build uses one MT word and rounds uint32 to float before division.
    # Its instruction sequence does not clamp a result rounded to 1.0.
    return float32(float32(rng.raw()) / UINT32_RANGE)


def bounded_raw(rng, bound):
    """Windows shuffle's rejection sampler; not the game's low-31-bit integer."""
    if type(bound) is not int or not 1 <= bound <= 0x7fffffff:
        raise ValueError('Shuffle bound must be a positive int32')
    if bound == 1:
        return 0
    limit = UINT32_RANGE // bound * bound
    while True:
        value = rng.raw()
        if value < limit:
            return value % bound


def shuffle(rng, values):
    # Forward Fisher-Yates, preserving draws even for self-swaps.
    for i in range(1, len(values)):
        j = bounded_raw(rng, i + 1)
        values[i], values[j] = values[j], values[i]

