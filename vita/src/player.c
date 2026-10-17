/* Initial-room movement subset. Combat/skills and PC parity remain pending. */
#include "player.h"
#include <math.h>
#include <string.h>
static int cell(float v){return (int)floorf(v/16.0f);}
unsigned player_interact_chests(NativePlayer *p,World *w){
 if(!p||!w)return 0;
 unsigned opened=0;
 float left=p->x,top=p->y;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=16||(e->flags&WORLD_ENTITY_OPENED))continue;
  /* Chest::Chest constructs a 12x9 body; Player::moveUp opens the touched chest. */
  float chest_x=(float)e->x_q8/256.0f,chest_y=(float)e->y_q8/256.0f;
  if(left<chest_x+12.0f&&left+p->w>chest_x&&top<chest_y+9.0f&&top+p->h>chest_y){
   e->flags|=WORLD_ENTITY_OPENED;p->chests_opened++;opened++;
   if(w->loot.enabled){uint32_t seed=(uint32_t)e->source_index*0x9e3779b9u^(uint32_t)e->x_q8^(uint32_t)e->y_q8^(uint32_t)p->chests_opened;uint16_t type;
    (void)world_spawn_chest_loot(w,e,w->loot_level,seed,&type);
   }
   break;
  }
 }
 if(!opened){
  for(unsigned i=0;i<w->entity_count;i++){
   WorldEntity *e=&w->entities[i];if(e->kind!=25||(e->flags&WORLD_ENTITY_PICKED_UP))continue;
   float x=(float)e->x_q8/256.0f,y=(float)e->y_q8/256.0f;
   if(left<x+16.0f&&left+p->w>x&&top<y+16.0f&&top+p->h>y){
    if(p->loadout.item_count<sizeof(p->loadout.items)/sizeof(p->loadout.items[0])){
     p->loadout.items[p->loadout.item_count++]=(StartingItem){e->item_type,1,START_MISC};
     e->flags|=WORLD_ENTITY_PICKED_UP;p->items_collected++;
    }
    break;
   }
  }
 }
 return opened;
}
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
/* Ghidra: Spikes::doCollision requires downward speed > 0.4 and sends
   DamageEventMeta(999, DAMAGE_PURE, true). Its constructor only activates
   over tile type 11. Other actor guards (water/resurrection/immunity) are not
   modeled by this runtime yet. */
