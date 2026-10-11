#include "menu.h"
const char *menu_screen_name(MenuScreen screen){
 const char *names[]={"TITLE","FADE_START","CLASS_PREVIEW","WORLD_BLOCKED","NOTICE","WORLD"};
 return screen>=MENU_TITLE&&screen<=MENU_WORLD?names[screen]:"INVALID";
}
void menu_tick(Menu *m){
 m->total_ticks++;
 if(m->screen==MENU_FADE&&++m->ticks>=90){m->screen=MENU_CLASS;m->ticks=0;}
}
void menu_input(Menu *m,unsigned buttons){
 unsigned pressed=buttons&~m->previous;m->previous=buttons;
 if(m->screen==MENU_TITLE){
  if((pressed&INPUT_UP)&&m->cursor>0)m->cursor--;
  if((pressed&INPUT_DOWN)&&m->cursor<3)m->cursor++;
  if((pressed&INPUT_ACCEPT)&&m->cursor<3){
   m->screen=m->cursor==0?MENU_FADE:MENU_NOTICE;m->notice=m->cursor;m->ticks=0;
  }
 }else if(m->screen==MENU_CLASS){
  if(pressed&INPUT_LEFT)m->player_class=(m->player_class+2)%3;
  if(pressed&INPUT_RIGHT)m->player_class=(m->player_class+1)%3;
  if(pressed&INPUT_ACCEPT)m->screen=MENU_BLOCKED;
  if(pressed&INPUT_BACK)m->screen=MENU_TITLE;
 }else if(m->screen==MENU_BLOCKED){
  if(pressed&INPUT_BACK)m->screen=MENU_CLASS;
 }else if(m->screen==MENU_NOTICE){
  if(pressed&(INPUT_BACK|INPUT_ACCEPT))m->screen=MENU_TITLE;
 }
}
