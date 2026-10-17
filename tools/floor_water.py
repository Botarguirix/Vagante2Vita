"""ACT_ONE liquid geometry and its generation-time random consumption.

This is reconstructed code. It uses caller-owned tiles and contains no assets.
It does not claim parity with a running PC game or implement fluid rendering.
"""
import math

try:
    from tools.generation_random import float32, rand_float
except ModuleNotFoundError:
    from generation_random import float32, rand_float

LIQUID_TILE_SIZE = 8


class WaterField:
    """A byte mass per half-tile, with a collision snapshot for this stage."""

    def __init__(self, tiles, fluid=None):
        if not tiles or not tiles[0]:
            raise ValueError('A nonempty rectangular tile grid is required')
        self.tile_height, self.tile_width = len(tiles), len(tiles[0])
        types = []
        for row in tiles:
            if len(row) != self.tile_width:
                raise ValueError('Tile rows must have equal widths')
            decoded = []
            for tile in row:
                if type(tile) is int:
                    kind = tile
                elif isinstance(tile, (list, tuple)) and len(tile) == 6:
                    kind = tile[0]
                else:
                    raise ValueError('Tiles must be types or six serialized fields')
                if type(kind) is not int or not 0 <= kind <= 127:
                    raise ValueError('Tile types must be nonnegative signed bytes')
                decoded.append(kind)
            types.append(decoded)
        self.solid = [[kind in (1, 7) for kind in row] for row in types]
        self.width, self.height = self.tile_width * 2, self.tile_height * 2
        self.frame = 0
        if fluid is not None:
            if len(fluid) != self.width * self.height:
                raise ValueError('Fluid buffer must contain exactly 2w * 2h bytes')
            self.fluid = bytearray(fluid)
        else:
            self.fluid = bytearray(self.width * self.height)
            for y, row in enumerate(types):
                for x, kind in enumerate(row):
                    if kind == 9:
                        for fy in (2 * y, 2 * y + 1):
                            i = fy * self.width + 2 * x
                            self.fluid[i:i + 2] = b'\x64\x64'

    def _open(self, x, y):
        return (0 <= x < self.tile_width and 0 <= y < self.tile_height
                and not self.solid[y][x])

    def _balance(self, current, neighbor):
        difference = self.fluid[current] - self.fluid[neighbor]
        amount = min(abs(difference) // 2, 10)
        if difference > 0:
            self.fluid[current] -= amount
            self.fluid[neighbor] += amount
        elif difference < 0:
            self.fluid[current] += amount
            self.fluid[neighbor] -= amount

    def _update(self, rng, particles_enabled, pool_remaining, audit):
        # In-place bottom-to-top sweep. Row zero is intentionally excluded.
        for y in range(self.height - 1, 0, -1):
            for x in range(self.width):
                i = y * self.width + x
                if not self.fluid[i]:
                    continue
                if y < self.height - 1 and self._open(x // 2, (y + 1) // 2):
                    below = i + self.width
                    mass, lower = self.fluid[i], self.fluid[below]
                    amount = 0
                    if lower < 100:
                        amount = min(mass, 100 - lower, 10)
                    elif lower < 110 and lower <= mass:
                        amount = min(mass - lower + 2, 10)
                    self.fluid[below] += amount
                    self.fluid[i] -= amount
                # This second early exit controls both later flows and RNG.
                if not self.fluid[i]:
                    continue
                audit['active_cells_after_gravity'] += 1
                if x > 0 and self._open((x - 1) // 2, y // 2):
                    self._balance(i, i - 1)
                if x < self.width - 1 and self._open((x + 1) // 2, y // 2):
                    self._balance(i, i + 1)
                if self.fluid[i] > 100 and self._open(x // 2, (y - 1) // 2):
                    above = i - self.width
                    if self.fluid[above] < self.fluid[i] - 2:
                        amount = min(self.fluid[i],
                                     self.fluid[i] - self.fluid[above] - 2, 30)
                        self.fluid[i] -= amount
                        self.fluid[above] += amount
                if rng is None:
                    continue
                # Every cell that survived gravity draws, even if later empty.
                chance = rng.game_int()
                audit['integer_draws'] += 1
                if (chance % 700 != 0 or self.fluid[i] <= 80
                        or not self._open(x // 2, y // 2)
                        or self._open(x // 2, (y + 1) // 2)):
                    continue
                # Position is sampled even with particles disabled or no slot.
                rand_float(rng)
                audit['float_draws'] += 1
                audit['bubble_candidates'] += 1
                if particles_enabled and pool_remaining:
                    # BUBBLE initialization draws randomNumber and subFrame.
                    rng.game_int()
                    rng.game_int()
                    audit['integer_draws'] += 2
                    audit['particles_spawned'] += 1
                    pool_remaining -= 1
        return pool_remaining

    def simulate(self, ticks=60, rng=None, *, particles_enabled=True,
                 pool_available=1024):
        """Advance ACT_ONE generation water, optionally retaining exact draws.

        Pool availability is an explicit input: these ticks never update or
        recycle particles. Geometry is deterministic even without an RNG.
        The caller must supply the correct incoming engine state and free
        particle count before claiming full generation-stream equivalence.
        """
        if type(ticks) is not int:
            raise ValueError('Tick count must be an integer')
        if type(particles_enabled) is not bool:
            raise ValueError('particles_enabled must be boolean')
        if type(pool_available) is not int or pool_available < 0:
            raise ValueError('Pool availability must be a nonnegative integer')
        if rng is not None and not all(callable(getattr(rng, m, None))
                                       for m in ('game_int', 'raw')):
            raise ValueError('RNG must provide game_int and raw methods')
        start_frame = self.frame
        audit = dict(ticks=max(ticks, 0), start_frame=start_frame,
                     active_cells_after_gravity=0, integer_draws=0,
                     float_draws=0, bubble_candidates=0, particles_spawned=0,
                     particles_enabled=particles_enabled,
                     pool_available_start=pool_available,
                     rng_consumption_reconstructed=rng is not None,
                     pc_runtime_parity_verified=False)
        for _ in range(max(ticks, 0)):
            self.frame += 1
            pool_available = self._update(rng, particles_enabled, pool_available, audit)
        audit['end_frame'] = self.frame
        audit['pool_available_end'] = pool_available
        audit['total_draws'] = audit['integer_draws'] + audit['float_draws']
        return audit

    def step(self, rng=None, *, particles_enabled=True, pool_available=1024):
        return self.simulate(1, rng, particles_enabled=particles_enabled,
                             pool_available=pool_available)

    def is_point_in_water(self, pixel_x, pixel_y):
        """Recovered supported-water surface predicate, using float32 steps."""
        if type(pixel_x) not in (int, float) or type(pixel_y) not in (int, float):
            raise ValueError('Point coordinates must be numbers')
        if not math.isfinite(pixel_x) or not math.isfinite(pixel_y):
            return False
        try:
            x = float32(float32(pixel_x) * float32(1 / LIQUID_TILE_SIZE))
            y = float32(float32(pixel_y) * float32(1 / LIQUID_TILE_SIZE))
        except OverflowError:
            return False
        # cvttss2si truncates, including small negative coordinates.
        fx, fy = int(x), int(y)
        if not 0 <= fx < self.width or not 0 <= fy < self.height:
            return False
        i = fy * self.width + fx
        if (fy < self.height - 1 and self._open(fx // 2, (fy + 1) // 2)
                and self.fluid[i + self.width] < 50):
            return False
        fraction_percent = float32(float32(y - float32(fy)) * float32(100))
        return fraction_percent > float32(100 - self.fluid[i])

