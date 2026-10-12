#pragma once
#include <stdint.h>
#include <stdio.h>
#define WORLD_WIDTH 40
#define WORLD_HEIGHT 24
#define WORLD_TILE_SIZE 16
#define WORLD_TILE_COUNT (WORLD_WIDTH*WORLD_HEIGHT)
enum { WORLD_LEFT=1, WORLD_UP=2, WORLD_RIGHT=4, WORLD_DOWN=8 };
typedef struct {int8_t type;int16_t layers[5];uint8_t solid,platform,faces;} WorldTile;
typedef struct {uint8_t kind,subtype;int16_t x,y;} WorldEntity;
#define WORLD_ENTITY_LIMIT 64
typedef struct {
 WorldEntity entities[WORLD_ENTITY_LIMIT];unsigned entity_count;
 WorldTile tiles[WORLD_TILE_COUNT];
 int width,height,open_sky,open_left,open_right,ready;
 int entrance_x,entrance_y,goal_x,goal_y;
} World;
typedef struct {int strength,dexterity,vitality,speed,stamina,intelligence,luck;} StartingStats;
int world_load(World *world,const char *path,FILE *log);
void world_collision_flags(World *world);
const WorldTile *world_tile(const World *world,int x,int y);
int world_solid(const World *world,int x,int y);
int starting_stats(int player_class,StartingStats *stats);

void world_auto_tiles(World *world);

int world_load_entities(World *world,const char *path,FILE *log);
