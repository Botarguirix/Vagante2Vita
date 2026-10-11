#include "world_visuals.h"
#include "resources.h"
#include <stdio.h>
#include <string.h>
static Texture terrain,sprites[7];
static int cached_class=-1;
static uint32_t terrain_full[640*384],terrain_low[320*192];
static void pixel(uint32_t *dst,const unsigned char *src);
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
 for(int y=0;y<384;y++)for(int x=0;x<640;x++){
  uint32_t color=0xff201810;pixel(&color,terrain.rgba+4*(y*640+x));
  terrain_full[y*640+x]=color;
  if(!(x&1)&&!(y&1))terrain_low[y/2*320+x/2]=color;
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
 if(terrain.rgba){
  const uint32_t *cached=d==2?terrain_low:terrain_full;
  memcpy(p,cached+(camera/d)*width,(size_t)width*height*sizeof(*p));
 }else for(int i=0;i<width*height;i++)p[i]=0xff201810;
 Texture *t=&sprites[player->motion];if(!t->rgba)return;
 int cols=t->w/24,frames=cols*(t->h/24);if(frames>8)frames=8;if(frames<1)return;
 unsigned frame=(player->ticks/7)%(unsigned)frames;
 if(player->motion==PLAYER_RISE||player->motion==PLAYER_FALL||player->motion==PLAYER_DUCK)frame=0;
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
}
