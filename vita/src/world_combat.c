#include "world.h"
#include "melee.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACT_ONE_CHAIN_DIAGNOSTICS
static void chain_contact_source(const WorldEntity *e,unsigned damage){
 if(damage)fprintf(stdout,"CHAIN_CONTACT_SOURCE kind=%u subtype=%u damage=%u x=%.2f y=%.2f\n",
                   e->kind,e->subtype,damage,e->x_q8/256.0f,e->y_q8/256.0f);
}
#else
#define chain_contact_source(e,damage) ((void)0)
#endif


typedef struct {const World *owner;uint16_t count;int32_t x_q8[WORLD_WORM_PATH_SAMPLES],y_q8[WORLD_WORM_PATH_SAMPLES];} WormPath;
static WormPath worm_paths[4];
static WormPath *worm_path_for(const World *w,int create){WormPath *empty=NULL;for(unsigned i=0;i<4;i++){if(worm_paths[i].owner==w)return &worm_paths[i];if(!worm_paths[i].owner&&!empty)empty=&worm_paths[i];}if(create&&empty){empty->owner=w;empty->count=0;return empty;}return NULL;}
static void worm_path_reset(const World *w){WormPath *p=worm_path_for(w,1);if(p)p->count=0;}
static void worm_path_push(World *w,float x,float y){
 WormPath *p=worm_path_for(w,1);if(!p)return;
 if(!p->count){p->x_q8[0]=(int32_t)lrintf(x*256.0f);p->y_q8[0]=(int32_t)lrintf(y*256.0f);p->count=1;return;}
 float last_x=p->x_q8[p->count-1]/256.0f;
 float last_y=p->y_q8[p->count-1]/256.0f;
 if(fabsf(x-last_x)<.05f&&fabsf(y-last_y)<.05f)return;
 if(p->count==WORLD_WORM_PATH_SAMPLES){
  memmove(p->x_q8,p->x_q8+1,(WORLD_WORM_PATH_SAMPLES-1u)*sizeof(p->x_q8[0]));
  memmove(p->y_q8,p->y_q8+1,(WORLD_WORM_PATH_SAMPLES-1u)*sizeof(p->y_q8[0]));
  p->count--;
 }
 p->x_q8[p->count]=(int32_t)lrintf(x*256.0f);
 p->y_q8[p->count]=(int32_t)lrintf(y*256.0f);
 p->count++;
}

static int worm_platform_landing(const World *w,float x,float y,float width,
                                 float height,float fall,float *landed_y){
 if(fall<=0.0f)return 0;
 float bottom=y+height,next_bottom=bottom+fall;
 int first_row=(int)floorf(bottom/WORLD_TILE_SIZE);
 int last_row=(int)floorf(next_bottom/WORLD_TILE_SIZE);
 int first_x=(int)floorf((x+0.01f)/WORLD_TILE_SIZE);
 int last_x=(int)floorf((x+width-0.01f)/WORLD_TILE_SIZE);
 for(int row=first_row;row<=last_row;row++){
  float top=(float)row*WORLD_TILE_SIZE;
  if(bottom>top+0.01f||next_bottom<top)continue;
  for(int tx=first_x;tx<=last_x;tx++){
   const WorldTile *tile=world_tile(w,tx,row);
   if(tile&&tile->platform){*landed_y=top-height;return 1;}
  }
 }
 return 0;
}

void world_worm_path_sample(const World *w,const WorldEntity *e,unsigned segment,float *x,float *y){
 WormPath *p=worm_path_for(w,0);if(!p||!p->count){*x=e->x_q8/256.0f;*y=e->y_q8/256.0f;return;}
 float target=(float)(segment+1u)*24.0f,travelled=0.0f;
 unsigned i=p->count-1u;
 float newer_x=p->x_q8[i]/256.0f,newer_y=p->y_q8[i]/256.0f;
 while(i>0){
  --i;float older_x=p->x_q8[i]/256.0f,older_y=p->y_q8[i]/256.0f;
  float dx=newer_x-older_x,dy=newer_y-older_y,distance=sqrtf(dx*dx+dy*dy);
  if(distance>0.0001f&&travelled+distance>=target){
   float t=(target-travelled)/distance;*x=newer_x-dx*t;*y=newer_y-dy*t;return;
  }
  travelled+=distance;newer_x=older_x;newer_y=older_y;
 }
 *x=p->x_q8[0]/256.0f;*y=p->y_q8[0]/256.0f;
}

void world_enemy_body(uint8_t subtype,float *width,float *height){
 switch(subtype){
  case 9:*width=7.0f;*height=12.0f;break;  /* Goblin::Goblin */
  case 10:*width=9.0f;*height=10.0f;break; /* Bat::Bat */
  case 11:*width=10.0f;*height=10.0f;break;/* Lurker::Lurker */
  case 12:*width=10.0f;*height=7.0f;break; /* Slime::Slime */
  case 14:*width=10.0f;*height=8.0f;break; /* Spider::Spider */
  case 84:*width=7.0f;*height=12.0f;break; /* Thief::Thief */
  case WORLD_BOSS_GOBLIN_KING:*width=14.0f;*height=12.0f;break; /* BossGoblinKing ctor */
  case WORLD_BOSS_DRAGON:*width=13.0f;*height=13.0f;break; /* BossDragon ctor */
  case WORLD_BOSS_WORM:*width=8.0f;*height=8.0f;break; /* BossWorm head constructor */
  default:*width=12.0f;*height=12.0f;break;
 }
}

static uint32_t next_contact_random(WorldEntity *e){
 uint32_t x=e->contact_rng_state;
 if(!x){x=e->source_index^((uint32_t)e->x_q8*0x9e3779b9u)^((uint32_t)e->y_q8*0x85ebca6bu)^0xBA7C0111u;if(!x)x=0x6d2b79f5u;}
 x^=x<<13;x^=x>>17;x^=x<<5;e->contact_rng_state=x;return x;
}

static void dragon_action_set(WorldEntity *e,uint8_t action,uint16_t duration){
 e->dragon_action=action;e->dragon_action_timer=duration;
}

/* The native Dragon queues these actions from addNewActions. Its native
 * MT19937 stream and full action queue are not recovered yet, so this runtime
 * alternates the confirmed one-shot and three-shot actions deterministically. */
static void dragon_choose_action(WorldEntity *e,float dx,float dy){
 if(e->dragon_action_timer){e->dragon_action_timer--;return;}
 switch(e->dragon_action){
  case WORLD_DRAGON_ACTION_IDLE:
   dragon_action_set(e,WORLD_DRAGON_ACTION_MOVE_PLAYER,20);break;
  case WORLD_DRAGON_ACTION_MOVE_PLAYER:
  case WORLD_DRAGON_ACTION_PATH_PLAYER:
   dragon_action_set(e,WORLD_DRAGON_ACTION_FACE_PLAYER,1);break;
  case WORLD_DRAGON_ACTION_FACE_PLAYER:
   if(fabsf(dx)<240.0f&&fabsf(dy)<240.0f){
    uint8_t action=(e->dragon_action_cycle++&1u)?
      WORLD_DRAGON_ACTION_SHOOT_PLAYER_ONCE:WORLD_DRAGON_ACTION_SHOOT_PLAYER;
    dragon_action_set(e,action,1);
   }
   else dragon_action_set(e,WORLD_DRAGON_ACTION_MOVE_PLAYER,20);
   break;
  case WORLD_DRAGON_ACTION_SHOOT_PLAYER:
  case WORLD_DRAGON_ACTION_SHOOT_PLAYER_ONCE:
   dragon_action_set(e,WORLD_DRAGON_ACTION_IDLE,30);break;
  default:
   dragon_action_set(e,WORLD_DRAGON_ACTION_MOVE_PLAYER,20);break;
 }
}

