import unittest
from tools.floor_room_graph import attempt_side_link,place_shop,room_graph

class Stream:
    def __init__(self,*values):self.values=iter(values);self.draws=0
    def game_int(self):self.draws+=1;return next(self.values)

def blank():return [[0]*8 for _ in range(5)]

class RoomGraphTests(unittest.TestCase):
    def test_cycle_chance_boundary_and_direction_order(self):
        links=blank();links[0][2]=4;links[0][3]=9;links[1][3]=3;links[1][2]=4
        rng=Stream(2,0,3,6)
        count,details=attempt_side_link(links,(2,0,2,2),4,4,rng)
        self.assertEqual(count,4);self.assertEqual(links[0][2],12);self.assertEqual(links[1][2],6)
        self.assertEqual(rng.draws,4);self.assertEqual(details['cycle_draws'],1)
        links[0][2]=4;links[1][2]=4
        count,_=attempt_side_link(links,(2,0,2,2),4,4,Stream(2,0,3,7))
        self.assertEqual(links[0][2],4);self.assertEqual(links[1][2],4)

    def test_new_neighbor_capacity_and_boundary(self):
        links=blank();links[0][2]=4;links[0][3]=1
        rng=Stream(2,0,3)
        count,details=attempt_side_link(links,(2,0,2,2),2,4,rng)
        self.assertEqual(count,3);self.assertEqual(links[1][2],2);self.assertTrue(details['new_room'])
        self.assertEqual(rng.draws,3)
        rng=Stream(2,0,0)
        count,_=attempt_side_link(links,(2,0,2,2),3,4,rng)
        self.assertEqual(count,3);self.assertEqual(rng.draws,3)
        links[1][2]=0;links[0][2]=4
        count,_=attempt_side_link(links,(2,0,2,2),2,2,Stream(2,0,3))
        self.assertEqual(count,2);self.assertEqual(links[1][2],0)

    def test_empty_source_consumes_only_coordinates(self):
        rng=Stream(0,0);count,details=attempt_side_link(blank(),(2,0,3,4),4,12,rng)
        self.assertEqual(count,4);self.assertEqual(rng.draws,2);self.assertIsNone(details['direction'])

    def test_shop_prefers_right_neighbor_and_left_fallback(self):
        links=blank();links[0][3]=1;links[0][5]=4
        shop=place_shop(links,Stream(4,0));self.assertEqual(shop['room_type'],260)
        self.assertEqual(shop['neighbor'],[5,0]);self.assertEqual(links[0][4],4)
        links=blank();links[0][3]=4
        shop=place_shop(links,Stream(4,0));self.assertEqual(shop['room_type'],257)
        self.assertEqual(links[0][4],1)
        self.assertIsNone(place_shop([[15]*8 for _ in range(5)],Stream()))

    def test_connected_graphs_and_rng_accounting(self):
        for floor in range(3):
            for seed in range(100):
                r=room_graph(seed,floor);self.assertEqual(r,room_graph(seed,floor))
                links=r['links_row_major'];side=r['side_expansion'];shop=r['shop']
                self.assertTrue(shop);self.assertGreaterEqual(side['rooms'],9)
                self.assertLessEqual(side['rooms'],side['max_rooms'])
                self.assertGreaterEqual(side['attempts'],side['attempt_limit'])
                self.assertEqual(r['rng_draws'],r['main_rng_draws']+2*side['attempts']+side['source_draws']+side['cycle_draws']+2*shop['attempts'])
                self.assertEqual(sum(bool(v) for row in links for v in row),side['rooms']+1)
                for y,row in enumerate(links):
                    for x,mask in enumerate(row):
                        self.assertEqual(mask&r['main_links_row_major'][y][x],r['main_links_row_major'][y][x])
                        for dx,dy,bit,opposite in ((-1,0,1,4),(1,0,4,1),(0,-1,2,8),(0,1,8,2)):
                            if mask&bit:
                                self.assertTrue(0<=x+dx<8 and 0<=y+dy<5)
                                self.assertTrue(links[y+dy][x+dx]&opposite)
                seen={tuple(r['start'])};todo=list(seen)
                while todo:
                    x,y=todo.pop()
                    for dx,dy,bit in ((-1,0,1),(1,0,4),(0,-1,2),(0,1,8)):
                        n=(x+dx,y+dy)
                        if links[y][x]&bit and n not in seen:seen.add(n);todo.append(n)
                self.assertEqual(len(seen),side['rooms']+1);self.assertIn(tuple(r['goal']),seen)
                self.assertFalse(r['floor_complete']);self.assertFalse(r['treasure_stage_implemented'])

if __name__=='__main__':unittest.main()
