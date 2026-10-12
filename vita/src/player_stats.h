#pragma once
#include "world.h"
typedef struct {
 int max_hp;float move_speed,melee_base,ranged_base,attack_speed_bonus;
 float damage_taken_multiplier,evasion,critical,max_air;
} DerivedStats;
typedef enum { START_MELEE,START_RANGED,START_AMMO } StartingItemRole;
typedef struct {int type,quantity;StartingItemRole role;} StartingItem;
typedef struct {StartingItem items[3];unsigned item_count;int skills[2];unsigned skill_count;} StartingLoadout;
void player_derive_stats(const StartingStats *base,DerivedStats *derived);
int player_starting_loadout(int player_class,StartingLoadout *loadout);
const char *starting_item_name(int type);
const char *starting_skill_name(int type);