static void dragon_fire(World *w,float x,float y,float px,float py,
                        float pw,float ph,float speed){
 if(w->entity_count>=WORLD_ENTITY_LIMIT)return;
 float sx=x+6.5f,sy=y+6.5f;
 float tx=px+pw*.5f,ty=py+ph*.5f;
 float dx=tx-sx,dy=ty-sy,distance=sqrtf(dx*dx+dy*dy);
 if(distance<.001f)distance=1.0f;
 WorldEntity *shot=&w->entities[w->entity_count++];memset(shot,0,sizeof(*shot));
 shot->kind=WORLD_KIND_DRAGON_FIREBALL;
 shot->x_q8=(int32_t)lrintf((sx-2.5f)*256.0f);
 shot->y_q8=(int32_t)lrintf((sy-2.5f)*256.0f);
 shot->x=(int16_t)(shot->x_q8>>8);shot->y=(int16_t)(shot->y_q8>>8);
 shot->vx=dx/distance*speed;shot->vy=dy/distance*speed;
 shot->hp=1.0f;shot->cooldown=180;shot->projectile_active=1;
}

/* First playable Worm-spit model. Ghidra confirms projectile type 10,
   poison classification, and 2.5 velocity; the damage and spread-angle
   constructor data-flow are unresolved, so damage 3 and straight aim remain
   explicit approximations. */
static void worm_spit_fire(World *w,WorldEntity *worm,float px,float py,
                           float pw,float ph){
 if(w->entity_count>=WORLD_ENTITY_LIMIT)return;
 float x=worm->x_q8/256.0f+4.0f,y=worm->y_q8/256.0f+4.0f;
 float dx=px+pw*.5f-x,dy=py+ph*.5f-y,distance=sqrtf(dx*dx+dy*dy);
 if(distance<.001f)distance=1.0f;
 WorldEntity *shot=&w->entities[w->entity_count++];memset(shot,0,sizeof(*shot));
 shot->kind=WORLD_KIND_WORM_SPIT;shot->subtype=4; /* DAMAGE_POISON */
 shot->variant=10; /* PROJECTILE_WORM_SHOT */
 shot->x_q8=(int32_t)lrintf((x-2.5f)*256.0f);
 shot->y_q8=(int32_t)lrintf((y-2.5f)*256.0f);
 shot->x=(int16_t)(shot->x_q8>>8);shot->y=(int16_t)(shot->y_q8>>8);
 shot->vx=dx/distance*WORLD_WORM_SPIT_SPEED;
 shot->vy=dy/distance*WORLD_WORM_SPIT_SPEED;
 shot->hp=1.0f;shot->cooldown=180;shot->projectile_active=1;
}

static const GoblinAttackFrame *goblin_attack_frame(const GoblinAttackData *attack,
                                                     const WorldEntity *enemy){
 if(!attack->ready||!enemy->goblin_swing_frame)return NULL;
 unsigned elapsed=enemy->goblin_swing_frame-1;
 for(unsigned i=0;i<attack->frame_count;i++){
  if(elapsed<attack->frames[i].duration)return attack->frames[i].enabled?&attack->frames[i]:NULL;
  elapsed-=attack->frames[i].duration;
 }
 return NULL;
}

static int point_in_polygon(const GoblinAttackPoint *p,unsigned n,float x,float y){
 int inside=0;
 for(unsigned i=0,j=n-1;i<n;j=i++){
  if((p[i].y>y)!=(p[j].y>y)&&x<(p[j].x-p[i].x)*(y-p[i].y)/(p[j].y-p[i].y)+p[i].x)inside=!inside;
 }
 return inside;
}

static int orientation(GoblinAttackPoint a,GoblinAttackPoint b,GoblinAttackPoint c){
 float cross=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
 return cross>0.0001f?1:cross< -0.0001f? -1:0;
}

static int on_segment(GoblinAttackPoint a,GoblinAttackPoint b,GoblinAttackPoint p){
 return p.x>=fminf(a.x,b.x)-0.0001f&&p.x<=fmaxf(a.x,b.x)+0.0001f&&
        p.y>=fminf(a.y,b.y)-0.0001f&&p.y<=fmaxf(a.y,b.y)+0.0001f;
}

static int segments_intersect(GoblinAttackPoint a,GoblinAttackPoint b,
                              GoblinAttackPoint c,GoblinAttackPoint d){
 int o1=orientation(a,b,c),o2=orientation(a,b,d),o3=orientation(c,d,a),o4=orientation(c,d,b);
 if(o1!=o2&&o3!=o4)return 1;
 return (!o1&&on_segment(a,b,c))||(!o2&&on_segment(a,b,d))||
        (!o3&&on_segment(c,d,a))||(!o4&&on_segment(c,d,b));
}

static int goblin_attack_hits_player(const GoblinAttackFrame *frame,const WorldEntity *enemy,
                                     float body_w,float body_h,float px,float py,float pw,float ph){
 float facing=enemy->goblin_facing_right?1.0f:-1.0f;
 float angle=frame->origin_degrees*0.0174532925199433f*facing,c=cosf(angle),s=sinf(angle);
 float ox=enemy->x_q8/256.0f+body_w*.5f+frame->origin_x*facing;
 float oy=enemy->y_q8/256.0f+body_h+frame->origin_y;
 GoblinAttackPoint polygon[WORLD_GOBLIN_ATTACK_VERTICES];
 for(unsigned i=0;i<frame->vertex_count;i++){
  float x=frame->vertices[i].x*facing,y=frame->vertices[i].y;
  polygon[i]=(GoblinAttackPoint){ox+c*x-s*y,oy+s*x+c*y};
 }
 const GoblinAttackPoint corners[4]={{px,py},{px+pw,py},{px+pw,py+ph},{px,py+ph}};
 for(unsigned i=0;i<frame->vertex_count;i++)
  if(polygon[i].x>=px&&polygon[i].x<=px+pw&&polygon[i].y>=py&&polygon[i].y<=py+ph)return 1;
 for(unsigned i=0;i<4;i++)if(point_in_polygon(polygon,frame->vertex_count,corners[i].x,corners[i].y))return 1;
 for(unsigned i=0;i<frame->vertex_count;i++)for(unsigned j=0;j<4;j++)
  if(segments_intersect(polygon[i],polygon[(i+1)%frame->vertex_count],corners[j],corners[(j+1)%4]))return 1;
 return 0;
}

static void goblin_attack_step(World *w,WorldEntity *e,float dx,float dy){
 const GoblinAttackData *attack=&w->goblin_attack;
 if(!attack->ready)return;
 if(e->goblin_attack_cooldown)e->goblin_attack_cooldown--;
 if(e->goblin_swing_frame){
  unsigned total=0;for(unsigned i=0;i<attack->frame_count;i++)total+=attack->frames[i].duration;
  if(e->goblin_swing_frame>=total)e->goblin_swing_frame=0;
  else e->goblin_swing_frame++;
  return;
 }
 if(e->goblin_prep_timer){
  e->goblin_prep_timer--;
  if(!e->goblin_prep_timer){
   e->goblin_attack_cooldown=attack->cooldown;e->goblin_hit=0;
   e->goblin_attack_damage=(uint8_t)(attack->damage_min+next_contact_random(e)%
                                     (attack->damage_max-attack->damage_min+1));
   e->goblin_swing_frame=1;
  }
  return;
 }
 unsigned facing_right=dx>0.0f;
 if(e->goblin_facing_right!=facing_right){e->goblin_facing_right=(uint8_t)facing_right;e->goblin_facing_frames=0;}
 else if(e->goblin_facing_frames<255)e->goblin_facing_frames++;
 if(!e->goblin_attack_cooldown&&e->goblin_facing_frames>10&&fabsf(dx)<55.0f&&fabsf(dy)<5.0f)
  e->goblin_prep_timer=attack->windup;
}

