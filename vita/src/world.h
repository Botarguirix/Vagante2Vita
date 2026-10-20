#pragma once
#include <stdint.h>
#include <stdio.h>
#include "melee.h"
#include "bow.h"
#define WORLD_WIDTH 40
#define WORLD_HEIGHT 24
#define WORLD_TILE_SIZE 16
#define WORLD_TILE_COUNT (WORLD_WIDTH*WORLD_HEIGHT)
#define WORLD_MAX_WIDTH 128
#define WORLD_MAX_HEIGHT 128
#define WORLD_CAPACITY 4096
enum { WORLD_LEFT=1, WORLD_UP=2, WORLD_RIGHT=4, WORLD_DOWN=8 };
enum { WORLD_KIND_BOSS=26, WORLD_KIND_DRAGON_FIREBALL=27, WORLD_KIND_WORM_SPIT=28, WORLD_BOSS_GOBLIN_KING=49, WORLD_BOSS_DRAGON=50, WORLD_BOSS_WORM=51 };
#define WORLD_GOBLIN_KING_HP 70.0f
#define WORLD_GOBLIN_KING_DAMAGE 8u
#define WORLD_DRAGON_HP 65.0f
#define WORLD_DRAGON_DAMAGE 5u
#define WORLD_DRAGON_FIREBALL_DAMAGE 5u
#define WORLD_WORM_DAMAGE 3u
#define WORLD_WORM_HP 100.0f
#define WORLD_WORM_SPIT_DAMAGE 3u
#define WORLD_WORM_SPIT_SPEED 2.5f
#define WORLD_WORM_SPIT_TAUNT_TICKS 180u
#define WORLD_WORM_SEGMENTS 20u
#define WORLD_WORM_PATH_POINTS 48u
#define WORLD_WORM_PATH_SAMPLES 512u
enum { WORLD_DRAGON_ACTION_IDLE=0, WORLD_DRAGON_ACTION_FACE_LEFT=1,
 WORLD_DRAGON_ACTION_FACE_RIGHT=2, WORLD_DRAGON_ACTION_FACE_PLAYER=3,
 WORLD_DRAGON_ACTION_MOVE_PLAYER=4, WORLD_DRAGON_ACTION_PATH_PLAYER=5,
 WORLD_DRAGON_ACTION_SHOOT_PLAYER=6, WORLD_DRAGON_ACTION_SHOOT_PLAYER_ONCE=7 };
