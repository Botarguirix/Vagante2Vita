#pragma once
#include <stdio.h>
#define BOW_FRAMES 8
#define ARROW_LIMIT 32
struct World;
typedef struct {unsigned sprite;float duration_factor,x,y,degrees,aim_x,aim_y,projectile_x,projectile_y,scale_x,scale_y;} BowFrame;
typedef struct {unsigned ready,count,draw_frames,sprite,duration;float anchor_x,anchor_y,arm_x,arm_y,velocity,damage_min,damage_max;BowFrame frames[BOW_FRAMES];} BowDefinition;
typedef struct {const BowDefinition *definition;unsigned active,keyframe,timer,serial;float attack_speed,angle;int facing;} BowState;
typedef struct {unsigned active,stuck,life,serial;float x,y,vx,vy,angle,damage_factor;int facing;} NativeArrow;
typedef struct {NativeArrow arrows[ARROW_LIMIT];unsigned fired,impacts,expired;} ArrowPool;
int bow_load(BowDefinition *definition,const char *path,FILE *log);
int bow_start(BowState *state,const BowDefinition *definition,float attack_speed,int facing,int ammo);
/* Returns one exactly on release; the last shot's geometry is stored by caller. */
int bow_step(BowState *state,int held);
float bow_projectile_speed(const BowState *state);
void bow_projectile_origin(const BowState *state,float cx,float bottom,float *x,float *y);
int arrow_launch(ArrowPool *pool,const struct World *world,float cx,float cy,float x,float y,float vx,float vy,float damage_factor);
void arrows_step(ArrowPool *pool,const struct World *world);
