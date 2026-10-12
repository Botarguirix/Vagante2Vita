#include "world_visuals.h"
#include "resources.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static Texture terrain,sprites[8],decor[3],attack_pose,weapon_atlas,bow_pose,bow_arm,arrow_sprite;
static int cached_class=-1;
static uint32_t terrain_full[640*384],terrain_low[320*192];
static void pixel(uint32_t *dst,const unsigned char *src);
void free_world_visuals(void){free_texture(&terrain);free_texture(&attack_pose);free_texture(&weapon_atlas);free_texture(&bow_pose);free_texture(&bow_arm);free_texture(&arrow_sprite);for(int i=0;i<8;i++)free_texture(&sprites[i]);for(int i=0;i<3;i++)free_texture(&decor[i]);cached_class=-1;}
int load_world_visuals(const char *directory,int player_class,FILE *log){
 if(player_class<0||player_class>=3)return 0;
 if(cached_class==player_class&&terrain.rgba)return 1;
 free_world_visuals();char path[256];
 snprintf(path,sizeof(path),"%s/intro-terrain.png",directory);
 if(!load_png_file(&terrain,path,log)||terrain.w!=640||terrain.h!=384)goto invalid;
 const char *classes[]={"knight","rogue","mage"};
 const char *names[]={"stand","walk","jump","fall","duck","climb","lookup"};
 for(int i=0;i<7;i++){
  snprintf(path,sizeof(path),"%s/%s/%s.png",directory,classes[player_class],names[i]);
  if(!load_png_file(&sprites[i],path,log)||sprites[i].w%24||sprites[i].h%24)goto invalid;
 }
 for(int y=0;y<384;y++)for(int x=0;x<640;x++){
  uint32_t color=0xff201810;pixel(&color,terrain.rgba+4*(y*640+x));
  terrain_full[y*640+x]=color;
  if(!(x&1)&&!(y&1))terrain_low[y/2*320+x/2]=color;
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
 snprintf(path,sizeof(path),"%s/%s/stab.png",directory,classes[player_class]);
 if(!load_png_file(&attack_pose,path,log)||attack_pose.w%24||attack_pose.h%24)free_texture(&attack_pose);
 snprintf(path,sizeof(path),"%s/items-weapons.png",directory);
 if(!load_png_file(&weapon_atlas,path,log)||weapon_atlas.w!=240||weapon_atlas.h!=240)free_texture(&weapon_atlas);
 if(player_class==1){
  snprintf(path,sizeof(path),"%s/rogue/aim.png",directory);
  if(!load_png_file(&bow_pose,path,log)||bow_pose.w%24||bow_pose.h%24)free_texture(&bow_pose);
  snprintf(path,sizeof(path),"%s/rogue/aim_arm.png",directory);
  if(!load_png_file(&bow_arm,path,log)||bow_arm.w!=24||bow_arm.h!=24)free_texture(&bow_arm);
  snprintf(path,sizeof(path),"%s/entities/dart.png",directory);
  if(!load_png_file(&arrow_sprite,path,log)||arrow_sprite.w!=16||arrow_sprite.h!=16)free_texture(&arrow_sprite);
 }
 cached_class=player_class;if(log)fprintf(log,"WORLD_VISUALS ready=1 class=%d terrain_cached=1 dynamic_lights=0\n",player_class);return 1;
 invalid:if(log)fprintf(log,"WORLD_VISUALS ready=0 error=missing_or_invalid_png\n");free_world_visuals();return 0;
}
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
 if(y1>=height)y1=height-1;if(x1>=width)x1=width-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  int sy=y*d-top,sx=x*d-left;if(sx<0||sy<0||sx>=size||sy>=size)continue;
  pixel(&p[y*width+x],t->rgba+4*((frame/cols*size+sy)*t->w+frame%cols*size+sx));
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
  if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;
 }
 int x0=(int)ceilf(minx/d),x1=(int)floorf(maxx/d),y0=(int)ceilf(miny/d),y1=(int)floorf(maxy/d);
 if(x0<0)x0=0;if(y0<0)y0=0;if(x1>=width)x1=width-1;if(y1>=height)y1=height-1;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  float dx=flip*(x*d-ox),dy=y*d-oy;
  int tx=(int)floorf((c*dx+s*dy)/scale_x+anchor_x*cell);
  int ty=(int)floorf((-s*dx+c*dy)/scale_y+anchor_y*cell);
  if(tx<0||ty<0||tx>=cell||ty>=cell)continue;
  int ax=(int)(frame%cols)*cell+tx,ay=(int)(frame/cols)*cell+ty;
  pixel(&p[y*width+x],t->rgba+4*(ay*t->w+ax));
 }
}
static void draw_equipment(uint32_t *p,const NativePlayer *player,int camera,int d,int width,int height){
 const MeleeFrame *k=melee_frame(&player->melee);const MeleeDefinition *weapon=player->melee.definition;
 if(k){
  float flip=player->facing<0?-1.0f:1.0f;
  draw_rotated(p,&weapon_atlas,20,weapon->sprite,player->x+player->w*0.5f+flip*k->x,player->y+player->h+k->y-camera,k->degrees*0.0174532925199433f,player->facing,k->scale_x,k->scale_y,weapon->anchor_x,weapon->anchor_y,d,width,height);
 }
 if(player->bow.active){
  const BowDefinition *b=player->bow.definition;const BowFrame *frame=&b->frames[player->bow.keyframe];
  float angle=player->bow.angle,c=cosf(angle),s=sinf(angle),flip=player->bow.facing<0?-1.0f:1.0f;
  float cx=player->x+player->w*0.5f,bottom=player->y+player->h-camera;
  draw_rotated(p,&bow_arm,24,0,cx+flip*frame->aim_x,bottom+frame->aim_y,angle,player->bow.facing,1,1,b->arm_x,b->arm_y,d,width,height);
  float ox=cx+flip*(frame->aim_x+c*frame->x-s*frame->y),oy=bottom+frame->aim_y+s*frame->x+c*frame->y;
  draw_rotated(p,&weapon_atlas,20,b->sprite,ox,oy,angle+frame->degrees*0.0174532925199433f,player->bow.facing,frame->scale_x,frame->scale_y,b->anchor_x,b->anchor_y,d,width,height);
 }
 for(unsigned i=0;i<ARROW_LIMIT;i++){
  const NativeArrow *a=&player->arrows.arrows[i];if(!a->active)continue;
  draw_rotated(p,&arrow_sprite,16,0,a->x,a->y-camera,a->angle,a->facing,1,1,0.75f,0.5f,d,width,height);
 }
}
void draw_world(uint32_t *p,const World *w,const NativePlayer *player,int reduction){
 int d=reduction==2?2:1,width=640/d,height=360/d;
 int camera=(int)(player->y-180);if(camera<0)camera=0;if(camera>24)camera=24;
 /* Quantize scrolling to logical pixels to preserve nearest-neighbor sampling. */
 camera=camera/d*d;
 if(terrain.rgba){
  const uint32_t *cached=d==2?terrain_low:terrain_full;
  memcpy(p,cached+(camera/d)*width,(size_t)width*height*sizeof(*p));
 }else for(int i=0;i<width*height;i++)p[i]=0xff201810;
 /* Rage markers have no Sprite in setTypeVars; trophy requires saved unlock.
    Preserve metadata, do not make invisible gameplay markers into fake art. */
 for(unsigned i=0;i<w->entity_count;i++){
  const WorldEntity *e=&w->entities[i];
  if(e->kind==1&&e->subtype==3)draw_prop(p,&decor[0],e->x,e->y-camera,24,0,d,width,height);
  else if(e->kind==1&&e->subtype==13)draw_prop(p,&decor[1],e->x,e->y-camera,16,player->ticks/5,d,width,height);
  else if(e->kind==2)draw_prop(p,&decor[2],e->x,e->y-camera,16,player->ticks/8+i,d,width,height);
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
 int left=(int)player->x-8,top=(int)(player->y+player->h)-24-camera;
 int y0=top>0?(top+d-1)/d:0,y1=(top+23)/d;
 int x0=left>0?(left+d-1)/d:0,x1=(left+23)/d;
 if(y1>=height)y1=height-1;if(x1>=width)x1=width-1;
 for(int y=y0;y<=y1;y++){
  int sy=y*d-top;if(sy<0||sy>=24)continue;
  for(int x=x0;x<=x1;x++){
   int sx=x*d-left;if(sx<0||sx>=24)continue;if(player->facing<0)sx=23-sx;
   pixel(&p[y*width+x],t->rgba+4*(((int)frame/cols*24+sy)*t->w+(int)frame%cols*24+sx));
  }
 }
 draw_equipment(p,player,camera,d,width,height);
}