/* Ghidra: Thief::runAI gates a melee attempt on visibility, horizontal distance
 * < 11 px, vertical distance < 6 px, and no active weapon swing; attack() then
 * sets a 60-tick cooldown and rolls constructor damage 1..2. The VWE3 packet
 * has no visibility/path/weapon-animation state, so preserve the confirmed
 * cooldown/damage and approximate the weapon window as a short facing swing. */
static void thief_attack_step(WorldEntity *e,float dx,float dy){
 if(e->thief_attack_cooldown)e->thief_attack_cooldown--;
 if(e->thief_swing_frames){
  if(!--e->thief_swing_frames)e->thief_hit=0;
  return;
 }
 if(!e->thief_attack_cooldown&&fabsf(dx)<11.0f&&fabsf(dy)<6.0f){
  e->thief_facing_right=(uint8_t)(dx>=0.0f);
  e->thief_swing_frames=8;e->thief_hit=0;e->thief_attack_cooldown=60;
 }
}

/* Ghidra: Lurker::Lurker initializes attackDelayTimer=16, attackReady=true,
 * and minDamage/maxDamage=4/16. Lurker::step starts the attack animation when
 * triggered and the timer reaches zero; Lurker::doCollision applies one
 * damage event during sensor/body collision. Sensor range and retract sprite
 * timing are not represented by the room packet, so their windows stay local
 * approximations. */
static void lurker_attack_step(WorldEntity *e,float dx,float dy){
 if(e->lurker_recovery)e->lurker_recovery--;
 if(e->lurker_attack_frames){e->lurker_attack_frames--;if(!e->lurker_attack_frames)e->lurker_recovery=32;return;}
 if(e->lurker_windup){
  e->lurker_windup--;
  if(!e->lurker_windup){e->lurker_attack_frames=8;e->lurker_hit=0;}
  return;
 }
 if(!e->lurker_recovery&&fabsf(dx)<24.0f&&fabsf(dy)<24.0f)
  e->lurker_windup=16;
}

static int spider_rect_solid(const World *w,float x,float y,float width,float height){
 int left=(int)floorf(x/16.0f),right=(int)floorf((x+width-.001f)/16.0f);
 int top=(int)floorf(y/16.0f),bottom=(int)floorf((y+height-.001f)/16.0f);
 for(int ty=top;ty<=bottom;ty++)for(int tx=left;tx<=right;tx++)
  if(world_solid(w,tx,ty))return 1;
 return 0;
}

static int spider_move_axis(const World *w,float *x,float *y,float amount,
                            float width,float height,int vertical){
 /* Valid enemy step speeds stay below 8 px. Reject corrupted or non-finite
    motion before converting its magnitude to an unsigned loop count. */
 if(!isfinite(amount)||fabsf(amount)>16.0f)return 0;
 if(fabsf(amount)<.0001f)return 0;
 unsigned steps=(unsigned)ceilf(fabsf(amount));float delta=amount/(float)steps;
 int landed=0;
 for(unsigned i=0;i<steps;i++){
  float nx=*x+(vertical?0.0f:delta),ny=*y+(vertical?delta:0.0f);
  if(!spider_rect_solid(w,nx,ny,width,height)){*x=nx;*y=ny;continue;}
  if(vertical&&delta>0.0f){int row=(int)floorf((ny+height-.001f)/16.0f);*y=row*16.0f-height;landed=1;}
  else if(vertical&&delta<0.0f){int row=(int)floorf(ny/16.0f);*y=(row+1)*16.0f;}
  return landed;
 }
 return landed;
}

/* BossGoblinKing::chargeAttack @ 0x0051b040 confirms 35/75 warning ticks,
 * a one-time 0.7 speed scale, a ±9.3 px/tick launch, then damping in
 * BossGoblinKing::step. The action queue's random selection, orientation bit
 * polarity, damage and full collision rules remain unresolved. This local
 * adapter starts the confirmed cycle when the visible target is within the
 * 128 px Manhattan aggro gate and resolves its 14x12 body against room tiles. */
static void goblin_king_charge_step(World *w,WorldEntity *e,float px,float py,float pw,float ph){
 float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
 float dx=px+pw*.5f-(x+7.0f),dy=py+ph*.5f-(y+6.0f);
 if(e->boss_charge_cooldown)e->boss_charge_cooldown--;
 if(!e->boss_charge_active){
  if(!e->boss_charge_cooldown&&fabsf(dx)+fabsf(dy)<128.0f){
   e->boss_charge_active=1;e->boss_charge_hit=0;e->boss_facing_right=(uint8_t)(dx>=0.0f);
   e->boss_charge_timer=e->boss_facing_right?35:75;e->vx*=.7f;
  }
 }else if(e->boss_charge_timer){
  if(!--e->boss_charge_timer)e->vx=e->boss_facing_right?9.3f:-9.3f;
 }else{
  float previous_x=x;
  (void)spider_move_axis(w,&x,&y,e->vx,14.0f,12.0f,0);
  if(fabsf(x-previous_x)<.0001f)e->vx=0.0f;else e->vx*=.94f;
  if(fabsf(e->vx)<1.0f){e->vx=0.0f;e->boss_charge_active=0;e->boss_charge_cooldown=180;}
  e->x_q8=(int32_t)lrintf(x*256.0f);e->y_q8=(int32_t)lrintf(y*256.0f);
  e->x=(int16_t)x;e->y=(int16_t)y;
 }
}

/* Bat::step uses a normalized target vector, 0.05 acceleration, a 30-tick
 * charge tell, a 20-tick speed-4 lunge and a 60-tick cooldown (Ghidra
 * 0x004fc7a0). PathManager's wall routing and sprite-driven charge cues are
 * not represented by the room packet, so this uses direct pursuit and the
 * shared tile collision sweep. */
static void bat_step(World *w,WorldEntity *e,float px,float py,float pw,float ph,
                     float body_w,float body_h){
 float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
 float dx=px+pw*.5f-(x+body_w*.5f),dy=py+ph*.5f-(y+body_h*.5f);
 float distance=sqrtf(dx*dx+dy*dy);
 float tx=distance>.0001f?dx/distance:0.0f,ty=distance>.0001f?dy/distance:0.0f;
 if(e->bat_charge_prep){
  e->vx=0.0f;e->vy=0.0f;
  if(!--e->bat_charge_prep){e->bat_charge_timer=20;e->vx=tx*4.0f;e->vy=ty*4.0f;}
 }else if(e->bat_charge_timer){
  if(!--e->bat_charge_timer){e->bat_charge_cooldown=60;e->vx*=.49f;e->vy*=.49f;}
 }else{
  e->vx+=tx*.05f;e->vy+=ty*.05f;
  float speed=sqrtf(e->vx*e->vx+e->vy*e->vy);
  if(speed>3.0f){e->vx=e->vx/speed*3.0f;e->vy=e->vy/speed*3.0f;}
  if(e->bat_charge_cooldown)e->bat_charge_cooldown--;
  else if(distance<48.0f)e->bat_charge_prep=30;
 }
 (void)spider_move_axis(w,&x,&y,e->vx,body_w,body_h,0);
 (void)spider_move_axis(w,&x,&y,e->vy,body_w,body_h,1);
 e->x_q8=(int32_t)lrintf(x*256.0f);e->y_q8=(int32_t)lrintf(y*256.0f);
 e->x=(int16_t)x;e->y=(int16_t)y;
}

