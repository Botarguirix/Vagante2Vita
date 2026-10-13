import unittest
from tools.generation_random import rand_float, bounded_raw, shuffle


class RawStream:
    def __init__(self, *values):
        self.values = iter(values)
        self.draws = 0

    def raw(self):
        self.draws += 1
        return next(self.values)

    def game_int(self):
        return self.raw() & 0x7fffffff


class RandomAdapterTests(unittest.TestCase):
    def test_float_uses_one_full_word_and_float32_rounding(self):
        rng = RawStream(0, 0x80000000, 0xffffffff, 0x80000081)
        self.assertEqual(rand_float(rng), 0.0)
        self.assertEqual(rand_float(rng), 0.5)
        self.assertEqual(rand_float(rng), 1.0)
        self.assertEqual(rand_float(rng), 0.5 + 2 ** -24)
        self.assertEqual(rng.draws, 4)

    def test_rejection_keeps_full_word_and_power_two_accepts_max(self):
        rng = RawStream(0xffffffff, 0xfffffffe, 0xffffffff, 0x80000000)
        self.assertEqual(bounded_raw(rng, 3), 2)
        self.assertEqual(rng.draws, 2)
        self.assertEqual(bounded_raw(rng, 4), 3)
        self.assertEqual(bounded_raw(rng, 3), 2)
        self.assertEqual(rng.draws, 4)
        self.assertEqual(bounded_raw(rng, 1), 0)
        self.assertEqual(rng.draws, 4)
        for bound in (0, -1, True, 0x80000000):
            with self.assertRaises(ValueError):
                bounded_raw(rng, bound)

    def test_forward_shuffle_consumes_self_swap_and_no_draw_for_short_lists(self):
        rng = RawStream(0, 2, 1)
        values = list('abcd')
        shuffle(rng, values)
        self.assertEqual(values, list('bdca'))
        self.assertEqual(rng.draws, 3)
        shuffle(rng, [])
        shuffle(rng, [1])
        self.assertEqual(rng.draws, 3)


if __name__ == '__main__':
    unittest.main()
