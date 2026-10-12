/* Initial-room movement subset. Combat/skills and PC parity remain pending. */
#include "player.h"
#include <math.h>
#include <string.h>
static int cell(float v){return (int)floorf(v/16.0f);}
int player_overlaps_solid(const NativePlayer *p,const World *w){
 for(int y=cell(p->y);y<=cell(p->y+p->h-0.001f);y++)
  for(int x=cell(p->x);x<=cell(p->x+p->w-0.001f);x++)if(world_solid(w,x,y))return 1;
 return 0;
}
static int ladder(const NativePlayer *p,const World *w){
 int x=cell(p->x+p->w*0.5f);
 for(int y=cell(p->y);y<=cell(p->y+p->h-0.001f);y++){
  const WorldTile *t=world_tile(w,x,y);if(t&&t->type==3)return 1;
 }
 return 0;
}
static int platform_crossed(const NativePlayer *p,const World *w,float old_bottom){
 if(p->drop_ticks||p->on_ladder||p->vy<0)return 0;
 float bottom=p->y+p->h;
 for(int y=cell(old_bottom);y<=cell(bottom);y++)for(int x=cell(p->x);x<=cell(p->x+p->w-0.001f);x++){
  const WorldTile *t=world_tile(w,x,y);float top=(float)y*16;
  if(t&&t->platform&&old_bottom<=top+0.001f&&bottom>top+0.001f)return 1;
 }
 return 0;
}
static void axis_move(NativePlayer *p,const World *w,float distance,int vertical){
 int parts=(int)ceilf(fabsf(distance)/0.25f);if(parts<1)return;
 float step=distance/(float)parts;
 for(int i=0;i<parts;i++){
  float before=vertical?p->y:p->x,old_bottom=p->y+p->h;
  if(vertical)p->y+=step;else p->x+=step;
  if(player_overlaps_solid(p,w)||(vertical&&step>0&&platform_crossed(p,w,old_bottom))){
   if(vertical){
    p->y=before;p->vy=0;
    if(step>0){
     /* Snap feet to the crossed top instead of leaving a fractional gap.
        Validate the candidate because walls/ceilings can also overlap. */
     float snapped=floorf((old_bottom+step-0.001f)/16.0f)*16.0f-p->h;
     if(snapped>=before-0.251f&&snapped<=before+step+0.001f){
      p->y=snapped;if(player_overlaps_solid(p,w))p->y=before;
     }
     p->on_ground=1;p->contacts|=WORLD_DOWN;
    }else p->contacts|=WORLD_UP;
   }
   else {p->x=before;p->vx=0;p->contacts|=step>0?WORLD_RIGHT:WORLD_LEFT;}
   return;
  }
 }
}
int player_spawn(NativePlayer *p,const World *w,int player_class){
 NativePlayer next;memset(&next,0,sizeof(next));
 if(!w->ready||!starting_stats(player_class,&next.stats))return 0;
 next.player_class=player_class;next.facing=1;next.w=8;next.h=12;
 next.x=(float)(w->entrance_x*16)+4;next.y=(float)(w->entrance_y*16)+16-next.h;
 player_derive_stats(&next.stats,&next.derived);
 if(!player_starting_loadout(player_class,&next.loadout))return 0;
 next.melee.definition=melee_find(&w->melee,(unsigned)next.loadout.items[0].type);
 next.max_hp=next.derived.max_hp;next.hp=next.max_hp;
 if(player_overlaps_solid(&next,w))return 0;
 *p=next;return 1;
}
/* Geometry adapter for the observed original hanging state. Pixel thresholds
   and mantle timing still require comparison with the PC runtime. */
