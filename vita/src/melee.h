#pragma once
#include <stdint.h>
#include <stdio.h>
#define MELEE_WEAPONS 16
#define MELEE_FRAMES 16
#define MELEE_SHAPES 4
#define MELEE_VERTICES 16
typedef struct {float x,y;} MeleePoint;
typedef struct {unsigned count,enabled;float damage_min,damage_max,knockback;MeleePoint vertices[MELEE_VERTICES];} MeleeShape;
typedef struct {unsigned sprite,enabled,charge;float x,y,degrees,duration_factor,scale_x,scale_y;} MeleeFrame;
typedef struct {unsigned type,sprite,duration,flags,frame_count,shape_count;float anchor_x,anchor_y;MeleeFrame frames[MELEE_FRAMES];MeleeShape shapes[MELEE_SHAPES];} MeleeDefinition;
typedef struct {MeleeDefinition weapons[MELEE_WEAPONS];unsigned count,ready;} MeleeDatabase;
typedef struct {const MeleeDefinition *definition;unsigned active,keyframe,timer,elapsed,charge_ticks,serial;float attack_speed;int facing;} MeleeState;
int melee_load(MeleeDatabase *database,const char *path,FILE *log);
const MeleeDefinition *melee_find(const MeleeDatabase *database,unsigned type);
int melee_start(MeleeState *state,const MeleeDefinition *definition,float attack_speed,int facing);
void melee_step(MeleeState *state,int held);
const MeleeFrame *melee_frame(const MeleeState *state);
unsigned melee_polygon(const MeleeState *state,unsigned shape,float cx,float bottom,MeleePoint *points);
int melee_hits_rect(const MeleeState *state,float cx,float bottom,float x,float y,float width,float height);
float melee_damage_rect(const MeleeState *state,float cx,float bottom,float x,float y,float width,float height);
