#include "player.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static World flat(void){World w;memset(&w,0,sizeof(w));w.width=40;w.height=24;w.ready=1;w.entrance_x=14;w.entrance_y=20;
 for(int x=0;x<40;x++)w.tiles[21*40+x].type=1;
 world_collision_flags(&w);return w;}
int main(int argc,char **argv){
 World w=flat();NativePlayer p;assert(player_spawn(&p,&w,0));
 for(int i=0;i<5;i++)player_step(&p,&w,0);assert(p.on_ground&&p.hp==100);
 float x=p.x;for(int i=0;i<60;i++){player_step(&p,&w,PLAYER_RIGHT);assert(!player_overlaps_solid(&p,&w));}assert(p.x>x+70);
 float y=p.y;player_step(&p,&w,PLAYER_JUMP);assert(p.vy<0);
 float top=p.y;for(int i=0;i<120;i++){player_step(&p,&w,PLAYER_JUMP);if(p.y<top)top=p.y;}assert(top<y-25&&p.on_ground);
 for(int i=0;i<20;i++)player_step(&p,&w,PLAYER_JUMP);assert(p.on_ground);
 player_step(&p,&w,PLAYER_DOWN);assert(p.ducking&&p.h==6);player_step(&p,&w,0);assert(!p.ducking&&p.h==12);

 w=flat();assert(player_spawn(&p,&w,1));
 for(int ywall=18;ywall<21;ywall++)w.tiles[ywall*40+20].type=1;world_collision_flags(&w);
 for(int i=0;i<200;i++)player_step(&p,&w,PLAYER_RIGHT);assert(p.x+p.w<=320.001f&&!player_overlaps_solid(&p,&w));
 w=flat();for(int tx=0;tx<40;tx++){w.tiles[21*40+tx].type=6;w.tiles[23*40+tx].type=1;}world_collision_flags(&w);
 assert(player_spawn(&p,&w,2));for(int i=0;i<5;i++)player_step(&p,&w,0);assert(p.on_ground&&p.hp==60);
 player_step(&p,&w,PLAYER_DOWN|PLAYER_JUMP);for(int i=0;i<60;i++)player_step(&p,&w,0);assert(p.on_ground&&p.y>340);
 w=flat();w.tiles[19*40+14].type=3;w.tiles[20*40+14].type=3;world_collision_flags(&w);
 assert(player_spawn(&p,&w,0));for(int i=0;i<5;i++)player_step(&p,&w,0);y=p.y;
 for(int i=0;i<10;i++)player_step(&p,&w,PLAYER_UP);assert(p.on_ladder&&p.y<y-10);
 player_step(&p,&w,PLAYER_JUMP);assert(!p.on_ladder&&p.vy<0);
 if(argc==2){assert(world_load(&w,argv[1],stdout));assert(player_spawn(&p,&w,0));
  for(int i=0;i<300;i++){unsigned input=i<130?PLAYER_RIGHT:PLAYER_LEFT;if(i==50||i==180)input|=PLAYER_JUMP;player_step(&p,&w,input);assert(isfinite(p.x)&&isfinite(p.y)&&!player_overlaps_solid(&p,&w));}
 }
 return 0;
}
