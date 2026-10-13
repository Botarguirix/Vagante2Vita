import unittest
from tools.floor_treasure import eligible_neighbor,place_treasure,treasure_graph
from test_floor_room_graph import Stream,blank

class TreasureTests(unittest.TestCase):
    def test_shop_excluded_and_right_priority(self):
        t=blank();t[0][3]=4;t[0][5]=257
        self.assertEqual(eligible_neighbor(t,4,0),(3,0,1,4))
        t[0][5]=1;self.assertEqual(eligible_neighbor(t,4,0),(5,0,4,1))
        t[0][3]=260;t[0][5]=257;self.assertIsNone(eligible_neighbor(t,4,0))

    def test_growth_occurs_after_third_sample_and_preserves_old_sample(self):
        t=blank();t[3][3]=4
        # Two occupied goal samples. Third uses rx=1 sample x=2, then grows.
        rng=Stream(1,1,1,1,0,1)
        r=place_treasure(t,[3,3],rng)
        self.assertEqual((r['x'],r['y']),(2,3));self.assertEqual(r['attempts'],3)
        self.assertEqual(r['radius_after_candidate'],[2,2]);self.assertEqual(rng.draws,6)
        self.assertEqual(t[3][2],516)

    def test_no_candidate_consumes_no_random(self):
        rng=Stream();self.assertIsNone(place_treasure(blank(),[3,3],rng));self.assertEqual(rng.draws,0)

    def test_all_graphs_connect_treasure_and_keep_shop(self):
        for floor in range(3):
            for seed in range(100):
                r=treasure_graph(seed,floor);t=r['treasure'];shop=r['shop'];types=r['room_types_row_major']
                self.assertTrue(t);self.assertEqual(types[shop['y']][shop['x']]&256,256)
                self.assertEqual(types[t['y']][t['x']]&512,512)
                self.assertNotEqual(t['neighbor'],[shop['x'],shop['y']])
                self.assertEqual(r['rng_draws']-r['pre_treasure_rng_draws'],2*t['attempts'])
                self.assertFalse(r['floor_complete']);self.assertFalse(r['secret_stage_implemented'])

if __name__=='__main__':unittest.main()
