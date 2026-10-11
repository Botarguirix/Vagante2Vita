#include "scene.h"
#include "resources.h"
#include <math.h>
#include <string.h>
static int divisor=1,render_width=640,render_height=360;
static int ceil_div(int n,int d){return n>=0?(n+d-1)/d:n/d;}
static void blit(uint32_t *p,int i,int x,int y,int sx0,int sy0,int width,int height,unsigned tint,unsigned alpha,int scale){
 Texture *t=&textures[i];if(!t->rgba||scale<1)return;
 int left=ceil_div(x,divisor),top=ceil_div(y,divisor);
 int right=ceil_div(x+width*scale,divisor),bottom=ceil_div(y+height*scale,divisor);
 if(left<0)left=0;if(top<0)top=0;if(right>render_width)right=render_width;if(bottom>render_height)bottom=render_height;
 for(int dy=top;dy<bottom;dy++){
  int sy=sy0+(dy*divisor-y)/scale;if(sy<0||sy>=t->h)continue;
  for(int dx=left;dx<right;dx++){
   int sx=sx0+(dx*divisor-x)/scale;if(sx<0||sx>=t->w)continue;
   const unsigned char *s=t->rgba+4*(sy*t->w+sx);unsigned a=s[3]*alpha/255;if(!a)continue;
   unsigned r=s[0]*(tint&255)/255,g=s[1]*((tint>>8)&255)/255,b=s[2]*((tint>>16)&255)/255;
   uint32_t *d=&p[dy*render_width+dx];
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
 int left=ceil_div(x,divisor),top=ceil_div(y,divisor),right=ceil_div(x+w,divisor),bottom=ceil_div(y+h,divisor);
 if(left<0)left=0;if(top<0)top=0;if(right>render_width)right=render_width;if(bottom>render_height)bottom=render_height;
 for(int yy=top;yy<bottom;yy++)for(int xx=left;xx<right;xx++)p[yy*render_width+xx]=color;
}
static void compose(uint32_t *p,const Menu *m,int reduction,int static_only){
 divisor=reduction==2?2:1;render_width=640/divisor;render_height=360/divisor;
 /* Measured time is mapped to 60 logic ticks. Exact original tween/day state is pending. */
 float move=(float)(m->total_ticks%240000),night=0.75f+0.25f*sinf((float)(m->total_ticks%24000)*0.0005f);
 for(int i=0;i<render_width*render_height;i++)p[i]=0xff20170f;
 layer(p,0,0,0xffffff,255);layer(p,1,0,0xffffff,(unsigned)(night*255));
 blit(p,2,3,45,0,0,128,128,0xffd0b0,(unsigned)(night*230),1);
 layer(p,3,move*0.0035f+move*0.005f,0xc0c0c0,120);
 layer(p,4,move*0.004f,0x9ab0b0,255);
 layer(p,5,move*0.19f,0x90aaaa,255);
 layer(p,6,move*0.3f+22,0xb0c0c0,255);
 if(!static_only){
 unsigned f=(unsigned)(m->total_ticks/7)%8;
 int x=130+(int)(2*sinf(move*0.07f));
 blit(p,8,x,210+(f==2?1:0),(f%4)*128,(f/4)*80,128,80,0xffffff,255,1);
 }
 blit(p,10,520+(int)fmodf(move*0.3f,400)-400,259,0,0,32,32,0xffffff,255,1);
 layer(p,7,move*0.6f,0x90aaaa,255);
 if(!static_only)for(int i=0;i<12;i++){
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
   if(!static_only){
   unsigned frame=(unsigned)(m->total_ticks/4)%8;
   int sprite=(m->screen==MENU_BLOCKED?15:18)+c;
   Texture *t=&textures[sprite];int cols=t->w/24;
   if(cols>0)blit(p,sprite,cx+15,172,(frame%cols)*24,(frame/cols)*24,24,24,0xffffff,255,3);
   }
  }
  blit(p,14,90,133,0,0,16,16,0xffffff,255,1);
  blit(p,14,526,133,16,0,16,16,0xffffff,255,1);
 }
 if(!static_only&&m->screen==MENU_FADE){
  unsigned a=(unsigned)(90-m->ticks)*255/90;
  for(int i=0;i<render_width*render_height;i++){uint32_t d=p[i];p[i]=0xff000000|((d&255)*a/255)|((((d>>8)&255)*a/255)<<8)|((((d>>16)&255)*a/255)<<16);}
 }
}
void draw_animated_scene(uint32_t *p,const Menu *m){draw_scene_resolution(p,m,1);}
void draw_scene(uint32_t *p){Menu m={0};draw_animated_scene(p,&m);}

/* One bounded RAM cache. Original textures remain intact; only scenery motion
   and decorative particles are disabled in the default lightweight profile. */
static uint32_t backdrop[640*360];
static int cache_valid,cache_divisor,cache_class;
static MenuScreen cache_screen;
static unsigned cache_rebuilds;
void scene_invalidate_cache(void){cache_valid=0;}
unsigned scene_cache_rebuilds(void){return cache_rebuilds;}
void draw_scene_resolution(uint32_t *p,const Menu *m,int reduction){compose(p,m,reduction,0);}
void draw_scene_profile(uint32_t *p,const Menu *m,int reduction,int lightweight){
 if(!lightweight){draw_scene_resolution(p,m,reduction);return;}
 Menu stable=*m;stable.total_ticks=0;stable.ticks=0;
 if(stable.screen==MENU_FADE)stable.screen=MENU_TITLE;
 int d=reduction==2?2:1;
 if(!cache_valid||cache_divisor!=d||cache_screen!=stable.screen||cache_class!=stable.player_class){
  compose(backdrop,&stable,reduction,1);
  cache_divisor=d;cache_screen=stable.screen;cache_class=stable.player_class;
  cache_valid=1;cache_rebuilds++;
 }
 divisor=d;render_width=640/d;render_height=360/d;
 memcpy(p,backdrop,(size_t)render_width*render_height*sizeof(*p));
 if(m->screen==MENU_CLASS||m->screen==MENU_BLOCKED){
  unsigned f=(unsigned)(m->total_ticks/4)%8;
  for(int c=0;c<3;c++){
   int sprite=(m->screen==MENU_BLOCKED?15:18)+c,cols=textures[sprite].w/24;
   if(cols>0)blit(p,sprite,127+c*144,172,(f%cols)*24,(f/cols)*24,24,24,0xffffff,255,3);
  }
 }else{
  unsigned f=(unsigned)(m->total_ticks/7)%8;
  blit(p,8,130,210+(f==2?1:0),(f%4)*128,(f/4)*80,128,80,0xffffff,255,1);
 }
 if(m->screen==MENU_FADE){
  unsigned a=(unsigned)(90-m->ticks)*255/90;
  for(int i=0;i<render_width*render_height;i++){
   uint32_t v=p[i];p[i]=0xff000000|((v&255)*a/255)|((((v>>8)&255)*a/255)<<8)|((((v>>16)&255)*a/255)<<16);
  }
 }
}