static int hit_active_spikes(const NativePlayer *p,World *w,float fall_speed){
 if(fall_speed<=0.4f)return 0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=17)continue;
  float x=(float)e->x_q8/256.0f,y=(float)e->y_q8/256.0f;
  const WorldTile *floor=world_tile(w,cell(x),cell(y));
  if(!floor||floor->type!=11)continue;
  if(p->x<x+16.0f&&p->x+p->w>x&&p->y<y+16.0f&&p->y+p->h>y+8.0f)return 1;
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
   else {
    p->x=before;
    int tile_x=step>0?cell(p->x+p->w):cell(p->x-0.001f);
    float safe_x=step>0?tile_x*16.0f-p->w-0.001f:(tile_x+1)*16.0f+0.001f;
    p->x=safe_x;
    if(player_overlaps_solid(p,w))p->x=before;
    p->vx=0;p->contacts|=step>0?WORLD_RIGHT:WORLD_LEFT;
   }
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
 if(player_class==1&&w->bow.ready)next.bow.definition=&w->bow;
 next.max_hp=next.derived.max_hp;next.hp=next.max_hp;
 if(player_overlaps_solid(&next,w))return 0;
 *p=next;return 1;
}
int player_carry_campaign_state(NativePlayer *next,const NativePlayer *previous,const World *w){
 if(!next||!previous||!w||next->player_class!=previous->player_class||previous->max_hp<1||
    previous->loadout.item_count>16||previous->loadout.skill_count>2)return 0;
 next->stats=previous->stats;next->derived=previous->derived;next->loadout=previous->loadout;
 next->max_hp=previous->max_hp;next->hp=previous->hp<0?0:previous->hp>next->max_hp?next->max_hp:previous->hp;
 next->chests_opened=previous->chests_opened;
 int melee_type=-1;
 for(unsigned i=0;i<next->loadout.item_count;i++)if(next->loadout.items[i].role==START_MELEE){melee_type=next->loadout.items[i].type;break;}
 next->melee.definition=melee_type>=0?melee_find(&w->melee,(unsigned)melee_type):NULL;
 next->bow.definition=next->player_class==1&&w->bow.ready?&w->bow:NULL;
 return 1;
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
void player_step(NativePlayer *p,World *w,unsigned input){
 if(!p||!w||p->dead)return;
 arrows_step(&p->arrows,w);
 unsigned pressed=input&~p->previous,released=p->previous&~input;p->previous=input;p->ticks++;p->contacts=0;
 if(p->drop_ticks)p->drop_ticks--;
 if(p->hang_cooldown)p->hang_cooldown--;
 if(p->hanging){
  if(!world_solid(w,p->hang_x,p->hang_y)){p->hanging=0;p->hang_cooldown=12;}
  else if(pressed&PLAYER_JUMP){p->hanging=0;p->hang_cooldown=12;p->vy=-3.5f;p->vx=-p->facing*1.5f;}
  else if(input&PLAYER_DOWN){p->hanging=0;p->hang_cooldown=12;}
  else {if(input&PLAYER_UP)mantle(p,w);return;}
 }
 int dir=!!(input&PLAYER_RIGHT)-!!(input&PLAYER_LEFT);if(dir)p->facing=dir;
 if((pressed&PLAYER_ATTACK)&&!p->bow.active&&!p->hanging&&!p->on_ladder&&!p->ducking)melee_start(&p->melee,p->melee.definition,p->derived.attack_speed_bonus,p->facing);
 if(pressed&PLAYER_UP)player_interact_chests(p,w);
 melee_step(&p->melee,!!(input&PLAYER_ATTACK));
 world_enemies_step(w,p->x,p->y,p->w,p->h,&p->melee);
 world_breakable_walls_step(w,p->x,p->y,p->w,p->h,&p->melee);
 StartingItem *ammo=NULL;
 for(unsigned i=0;i<p->loadout.item_count;i++)if(p->loadout.items[i].type==3)ammo=&p->loadout.items[i];
 if((pressed&PLAYER_RANGE)&&!p->melee.active&&!p->hanging&&!p->on_ladder&&!p->ducking)
  bow_start(&p->bow,p->bow.definition,p->derived.attack_speed_bonus,p->facing,ammo?ammo->quantity:0);
 BowState shot=p->bow;int fire=0;
 if(p->bow.active){
  p->bow.facing=p->facing;p->bow.angle=((input&PLAYER_UP)?-0.785398163f:(input&PLAYER_DOWN)?0.785398163f:0);
  shot=p->bow;fire=bow_step(&p->bow,!!(input&PLAYER_RANGE));
 }
 int touching_ladder=ladder(p,w);
 if(!p->bow.active&&(input&(PLAYER_UP|PLAYER_DOWN))&&touching_ladder&&!p->ducking)p->on_ladder=1;
 if(!touching_ladder)p->on_ladder=0;
 if((pressed&PLAYER_JUMP)&&(input&PLAYER_DOWN)&&p->on_ground){p->drop_ticks=12;p->on_ground=0;}
 else if((pressed&PLAYER_JUMP)&&(p->on_ground||p->on_ladder)){p->vy=-3.5f;p->on_ground=0;p->on_ladder=0;}
 if((released&PLAYER_JUMP)&&p->vy<0)p->vy*=0.6666667f;
 if(!p->bow.active&&(input&PLAYER_DOWN)&&p->on_ground&&!p->on_ladder&&!p->ducking){p->y+=6;p->h=6;p->ducking=1;}
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
 float fall_speed=p->vy;p->on_ground=0;axis_move(p,w,p->vx,0);axis_move(p,w,p->vy,1);
 if(hit_active_spikes(p,w,fall_speed)){
  /* The observed pure-damage payload is far above every base-class HP value. */
  p->hp=0;p->dead=1;p->death_cause=PLAYER_DEATH_SPIKES;p->motion=PLAYER_FALL;return;
 }
 if(world_dart_traps_step(w,p->x,p->y,p->w,p->h)){
  int damage=(int)ceilf(5.0f*p->derived.damage_taken_multiplier);
  if(damage<1)damage=1;
  p->hp-=damage;if(p->hp<0)p->hp=0;
  if(!p->hp){p->dead=1;p->death_cause=PLAYER_DEATH_DART_TRAP;p->motion=PLAYER_FALL;return;}
 }
 unsigned enemy_damage=world_enemies_contact_damage(w,p->x,p->y,p->w,p->h);
 if(enemy_damage){
  int damage=(int)ceilf((float)enemy_damage*p->derived.damage_taken_multiplier);
  if(damage<1)damage=1;
  p->hp-=damage;if(p->hp<0)p->hp=0;
  if(!p->hp){p->dead=1;p->death_cause=PLAYER_DEATH_ENEMY;p->motion=PLAYER_FALL;return;}
 }
 if(fire&&ammo&&ammo->quantity>0){
  float x,y;bow_projectile_origin(&shot,p->x+p->w*0.5f,p->y+p->h,&x,&y);
  float speed=bow_projectile_speed(&shot),flip=shot.facing<0?-1.0f:1.0f;
  float charge=shot.keyframe?fminf(1,(float)(shot.keyframe+1)/shot.definition->draw_frames):0;
  float damage=shot.definition->damage_min+(shot.definition->damage_max-shot.definition->damage_min)*charge;
  if(arrow_launch(&p->arrows,w,p->x+p->w*0.5f,p->y+p->h-6,x,y,flip*cosf(shot.angle)*speed,sinf(shot.angle)*speed,damage))ammo->quantity--;
 }
 if(!p->melee.active&&!p->bow.active)try_ledge(p,w,dir);
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
 return m<=PLAYER_HANG?names[m]:"INVALID";
}
int player_boulder_contact(NativePlayer *p,World *w){
 if(!p||!w||p->dead)return 0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];
  if((e->kind!=20&&e->kind!=22)||!e->boulder_present||!e->boulder_released||e->boulder_player_hit||e->boulder_vy<=.32f)continue;
  float x=e->boulder_x_q8/256.0f,y=e->boulder_y_q8/256.0f;
  if(p->x>=x+15.0f||p->x+p->w<=x||p->y>=y+16.0f||p->y+p->h<=y)continue;
  /* Boulder::doCollision assigns 20 damage to an ordinary square Boulder. */
  e->boulder_player_hit=1;
  int damage=(int)ceilf(20.0f*p->derived.damage_taken_multiplier);
  if(damage<1)damage=1;
  if(damage>30)damage=30;
  p->hp-=damage;if(p->hp<0)p->hp=0;
  if(!p->hp){p->dead=1;p->death_cause=PLAYER_DEATH_BOULDER;p->motion=PLAYER_FALL;}
  return 1;
 }
 return 0;
}

const char *player_death_cause_name(PlayerDeathCause cause){
 switch(cause){case PLAYER_DEATH_SPIKES:return "spikes";case PLAYER_DEATH_DART_TRAP:return "dart_trap";case PLAYER_DEATH_ENEMY:return "enemy";case PLAYER_DEATH_BOULDER:return "boulder";default:return "unknown";}
}
