#include "scene.h"
#include "resources.h"
#include <math.h>
static void blit(uint32_t *p,int i,int x,int y,int sx0,int sy0,int width,int height,unsigned tint,unsigned alpha,int scale){
 Texture *t=&textures[i];if(!t->rgba||scale<1)return;
 for(int sy=0;sy<height;sy++)for(int sx=0;sx<width;sx++){
  if(sx0+sx<0||sx0+sx>=t->w||sy0+sy<0||sy0+sy>=t->h)continue;
  const unsigned char *s=t->rgba+4*((sy0+sy)*t->w+sx0+sx);
  unsigned a=s[3]*alpha/255;if(!a)continue;
  unsigned r=s[0]*(tint&255)/255,g=s[1]*((tint>>8)&255)/255,b=s[2]*((tint>>16)&255)/255;
  for(int yy=0;yy<scale;yy++)for(int xx=0;xx<scale;xx++){
   int dx=x+sx*scale+xx,dy=y+sy*scale+yy;if(dx<0||dx>=640||dy<0||dy>=360)continue;
   uint32_t *d=&p[dy*640+dx];
   if(a==255){*d=0xff000000|r|g<<8|b<<16;continue;}
   unsigned rr=((*d&255)*(255-a)+r*a)/255;
   unsigned gg=(((*d>>8)&255)*(255-a)+g*a)/255,bb=(((*d>>16)&255)*(255-a)+b*a)/255;
   *d=0xff000000|rr|gg<<8|bb<<16;
  }
 }
}
static void layer(uint32_t *p,int i,float offset,unsigned tint,unsigned alpha){
 int x=(int)fmodf(offset,400.0f)-400;
 for(;x<640;x+=400)blit(p,i,x,0,0,0,textures[i].w,textures[i].h,tint,alpha,1);
}
static void rect(uint32_t *p,int x,int y,int w,int h,uint32_t color){
 for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)if(xx>=0&&xx<640&&yy>=0&&yy<360)p[yy*640+xx]=color;
}
void draw_animated_scene(uint32_t *p,const Menu *m){
 /* Measured time is mapped to 60 logic ticks. Exact original tween/day state is pending. */
 float move=(float)(m->total_ticks%240000),night=0.75f+0.25f*sinf((float)(m->total_ticks%24000)*0.0005f);
 for(int i=0;i<640*360;i++)p[i]=0xff20170f;
 layer(p,0,0,0xffffff,255);layer(p,1,0,0xffffff,(unsigned)(night*255));
 blit(p,2,3,45,0,0,128,128,0xffd0b0,(unsigned)(night*230),1);
 layer(p,3,move*0.0035f+move*0.005f,0xc0c0c0,120);
 layer(p,4,move*0.004f,0x9ab0b0,255);
 layer(p,5,move*0.19f,0x90aaaa,255);
 layer(p,6,move*0.3f+22,0xb0c0c0,255);
 unsigned f=(unsigned)(m->total_ticks/7)%8;
 int x=130+(int)(2*sinf(move*0.07f));
 blit(p,8,x,210+(f==2?1:0),(f%4)*128,(f/4)*80,128,80,0xffffff,255,1);
 blit(p,10,520+(int)fmodf(move*0.3f,400)-400,259,0,0,32,32,0xffffff,255,1);
 layer(p,7,move*0.6f,0x90aaaa,255);
 for(int i=0;i<12;i++){
  int fx=(int)(i*61+move*0.3f+12*sinf(move*0.013f+i))%680-20;
  int fy=180+i%4*33+(int)(10*sinf(move*0.02f+i*2));
  unsigned pulse=(unsigned)(120+100*sinf(move*0.05f+i));
  unsigned ff=(unsigned)(m->total_ticks/8+i)%4;
  blit(p,21,fx,fy,ff*16,0,16,16,0xffffff,pulse,1);
 }
 if(m->screen==MENU_TITLE||m->screen==MENU_FADE||m->screen==MENU_NOTICE){
  blit(p,9,(640-textures[9].w)/2,39,0,0,textures[9].w,textures[9].h,0xffffff,255,1);
 }
 if(m->screen==MENU_CLASS||m->screen==MENU_BLOCKED){
  /* Original class atlas cells are 64x64. Simplified native layout, base classes only. */
  rect(p,80,24,480,280,0xff20170f);
  for(int c=0;c<3;c++){
   int cx=112+c*144;
   rect(p,cx-8,76,112,160,c==m->player_class?0xff607d88:0xff40535c);
   blit(p,11,cx-16,68,0,0,136,106,0xffffff,255,1);
   blit(p,12,cx+10,81,(c%2)*64,(c/2)*64,64,64,0xc0c0c0,255,1);
   blit(p,13,cx+10,140,(c%2)*64,(c/2)*64,64,64,0xffffff,255,1);
   unsigned frame=(unsigned)(m->total_ticks/4)%8;
   int sprite=(m->screen==MENU_BLOCKED?15:18)+c;
   Texture *t=&textures[sprite];int cols=t->w/24;
   if(cols>0)blit(p,sprite,cx+15,172,(frame%cols)*24,(frame/cols)*24,24,24,0xffffff,255,3);
  }
  blit(p,14,90,133,0,0,16,16,0xffffff,255,1);
  blit(p,14,526,133,16,0,16,16,0xffffff,255,1);
 }
 if(m->screen==MENU_FADE){
  unsigned a=(unsigned)(90-m->ticks)*255/90;
  for(int i=0;i<640*360;i++){uint32_t d=p[i];p[i]=0xff000000|((d&255)*a/255)|((((d>>8)&255)*a/255)<<8)|((((d>>16)&255)*a/255)<<16);}
 }
}
void draw_scene(uint32_t *p){Menu m={0};draw_animated_scene(p,&m);}