/* Slime::step @ 0x0078fab0 starts the normal lunge after a 60-frame tell,
 * launches at speed 7 for 25 frames, and sets a 150-frame cooldown. The normal
 * variant checks a <50 px horizontal window, near-equal vertical positions,
 * jumpTimer>30, and a 1-in-20 random roll. SlimeType is absent from VWE3, so
 * this reproduces the normal variant only. */
static void slime_lunge_step(WorldEntity *e,float dx,float dy){
 if(e->slime_lunge_cooldown)e->slime_lunge_cooldown--;
 if(e->slime_lunge_prep){
  e->vx*=.95f;
  if(!--e->slime_lunge_prep){
   e->slime_lunge_frames=25;e->slime_lunge_hit=0;
   e->vx=e->slime_facing_left?-7.0f:7.0f;
  }
  return;
 }
 if(e->slime_lunge_frames){
  if(!--e->slime_lunge_frames)e->slime_lunge_cooldown=150;
  return;
 }
 if(e->slime_lunge_timer)e->slime_lunge_timer--;
 if(dx<0.0f)e->slime_facing_left=1;else if(dx>0.0f)e->slime_facing_left=0;
 if(!e->slime_lunge_cooldown&&e->slime_lunge_timer>30&&
    fabsf(dx)<50.0f&&fabsf(dy)<2.0f&&next_contact_random(e)%20u==0)
  e->slime_lunge_prep=60;
}

/* Slime::step also performs a periodic hop: the timer is initialized to
 * 80..200 frames, then the slime launches with horizontal unit velocity and
 * vertical velocity in [-3.6,-1]. Gravity and floor bounce are reconstructed
 * from the native branch; VWE3 does not carry SlimeType, so variant-specific
 * movement remains unavailable. */
static void slime_step(World *w,WorldEntity *e,float px,float py,float pw,float ph,
                       float body_w,float body_h){
 float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
 float dx=px+pw*.5f-(x+body_w*.5f),dy=py+ph*.5f-(y+body_h*.5f);
 if(!e->slime_jump_initialized){
  e->slime_jump_timer=(uint16_t)(80u+next_contact_random(e)%121u);
  e->slime_lunge_timer=e->slime_jump_timer;
  e->slime_jump_initialized=1;
 }
 if(e->slime_jump_timer)e->slime_jump_timer--;
 if(!e->slime_jump_timer&&!e->slime_lunge_prep&&!e->slime_lunge_frames){
  float u=(float)(next_contact_random(e)>>8)*(1.0f/16777216.0f);
  e->vx=e->slime_facing_left?-1.0f:1.0f;e->vy=-1.0f-u*2.6f;
  e->slime_jump_timer=(uint16_t)(80u+next_contact_random(e)%120u);
  e->slime_lunge_timer=e->slime_jump_timer;
  e->slime_bounces=0;
 }
 if(e->slime_jump_timer<30&&!e->slime_lunge_prep&&!e->slime_lunge_frames)e->vx*=.8f;
 slime_lunge_step(e,dx,dy);
 if(!e->slime_lunge_prep&&!e->slime_lunge_frames){
  if(e->slime_jump_timer>30&&fabsf(dx)<88.0f&&fabsf(dy)<30.0f){
   e->vx+=dx>0.0f?.035f:-.035f;if(e->vx>.7f)e->vx=.7f;if(e->vx<-.7f)e->vx=-.7f;
  }else if(e->slime_jump_timer>30){e->vx*=.94f;if(fabsf(e->vx)<.04f)e->vx=0.0f;}
 }
 (void)spider_move_axis(w,&x,&y,e->vx,body_w,body_h,0);
 if(e->vy<8.0f)e->vy+=.16f;
 if(e->vy>8.0f)e->vy=8.0f;
 int landed=spider_move_axis(w,&x,&y,e->vy,body_w,body_h,1);
 if(landed){
  if(e->vy>1.0f&&e->slime_bounces<2){e->vy=-e->vy*.95f;e->slime_bounces++;}
  else {e->vy=0.0f;e->slime_bounces=0;}
 }
 e->x_q8=(int32_t)lrintf(x*256.0f);e->y_q8=(int32_t)lrintf(y*256.0f);
 e->x=(int16_t)x;e->y=(int16_t)y;
}

/* Spider::runAI sets ACT_JUMP after a 20/30-frame tell. At launch its
 * native constants are angle 1.256637 + U*0.1 and speed 3 + U*2,
 * U uniform on [0,1] (Ghidra listing 0x007b18a0 and data at 0x00b81f90).
 * Entity movement uses the shared 0.16 gravity seen in player/world physics. */
static void spider_step(World *w,WorldEntity *e,float px,float py,float pw,float ph,
                        float body_w,float body_h){
 float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
 float dx=px+pw*.5f-(x+body_w*.5f),dy=py+ph*.5f-(y+body_h*.5f);
 int support=(int)floorf((y+body_h+.5f)/16.0f);
 int grounded=world_solid(w,(int)floorf((x+body_w*.5f)/16.0f),support);
 if(e->spider_recovery)e->spider_recovery--;
 if(e->spider_jump_tell){
  e->vx*=.5f;
  e->spider_jump_tell--;
  if(!e->spider_jump_tell){
   float u_angle=(float)(next_contact_random(e)>>8)*(1.0f/16777216.0f);
   float u_speed=(float)(next_contact_random(e)>>8)*(1.0f/16777216.0f);
   float angle=1.256637096f+u_angle*.1f;
   float speed=3.0f+2.0f*u_speed;
   float direction=dx<0.0f?-1.0f:1.0f;
   e->vx=cosf(angle)*speed*direction;e->vy=-sinf(angle)*speed;
   e->spider_jumping=1;
  }
 }else if(!e->spider_jumping&&!e->spider_recovery&&grounded&&
          fabsf(dx)>=16.0f&&fabsf(dx)<64.0f&&fabsf(dy)<32.0f){
  /* The close-range native branch sets jumpTellFrames to 0x14. */
  e->spider_jump_tell=20;e->vx*=.5f;
 }else if(!e->spider_jumping&&!e->spider_jump_tell&&grounded&&fabsf(dx)<88.0f&&fabsf(dy)<30.0f){
  e->vx+=dx>0.0f?.035f:-.035f;if(e->vx>.7f)e->vx=.7f;if(e->vx<-.7f)e->vx=-.7f;
 }else if(!e->spider_jumping){
  e->vx*=.94f;if(fabsf(e->vx)<.04f)e->vx=0.0f;
 }

 int landed=spider_move_axis(w,&x,&y,e->vx,body_w,body_h,0);
 (void)landed;
 if(e->vy<8.0f)e->vy+=.16f;
 if(e->vy>8.0f)e->vy=8.0f;
 landed=spider_move_axis(w,&x,&y,e->vy,body_w,body_h,1);
 if(landed){e->vy=0.0f;if(e->spider_jumping){e->spider_jumping=0;e->spider_recovery=30;}}
 e->x_q8=(int32_t)lrintf(x*256.0f);e->y_q8=(int32_t)lrintf(y*256.0f);
 e->x=(int16_t)x;e->y=(int16_t)y;
}

