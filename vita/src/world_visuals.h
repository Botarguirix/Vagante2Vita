#pragma once
#include "player.h"
#include <stdint.h>
int load_world_visuals(const char *directory,int player_class,const World *world,FILE *log);
void free_world_visuals(void);
void draw_world(uint32_t *pixels,const World *world,const NativePlayer *player,int reduction);
