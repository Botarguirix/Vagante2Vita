#pragma once
#include "world.h"
#include "player_stats.h"
enum { PLAYER_LEFT=1,PLAYER_RIGHT=2,PLAYER_UP=4,PLAYER_DOWN=8,PLAYER_JUMP=16,PLAYER_SLOW=32,PLAYER_ATTACK=64,PLAYER_RANGE=128 };
typedef enum { PLAYER_STAND,PLAYER_WALK,PLAYER_RISE,PLAYER_FALL,PLAYER_DUCK,PLAYER_CLIMB,PLAYER_LOOK,PLAYER_HANG } PlayerMotion;
typedef enum { PLAYER_DEATH_NONE,PLAYER_DEATH_SPIKES,PLAYER_DEATH_DART_TRAP,PLAYER_DEATH_ENEMY,PLAYER_DEATH_BOULDER } PlayerDeathCause;
typedef struct {
 float x,y,vx,vy,w,h;int player_class,facing,on_ground,on_ladder,ducking,webbed,hurt_flash;
 unsigned chests_opened,items_collected;
 int hp,max_hp,dead,death_cause,drop_ticks,hanging,hang_x,hang_y,hang_cooldown;unsigned previous,ticks,contacts;
 StartingStats stats;DerivedStats derived;StartingLoadout loadout;MeleeState melee;BowState bow;ArrowPool arrows;PlayerMotion motion;
} NativePlayer;
int player_spawn(NativePlayer *player,const World *world,int player_class);
int player_carry_campaign_state(NativePlayer *next,const NativePlayer *previous,const World *world);
void player_step(NativePlayer *player,World *world,unsigned input);
unsigned player_interact_chests(NativePlayer *player,World *world);
int player_overlaps_solid(const NativePlayer *player,const World *world);
int player_boulder_contact(NativePlayer *player,World *world);
const char *player_motion_name(PlayerMotion motion);
const char *player_death_cause_name(PlayerDeathCause cause);