void world_enemies_step(World *w,float px,float py,float pw,float ph,const MeleeState *attack){
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];
  if(e->kind==WORLD_KIND_WORM_SPIT&&e->hp>0.0f){
   if(e->cooldown&&!--e->cooldown){e->hp=0.0f;continue;}
   float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
   float nx=x+e->vx,ny=y+e->vy;
   if(spider_rect_solid(w,nx-2.5f,ny-2.5f,5.0f,5.0f)){e->hp=0.0f;continue;}
   e->vy+=0.01f; /* Projectile::step's owner-scaled Worm-shot increment. */
   e->x_q8=(int32_t)lrintf(nx*256.0f);e->y_q8=(int32_t)lrintf(ny*256.0f);
   e->x=(int16_t)nx;e->y=(int16_t)ny;
   continue;
  }
  if(e->kind==WORLD_KIND_DRAGON_FIREBALL&&e->hp>0.0f){
   if(e->cooldown&&!--e->cooldown){e->hp=0.0f;continue;}
   float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
   float nx=x+e->vx,ny=y+e->vy;
   if(spider_rect_solid(w,nx,ny,5.0f,5.0f)){e->hp=0.0f;continue;}
   e->x_q8=(int32_t)lrintf(nx*256.0f);e->y_q8=(int32_t)lrintf(ny*256.0f);
   e->x=(int16_t)nx;e->y=(int16_t)ny;
   continue;
  }
  if(e->kind==WORLD_KIND_BOSS&&
     (e->subtype==WORLD_BOSS_GOBLIN_KING||e->subtype==WORLD_BOSS_DRAGON||e->subtype==WORLD_BOSS_WORM)&&e->hp>0){
   if(e->contact_cooldown)e->contact_cooldown--;
   if(!isfinite(e->vx)||fabsf(e->vx)>16.0f)e->vx=0.0f;
   float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
   float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
   float damage=attack&&attack->active
       ?melee_damage_rect(attack,px+pw*.5f,py+ph,ex,ey,body_w,body_h):0.0f;
   if(e->subtype==WORLD_BOSS_WORM&&attack&&attack->active&&!damage){
    for(unsigned segment=0;segment<WORLD_WORM_SEGMENTS&&!damage;segment++){
     float sx,sy;world_worm_path_sample(w,e,segment,&sx,&sy);
     damage=melee_damage_rect(attack,px+pw*.5f,py+ph,sx,sy,8.0f,8.0f);
    }
   }
   if(damage>0.0f&&attack->serial!=e->hit_attack_serial){
    e->hp-=damage;e->hit_attack_serial=attack->serial;
    if(e->hp<=0.0f){e->hp=0.0f;e->boss_charge_active=0;e->boss_charge_timer=0;e->vx=0.0f;
     world_record_boss_defeat(w,e);
     int reward=world_spawn_boss_reward_chest(w,e);
     if(!reward)e->boss_reward_spawned=1;
     continue;}
   }
   if(e->subtype==WORLD_BOSS_GOBLIN_KING)goblin_king_charge_step(w,e,px,py,pw,ph);
   else if(e->subtype==WORLD_BOSS_DRAGON){
    float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
    float dx=px+pw*.5f-(x+body_w*.5f),dy=py+ph*.5f-(y+body_h*.5f);
    float distance=sqrtf(dx*dx+dy*dy);
    if(distance<.001f)distance=1.0f;
    e->boss_facing_right=(uint8_t)(dx>=0.0f);
    if(!e->dragon_initialized){e->dragon_initialized=1;dragon_action_set(e,WORLD_DRAGON_ACTION_IDLE,30);}
    dragon_choose_action(e,dx,dy);
    if(e->dragon_action==WORLD_DRAGON_ACTION_MOVE_PLAYER||
       e->dragon_action==WORLD_DRAGON_ACTION_PATH_PLAYER){
     float target_vx=dx/distance*1.5f,target_vy=dy/distance*1.5f;
     e->vx+=(target_vx-e->vx)*.08f;e->vy+=(target_vy-e->vy)*.08f;
     float speed=sqrtf(e->vx*e->vx+e->vy*e->vy);
     if(speed>1.5f){e->vx=e->vx/speed*1.5f;e->vy=e->vy/speed*1.5f;}
    }else{e->vx*=.82f;e->vy*=.82f;if(fabsf(e->vx)<.04f)e->vx=0;if(fabsf(e->vy)<.04f)e->vy=0;}
    (void)spider_move_axis(w,&x,&y,e->vx,body_w,body_h,0);
    (void)spider_move_axis(w,&x,&y,e->vy,body_w,body_h,1);
    e->x_q8=(int32_t)lrintf(x*256.0f);e->y_q8=(int32_t)lrintf(y*256.0f);
    e->x=(int16_t)x;e->y=(int16_t)y;
    if(e->dragon_attack_active){
     if(e->dragon_attack_timer)e->dragon_attack_timer--;
     if(!e->dragon_attack_timer){
      dragon_fire(w,x,y,px,py,pw,ph,1.45f);
      if(e->dragon_attack_shots_remaining)e->dragon_attack_shots_remaining--;
      if(e->dragon_attack_shots_remaining)e->dragon_attack_timer=55;
      else e->dragon_attack_active=0;
     }
    }else if((e->dragon_action==WORLD_DRAGON_ACTION_SHOOT_PLAYER||
             e->dragon_action==WORLD_DRAGON_ACTION_SHOOT_PLAYER_ONCE)&&
            distance<240.0f&&world_line_of_sight(w,x+body_w*.5f,y+body_h*.5f,
                                                 px+pw*.5f,py+ph*.5f)){
     e->dragon_attack_active=1;
     e->dragon_attack_shots_remaining=
       e->dragon_action==WORLD_DRAGON_ACTION_SHOOT_PLAYER?3u:1u;
     e->dragon_attack_timer=55;
     /* Ghidra's action parameter is its required projectile count. */
    }
   }else if(e->subtype==WORLD_BOSS_WORM){
    /* Provisional articulated crawler: the head follows the player on the
       ground and each of the 20 linked body pieces follows a delayed path. */
    if(!e->worm_initialized){
     e->worm_initialized=1;e->worm_segment_count=WORLD_WORM_SEGMENTS;
     worm_path_reset(w);worm_path_push(w,e->x_q8/256.0f,e->y_q8/256.0f);
    }
    float x=e->x_q8/256.0f,y=e->y_q8/256.0f;
    float target=px+pw*.5f-(x+body_w*.5f);
    e->boss_facing_right=(uint8_t)(target>=0.0f);
    e->vx+=(target>0?0.22f:-0.22f);
    if(e->vx>1.5f)e->vx=1.5f;
    if(e->vx< -1.5f)e->vx= -1.5f;
    int dir=e->vx>0?1:e->vx<0?-1:0;
    if(dir&&!world_solid(w,(int)((x+(dir>0?body_w:-1))/16.0f),(int)((y+body_h-1)/16.0f)))x+=e->vx;
    else if(dir)e->vx=0.0f;
    int support=(int)((y+body_h+1)/16.0f);
    if(!world_solid(w,(int)((x+body_w*.5f)/16.0f),support)){
     float landed_y;
     if(worm_platform_landing(w,x,y,body_w,body_h,1.8f,&landed_y))y=landed_y;
     else y+=1.8f;
    }else y=support*16.0f-body_h;
    e->x_q8=(int32_t)lrintf(x*256.0f);e->y_q8=(int32_t)lrintf(y*256.0f);
    e->x=(int16_t)x;e->y=(int16_t)y;
    worm_path_push(w,x,y);
    if(++e->worm_animation_timer>=6u){e->worm_animation_timer=0;e->worm_animation_frame++;}
    e->worm_animation_frame%=16u;
    float dx=px+pw*.5f-(x+4.0f),dy=py+ph*.5f-(y+4.0f);
    float distance=sqrtf(dx*dx+dy*dy);
    int can_attack=distance<=320.0f&&world_line_of_sight(w,x+4.0f,y+4.0f,
                                                        px+pw*.5f,py+ph*.5f);
    if(e->worm_behavior_timer)e->worm_behavior_timer--;
    if(!can_attack){e->worm_behavior=0;e->worm_spit_shots_remaining=0;}
    else if(e->worm_behavior==0&&e->worm_behavior_timer==0){
     e->worm_behavior=7;e->worm_spit_shots_remaining=3;e->worm_behavior_timer=20;
    }else if(e->worm_behavior==7&&e->worm_behavior_timer==0){
     worm_spit_fire(w,e,px,py,pw,ph);
     if(e->worm_spit_shots_remaining)e->worm_spit_shots_remaining--;
     /* Ghidra's ATTACK completion enters TAUNT for 0xb4 (180) frames. */
     e->worm_behavior_timer=e->worm_spit_shots_remaining?28u:
       WORLD_WORM_SPIT_TAUNT_TICKS;
     if(!e->worm_spit_shots_remaining)e->worm_behavior=0;
    }
   }
   continue;
  }
  if(e->kind!=24||e->hp<=0)continue;
  if(!isfinite(e->vx)||fabsf(e->vx)>16.0f)e->vx=0.0f;
  if(!isfinite(e->vy)||fabsf(e->vy)>16.0f)e->vy=0.0f;
  if(e->cooldown)e->cooldown--;
  if(e->contact_cooldown)e->contact_cooldown--;
  float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
  float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
  float dx=px+pw*.5f-(ex+body_w*.5f),dy=py+ph*.5f-(ey+body_h*.5f);
  if(e->subtype==9)goblin_attack_step(w,e,dx,dy);
  if(e->subtype==84)thief_attack_step(e,dx,dy);
  if(e->subtype==11)lurker_attack_step(e,dx,dy);
  float damage=attack&&attack->active?melee_damage_rect(attack,px+pw*.5f,py+ph,ex,ey,body_w,body_h):0;
  if(damage>0&&!e->cooldown&&attack->serial!=e->hit_attack_serial){
   e->hp-=damage;e->hit_attack_serial=attack->serial;e->cooldown=12;if(e->hp<=0){e->hp=0;continue;}
  }
   if(e->subtype==14){spider_step(w,e,px,py,pw,ph,body_w,body_h);continue;}
   if(e->subtype==10){bat_step(w,e,px,py,pw,ph,body_w,body_h);continue;}
   if(e->subtype==11){e->vx=0.0f;}
   else if(e->subtype==12){slime_step(w,e,px,py,pw,ph,body_w,body_h);continue;}
  else if(e->subtype==9&&(e->goblin_prep_timer||e->goblin_swing_frame))e->vx*=.8f;
  else if(fabsf(dx)<88&&fabsf(dy)<30){e->vx+=dx>0?.035f:-.035f;if(e->vx>.7f)e->vx=.7f;if(e->vx<-.7f)e->vx=-.7f;}
  else {e->vx*=.94f;if(fabsf(e->vx)<.04f)e->vx=0;}
  int dir=e->vx>0?1:e->vx<0?-1:0;
  if(dir&&!world_solid(w,(int)((ex+(dir>0?body_w: -1))/16),(int)((ey+body_h-1)/16)))ex+=e->vx;
  else if(dir)e->vx=-e->vx*.5f;
  int support=(int)((ey+body_h+1)/16);
  if(!world_solid(w,(int)((ex+body_w*.5f)/16),support))ey+=1.8f;
  else ey=support*16-body_h;
  e->x_q8=(int32_t)lrintf(ex*256);e->y_q8=(int32_t)lrintf(ey*256);e->x=(int16_t)ex;e->y=(int16_t)ey;
 }
}

