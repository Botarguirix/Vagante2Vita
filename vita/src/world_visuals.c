#include "world_visuals.h"
#include "resources.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
static Texture terrain,sprites[8],decor[3],attack_pose,weapon_atlas,bow_pose,bow_arm,arrow_sprite;
static Texture actone_chest,actone_spikes,actone_darttrap,actone_boulder_trap,actone_breakable_wall;
static Texture actone_boulder_blocker,actone_boulders,actone_shake_boulder;
static Texture loot_atlases[7];
enum { ENEMY_GOBLIN,ENEMY_BAT,ENEMY_LURKER,ENEMY_SLIME,ENEMY_FAMILY_COUNT };
enum { ENEMY_STAND,ENEMY_MOVE,ENEMY_AIR,ENEMY_HURT,ENEMY_ATTACK,ENEMY_STATE_COUNT };
static Texture enemy_sprites[ENEMY_FAMILY_COUNT][ENEMY_STATE_COUNT];
static const char *enemy_folders[ENEMY_FAMILY_COUNT]={"goblin","bat","lurker","slime"};
static const char *enemy_names[ENEMY_FAMILY_COUNT][ENEMY_STATE_COUNT]={
 {"stand.png","move.png","air.png","hurt.png","attack.png"},
 {"stand.png","move.png","air.png","hurt.png","attack.png"},
 {"stand.png","move.png","air.png","hurt.png","attack.png"},
 {"stand.png","move.png","air.png","hurt.png","attack.png"}
};
static const char *loot_atlas_names[7]={"items.png","items-weapons.png","items-armor.png","potions.png","scrolls.png","spell_books.png","items-explosives.png"};
static Texture actone_doodads[22];
static const char *actone_doodad_names[22]={
 [1]="counter.png",[2]="shop_sign.png",[3]="torch.png",[4]="decorative_helmet.png",
 [5]="decorative_shield.png",[13]="flower_1.png",[14]="spellbook_altar.png",
 [19]="waterfall_1_2.png",[20]="waterfall_1_3.png",[21]="waterfall_1_4.png"
};
static int cached_class=-1;
static char cached_directory[256];
static char cached_terrain_file[128];
static uint32_t *terrain_full,*terrain_low;
static void pixel(uint32_t *dst,const unsigned char *src);
void free_world_visuals(void){free(terrain_low);terrain_low=NULL;terrain_full=NULL;free_texture(&terrain);free_texture(&attack_pose);free_texture(&weapon_atlas);free_texture(&bow_pose);free_texture(&bow_arm);free_texture(&arrow_sprite);free_texture(&actone_chest);free_texture(&actone_spikes);free_texture(&actone_darttrap);free_texture(&actone_boulder_trap);free_texture(&actone_breakable_wall);free_texture(&actone_boulder_blocker);free_texture(&actone_boulders);free_texture(&actone_shake_boulder);for(int i=0;i<ENEMY_FAMILY_COUNT;i++)for(int j=0;j<ENEMY_STATE_COUNT;j++)free_texture(&enemy_sprites[i][j]);for(int i=0;i<7;i++)free_texture(&loot_atlases[i]);for(int i=0;i<22;i++)free_texture(&actone_doodads[i]);for(int i=0;i<8;i++)free_texture(&sprites[i]);for(int i=0;i<3;i++)free_texture(&decor[i]);cached_class=-1;cached_directory[0]=0;cached_terrain_file[0]=0;}
int load_world_visuals_for_terrain(const char *directory,const char *terrain_file,int player_class,const World *world,FILE *log){
 if(!directory||!terrain_file||!terrain_file[0]||terrain_file[0]=='/'||strstr(terrain_file,"..")||player_class<0||player_class>=3||!world->ready||strlen(directory)>=sizeof(cached_directory)||strlen(terrain_file)>=sizeof(cached_terrain_file)||strlen(directory)+1+strlen(terrain_file)>=256)return 0;
 if(cached_class==player_class&&terrain.rgba&&!strcmp(cached_directory,directory)&&!strcmp(cached_terrain_file,terrain_file)&&terrain.w==world->width*16&&terrain.h==world->height*16)return 1;
 free_world_visuals();char path[256];
 snprintf(path,sizeof(path),"%s/%s",directory,terrain_file);
 if(!load_png_file(&terrain,path,log)||terrain.w!=world->width*16||terrain.h!=world->height*16)goto invalid;
 const char *classes[]={"knight","rogue","mage"};
 const char *names[]={"stand","walk","jump","fall","duck","climb","lookup"};
 for(int i=0;i<7;i++){
  snprintf(path,sizeof(path),"%s/%s/%s.png",directory,classes[player_class],names[i]);
  if(!load_png_file(&sprites[i],path,log)||sprites[i].w%24||sprites[i].h%24)goto invalid;
 }
 /* Reuse decoded RGBA storage for opaque terrain, avoiding a second full map. */
 terrain_full=(uint32_t *)terrain.rgba;
 terrain_low=malloc((size_t)(terrain.w/2)*(terrain.h/2)*sizeof(*terrain_low));if(!terrain_low)goto invalid;
 for(int y=0;y<terrain.h;y++)for(int x=0;x<terrain.w;x++){
  uint32_t color=0xff201810;pixel(&color,terrain.rgba+4*(y*terrain.w+x));
  terrain_full[y*terrain.w+x]=color;
  if(!(x&1)&&!(y&1))terrain_low[y/2*(terrain.w/2)+x/2]=color;
 }
 snprintf(path,sizeof(path),"%s/%s/hang.png",directory,classes[player_class]);
 if(!load_png_file(&sprites[7],path,log)||sprites[7].w%24||sprites[7].h%24){
  free_texture(&sprites[7]);if(log)fprintf(log,"WORLD_OPTIONAL hang_sprite=0 fallback=climb\n");
 }
 const char *props[]={"torch","flower_1","little_firefly"};
 for(int i=0;i<3;i++){
  snprintf(path,sizeof(path),"%s/entities/%s.png",directory,props[i]);
  if(!load_png_file(&decor[i],path,log)){if(log)fprintf(log,"WORLD_OPTIONAL entity_sprite=%s ready=0\n",props[i]);}
 }
 /* ACT_ONE art is optional per entity type. Only decode doodad atlases used by this floor. */
 snprintf(path,sizeof(path),"%s/entities/chest.png",directory);
 if(!load_png_file(&actone_chest,path,log))free_texture(&actone_chest);
 snprintf(path,sizeof(path),"%s/entities/spikes.png",directory);
 if(!load_png_file(&actone_spikes,path,log))free_texture(&actone_spikes);
 snprintf(path,sizeof(path),"%s/entities/arrow_trap.png",directory);
 if(!load_png_file(&actone_darttrap,path,log))free_texture(&actone_darttrap);
 snprintf(path,sizeof(path),"%s/entities/trap_trigger.png",directory);
 if(!load_png_file(&actone_boulder_trap,path,log)||actone_boulder_trap.w!=32||actone_boulder_trap.h!=32)free_texture(&actone_boulder_trap);
 snprintf(path,sizeof(path),"%s/entities/breakable_wall.png",directory);
 if(!load_png_file(&actone_breakable_wall,path,log)||actone_breakable_wall.w!=16||actone_breakable_wall.h!=16)free_texture(&actone_breakable_wall);
 snprintf(path,sizeof(path),"%s/entities/boulder_blocker.png",directory);
 if(!load_png_file(&actone_boulder_blocker,path,log)||actone_boulder_blocker.w!=16||actone_boulder_blocker.h!=16)free_texture(&actone_boulder_blocker);
 snprintf(path,sizeof(path),"%s/entities/boulders.png",directory);
 if(!load_png_file(&actone_boulders,path,log)||actone_boulders.w!=16||actone_boulders.h!=16)free_texture(&actone_boulders);
 snprintf(path,sizeof(path),"%s/entities/shake_boulder.png",directory);
 if(!load_png_file(&actone_shake_boulder,path,log)||actone_shake_boulder.w!=16||actone_shake_boulder.h!=16)free_texture(&actone_shake_boulder);
 unsigned enemy_families=0;
 for(unsigned i=0;i<world->entity_count;i++)if(world->entities[i].kind==24&&world->entities[i].subtype>=9&&world->entities[i].subtype<=12)
  enemy_families|=1u<<(world->entities[i].subtype-9);
 for(int family=0;family<ENEMY_FAMILY_COUNT;family++)if(enemy_families&(1u<<family)){
  for(int state=0;state<ENEMY_STATE_COUNT;state++){
   snprintf(path,sizeof(path),"%s/enemies/%s/%s",directory,enemy_folders[family],enemy_names[family][state]);
   if(!load_png_file(&enemy_sprites[family][state],path,log)||enemy_sprites[family][state].w%16||
      (family==ENEMY_GOBLIN?enemy_sprites[family][state].h!=17:enemy_sprites[family][state].h%16))goto invalid;
  }
 }
 for(unsigned i=0;i<world->entity_count;i++){
  unsigned v=world->entities[i].variant;
  if(v>=22||!actone_doodad_names[v]||actone_doodads[v].rgba)continue;
  snprintf(path,sizeof(path),"%s/entities/doodads/%s",directory,actone_doodad_names[v]);
  if(!load_png_file(&actone_doodads[v],path,log))free_texture(&actone_doodads[v]);
 }
 snprintf(path,sizeof(path),"%s/entities/dart.png",directory);
 if(!load_png_file(&arrow_sprite,path,log)||arrow_sprite.w!=16||arrow_sprite.h!=16)free_texture(&arrow_sprite);
 snprintf(path,sizeof(path),"%s/%s/stab.png",directory,classes[player_class]);
 if(!load_png_file(&attack_pose,path,log)||attack_pose.w%24||attack_pose.h%24)free_texture(&attack_pose);
 snprintf(path,sizeof(path),"%s/items-weapons.png",directory);
 if(!load_png_file(&weapon_atlas,path,log)||weapon_atlas.w!=240||weapon_atlas.h!=240)free_texture(&weapon_atlas);
 if(world->loot.enabled)for(int i=0;i<7;i++){
  snprintf(path,sizeof(path),"%s/items/%s",directory,loot_atlas_names[i]);
  if(!load_png_file(&loot_atlases[i],path,log))goto invalid;
  if((i==0||i==3||i==5)&&(loot_atlases[i].w%16||loot_atlases[i].h%16))goto invalid;
  if((i==1||i==2||i==4||i==6)&&(loot_atlases[i].w%20||loot_atlases[i].h%20))goto invalid;
 }
 if(player_class==1){
  snprintf(path,sizeof(path),"%s/rogue/aim.png",directory);
  if(!load_png_file(&bow_pose,path,log)||bow_pose.w%24||bow_pose.h%24)free_texture(&bow_pose);
  snprintf(path,sizeof(path),"%s/rogue/aim_arm.png",directory);
  if(!load_png_file(&bow_arm,path,log)||bow_arm.w!=24||bow_arm.h!=24)free_texture(&bow_arm);
 }
 cached_class=player_class;strcpy(cached_directory,directory);strcpy(cached_terrain_file,terrain_file);if(log)fprintf(log,"WORLD_VISUALS ready=1 class=%d terrain=%s dynamic_lights=0\n",player_class,terrain_file);return 1;
 invalid:if(log)fprintf(log,"WORLD_VISUALS ready=0 error=missing_or_invalid_png\n");free_world_visuals();return 0;
}
int load_world_visuals(const char *directory,int player_class,const World *world,FILE *log){return load_world_visuals_for_terrain(directory,"intro-terrain.png",player_class,world,log);}
static void pixel(uint32_t *dst,const unsigned char *src){
 unsigned a=src[3];if(!a)return;
 if(a==255){*dst=0xff000000|src[0]|src[1]<<8|src[2]<<16;return;}
 unsigned r=((*dst&255)*(255-a)+src[0]*a)/255;
 unsigned g=(((*dst>>8)&255)*(255-a)+src[1]*a)/255;
 unsigned b=(((*dst>>16)&255)*(255-a)+src[2]*a)/255;
 *dst=0xff000000|r|g<<8|b<<16;
}
static void draw_prop(uint32_t *p,const Texture *t,int left,int top,int size,unsigned frame,int d,int width,int height){
 if(!t->rgba||t->w<size||t->h<size)return;
 int cols=t->w/size,frames=cols*(t->h/size);frame%=frames;
 int y0=top>0?(top+d-1)/d:0,y1=(top+size-1)/d;
 int x0=left>0?(left+d-1)/d:0,x1=(left+size-1)/d;
 if(y1>=height)y1=height-1;
 if(x1>=width)x1=width-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  int sy=y*d-top,sx=x*d-left;if(sx<0||sy<0||sx>=size||sy>=size)continue;
  pixel(&p[y*width+x],t->rgba+4*((frame/cols*size+sy)*t->w+frame%cols*size+sx));
 }
}
static void draw_prop_flipped(uint32_t *p,const Texture *t,int left,int top,int size,unsigned frame,int flip_x,int d,int width,int height){
 if(!flip_x){draw_prop(p,t,left,top,size,frame,d,width,height);return;}
 if(!t->rgba||t->w<size||t->h<size)return;
 int cols=t->w/size,frames=cols*(t->h/size);frame%=frames;
 int y0=top>0?(top+d-1)/d:0,y1=(top+size-1)/d;
 int x0=left>0?(left+d-1)/d:0,x1=(left+size-1)/d;
 if(y1>=height)y1=height-1;
 if(x1>=width)x1=width-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  int sy=y*d-top,sx=x*d-left;if(sx<0||sy<0||sx>=size||sy>=size)continue;
  int source_x=flip_x?size-1-sx:sx;
  pixel(&p[y*width+x],t->rgba+4*((frame/cols*size+sy)*t->w+frame%cols*size+source_x));
 }
}
static void draw_enemy_frame(uint32_t *p,const Texture *t,int left,int top,int cell_w,int cell_h,unsigned frame,int flip_x,int d,int width,int height){
 if(!t->rgba||cell_w<=0||cell_h<=0||t->w<cell_w||t->h<cell_h)return;
 int cols=t->w/cell_w,rows=t->h/cell_h,frames=cols*rows;frame%=frames;
 int x0=left>0?(left+d-1)/d:0,y0=top>0?(top+d-1)/d:0;
 int x1=(left+cell_w-1)/d,y1=(top+cell_h-1)/d;
 if(x1>=width)x1=width-1;
 if(y1>=height)y1=height-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  int sx=x*d-left,sy=y*d-top;if(sx<0||sy<0||sx>=cell_w||sy>=cell_h)continue;
  int source_x=flip_x?cell_w-1-sx:sx;
  int atlas_x=(int)(frame%(unsigned)cols)*cell_w+source_x;
  int atlas_y=(int)(frame/(unsigned)cols)*cell_h+sy;
  pixel(&p[y*width+x],t->rgba+4*(atlas_y*t->w+atlas_x));
 }
}
static void draw_sprite_rect(uint32_t *p,const Texture *t,int left,int top,int cell_w,int cell_h,unsigned frame,int d,int width,int height){
 if(!t->rgba||t->w<cell_w||t->h<cell_h)return;
 int cols=t->w/cell_w,frames=cols*(t->h/cell_h);frame%=frames;
 int y0=top>0?(top+d-1)/d:0,y1=(top+cell_h-1)/d;
 int x0=left>0?(left+d-1)/d:0,x1=(left+cell_w-1)/d;
 if(y1>=height)y1=height-1;
 if(x1>=width)x1=width-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  int sy=y*d-top,sx=x*d-left;if(sx<0||sy<0||sx>=cell_w||sy>=cell_h)continue;
  pixel(&p[y*width+x],t->rgba+4*((frame/cols*cell_h+sy)*t->w+frame%cols*cell_w+sx));
 }
}
static void draw_rotated(uint32_t *p,const Texture *t,int cell,unsigned frame,float ox,float oy,float angle,int facing,float scale_x,float scale_y,float anchor_x,float anchor_y,int d,int width,int height){
 if(!t->rgba||t->w<cell||t->h<cell)return;
 unsigned cols=t->w/cell,cells=cols*(t->h/cell);if(frame>=cells)frame=cells-1;
 float c=cosf(angle),s=sinf(angle),flip=facing<0?-1.0f:1.0f;
 float minx=9999,miny=9999,maxx=-9999,maxy=-9999;
 for(int i=0;i<4;i++){
  float u=((i&1?(float)cell:0)-anchor_x*cell)*scale_x;
  float v=((i&2?(float)cell:0)-anchor_y*cell)*scale_y;
  float x=ox+flip*(c*u-s*v),y=oy+s*u+c*v;
  if(x<minx)minx=x;
  if(x>maxx)maxx=x;
  if(y<miny)miny=y;
  if(y>maxy)maxy=y;
 }
 int x0=(int)ceilf(minx/d),x1=(int)floorf(maxx/d),y0=(int)ceilf(miny/d),y1=(int)floorf(maxy/d);
 if(x0<0)x0=0;
 if(y0<0)y0=0;
 if(x1>=width)x1=width-1;
 if(y1>=height)y1=height-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  float dx=flip*(x*d-ox),dy=y*d-oy;
  int tx=(int)floorf((c*dx+s*dy)/scale_x+anchor_x*cell);
  int ty=(int)floorf((-s*dx+c*dy)/scale_y+anchor_y*cell);
  if(tx<0||ty<0||tx>=cell||ty>=cell)continue;
  int ax=(int)(frame%cols)*cell+tx,ay=(int)(frame/cols)*cell+ty;
  pixel(&p[y*width+x],t->rgba+4*(ay*t->w+ax));
 }
}
static void draw_equipment(uint32_t *p,const NativePlayer *player,WorldCamera camera,int d,int width,int height){
 const MeleeFrame *k=melee_frame(&player->melee);const MeleeDefinition *weapon=player->melee.definition;
 if(k){
  float flip=player->facing<0?-1.0f:1.0f;
  draw_rotated(p,&weapon_atlas,20,weapon->sprite,player->x+player->w*0.5f+flip*k->x-camera.x,player->y+player->h+k->y-camera.y,k->degrees*0.0174532925199433f,player->facing,k->scale_x,k->scale_y,weapon->anchor_x,weapon->anchor_y,d,width,height);
 }
 if(player->bow.active){
  const BowDefinition *b=player->bow.definition;const BowFrame *frame=&b->frames[player->bow.keyframe];
  float angle=player->bow.angle,c=cosf(angle),s=sinf(angle),flip=player->bow.facing<0?-1.0f:1.0f;
  float cx=player->x+player->w*0.5f-camera.x,bottom=player->y+player->h-camera.y;
  draw_rotated(p,&bow_arm,24,0,cx+flip*frame->aim_x,bottom+frame->aim_y,angle,player->bow.facing,1,1,b->arm_x,b->arm_y,d,width,height);
  float ox=cx+flip*(frame->aim_x+c*frame->x-s*frame->y),oy=bottom+frame->aim_y+s*frame->x+c*frame->y;
  draw_rotated(p,&weapon_atlas,20,b->sprite,ox,oy,angle+frame->degrees*0.0174532925199433f,player->bow.facing,frame->scale_x,frame->scale_y,b->anchor_x,b->anchor_y,d,width,height);
 }
 for(unsigned i=0;i<ARROW_LIMIT;i++){
  const NativeArrow *a=&player->arrows.arrows[i];if(!a->active)continue;
  draw_rotated(p,&arrow_sprite,16,0,a->x-camera.x,a->y-camera.y,a->angle,a->facing,1,1,0.75f,0.5f,d,width,height);
 }
}
void draw_world(uint32_t *p,const World *w,const NativePlayer *player,int reduction){
 int d=reduction==2?2:1,width=640/d,height=360/d;
 WorldCamera camera=world_camera(w,player->x+player->w*0.5f,player->y);
 /* Copy visible rows only; cache dimensions belong to the loaded terrain. */
 for(int y=0;y<height;y++){
  int sy=y*d+camera.y;
  uint32_t *row=p+y*width;
  int available=(terrain.w-camera.x)/d;if(available>width)available=width;
  if(terrain_full&&sy<terrain.h&&available>0){
   const uint32_t *cached=d==2?terrain_low:terrain_full;
   memcpy(row,cached+(sy/d)*(terrain.w/d)+camera.x/d,(size_t)available*sizeof(*row));
  }else available=0;
  if(available<0)available=0;
  for(int x=available;x<width;x++)row[x]=0xff201810;
 }
 /* Terrain is supplied as a precomposed image. A destroyed BreakableWall
    tile cannot be recomposed from its source layers at runtime, so expose the
    cave base color in that cell before drawing remaining entities. */
 for(unsigned i=0;i<w->entity_count;i++){
  const WorldEntity *e=&w->entities[i];if(e->kind!=21||!e->wall_tile_destroyed)continue;
  int tile_x=(e->x_q8+WORLD_TILE_SIZE*128+26)/(WORLD_TILE_SIZE*256);
  int tile_y=(e->y_q8+WORLD_TILE_SIZE*128+26)/(WORLD_TILE_SIZE*256);
  int left=(tile_x*WORLD_TILE_SIZE-camera.x)/d;
  int top=(tile_y*WORLD_TILE_SIZE-camera.y)/d;
  int size=16/d;
  for(int yy=0;yy<size;yy++)for(int xx=0;xx<size;xx++){
   int sx=left+xx,sy=top+yy;if(sx>=0&&sx<width&&sy>=0&&sy<height)p[sy*width+sx]=0xff201810;
  }
 }
 /* Rage markers have no Sprite in setTypeVars; trophy requires saved unlock.
    Preserve metadata, do not make invisible gameplay markers into fake art. */
 for(unsigned i=0;i<w->entity_count;i++){
  const WorldEntity *e=&w->entities[i];
  if(e->kind==25&&!(e->flags&WORLD_ENTITY_PICKED_UP)){
   int left=(int)(e->x_q8/256)-camera.x,top=(int)(e->y_q8/256)-camera.y;
   if(e->item_atlas<7&&e->item_sprite!=0xffff){int cell=(e->item_atlas==0||e->item_atlas==3||e->item_atlas==5)?16:20;draw_sprite_rect(p,&loot_atlases[e->item_atlas],left,top,cell,cell,e->item_sprite,d,width,height);}
   else {int x=left/d,y=top/d;if(x>=0&&x<width&&y>=0&&y<height)p[y*width+x]=0xffffd040;}
  }
  if(e->kind==24&&e->hp>0){
   if(e->subtype>=9&&e->subtype<=12){
    unsigned family=(unsigned)(e->subtype-9);float ex=e->x_q8/256.0f,ey=e->y_q8/256.0f;
    float body_w,body_h;world_enemy_body(e->subtype,&body_w,&body_h);
    int support=(int)floorf((ey+body_h+1.0f)/16.0f);
    int tile_x=(int)floorf((ex+body_w*.5f)/16.0f);
    int airborne=!world_solid(w,tile_x,support);
    unsigned state=ENEMY_STAND;
    if(e->cooldown&&e->hit_attack_serial)state=ENEMY_HURT;
    else if(e->subtype==9&&(e->goblin_prep_timer||e->goblin_swing_frame))state=ENEMY_ATTACK;
    else if(airborne)state=ENEMY_AIR;
    else if(fabsf(e->vx)>.04f)state=ENEMY_MOVE;
    const Texture *sprite=&enemy_sprites[family][state];
    int cell_h=family==ENEMY_GOBLIN?17:16;
    unsigned frame_index=(unsigned)(player->ticks/6+e->source_index*3);
    if(state==ENEMY_STAND||state==ENEMY_HURT)frame_index=0;
    if(state==ENEMY_ATTACK)frame_index=e->goblin_swing_frame?e->goblin_swing_frame/5:0;
    int left=(int)lrintf(ex-camera.x),top=(int)lrintf(ey-camera.y);
    int flip=family==ENEMY_GOBLIN?!e->goblin_facing_right:e->vx<-.01f;
    draw_enemy_frame(p,sprite,left,top,16,cell_h,frame_index,flip,d,width,height);
   }else{
    uint32_t color=0xffff00ff;int left=(e->x-camera.x)/d,top=(e->y-camera.y)/d;
    if(left>=0&&left<width&&top>=0&&top<height)p[top*width+left]=color;
   }
  }
  if(e->kind==16){draw_prop(p,&actone_chest,e->x-camera.x,e->y-camera.y,16,(e->flags&WORLD_ENTITY_OPENED)?1:0,d,width,height);}
  else if(e->kind==17){draw_prop(p,&actone_spikes,e->x-camera.x,e->y-camera.y,16,0,d,width,height);}
  else if(e->kind==21&&e->hp>0.0f){draw_prop(p,&actone_breakable_wall,e->x-camera.x,e->y-camera.y,16,0,d,width,height);}
  else if(e->kind==18){
   draw_prop_flipped(p,&actone_darttrap,e->x-camera.x,e->y-camera.y,16,0,(e->flags&WORLD_ENTITY_FACING_LEFT)!=0,d,width,height);
   if(e->projectile_active)draw_rotated(p,&arrow_sprite,16,0,e->projectile_x_q8/256.0f-camera.x,e->projectile_y_q8/256.0f-camera.y,0.0f,e->projectile_direction,1,1,0.75f,0.5f,d,width,height);
  }
  else if(e->kind==20){
   draw_prop(p,&actone_boulder_trap,e->x-camera.x,e->y-camera.y,16,e->boulder_plate_down?1:0,d,width,height);
   if(e->boulder_present)draw_prop(p,&actone_boulders,e->boulder_x_q8/256-camera.x,e->boulder_y_q8/256-camera.y,16,0,d,width,height);
  }
  else if(e->kind==22&&e->boulder_present){
   const Texture *boulder=e->boulder_released?&actone_boulders:&actone_shake_boulder;
   draw_prop(p,boulder,e->boulder_x_q8/256-camera.x,e->boulder_y_q8/256-camera.y,16,0,d,width,height);
  }
  else if(e->kind==23){
   /* BoulderBlocker::draw anchors its 16x16 sprite at the entity origin and
      flips when facingLeft is false (confirmed in the original binary). */
   draw_prop_flipped(p,&actone_boulder_blocker,e->x-camera.x,e->y-camera.y,16,0,
                     !(e->flags&WORLD_ENTITY_FACING_LEFT),d,width,height);
  }
  else if(e->kind==19&&e->variant<22&&actone_doodad_names[e->variant]){
   const Texture *t=&actone_doodads[e->variant];int cw=16,ch=16;
   if(e->variant==1){cw=32;ch=32;}else if(e->variant==2){cw=32;ch=16;}
   else if(e->variant==3){cw=24;ch=24;}else if(e->variant==14){cw=48;ch=48;}
   else if(e->variant>=19){cw=80;ch=(e->variant==19?32:e->variant==20?48:64);}
   draw_sprite_rect(p,t,e->x-camera.x,e->y-camera.y,cw,ch,player->ticks/8,d,width,height);
  }
  else if(e->kind==1&&e->subtype==3)draw_prop(p,&decor[0],e->x-camera.x,e->y-camera.y,24,0,d,width,height);
  else if(e->kind==1&&e->subtype==13)draw_prop(p,&decor[1],e->x-camera.x,e->y-camera.y,16,player->ticks/5,d,width,height);
  else if(e->kind==2)draw_prop(p,&decor[2],e->x-camera.x,e->y-camera.y,16,player->ticks/8+i,d,width,height);
 }
 Texture *t=&sprites[player->motion];if(player->motion==PLAYER_HANG&&!t->rgba)t=&sprites[PLAYER_CLIMB];
 const MeleeFrame *attack=melee_frame(&player->melee);if(attack&&attack_pose.rgba)t=&attack_pose;
 if(player->bow.active&&bow_pose.rgba)t=&bow_pose;
 if(!t->rgba)return;
 int cols=t->w/24,frames=cols*(t->h/24);if(frames>8)frames=8;if(frames<1)return;
 unsigned frame=(player->ticks/7)%(unsigned)frames;
 if(player->motion==PLAYER_RISE||player->motion==PLAYER_FALL||player->motion==PLAYER_DUCK)frame=0;
 if(attack&&t==&attack_pose)frame=attack->sprite;
 if(player->bow.active&&t==&bow_pose)frame=player->bow.definition->frames[player->bow.keyframe].sprite;
 if(frame>=(unsigned)(cols*(t->h/24)))frame=(unsigned)(cols*(t->h/24))-1;
 int left=(int)player->x-8-camera.x,top=(int)(player->y+player->h)-24-camera.y;
 int y0=top>0?(top+d-1)/d:0,y1=(top+23)/d;
 int x0=left>0?(left+d-1)/d:0,x1=(left+23)/d;
 if(y1>=height)y1=height-1;
 if(x1>=width)x1=width-1;
 for(int y=y0;y<=y1;y++){
  int sy=y*d-top;if(sy<0||sy>=24)continue;
  for(int x=x0;x<=x1;x++){
   int sx=x*d-left;if(sx<0||sx>=24)continue;if(player->facing<0)sx=23-sx;
   pixel(&p[y*width+x],t->rgba+4*(((int)frame/cols*24+sy)*t->w+(int)frame%cols*24+sx));
  }
 }
 draw_equipment(p,player,camera,d,width,height);
}