static void try_ledge(NativePlayer *p,const World *w,int dir){
 if(!dir||p->on_ground||p->on_ladder||p->ducking||p->hang_cooldown||p->vy<0)return;
 float edge=dir>0?p->x+p->w:p->x;
 int tx=cell(edge+(dir>0?0.5f:-0.5f)),ty=cell(p->y+4);
 const WorldTile *t=world_tile(w,tx,ty);
 if(!t||!t->solid||world_solid(w,tx,ty-1))return;
 float top=ty*16.0f,boundary=(dir>0?tx:tx+1)*16.0f;
 if(fabsf(edge-boundary)>0.75f||fabsf(p->y-top)>4)return;
 NativePlayer next=*p;next.x=dir>0?boundary-p->w:boundary;next.y=top;
 if(player_overlaps_solid(&next,w))return;
 next.vx=next.vy=0;next.hanging=1;next.hang_x=tx;next.hang_y=ty;
 next.facing=dir;next.motion=PLAYER_HANG;*p=next;
}
static int mantle(NativePlayer *p,const World *w){
 NativePlayer next=*p;
 float target_y=p->hang_y*16.0f-p->h;
 float target_x=p->facing>0?p->hang_x*16.0f:(p->hang_x+1)*16.0f-p->w;
 axis_move(&next,w,target_y-next.y,1);
 if(fabsf(next.y-target_y)>0.001f)return 0;
 axis_move(&next,w,target_x-next.x,0);
 if(fabsf(next.x-target_x)>0.001f||player_overlaps_solid(&next,w))return 0;
 next.hanging=0;next.on_ground=1;next.vx=next.vy=0;next.motion=PLAYER_STAND;
 next.hang_cooldown=12;*p=next;return 1;
}
void player_step(NativePlayer *p,const World *w,unsigned input){
 unsigned pressed=input&~p->previous,released=p->previous&~input;p->previous=input;p->ticks++;p->contacts=0;
 if(p->drop_ticks)p->drop_ticks--;
 if(p->hang_cooldown)p->hang_cooldown--;
 if(p->hanging){
  if(!world_solid(w,p->hang_x,p->hang_y)){p->hanging=0;p->hang_cooldown=12;}
  else if(pressed&PLAYER_JUMP){p->hanging=0;p->hang_cooldown=12;p->vy=-3.5f;p->vx=-p->facing*1.5f;}
  else if(input&PLAYER_DOWN){p->hanging=0;p->hang_cooldown=12;}
  else {if(input&PLAYER_UP)mantle(p,w);return;}
 }
 if((pressed&PLAYER_ATTACK)&&!p->hanging&&!p->on_ladder&&!p->ducking)melee_start(&p->melee,p->melee.definition,p->derived.attack_speed_bonus,p->facing);
 melee_step(&p->melee,!!(input&PLAYER_ATTACK));
 int dir=!!(input&PLAYER_RIGHT)-!!(input&PLAYER_LEFT);
 int touching_ladder=ladder(p,w);
 if((input&(PLAYER_UP|PLAYER_DOWN))&&touching_ladder&&!p->ducking)p->on_ladder=1;
 if(!touching_ladder)p->on_ladder=0;
 if((pressed&PLAYER_JUMP)&&(input&PLAYER_DOWN)&&p->on_ground){p->drop_ticks=12;p->on_ground=0;}
 else if((pressed&PLAYER_JUMP)&&(p->on_ground||p->on_ladder)){p->vy=-3.5f;p->on_ground=0;p->on_ladder=0;}
 if((released&PLAYER_JUMP)&&p->vy<0)p->vy*=0.6666667f;
 if((input&PLAYER_DOWN)&&p->on_ground&&!p->on_ladder&&!p->ducking){p->y+=6;p->h=6;p->ducking=1;}
 if(!(input&PLAYER_DOWN)&&p->ducking){
  p->y-=6;p->h=12;
  if(player_overlaps_solid(p,w)){p->y+=6;p->h=6;}else p->ducking=0;
 }
 float limit=p->derived.move_speed;
 if(p->ducking||(input&PLAYER_SLOW))limit*=0.5f;
 if(dir){p->facing=dir;p->vx+=(float)dir*(p->on_ground?0.3f:0.2f);}
 else p->vx*=0.8f;
 if(fabsf(p->vx)<0.01f)p->vx=0;
 if(p->vx>limit)p->vx=limit;
 if(p->vx< -limit)p->vx=-limit;
 if(p->on_ladder)p->vy=(float)(!!(input&PLAYER_DOWN)-!!(input&PLAYER_UP))*1.4f;
 else {p->vy+=0.16f;if(p->vy>8)p->vy=8;}
 p->on_ground=0;axis_move(p,w,p->vx,0);axis_move(p,w,p->vy,1);
 if(!p->melee.active)try_ledge(p,w,dir);
 p->melee.facing=p->facing;
 if(p->hanging)p->motion=PLAYER_HANG;
 else if(p->on_ladder)p->motion=PLAYER_CLIMB;
 else if(p->ducking)p->motion=PLAYER_DUCK;
 else if(!p->on_ground)p->motion=p->vy<0?PLAYER_RISE:PLAYER_FALL;
 else if(dir)p->motion=PLAYER_WALK;
 else p->motion=(input&PLAYER_UP)?PLAYER_LOOK:PLAYER_STAND;
}
const char *player_motion_name(PlayerMotion m){
 const char *names[]={"STAND","WALK","RISE","FALL","DUCK","CLIMB","LOOK","HANG"};
 return m>=PLAYER_STAND&&m<=PLAYER_HANG?names[m]:"INVALID";
}