/* Spider::runAI emits ProjectileType 19 (WEB_BALL), damage 7 and a velocity
 * vector of magnitude 4. Ghidra's Projectile::doCollision creates SpellType 5
 * (web) on a unit hit; the projectile sticks on its first collision. The
 * attack selection (willWeb) is 10% inside 64 px; windup is approximated at
 * 24 ticks because the extracted animation completion flag is asset-driven. */
int world_spider_web_step(World *w,float px,float py,float pw,float ph){
 int hit=0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=24||e->subtype!=14||e->hp<=0)continue;
  if(e->spider_web_cooldown)e->spider_web_cooldown--;
  if(e->spider_webbed)e->spider_webbed--;
  if(e->spider_web_prep){
   e->spider_web_prep--;
   if(!e->spider_web_prep&&!e->projectile_active){
    float sx=e->x_q8/256.0f+4.0f,sy=e->y_q8/256.0f+4.0f;
    float tx=px+pw*.5f-sx,ty=py+ph*.5f-sy,len=sqrtf(tx*tx+ty*ty);
    if(len<.001f)len=1.0f;
    e->spider_web_x_q8=(int32_t)lrintf(sx*256.0f);e->spider_web_y_q8=(int32_t)lrintf(sy*256.0f);
    e->spider_web_vx=tx/len*4.0f;e->spider_web_vy=ty/len*4.0f;
    e->projectile_active=1;e->spider_web_hit=0;e->spider_web_frames=90;
   }
  }else if(!e->projectile_active&&!e->spider_web_cooldown&&!e->spider_jumping){
   float dx=px+pw*.5f-(e->x_q8/256.0f+4.0f),dy=py+ph*.5f-(e->y_q8/256.0f+4.0f);
   if(fabsf(dx)<64.0f&&fabsf(dy)<40.0f&&(next_contact_random(e)%10u)==0){
    e->spider_web_prep=24;e->spider_web_cooldown=150;
   }
  }
  if(!e->projectile_active)continue;
  if(e->spider_web_frames)e->spider_web_frames--;else {e->projectile_active=0;continue;}
  float x=e->spider_web_x_q8/256.0f,y=e->spider_web_y_q8/256.0f;
  float nx=x+e->spider_web_vx,ny=y+e->spider_web_vy;
  float dx=e->spider_web_vx/4.0f,dy=e->spider_web_vy/4.0f;
  int stuck=0;float contact_x=x,contact_y=y;
  for(int step=1;step<=4&&!stuck;step++){
   float qx=x+dx*step,qy=y+dy*step;
   if(spider_rect_solid(w,qx-2.5f,qy-2.5f,5.0f,5.0f)){stuck=1;contact_x=qx;contact_y=qy;break;}
   if(!e->spider_web_hit&&qx<px+pw&&qx+5.0f>px&&qy<py+ph&&qy+5.0f>py){
    e->spider_web_hit=1;e->spider_webbed=90;hit=7;stuck=1;contact_x=qx;contact_y=qy;
   }
  }
  e->spider_web_x_q8=(int32_t)lrintf((stuck?contact_x:nx)*256.0f);
  e->spider_web_y_q8=(int32_t)lrintf((stuck?contact_y:ny)*256.0f);
  if(stuck)e->projectile_active=0;
 }
 return hit;
}

