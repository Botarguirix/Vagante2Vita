import unittest
from tools.room_connectivity import RoomGeometry, decode_room, fits_vertically, audit


def room(rows):
    return RoomGeometry("fixture", 0, tuple(tuple(r) for r in rows))


class ConnectivityTests(unittest.TestCase):
    def test_directional_ladder_and_platform_support(self):
        upper = room([[0,0,0], [0,0,0], [1,3,1]])
        lower = room([[1,0,1], [0,1,0], [0,0,0]])
        self.assertTrue(fits_vertically(upper, lower))
        self.assertFalse(fits_vertically(lower, upper))
        self.assertFalse(fits_vertically(upper, room([[1,0,1], [0,0,0]])))
        self.assertTrue(fits_vertically(room([[0], [0], [6]]), room([[3], [0]])))

    def test_side_clearance_and_neighbor_support(self):
        upper = room([[0,0], [0,0], [0,1]])
        lower = room([[0,0], [0,6]])
        self.assertTrue(fits_vertically(upper, lower))
        blocked = room([[1,0], [1,0], [0,1]])
        self.assertFalse(fits_vertically(blocked, lower))
        self.assertFalse(fits_vertically(upper, room([[1,0], [1,0]])))

    def test_step_branch_requires_headroom(self):
        upper = room([[0,0], [0,1], [0,0]])
        lower = room([[0,1], [1,1]])
        self.assertTrue(fits_vertically(upper, lower))
        self.assertFalse(fits_vertically(room([[0,1], [0,1], [0,0]]), lower))

    def test_open_edges_alone_are_insufficient(self):
        self.assertFalse(fits_vertically(room([[0], [0], [0]]), room([[0], [1]])))
        self.assertFalse(fits_vertically(room([[3]]), room([[0], [1]])))
        self.assertFalse(fits_vertically(room([[0], [0], [3]]), room([[0,0], [1,1]])))

    def test_dimension_flags_and_invalid_fields(self):
        raw = dict(uid="own", tiles=[[0,-1,-1,-1,-1,-1]*4]*6)
        self.assertEqual(decode_room(raw, 0x3000, 2, 3).width, 4)
        with self.assertRaises(ValueError): decode_room(raw, 0, 2, 3)
        raw['tiles'][0] = [False,-1,-1,-1,-1,-1]*4
        with self.assertRaises(ValueError): decode_room(raw, 0x3000, 2, 3)

    def test_audit_counts_and_preserves_direction(self):
        a=dict(uid="a", tiles=[[0,-1,-1,-1,-1,-1],[0,-1,-1,-1,-1,-1],[3,-1,-1,-1,-1,-1]])
        b=dict(uid="b", tiles=[[0,-1,-1,-1,-1,-1],[1,-1,-1,-1,-1,-1],[0,-1,-1,-1,-1,-1]])
        d=dict(act_specifications=[dict(width=1,height=3)], acts=[dict(rooms={"8":[a], "2":[b]})])
        report=audit(d)
        self.assertEqual(report['acts'][0]['compatible_pairs'], [('a','b')])
        self.assertFalse(report['floor_generation_implemented'])
        d['acts'][0]['rooms']['2'][0]['uid']='a'
        with self.assertRaises(ValueError): audit(d)

if __name__ == '__main__': unittest.main()
