#include "scene.h"
#include "resources.h"
static void blit(uint32_t *p,int i,int x,int y,int width){
 Texture *t=&textures[i];if(!t->rgba)return;
 if(width>t->w)width=t->w;
 for(int sy=0;sy<(i==8?80:t->h);sy++)for(int sx=0;sx<width;sx++){
  int dx=x+sx,dy=y+sy;if(dx<0||dx>=640||dy<0||dy>=360)continue;
  const unsigned char *s=t->rgba+4*(sy*t->w+sx);unsigned a=s[3];if(!a)continue;
  uint32_t d=p[dy*640+dx];unsigned r=((d&255)*(255-a)+s[0]*a)/255;
  unsigned g=(((d>>8)&255)*(255-a)+s[1]*a)/255,b=(((d>>16)&255)*(255-a)+s[2]*a)/255;
  p[dy*640+dx]=0xff000000|r|g<<8|b<<16;
 }
}
void draw_scene(uint32_t *p){
 for(int i=0;i<640*360;i++)p[i]=0xff20170f;
 for(int i=0;i<8;i++){
  if(i==2){blit(p,i,3,45,textures[i].w);continue;}
  blit(p,i,0,0,textures[i].w);blit(p,i,400,0,textures[i].w);
 }
 blit(p,8,94,200,128);
 blit(p,9,(640-textures[9].w)/2,39,textures[9].w);
}