/* Bat::doCollision rolls 1..6 and gates contact for 30 frames. Goblin
 * instead uses the private goblin-stab polygon after its 16-frame tell;
 * the attack script is converted into the external VGA1 package. */
unsigned world_enemies_contact_damage(World *w,float px,float py,float pw,float ph){
 unsigned damage=0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];
  if(e->kind==WORLD_KIND_DRAGON_FIREBALL&&e->hp>0.0f&&
     px<e->x_q8/256.0f+5.0f&&px+pw>e->x_q8/256.0f&&
     py<e->y_q8/256.0f+5.0f&&py+ph>e->y_q8/256.0f){
   e->hp=0.0f;e->projectile_active=0;damage+=WORLD_DRAGON_FIREBALL_DAMAGE;chain_contact_source(e,WORLD_DRAGON_FIREBALL_DAMAGE);
   continue;
  }
  if(e->kind==WORLD_KIND_WORM_SPIT&&e->hp>0.0f&&
     px<e->x_q8/256.0f+2.5f&&px+pw>e->x_q8/256.0f-2.5f&&
     py<e->y_q8/256.0f+2.5f&&py+ph>e->y_q8/256.0f-2.5f){
   e->hp=0.0f;e->projectile_active=0;damage+=WORLD_WORM_SPIT_DAMAGE;chain_contact_source(e,WORLD_WORM_SPIT_DAMAGE);
   continue;
  }
  if(e->kind==WORLD_KIND_BOSS&&e->subtype==WORLD_BOSS_DRAGON&&e->hp>0&&
     !e->contact_cooldown){
   float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
   float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
   if(px<ex+body_w&&px+pw>ex&&py<ey+body_h&&py+ph>ey){
    e->contact_cooldown=30;damage+=WORLD_DRAGON_DAMAGE;chain_contact_source(e,WORLD_DRAGON_DAMAGE);
   }
   continue;
  }
  if(e->kind==WORLD_KIND_BOSS&&e->subtype==WORLD_BOSS_WORM&&e->hp>0&&
     !e->contact_cooldown){
   float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
   int overlap=px<ex+8.0f&&px+pw>ex&&py<ey+8.0f&&py+ph>ey;
   for(unsigned segment=0;segment<WORLD_WORM_SEGMENTS&&!overlap;segment++){
    float sx,sy;world_worm_path_sample(w,e,segment,&sx,&sy);
    overlap=px<sx+8.0f&&px+pw>sx&&py<sy+8.0f&&py+ph>sy;
   }
   if(overlap){
    e->contact_cooldown=30;
    /* BossWorm::initializeStats uses damageScaleFactor * 3; doCollision
       applies damage - 1 + uniform_int(0, 2) to the struck unit. */
    unsigned hit_damage=WORLD_WORM_DAMAGE-1u+next_contact_random(e)%3u;damage+=hit_damage;chain_contact_source(e,hit_damage);
   }
   continue;
  }
  if(e->kind==WORLD_KIND_BOSS&&e->subtype==WORLD_BOSS_GOBLIN_KING&&
     e->hp>0&&e->boss_charge_active&&!e->boss_charge_timer&&!e->boss_charge_hit){
   float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
   if(px<ex+14.0f&&px+pw>ex&&py<ey+12.0f&&py+ph>ey){
    e->boss_charge_hit=1;damage+=WORLD_GOBLIN_KING_DAMAGE;chain_contact_source(e,WORLD_GOBLIN_KING_DAMAGE);
   }
   continue;
  }
  if(e->kind!=24||e->hp<=0)continue;
  float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
  float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
  if(e->subtype==10&&!e->contact_cooldown&&px<ex+body_w&&px+pw>ex&&py<ey+body_h&&py+ph>ey){
   e->contact_cooldown=30;unsigned hit_damage=1u+next_contact_random(e)%6u;damage+=hit_damage;chain_contact_source(e,hit_damage);
  }else if(e->subtype==14&&!e->contact_cooldown&&e->spider_jumping&&
           fabsf(e->vx)>=0.2f&&px<ex+body_w&&px+pw>ex&&py<ey+body_h&&py+ph>ey){
   e->contact_cooldown=30;unsigned hit_damage=5u+next_contact_random(e)%6u;damage+=hit_damage;chain_contact_source(e,hit_damage);
   }else if(e->subtype==11&&e->lurker_attack_frames&&!e->lurker_hit&&
           px<ex+body_w&&px+pw>ex&&py<ey+body_h&&py+ph>ey){
   e->lurker_hit=1;e->lurker_recovery=32;
   unsigned hit_damage=4u+next_contact_random(e)%13u;damage+=hit_damage;chain_contact_source(e,hit_damage);
  }else if(e->subtype==84&&e->thief_swing_frames&&!e->thief_hit&&
           py<ey+body_h&&py+ph>ey&&
           (e->thief_facing_right
             ?px<ex+body_w+9.0f&&px+pw>ex+body_w-1.0f
             :px<ex+1.0f&&px+pw>ex-9.0f)){
   e->thief_hit=1;unsigned hit_damage=1u+next_contact_random(e)%2u;damage+=hit_damage;chain_contact_source(e,hit_damage);
   }else if(e->subtype==12&&e->slime_lunge_frames&&!e->slime_lunge_hit&&
            px<ex+body_w&&px+pw>ex&&py<ey+body_h&&py+ph>ey){
    e->slime_lunge_hit=1;e->contact_cooldown=25;
    unsigned hit_damage=3u+next_contact_random(e)%3u;damage+=hit_damage;chain_contact_source(e,hit_damage);
   }else if(e->subtype==9&&!e->goblin_hit){
   const GoblinAttackFrame *frame=goblin_attack_frame(&w->goblin_attack,e);
   if(frame&&goblin_attack_hits_player(frame,e,body_w,body_h,px,py,pw,ph)){
    float multiplier=(frame->damage_min+frame->damage_max)*.5f;
    unsigned hit_damage=(unsigned)ceilf(e->goblin_attack_damage*multiplier);damage+=hit_damage;chain_contact_source(e,hit_damage);
    e->goblin_hit=1;
   }
  }
 }
 return damage;
}

/* BreakableWall starts with 14 HP. Physical melee hits set a 15-frame hurt
   timer; destroying one wall also destroys its orthogonally connected group
   and the corresponding destructible terrain cells (Ghidra: hurt/kill and
   Level::destroyTiles). */
