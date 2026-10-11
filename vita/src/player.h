#pragma once
#include "world.h"
enum { PLAYER_LEFT=1,PLAYER_RIGHT=2,PLAYER_UP=4,PLAYER_DOWN=8,PLAYER_JUMP=16,PLAYER_SLOW=32 };
typedef enum { PLAYER_STAND,PLAYER_WALK,PLAYER_RISE,PLAYER_FALL,PLAYER_DUCK,PLAYER_CLIMB,PLAYER_LOOK } PlayerMotion;
typedef struct {
 float x,y,vx,vy,w,h;int player_class,facing,on_ground,on_ladder,ducking;
 int hp,max_hp,drop_ticks;unsigned previous,ticks,contacts;
 StartingStats stats;PlayerMotion motion;
} NativePlayer;
int player_spawn(NativePlayer *player,const World *world,int player_class);
void player_step(NativePlayer *player,const World *world,unsigned input);
int player_overlaps_solid(const NativePlayer *player,const World *world);
const char *player_motion_name(PlayerMotion motion);
