import unittest
from tools.floor_main_path import MT19937,main_path,initial_tried,choose_least_used

class MainPathTests(unittest.TestCase):
    def test_reference_mt_seed5489(self):
        rng=MT19937(5489)
        self.assertEqual([rng.raw() for _ in range(10)], [3499211612,581869302,3890346734,3586334585,545404204,4161255391,3922919429,949333985,2715962298,1323567403])

    def test_distribution_is_low31_not_scaled(self):
        rng=MT19937(5489)
        self.assertEqual(rng.game_int(),1351727964)
        self.assertEqual(rng.game_int(),581869302)
        self.assertEqual(rng.draws,2)
        with self.assertRaises(ValueError):MT19937(-1)
        with self.assertRaises(ValueError):MT19937(1<<32)

    def test_boundary_sides(self):
        self.assertEqual(initial_tried(2,0,2,0,3,4),3)
        self.assertEqual(initial_tried(4,3,2,0,3,4),12)
        self.assertEqual(initial_tried(3,1,2,0,3,4),0)

    def test_paths_all_three_floor_parameters(self):
        for floor in range(3):
            for seed in range(100):
                a=main_path(seed,floor);path=a['path'];ox,oy,w,h=a['active_region']
                self.assertEqual(a,main_path(seed,floor))
                self.assertEqual(list(path[0]),a['start']);self.assertEqual(list(path[-1]),a['goal'])
                self.assertEqual(len(set(path)),len(path))
                self.assertTrue(all(ox<=x<ox+w and oy<=y<oy+h for x,y in path))
                self.assertEqual(a['rng_draws'],2+2*a['attempts']+1)
                for (x,y),(nx,ny) in zip(path,path[1:]):
                    self.assertEqual(abs(x-nx)+abs(y-ny),1)
                    d=[(-1,0),(0,-1),(1,0),(0,1)].index((nx-x,ny-y))
                    self.assertTrue(a['links_row_major'][y][x]&(1<<d))
                    self.assertTrue(a['links_row_major'][ny][nx]&(1<<((d+2)%4)))
                self.assertFalse(a['floor_complete'])
        with self.assertRaises(ValueError):main_path(1,3)

    def test_rejected_room_still_increments_frequency(self):
        frequencies={};rng=MT19937(1)
        first=choose_least_used(['one','two'],frequencies,rng)
        self.assertEqual(frequencies[first],1)
        second=choose_least_used(['one','two'],frequencies,rng)
        self.assertNotEqual(first,second)
        self.assertEqual(frequencies,{'one':1,'two':1})
        frequencies={'one':3,'two':4};self.assertEqual(choose_least_used(['one','two'],frequencies,rng),'one')
        before=rng.draws;self.assertIsNone(choose_least_used([],frequencies,rng));self.assertEqual(before,rng.draws)
        with self.assertRaises(ValueError):choose_least_used(['one','one'],{},rng)

if __name__=='__main__':unittest.main()
