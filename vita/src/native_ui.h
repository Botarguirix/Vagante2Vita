#pragma once
#include "player.h"
/* 960x544 framebuffer, 1024 pixels per row. Shared Vita and host rendering. */
void native_text(uint32_t *frame,int x,int y,const char *s,uint32_t color);
void draw_character_panel(uint32_t *frame,const NativePlayer *player,int page);
