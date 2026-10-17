#include "world.h"
#include "melee.h"
#include <math.h>
#include <stdlib.h>

void world_enemy_body(uint8_t subtype,float *width,float *height){
 switch(subtype){
  case 9:*width=7.0f;*height=12.0f;break;  /* Goblin::Goblin */
  case 10:*width=9.0f;*height=10.0f;break; /* Bat::Bat */
  case 11:*width=10.0f;*height=10.0f;break;/* Lurker::Lurker */
  case 12:*width=10.0f;*height=7.0f;break; /* Slime::Slime */
  default:*width=12.0f;*height=12.0f;break;
 }
}

static uint32_t next_contact_random(WorldEntity *e){
 uint32_t x=e->contact_rng_state;
 if(!x){x=e->source_index^((uint32_t)e->x_q8*0x9e3779b9u)^((uint32_t)e->y_q8*0x85ebca6bu)^0xBA7C0111u;if(!x)x=0x6d2b79f5u;}
 x^=x<<13;x^=x>>17;x^=x<<5;e->contact_rng_state=x;return x;
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

void world_enemies_step(World *w,float px,float py,float pw,float ph,const MeleeState *attack){
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=24||e->hp<=0)continue;
  if(e->cooldown)e->cooldown--;
  if(e->contact_cooldown)e->contact_cooldown--;
  float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
  float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
  float dx=px+pw*.5f-(ex+body_w*.5f),dy=py+ph*.5f-(ey+body_h*.5f);
  if(e->subtype==9)goblin_attack_step(w,e,dx,dy);
  float damage=attack&&attack->active?melee_damage_rect(attack,px+pw*.5f,py+ph,ex,ey,body_w,body_h):0;
  if(damage>0&&!e->cooldown&&attack->serial!=e->hit_attack_serial){
   e->hp-=damage;e->hit_attack_serial=attack->serial;e->cooldown=12;if(e->hp<=0){e->hp=0;continue;}
  }
  if(e->subtype==9&&(e->goblin_prep_timer||e->goblin_swing_frame))e->vx*=.8f;
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

/* Bat::doCollision rolls 1..6 and gates contact for 30 frames. Goblin
 * instead uses the private goblin-stab polygon after its 16-frame tell;
 * the attack script is converted into the external VGA1 package. */
unsigned world_enemies_contact_damage(World *w,float px,float py,float pw,float ph){
 unsigned damage=0;
 for(unsigned i=0;i<w->entity_count;i++){
  WorldEntity *e=&w->entities[i];if(e->kind!=24||e->hp<=0)continue;
  float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
  float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
  if(e->subtype==10&&!e->contact_cooldown&&px<ex+body_w&&px+pw>ex&&py<ey+body_h&&py+ph>ey){
   e->contact_cooldown=30;damage+=1u+next_contact_random(e)%6u;
  }else if(e->subtype==9&&!e->goblin_hit){
   const GoblinAttackFrame *frame=goblin_attack_frame(&w->goblin_attack,e);
   if(frame&&goblin_attack_hits_player(frame,e,body_w,body_h,px,py,pw,ph)){
    float multiplier=(frame->damage_min+frame->damage_max)*.5f;
    damage+=(unsigned)ceilf(e->goblin_attack_damage*multiplier);
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
