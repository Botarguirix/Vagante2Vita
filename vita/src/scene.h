#pragma once
#include <stdint.h>
#include "menu.h"
void draw_scene(uint32_t *pixels);
void draw_animated_scene(uint32_t *pixels,const Menu *menu);

void draw_scene_resolution(uint32_t *pixels,const Menu *menu,int reduction);

void draw_scene_profile(uint32_t *pixels,const Menu *menu,int reduction,int lightweight);
void scene_invalidate_cache(void);
unsigned scene_cache_rebuilds(void);
