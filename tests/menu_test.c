#include "menu.h"
#include <assert.h>
int main(void){
 Menu m={0};menu_input(&m,INPUT_UP);assert(m.cursor==0);
 menu_input(&m,0);menu_input(&m,INPUT_DOWN);assert(m.cursor==1);
 menu_input(&m,INPUT_DOWN);assert(m.cursor==1);
 menu_input(&m,0);menu_input(&m,INPUT_UP);menu_input(&m,0);
 menu_input(&m,INPUT_ACCEPT);assert(m.screen==MENU_FADE);
 for(int i=0;i<89;i++)menu_tick(&m);assert(m.screen==MENU_FADE);
 menu_tick(&m);assert(m.screen==MENU_CLASS);
 menu_input(&m,INPUT_ACCEPT);assert(m.screen==MENU_CLASS);
 menu_input(&m,0);menu_input(&m,INPUT_LEFT);assert(m.player_class==2);
 menu_input(&m,0);menu_input(&m,INPUT_RIGHT);assert(m.player_class==0);
 menu_input(&m,0);menu_input(&m,INPUT_ACCEPT);assert(m.screen==MENU_BLOCKED);
 menu_input(&m,0);menu_input(&m,INPUT_BACK);assert(m.screen==MENU_CLASS);
 menu_input(&m,0);menu_input(&m,INPUT_BACK);assert(m.screen==MENU_TITLE);
 return 0;
}