typedef struct {int8_t type;int16_t layers[5];uint8_t solid,platform,faces;} WorldTile;
typedef struct {uint8_t kind,subtype;int16_t x,y;uint8_t flags;uint16_t variant;int32_t x_q8,y_q8;uint32_t source_index;float vx,vy,hp;uint16_t cooldown,contact_cooldown;unsigned hit_attack_serial;int32_t projectile_x_q8,projectile_y_q8;int8_t projectile_direction;uint8_t projectile_active,fired_once;uint8_t boulder_plate_down,boulder_initialized,boulder_triggered,boulder_release_frames,boulder_release_ready,boulder_present,boulder_released,boulder_landed,boulder_player_hit;int32_t boulder_x_q8,boulder_y_q8;float boulder_vy;uint16_t item_type,item_sprite;uint8_t item_atlas,wall_tile_destroyed,boss_reward,boss_reward_spawned;uint32_t contact_rng_state;uint16_t goblin_prep_timer,goblin_attack_cooldown;uint8_t goblin_swing_frame,goblin_facing_right,goblin_facing_frames,goblin_hit;uint16_t thief_attack_cooldown;uint8_t thief_swing_frames,thief_facing_right,thief_hit;uint16_t boss_charge_timer,boss_charge_cooldown;uint8_t boss_charge_active,boss_facing_right,boss_charge_hit;uint8_t dragon_attack_active,dragon_attack_shots_remaining,dragon_action_cycle;uint16_t dragon_attack_timer;uint8_t dragon_action;uint16_t dragon_action_timer;uint8_t dragon_initialized,worm_initialized,worm_segment_count,worm_behavior,worm_animation_frame,worm_spit_shots_remaining;uint16_t worm_behavior_timer,worm_animation_timer;uint8_t goblin_attack_damage,spider_jumping,spider_jump_tell,spider_recovery;uint8_t spider_web_prep,spider_web_hit,spider_webbed;uint16_t spider_web_cooldown,spider_web_frames;int32_t spider_web_x_q8,spider_web_y_q8;float spider_web_vx,spider_web_vy;uint16_t lurker_windup,lurker_attack_frames,lurker_recovery;uint8_t lurker_hit;uint16_t bat_charge_prep,bat_charge_timer,bat_charge_cooldown;uint16_t slime_jump_timer,slime_lunge_timer,slime_lunge_prep,slime_lunge_frames,slime_lunge_cooldown;uint8_t slime_lunge_hit,slime_facing_left,slime_jump_initialized,slime_bounces;} WorldEntity;
#define WORLD_ENTITY_OPENED 1u
#define WORLD_ENTITY_FACING_LEFT 2u
#define WORLD_ENTITY_CAN_RELOAD 4u
#define WORLD_ENTITY_PICKED_UP 8u
#define WORLD_ENTITY_LIMIT 256
#define WORLD_LOOT_POOL_LIMIT 26
#define WORLD_LOOT_ROW_LIMIT 256
#define WORLD_LOOT_EDGE_LIMIT 64
typedef struct {uint8_t id;uint16_t first,count;} LootPoolRange;
typedef struct {uint32_t min_level,max_level;uint16_t weight,item_type;uint8_t flags,atlas;uint16_t sprite;} LootRarityRow;
typedef struct {uint8_t child;uint32_t weight_milli;} LootPoolEdge;
typedef struct {float x,y;} GoblinAttackPoint;
#define WORLD_GOBLIN_ATTACK_FRAMES 8
#define WORLD_GOBLIN_ATTACK_VERTICES 16
typedef struct {uint16_t duration;uint8_t enabled,vertex_count;float origin_x,origin_y,origin_degrees,damage_min,damage_max;GoblinAttackPoint vertices[WORLD_GOBLIN_ATTACK_VERTICES];} GoblinAttackFrame;
typedef struct {uint16_t windup,cooldown,damage_min,damage_max;uint8_t frame_count,ready;GoblinAttackFrame frames[WORLD_GOBLIN_ATTACK_FRAMES];} GoblinAttackData;
typedef struct {
 LootPoolRange direct[WORLD_LOOT_POOL_LIMIT];uint8_t direct_count;
 LootRarityRow rows[WORLD_LOOT_ROW_LIMIT];uint16_t row_count;
 LootPoolRange nested[WORLD_LOOT_POOL_LIMIT];uint8_t nested_count;
 LootPoolEdge edges[WORLD_LOOT_EDGE_LIMIT];uint16_t edge_count;
 uint16_t spawned_unique[32];uint8_t spawned_unique_count,ready,enabled;
 uint32_t rng_state;
} WorldLoot;
typedef struct World {
 BowDefinition bow;
 MeleeDatabase melee;
 WorldEntity entities[WORLD_ENTITY_LIMIT];unsigned entity_count;
 unsigned bosses_loaded,bosses_defeated;
 uint8_t bosses_defeated_mask;
 WorldLoot loot;int loot_level;GoblinAttackData goblin_attack;
 WorldTile tiles[WORLD_CAPACITY];
 int width,height,open_sky,open_left,open_right,ready;
 int entrance_x,entrance_y,goal_x,goal_y;
} World;
typedef struct {int strength,dexterity,vitality,speed,stamina,intelligence,luck;} StartingStats;
int world_load(World *world,const char *path,FILE *log);
void world_collision_flags(World *world);
const WorldTile *world_tile(const World *world,int x,int y);
int world_solid(const World *world,int x,int y);
int world_goal_touched(const World *world,float left,float top,float width,float height);
int world_line_of_sight(const World *world,float x0,float y0,float x1,float y1);
int starting_stats(int player_class,StartingStats *stats);

void world_auto_tiles(World *world);

int world_load_entities(World *world,const char *path,FILE *log);
int world_load_enemies(World *world,const char *path,FILE *log);
int world_load_loot(World *world,const char *path,FILE *log);
int world_load_goblin_attack(World *world,const char *path,FILE *log);
int world_spawn_chest_loot(World *world,WorldEntity *chest,int level,uint32_t seed,uint16_t *item_type);
int world_spawn_boss_reward_chest(World *world,WorldEntity *boss);
int world_load_loot_from_file(World *world,FILE *file,FILE *log);
void world_enemies_step(World *world,float player_x,float player_y,float player_w,float player_h,const MeleeState *attack);
void world_record_boss_defeat(World *world,WorldEntity *boss);
unsigned world_enemies_contact_damage(World *world,float player_x,float player_y,float player_w,float player_h);
int world_spider_web_step(World *world,float player_x,float player_y,float player_w,float player_h);
void world_enemy_body(uint8_t subtype,float *width,float *height);
void world_worm_path_sample(const World *world,const WorldEntity *worm,unsigned segment,float *x,float *y);
unsigned world_breakable_walls_step(World *world,float player_x,float player_y,float player_w,float player_h,const MeleeState *attack);
int world_dart_traps_step(World *world,float player_x,float player_y,float player_w,float player_h);
unsigned world_boulder_traps_step(World *world,float player_x,float player_y,float player_w,float player_h);
void world_boulders_step(World *world);

typedef struct {int x,y;} WorldCamera;
WorldCamera world_camera(const World *world,float center_x,float center_y);
