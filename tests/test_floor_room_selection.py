import unittest
from tools.floor_room_selection import select_room_grid
from tools.floor_main_path import MT19937

def raw(uid,rows):return {'uid':uid,'tiles':[[n for t in row for n in (t,-1,-1,-1,-1,-1)] for row in rows]}

class SelectionTests(unittest.TestCase):
    def test_horizontal_retries_count_and_keep_last_invalid(self):
        a={'rooms':{'4':[raw('left',[[1,1]]*3)],'1':[raw('right',[[1,1]]*3)]}}
        rng=MT19937(1);r=select_room_grid([[4,1]],a,{'width':2,'height':3},rng)
        self.assertEqual(r['frequencies'],{'left':1,'right':21})
        self.assertEqual(rng.draws,22);self.assertFalse(r['all_geometry_compatible'])
        self.assertTrue(r['placements'][1]['retained_after_retry_limit'])

    def test_column_order_and_unused_direction_does_not_restrict(self):
        a={'rooms':{'0':[raw('open',[[0,0]]*3)]}}
        rng=MT19937(2);r=select_room_grid([[0,0],[0,0]],a,{'width':2,'height':3},rng)
        self.assertEqual([(p['x'],p['y']) for p in r['placements']],[(0,0),(0,1),(1,0),(1,1)])
        self.assertTrue(r['all_geometry_compatible']);self.assertEqual(rng.draws,4)

    def test_vertical_clearance_from_previous_selected_row(self):
        a={'rooms':{'8':[raw('upper',[[0,0],[0,0],[1,3]])], '2':[raw('lower',[[1,0],[1,1],[0,0]])]}}
        r=select_room_grid([[8],[2]],a,{'width':2,'height':3},MT19937(1))
        self.assertTrue(r['all_geometry_compatible']);self.assertEqual(r['placements'][1]['attempts'],1)
        with self.assertRaises(ValueError):select_room_grid([[4096]],a,{'width':2,'height':3},MT19937(1))
        with self.assertRaises(ValueError):select_room_grid([[15]],a,{'width':2,'height':3},MT19937(1))

if __name__=='__main__':unittest.main()