unsigned world_breakable_walls_step(World *w,float px,float py,float pw,float ph,const MeleeState *attack){
 if(!w||!attack)return 0;
 unsigned removed=0;int tiles_changed=0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *wall=&w->entities[i];
  if(wall->kind!=21||wall->hp<=0)continue;
  if(wall->cooldown)wall->cooldown--;
  float x=wall->x_q8/256.0f,y=wall->y_q8/256.0f;
  float damage=attack->active?melee_damage_rect(attack,px+pw*.5f,py+ph,x,y,16.0f,16.0f):0.0f;
  if(damage<=0.0f||wall->cooldown||attack->serial==wall->hit_attack_serial)continue;
  wall->hp-=damage;wall->hit_attack_serial=attack->serial;wall->cooldown=15;
  if(wall->hp>0.0f)continue;

  /* BreakableWall::kill walks the set built by doCollision. Recreate its
     connected component from adjacent 16-pixel cells. */
  unsigned queue[WORLD_ENTITY_LIMIT],head=0,tail=0;
  wall->hp=0.0f;queue[tail++]=i;
  while(head<tail){
   WorldEntity *current=&w->entities[queue[head++]];
   /* The native body is 16.2 pixels wide and Level::destroyTiles receives
      its center tile; the constructor's 0.1-pixel alignment offset is
      included so positions such as 79.9 still resolve to tile 5. */
   int cx=(current->x_q8+WORLD_TILE_SIZE*128+26)/(WORLD_TILE_SIZE*256);
   int cy=(current->y_q8+WORLD_TILE_SIZE*128+26)/(WORLD_TILE_SIZE*256);
   int tx=cx,ty=cy;
   if(tx>=0&&ty>=0&&tx<w->width&&ty<w->height){
    WorldTile *tile=&w->tiles[ty*w->width+tx];
    if(tile->type==1||tile->type==6){tile->type=0;tile->solid=0;tile->platform=0;tile->layers[2]=-1;current->wall_tile_destroyed=1;tiles_changed=1;}
   }
   for(unsigned j=0;j<w->entity_count;j++){
    WorldEntity *next=&w->entities[j];if(next->kind!=21||next->hp<=0)continue;
    int nx=(next->x_q8+WORLD_TILE_SIZE*128+26)/(WORLD_TILE_SIZE*256);
    int ny=(next->y_q8+WORLD_TILE_SIZE*128+26)/(WORLD_TILE_SIZE*256);
    if((abs(nx-cx)+abs(ny-cy))>1)continue;
    next->hp=0.0f;
    if(tail<WORLD_ENTITY_LIMIT)queue[tail++]=j;
   }
   removed++;
  }
 }
 if(tiles_changed){world_collision_flags(w);world_auto_tiles(w);}
 return removed;
}

/*
 * Ghidra's x86 listing/PDB confirm ArrowTrap type, 5 px/tick horizontal
 * velocity, 5 physical damage, and a 6x6 Projectile body. The corridor
 * trigger is still an approximation of DartTrap::doCollision's actor checks.
 */
int world_dart_traps_step(World *w,float px,float py,float pw,float ph){
 int hit=0;
 const float projectile_size=6.0f;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=18)continue;
  if(e->cooldown)e->cooldown--;
  float tx=e->x_q8/256.0f,ty=e->y_q8/256.0f;
  if(!e->projectile_active){
   if((e->flags&WORLD_ENTITY_CAN_RELOAD)?(!e->fired_once&&e->cooldown==0):!e->fired_once){
    float player_center=px+pw*0.5f,trap_center=tx+8.0f;
    float distance=(e->flags&WORLD_ENTITY_FACING_LEFT)?trap_center-player_center:player_center-trap_center;
    if(distance<0||distance>96.0f||py+ph<ty||py>ty+16.0f)continue;
    e->projectile_active=1;e->projectile_direction=(e->flags&WORLD_ENTITY_FACING_LEFT)?-1:1;
    e->projectile_x_q8=(int32_t)lrintf((tx+(e->projectile_direction<0?-16.0f:16.0f))*256.0f);
    e->projectile_y_q8=(int32_t)lrintf((ty+6.0f)*256.0f);e->fired_once=1;
   }
   continue;
  }
  float old_x=e->projectile_x_q8/256.0f;
  float next_x=old_x+5.0f*e->projectile_direction;
  float y=e->projectile_y_q8/256.0f;
  int blocked=0;
  int min_x=(int)floorf(next_x/16.0f),max_x=(int)floorf((next_x+projectile_size-0.001f)/16.0f);
  int min_y=(int)floorf(y/16.0f),max_y=(int)floorf((y+projectile_size-0.001f)/16.0f);
  for(int ty=min_y;ty<=max_y&&!blocked;ty++)
   for(int tx=min_x;tx<=max_x;tx++)if(world_solid(w,tx,ty)){blocked=1;break;}
  if(blocked){e->projectile_active=0;if(e->flags&WORLD_ENTITY_CAN_RELOAD){e->cooldown=130;e->fired_once=0;}continue;}
  e->projectile_x_q8=(int32_t)lrintf(next_x*256.0f);
  if(next_x+projectile_size<0||next_x>=w->width*16){e->projectile_active=0;if(e->flags&WORLD_ENTITY_CAN_RELOAD){e->cooldown=130;e->fired_once=0;}continue;}
  if(next_x<px+pw&&next_x+projectile_size>px&&y<py+ph&&y+projectile_size>py){
   hit=1;e->projectile_active=0;if(e->flags&WORLD_ENTITY_CAN_RELOAD){e->cooldown=130;e->fired_once=0;}else e->fired_once=1;
  }
 }
 return hit;
}

/*
 * BoulderTrap::step/trigger from the local Ghidra output. The plate starts
 * down, rises after its 60-frame reset delay, and only records its first
 * activation when it was previously up. It records the linked Boulder's
 * release signal on the sixth step after activation. The linked square
 * Boulder is released into the world simulation when that signal occurs.
 */
unsigned world_boulder_traps_step(World *w,float px,float py,float pw,float ph){
 unsigned triggered=0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=20)continue;
  if(e->boulder_triggered&&!e->boulder_release_ready&&e->boulder_release_frames<6){
   e->boulder_release_frames++;
   if(e->boulder_release_frames==6){e->boulder_release_ready=1;if(e->boulder_present){e->boulder_released=1;e->boulder_vy=.32f;}}
  }
  float tx=e->x_q8/256.0f,ty=e->y_q8/256.0f;
  int overlap=px<tx+16.0f&&px+pw>tx&&py<ty+16.0f&&py+ph>ty;
  unsigned initialized_now=0;
  if(!e->boulder_initialized){e->boulder_initialized=1;e->boulder_plate_down=1;e->cooldown=60;initialized_now=1;}
  unsigned was_down=e->boulder_plate_down!=0;
  if(!initialized_now&&e->cooldown){e->cooldown--;if(!e->cooldown)e->boulder_plate_down=0;}
  if(overlap){
   if(!e->boulder_triggered&&!was_down){e->boulder_triggered=1;e->boulder_release_frames=0;e->boulder_release_ready=0;triggered++;}
   e->boulder_plate_down=1;
  }else if(!e->cooldown&&!e->boulder_triggered)e->boulder_plate_down=0;
 }
 return triggered;
}

/* Boulder::release sets square velocity to 2*g; Boulder::step adds g each
   frame (Ghidra: 0.16 fallback). Resolve its 16x16 body against solid tiles. */
void world_boulders_step(World *w){
 const float gravity=.16f,terminal=12.0f,size=16.0f;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if((e->kind!=20&&e->kind!=22)||!e->boulder_present)continue;
  if(e->kind==22&&!e->boulder_released){
   if(e->cooldown){e->cooldown--;continue;}
   e->boulder_released=1;e->boulder_vy=.32f;continue;
  }
  if(!e->boulder_released||e->boulder_landed)continue;
  float x=e->boulder_x_q8/256.0f,y=e->boulder_y_q8/256.0f;
  if(e->boulder_vy<0)e->boulder_vy=0;
  e->boulder_vy+=gravity;if(e->boulder_vy>terminal)e->boulder_vy=terminal;
  float next=y+e->boulder_vy;int min_x=(int)floorf(x/16.0f),max_x=(int)floorf((x+size-.001f)/16.0f);
  int row=(int)floorf((next+size-.001f)/16.0f),hit=0;
  for(int tx=min_x;tx<=max_x;tx++)if(world_solid(w,tx,row)){hit=1;break;}
  if(hit){y=row*16.0f-size;e->boulder_vy=0;e->boulder_landed=1;}else y=next;
  e->boulder_y_q8=(int32_t)lrintf(y*256.0f);
 }
}
