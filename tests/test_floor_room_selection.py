import unittest
from tools.floor_layout import boss_room_spawn_geometry, boss_spawn_random_fallback
from tools.floor_room_selection import select_room_grid, choose_boss_room
from tools.floor_main_path import MT19937

def raw(uid,rows):return {'uid':uid,'tiles':[[n for t in row for n in (t,-1,-1,-1,-1,-1)] for row in rows]}

class SelectionTests(unittest.TestCase):
    def test_boss_spawn_geometry_requires_open_2x2_reachability_and_support(self):
        tiles = [[[0] for _ in range(12)] for _ in range(12)]
        types = [[15] * 3 for _ in range(3)]
        tiles[6][4][0] = 1
        tiles[6][5][0] = 1
        result = boss_room_spawn_geometry(tiles, types, {'width': 4, 'height': 4}, (1, 1))
        self.assertEqual(result['candidate_count'], 1)
        self.assertEqual(result['anchors'][0]['tile'], [4, 4])
        self.assertEqual(result['anchors'][0]['support_tiles'], [[4, 6], [5, 6]])
        self.assertTrue(result['entity_blockers_applied'])
        self.assertEqual(result['after_static_entity_filter_count'], 1)
        self.assertFalse(result['rng_shuffle_applied'])
        self.assertFalse(result['native_spawn_ready'])

    def test_boss_spawn_static_entity_filter_uses_native_entity_types(self):
        tiles = [[[0] for _ in range(12)] for _ in range(12)]
        types = [[15] * 3 for _ in range(3)]
        tiles[6][4][0] = tiles[6][5][0] = 1
        entities = [
            {'definition': {'type': 'Spikes'}, 'world_pixel': [64, 80]},
            {'definition': {'type': 'Chest'}, 'world_pixel': [64, 80]},
        ]
        result = boss_room_spawn_geometry(
            tiles, types, {'width': 4, 'height': 4}, (1, 1), entities)
        self.assertEqual(result['static_entity_blockers'], [
            {'index': 0, 'type': 'Spikes', 'entity_type': 69, 'center_tile': [4, 5]}])
        self.assertEqual(result['candidate_count'], 1)
        self.assertEqual(result['after_static_entity_filter_count'], 0)
        self.assertEqual(result['anchors'][0]['blocked_by_static_entities'],
                         result['static_entity_blockers'])

    def test_boss_spawn_blocker_centers_use_each_native_body_size(self):
        tiles = [[[0] for _ in range(12)] for _ in range(12)]
        types = [[15] * 3 for _ in range(3)]
        tiles[6][4][0] = tiles[6][5][0] = 1
        blocker_names = ('Spikes', 'BoulderTrap', 'SpikeTrap', 'Shrine')
        entities = [
            {'definition': {'type': name}, 'world_pixel': [64, 72]}
            for name in blocker_names]
        result = boss_room_spawn_geometry(
            tiles, types, {'width': 4, 'height': 4}, (1, 1), entities)
        self.assertEqual([b['entity_type'] for b in result['static_entity_blockers']],
                         [69, 71, 75])
        self.assertEqual([b['center_tile'] for b in result['static_entity_blockers']],
                         [[4, 5], [4, 5], [4, 5]])
        self.assertEqual(result['after_static_entity_filter_count'], 0)
        self.assertEqual(result['anchors'][0]['blocked_by_static_entities'],
                         result['static_entity_blockers'][:3])
        shrine_only = boss_room_spawn_geometry(
            tiles, types, {'width': 4, 'height': 4}, (1, 1), [entities[3]])
        self.assertEqual(shrine_only['after_static_entity_filter_count'], 1)
        self.assertEqual(shrine_only['static_entity_blockers'], [])

    def test_dynamic_thief_locations_block_using_tile_coordinates(self):
        tiles = [[[0] for _ in range(12)] for _ in range(12)]
        types = [[15] * 3 for _ in range(3)]
        tiles[6][4][0] = tiles[6][5][0] = 1
        result = boss_room_spawn_geometry(
            tiles, types, {'width': 4, 'height': 4}, (1, 1),
            dynamic_thief_tiles=[[4, 5]])
        self.assertEqual(result['static_entity_blockers'], [
            {'index': 0, 'type': 'Thief', 'entity_type': 84, 'center_tile': [4, 5]}])
        self.assertEqual(result['after_static_entity_filter_count'], 0)
        self.assertFalse(result['native_spawn_ready'])

    def test_thief_spawn_gate_is_reported_without_claiming_spawn_parity(self):
        from tools.floor_layout import provisional_enemies
        from tools.floor_main_path import MT19937
        tiles = [[[0] for _ in range(20)] for _ in range(20)]
        placements = [dict(x=0, y=1, room_type=0, uid='room')]
        trace = {}
        provisional_enemies(tiles, [[15]], placements,
                            {'width': 20, 'height': 20}, MT19937(5),
                            [1, 1], [18, 18], rng_trace=trace)
        thief = trace['thief_spawn_branch']
        self.assertEqual(thief['gate_remaining'], 0)
        self.assertEqual(thief['spawn_type'], 84)
        self.assertFalse(thief['native_parity'])
        self.assertEqual(trace['dynamic_thief_spawns'], [])

    def test_act_one_thief_budget_is_one_in_five_only_after_floor_zero(self):
        from tools.floor_layout import act_one_thief_spawn_budget_for_roll
        self.assertEqual([act_one_thief_spawn_budget_for_roll(n, 0)
                          for n in range(10)], [0] * 10)
        self.assertEqual([act_one_thief_spawn_budget_for_roll(n, 1)
                          for n in range(10)], [1, 0, 0, 0, 0, 1, 0, 0, 0, 0])
        self.assertEqual(act_one_thief_spawn_budget_for_roll(10, 2), 1)

    def test_thief_candidate_requires_both_native_distance_axes_and_room_path(self):
        from tools.floor_layout import act_one_thief_spawn_candidate
        tiles = [[[0] for _ in range(32)] for _ in range(32)]
        types = [[15, 15], [15, 15]]
        spec = {'width': 16, 'height': 16}
        self.assertFalse(act_one_thief_spawn_candidate(
            1, 14, 15, (0, 0), tiles, types, spec))
        self.assertFalse(act_one_thief_spawn_candidate(
            1, 16, 14, (0, 0), tiles, types, spec))
        self.assertTrue(act_one_thief_spawn_candidate(
            1, 15, 15, (0, 0), tiles, types, spec))
        tiles[15][15][0] = 1
        self.assertFalse(act_one_thief_spawn_candidate(
            1, 15, 15, (0, 0), tiles, types, spec))

    def test_thief_spawn_x_uses_low_four_random_bits(self):
        from tools.floor_layout import act_one_thief_spawn_position
        self.assertEqual(act_one_thief_spawn_position(12, 7, 0x1234567b), [203, 112])

    def test_boss_fallback_collects_four_open_positions_and_preserves_duplicates(self):
        tiles = [[[1] for _ in range(12)] for _ in range(12)]
        tiles[5][5][0] = 0
        rng = MT19937(123)
        result = boss_spawn_random_fallback(
            tiles, {'width': 4, 'height': 4}, (1, 1), rng)
        expected = [[5, 5]] * 4
        self.assertEqual(result['candidates_before_shuffle'], expected)
        self.assertEqual(result['candidates_after_shuffle'], expected)
        self.assertEqual(result['selected_tile'], [5, 5])
        self.assertTrue(result['duplicate_candidates_preserved'])
        self.assertTrue(result['exact_rng_state_required'])
        self.assertGreaterEqual(result['attempts'], 4)
        self.assertGreaterEqual(result['rng_draws'], 2 * result['attempts'] + 3)

    def test_boss_fallback_has_a_test_guard_for_rooms_without_open_tiles(self):
        tiles = [[[1] for _ in range(12)] for _ in range(12)]
        rng = MT19937(1)
        with self.assertRaisesRegex(RuntimeError, 'four open room tiles'):
            boss_spawn_random_fallback(
                tiles, {'width': 4, 'height': 4}, (1, 1), rng, max_attempts=4)
        self.assertEqual(rng.draws, 8)

    def test_boss_spawn_geometry_rejects_missing_support_and_bad_room(self):
        tiles = [[[0] for _ in range(12)] for _ in range(12)]
        types = [[15] * 3 for _ in range(3)]
        result = boss_room_spawn_geometry(tiles, types, {'width': 4, 'height': 4}, (1, 1))
        self.assertEqual(result['anchors'], [])
        with self.assertRaisesRegex(ValueError, 'out of bounds'):
            boss_room_spawn_geometry(tiles, types, {'width': 4, 'height': 4}, (3, 3))

    def test_boss_room_is_first_max_open_room_far_from_endpoints(self):
        placements=[]
        rooms={}
        data=[('near_start',0,10),('too_near',0,20),('distance_two',0,30),
              ('tie_first',0,4),('tie_second',0,4),('secret',0,40),
              ('bright_sky',0x400,50),('shop',0x100,60),('treasure',0x200,70),
              ('near_goal',0,80)]
        for x,(uid,kind,open_count) in enumerate(data):
            tile_types=[0]*open_count+[1]*max(0,80-open_count)
            rows=[tile_types[i:i+10] for i in range(0,80,10)]
            rooms.setdefault(str(kind),[]).append(raw(uid,rows))
            placements.append(dict(x=x,y=0,room_type=kind,uid=uid))
        result=choose_boss_room(placements,{'rooms':rooms},[0,0],[9,0],
                                secret_rooms=[(5,0)])
        self.assertEqual(result['room'],dict(x=3,y=0,uid='tie_first',open_tiles=4))
        self.assertEqual(result['candidate_count'],2)
        self.assertEqual(result['selection'],'maximum_non_type_1_tiles_first_x_major')

    def test_boss_room_falls_back_to_goal_when_no_candidate(self):
        rooms={'rooms':{'0':[raw('only',[[0]*10]*8)]}}
        placements=[dict(x=0,y=0,room_type=0,uid='only')]
        result=choose_boss_room(placements,rooms,[0,0],[2,2])
        self.assertEqual(result['candidate_count'],0)
        self.assertEqual(result['room']['x'],2)
        self.assertEqual(result['room']['y'],2)

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
