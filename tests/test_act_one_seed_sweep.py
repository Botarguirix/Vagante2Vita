import json
import unittest
from pathlib import Path

from tools.floor_layout import floor_layout
from tools.export_act_one_world import encode_enemies


class ActOneSeedSweepTests(unittest.TestCase):
    def test_all_act_one_room_entity_types_have_rng_classification(self):
        database_path = Path(__file__).parents[1] / '_local/assets/rooms.json'
        if not database_path.is_file():
            self.skipTest('Private local Vagante room database is not available')
        database = json.loads(database_path.read_text('utf-8'))
        entities = [entity for rooms in database['acts'][0]['rooms'].values()
                    for room in rooms for entity in room.get('entities', [])]
        kinds = {entity['type'] for entity in entities}
        doodad_types = {entity.get('doodadType') for entity in entities
                        if entity['type'] == 'Doodad'}
        self.assertEqual(kinds, {'Chest', 'Spikes', 'DartTrap', 'Doodad',
                                 'BoulderTrap', 'BreakableWall', 'Boulder',
                                 'BoulderBlocker'})
        self.assertEqual(doodad_types, {1, 2, 4, 5, 14, 19, 20, 21})

        from tools.floor_layout import consume_placed_entity_rng
        from tools.floor_main_path import MT19937
        locations = [dict(room=[0, 0], source_uid='catalog-audit',
                          source_entity_index=index,
                          definition=entity)
                     for index, entity in enumerate(entities)]
        ledger = consume_placed_entity_rng(MT19937(1), locations)
        self.assertEqual(ledger['unclassified_entity_count'], 0)

    def test_act_one_seed_with_native_spider_branch_exports_vwe3(self):
        database_path = Path(__file__).parents[1] / '_local/assets/rooms.json'
        if not database_path.is_file():
            self.skipTest('Private local Vagante room database is not available')
        database = json.loads(database_path.read_text('utf-8'))
        report = floor_layout(1, 1, database)['tile_layout']
        spiders = [enemy for enemy in report['provisional_enemies']
                   if enemy['subtype'] == 14]
        self.assertEqual(len(spiders), 1)
        packet, counts = encode_enemies(report['provisional_enemies'], 80, 40)
        self.assertEqual(packet[:4], b'VWE3')
        self.assertEqual(counts[14], 1)

    def test_act_one_thief_branch_exports_native_subtype_and_position(self):
        database_path = Path(__file__).parents[1] / '_local/assets/rooms.json'
        if not database_path.is_file():
            self.skipTest('Private local Vagante room database is not available')
        database = json.loads(database_path.read_text('utf-8'))
        report = floor_layout(31, 1, database)
        layout = report['tile_layout']
        trace = layout['provisional_enemy_rng_trace']
        spawns = trace['dynamic_thief_spawns']
        self.assertEqual(len(spawns), 1)
        spawn = spawns[0]
        self.assertGreaterEqual(abs(spawn['tile'][0] - spawn['entrance_tile'][0]), 15)
        self.assertGreaterEqual(abs(spawn['tile'][1] - spawn['entrance_tile'][1]), 15)
        self.assertEqual(spawn['world_pixel'], [
            spawn['tile'][0] * 16 + (spawn['position_roll'] & 15),
            spawn['tile'][1] * 16])
        thief_records = [enemy for enemy in layout['provisional_enemies']
                         if enemy['subtype'] == 84]
        self.assertEqual(len(thief_records), 1)
        thief_blockers = [blocker for blocker in
                          report['boss_spawn_geometry']['static_entity_blockers']
                          if blocker['type'] == 'Thief']
        self.assertEqual(thief_blockers, [{
            'index': 0, 'type': 'Thief', 'entity_type': 84, 'center_tile': [33, 9]}])
        packet, counts = encode_enemies(thief_records, 80, 40)
        self.assertEqual(packet[:4], b'VWE3')
        self.assertEqual(counts[84], 1)

    def test_twenty_seeds_across_act_one_floors_fit_runtime_and_have_tile_routes(self):
        database_path = Path(__file__).parents[1] / '_local/assets/rooms.json'
        if not database_path.is_file():
            self.skipTest('Private local Vagante room database is not available')
        database = json.loads(database_path.read_text('utf-8'))
        blocked_candidates_rejected = 0
        sorted_goal_candidate_counts = []
        thief_gate_counts = [0, 0, 0]
        for floor_index in range(3):
            for seed in range(20):
                with self.subTest(floor=floor_index, seed=seed):
                    report = floor_layout(seed, floor_index, database)
                    layout = report['tile_layout']
                    rng_trace = layout['provisional_enemy_rng_trace']
                    thief_gate = rng_trace['thief_spawn_budget']
                    thief_gate_counts[floor_index] += thief_gate['initial_budget']
                    self.assertEqual(
                        thief_gate['initial_budget'],
                        int(floor_index > 0 and thief_gate['initial_roll'] % 5 == 0))
                    self.assertTrue(thief_gate['spawn_geometry_simulated'])
                    thief_spawns = rng_trace['dynamic_thief_spawns']
                    self.assertLessEqual(len(thief_spawns), 1)
                    self.assertEqual(thief_gate['remaining'],
                                     thief_gate['initial_budget'] - len(thief_spawns))
                    self.assertEqual(thief_gate['spawn_position_draw_consumed'],
                                     bool(thief_spawns))
                    self.assertTrue(all(spawn['spawn_type'] == 84
                                        for spawn in thief_spawns))
                    endpoint = layout['endpoints']
                    goal_search = layout['goal_candidates_after_terrain']['candidates']
                    fallback = layout['goal_fallback_candidates_after_terrain']
                    final_goal_candidate_count = (len(goal_search) if len(goal_search) >= 8
                                                  else len((fallback or {}).get('preferred')
                                                           or (fallback or {}).get('near_water')
                                                           or []))
                    sorted_goal_candidate_counts.append(final_goal_candidate_count)
                    self.assertEqual(endpoint['goal_sort']['candidate_count'],
                                     final_goal_candidate_count)
                    self.assertEqual(endpoint['goal_sort']['algorithm'],
                                     'msvc_insertion_sort')
                    self.assertTrue(endpoint['goal_sort']['exact_tie_order'])
                    boss_room = report['boss_room_selection']
                    boss_geometry = report['boss_spawn_geometry']
                    self.assertEqual(
                        boss_room['selection'],
                        'maximum_non_type_1_tiles_first_x_major')
                    self.assertGreaterEqual(boss_room['room']['x'], 0)
                    self.assertLess(boss_room['room']['x'], 8)
                    self.assertGreaterEqual(boss_room['room']['y'], 0)
                    self.assertLess(boss_room['room']['y'], 5)
                    self.assertFalse(rng_trace['native_parity'])
                    self.assertEqual(rng_trace['room_list_filter']['excluded_top_row_count'], 8)
                    self.assertTrue(all(room[1] != 0 for room in rng_trace['room_order']))
                    if boss_room['room']['y'] == 0:
                        self.assertIsNone(rng_trace['boss_call'])
                        self.assertEqual(rng_trace['boss_not_called']['room'],
                                         [boss_room['room']['x'], 0])
                    else:
                        self.assertEqual(rng_trace['boss_call']['room'],
                                         [boss_room['room']['x'], boss_room['room']['y']])
                        self.assertEqual(
                            rng_trace['rooms'][rng_trace['boss_call']['order_index']]['draw_start'],
                            rng_trace['boss_call']['draw_index'])
                    self.assertEqual(rng_trace['exit_draw'] - rng_trace['entry_draw'],
                                     rng_trace['draws_consumed'])
                    self.assertEqual(boss_geometry['candidate_count'],
                                     len(boss_geometry['anchors']))
                    self.assertEqual(
                        boss_geometry['after_static_entity_filter_count'],
                        len(boss_geometry['after_static_entity_filter']))
                    self.assertEqual(
                        sum(not anchor['blocked_by_static_entities']
                            for anchor in boss_geometry['anchors']),
                        boss_geometry['after_static_entity_filter_count'])
                    self.assertTrue(boss_geometry['entity_blockers_applied'])
                    self.assertFalse(boss_geometry['entity_filter_complete'])
                    self.assertFalse(boss_geometry['rng_shuffle_applied'])
                    self.assertFalse(boss_geometry['native_spawn_ready'])
                    for anchor in boss_geometry['anchors']:
                        self.assertEqual(len(anchor['body_paths']), 2)
                        self.assertTrue(all(all(column) for column in anchor['body_paths']))
                    self.assertEqual((layout['width'], layout['height']), (80, 40))
                    self.assertTrue(endpoint['blocked_room_entities_checked'])
                    self.assertEqual(endpoint['blocked_room_entity_tile_count'],
                                     len(endpoint['blocked_room_entity_tiles']))
                    blocked = {tuple(point) for point in endpoint['blocked_room_entity_tiles']}
                    blocked_candidates_rejected += (
                        layout['entrance_candidates_after_terrain']['blocked_candidate_count']
                        + endpoint['blocked_goal_candidate_count'])
                    if endpoint['entrance_method'] == 'candidate':
                        self.assertNotIn(tuple(endpoint['entrance']), blocked)
                    if endpoint['goal_method'] == 'candidate':
                        self.assertNotIn(tuple(endpoint['goal']), blocked)
                    self.assertLessEqual(len(layout['entity_locations']), 256)
                    self.assertTrue(endpoint['native_player_spawn_geometry']['clear'])
                    self.assertTrue(endpoint['native_player_spawn_geometry']['supported'])
                    self.assertTrue(layout['native_tile_path']['reachable'])
                    self.assertTrue(endpoint['candidate_shuffle_reconstructed'])
                    entrance_list = (layout['entrance_candidates_after_terrain']['preferred']
                                     or layout['entrance_candidates_after_terrain']['near_water'])
                    self.assertGreaterEqual(
                        endpoint['candidate_shuffle_rng_draws']['entrance'],
                        max(0, len(entrance_list) - 1))
                    goal_candidates = goal_search
                    self.assertGreaterEqual(
                        endpoint['candidate_shuffle_rng_draws']['goal'],
                        max(0, len(goal_candidates) - 1))
                    self.assertTrue(layout['entrance_candidates_after_terrain']['shuffled']
                                    or not (layout['entrance_candidates_after_terrain']['preferred']
                                            or layout['entrance_candidates_after_terrain']['near_water']))
                    self.assertTrue(layout['goal_candidates_after_terrain']['shuffled']
                                    or not layout['goal_candidates_after_terrain']['candidates'])
                    fallback = layout['goal_fallback_candidates_after_terrain']
                    if len(goal_candidates) < 8:
                        self.assertIsNotNone(fallback)
                        fallback_list = fallback['preferred'] or fallback['near_water']
                        self.assertGreaterEqual(
                            endpoint['candidate_shuffle_rng_draws']['goal_fallback'],
                            max(0, len(fallback_list) - 1))
                    else:
                        self.assertIsNone(fallback)
                    self.assertEqual(endpoint['goal_comparator_fields'][-1], 'y_desc')
                    water = layout['water']
                    self.assertTrue(water['rng_consumption_reconstructed'])
                    self.assertTrue(water['particle_randomness_included'])
                    self.assertEqual(water['particle_pool_capacity_recovered'], 1024)
                    self.assertEqual(water['rng_draws_after'] - water['rng_draws_before'],
                                     water['total_draws'])
                    self.assertEqual(layout['rng_draws_after_water'], water['rng_draws_after'])
                    entity_rng = layout['room_entity_rng']
                    placed_rng = entity_rng['placed_entity_rng']
                    self.assertEqual(len(layout['entity_locations']),
                                     len(placed_rng)
                                     + entity_rng['rng_free_placed_entity_count']
                                     + entity_rng['deferred_placed_entity_count']
                                     + entity_rng['unclassified_placed_entity_count'])
                    self.assertEqual(entity_rng['rng_free_placed_entity_count'],
                                     len(entity_rng['rng_free_placed_entities']))
                    self.assertEqual(entity_rng['deferred_placed_entity_count'],
                                     len(entity_rng['deferred_placed_entities']))
                    self.assertEqual(entity_rng['unclassified_placed_entity_count'],
                                     len(entity_rng['unclassified_placed_entities']))
                    self.assertEqual(entity_rng['unclassified_placed_entity_count'], 0)
                    self.assertEqual(entity_rng['placed_entity_draws'],
                                     sum(entity['draws'] for entity in placed_rng))
                    self.assertEqual(entity_rng['placed_entity_draws'],
                                     sum(entity_rng['placed_entity_draws_by_reason'].values()))
                    self.assertTrue(all(entity['draws'] == len(entity['reasons'])
                                        for entity in placed_rng))
                    self.assertEqual(entity_rng['known_item_constructor_draws'],
                                     entity_rng['unheld_book_pedestals'])
                    self.assertEqual(len(entity_rng['spellbook_rng']),
                                     entity_rng['unheld_book_pedestals'])
                    self.assertEqual(entity_rng['spellbook_rng_draws'],
                                     sum(book['draws'] for book in entity_rng['spellbook_rng']))
                    for book in entity_rng['spellbook_rng']:
                        self.assertGreaterEqual(book['draws'], 2)
                        self.assertGreaterEqual(len(book['skill_types']), 1)
                        self.assertLessEqual(len(book['skill_types']), 5)
                    self.assertTrue(entity_rng['known_draws_consumed'])
                    self.assertTrue(entity_rng['spellbook_skill_draws_included'])
                    self.assertTrue(entity_rng['full_room_entity_rng_reconstructed'])
                    self.assertEqual(report['generation_rng_complete_through'],
                                     'known_placed_entity_and_book_rng')
                    self.assertIn('candidate sets',
                                  report['endpoint_rng_incomplete_reason'])
                    self.assertIn('after addStartAndGoal',
                                  report['generation_rng_incomplete_reason'])
                    endpoint_rng = layout['endpoints']['rng_draw_interval']
                    self.assertEqual(endpoint_rng[1] - endpoint_rng[0],
                                     layout['endpoints']['rng_draw_count'])
                    self.assertEqual(
                        layout['provisional_enemy_rng_trace']['entry_draw'],
                        endpoint_rng[1])
                    self.assertFalse(layout['floor_complete'])
        self.assertEqual(len(sorted_goal_candidate_counts), 60)
        self.assertLess(max(sorted_goal_candidate_counts), 32)
        self.assertGreater(blocked_candidates_rejected, 0)
        self.assertEqual(thief_gate_counts[0], 0)
        self.assertGreater(thief_gate_counts[1], 0)
        self.assertGreater(thief_gate_counts[2], 0)


if __name__ == '__main__':
    unittest.main()
