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
 next.max_hp=50+next.stats.vitality*10;next.hp=next.max_hp;
 if(player_overlaps_solid(&next,w))return 0;
 *p=next;return 1;
}
void player_step(NativePlayer *p,const World *w,unsigned input){
 unsigned pressed=input&~p->previous,released=p->previous&~input;p->previous=input;p->ticks++;p->contacts=0;
 if(p->drop_ticks)p->drop_ticks--;
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
 float limit=1.4f+(float)p->stats.speed*0.1f;
 if(p->ducking||(input&PLAYER_SLOW))limit*=0.5f;
 if(dir){p->facing=dir;p->vx+=(float)dir*(p->on_ground?0.3f:0.2f);}
 else p->vx*=0.8f;
 if(fabsf(p->vx)<0.01f)p->vx=0;
 if(p->vx>limit)p->vx=limit;
 if(p->vx< -limit)p->vx=-limit;
 if(p->on_ladder)p->vy=(float)(!!(input&PLAYER_DOWN)-!!(input&PLAYER_UP))*1.4f;
 else {p->vy+=0.16f;if(p->vy>8)p->vy=8;}
 p->on_ground=0;axis_move(p,w,p->vx,0);axis_move(p,w,p->vy,1);
 if(p->on_ladder)p->motion=PLAYER_CLIMB;
 else if(p->ducking)p->motion=PLAYER_DUCK;
 else if(!p->on_ground)p->motion=p->vy<0?PLAYER_RISE:PLAYER_FALL;
 else if(dir)p->motion=PLAYER_WALK;
 else p->motion=(input&PLAYER_UP)?PLAYER_LOOK:PLAYER_STAND;
}
const char *player_motion_name(PlayerMotion m){
 const char *names[]={"STAND","WALK","RISE","FALL","DUCK","CLIMB","LOOK"};
 return m>=PLAYER_STAND&&m<=PLAYER_LOOK?names[m]:"INVALID";
}
