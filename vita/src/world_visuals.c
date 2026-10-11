#include "world_visuals.h"
#include "resources.h"
#include <stdio.h>
#include <string.h>
static Texture terrain,sprites[7];
static int cached_class=-1;
void free_world_visuals(void){free_texture(&terrain);for(int i=0;i<7;i++)free_texture(&sprites[i]);cached_class=-1;}
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
void draw_world(uint32_t *p,const World *w,const NativePlayer *player,int reduction){
 (void)w;int d=reduction==2?2:1,width=640/d,height=360/d;
 int camera=(int)(player->y-180);if(camera<0)camera=0;if(camera>24)camera=24;
 /* Quantize scrolling to logical pixels to preserve nearest-neighbor sampling. */
 camera=camera/d*d;
 for(int y=0;y<height;y++)for(int x=0;x<width;x++){
  p[y*width+x]=0xff201810;
  if(terrain.rgba)pixel(&p[y*width+x],terrain.rgba+4*((y*d+camera)*terrain.w+x*d));
 }
 Texture *t=&sprites[player->motion];if(!t->rgba)return;
 int cols=t->w/24,frames=cols*(t->h/24);if(frames>8)frames=8;if(frames<1)return;
 unsigned frame=(player->ticks/7)%(unsigned)frames;
 if(player->motion==PLAYER_RISE||player->motion==PLAYER_FALL||player->motion==PLAYER_DUCK)frame=0;
 int left=(int)player->x-8,top=(int)(player->y+player->h)-24-camera;
 for(int y=0;y<height;y++){
  int sy=y*d-top;if(sy<0||sy>=24)continue;
  for(int x=0;x<width;x++){
   int sx=x*d-left;if(sx<0||sx>=24)continue;if(player->facing<0)sx=23-sx;
   pixel(&p[y*width+x],t->rgba+4*(((int)frame/cols*24+sy)*t->w+(int)frame%cols*24+sx));
  }
 }
}
