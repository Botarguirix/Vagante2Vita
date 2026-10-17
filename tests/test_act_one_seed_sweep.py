import json
import unittest
from pathlib import Path

from tools.floor_layout import floor_layout
from tools.export_act_one_world import encode_enemies


class ActOneSeedSweepTests(unittest.TestCase):
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

    def test_twenty_seeds_across_act_one_floors_fit_runtime_and_have_tile_routes(self):
        database_path = Path(__file__).parents[1] / '_local/assets/rooms.json'
        if not database_path.is_file():
            self.skipTest('Private local Vagante room database is not available')
        database = json.loads(database_path.read_text('utf-8'))
        blocked_candidates_rejected = 0
        for floor_index in range(3):
            for seed in range(20):
                with self.subTest(floor=floor_index, seed=seed):
                    report = floor_layout(seed, floor_index, database)
                    layout = report['tile_layout']
                    endpoint = layout['endpoints']
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
                    goal_candidates = layout['goal_candidates_after_terrain']['candidates']
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
                    self.assertFalse(entity_rng['full_room_entity_rng_reconstructed'])
                    self.assertEqual(report['generation_rng_complete_through'],
                                     'known_placed_entity_and_book_rng')
                    self.assertIn('other room-entity',
                                  report['generation_rng_incomplete_reason'])
                    self.assertFalse(layout['floor_complete'])
        self.assertGreater(blocked_candidates_rejected, 0)


if __name__ == '__main__':
    unittest.main()
