#pragma once
#include <stdint.h>
enum { INPUT_UP=1,INPUT_DOWN=2,INPUT_LEFT=4,INPUT_RIGHT=8,INPUT_ACCEPT=16,INPUT_BACK=32 };
typedef enum { MENU_TITLE,MENU_FADE,MENU_CLASS,MENU_BLOCKED,MENU_NOTICE } MenuScreen;
typedef struct { MenuScreen screen; int cursor,player_class,notice,ticks; uint64_t total_ticks; unsigned previous; } Menu;
void menu_input(Menu *menu,unsigned buttons);
void menu_tick(Menu *menu);
const char *menu_screen_name(MenuScreen screen);
