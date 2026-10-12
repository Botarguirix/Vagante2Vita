#include "bow.h"
#include "world.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static float f32(const unsigned char *p){uint32_t u=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;float f;memcpy(&f,&u,4);return f;}
static int bounded(float v,float lo,float hi){return isfinite(v)&&v>=lo&&v<=hi;}
int bow_load(BowDefinition *d,const char *path,FILE *log){
 FILE *f=fopen(path,"rb");if(!f){if(log)fprintf(log,"BOW_DATA ready=0 error=open path=%s\n",path);return 0;}
 unsigned char h[44];BowDefinition next;memset(&next,0,sizeof(next));
 if(fread(h,1,44,f)!=44||memcmp(h,"VWB1",4)||u16(h+4)!=1||u16(h+14))goto bad;
 next.count=u16(h+6);next.draw_frames=u16(h+8);next.sprite=u16(h+10);next.duration=u16(h+12);
 next.anchor_x=f32(h+16);next.anchor_y=f32(h+20);next.arm_x=f32(h+24);next.arm_y=f32(h+28);next.velocity=f32(h+32);next.damage_min=f32(h+36);next.damage_max=f32(h+40);
 if(next.count<3||next.count>BOW_FRAMES||next.draw_frames<1||next.draw_frames>=next.count||next.sprite>=144||next.duration<1||next.duration>120||!bounded(next.anchor_x,0,1)||!bounded(next.anchor_y,0,1)||!bounded(next.arm_x,0,1)||!bounded(next.arm_y,0,1)||!bounded(next.velocity,-1,64)||!bounded(next.damage_min,0,100)||!bounded(next.damage_max,next.damage_min,100))goto bad;
 for(unsigned i=0;i<next.count;i++){
  if(fread(h,1,44,f)!=44||u16(h)>=9||u16(h+2)<1||u16(h+2)>120)goto bad;
  BowFrame *k=&next.frames[i];k->sprite=u16(h);k->duration_factor=f32(h+4);k->x=f32(h+8);k->y=f32(h+12);k->degrees=f32(h+16);k->aim_x=f32(h+20);k->aim_y=f32(h+24);k->projectile_x=f32(h+28);k->projectile_y=f32(h+32);k->scale_x=f32(h+36);k->scale_y=f32(h+40);
  if(!bounded(k->duration_factor,0.001f,8)||!bounded(k->x,-64,64)||!bounded(k->y,-64,64)||!bounded(k->degrees,-360,360)||!bounded(k->aim_x,-64,64)||!bounded(k->aim_y,-64,64)||!bounded(k->projectile_x,-64,64)||!bounded(k->projectile_y,-64,64)||!bounded(k->scale_x,0.1f,4)||!bounded(k->scale_y,0.1f,4))goto bad;
 }
 if(fgetc(f)!=EOF||ferror(f))goto bad;
 fclose(f);next.ready=1;*d=next;
 if(log)fprintf(log,"BOW_DATA ready=1 keyframes=%u draw_frames=%u\n",next.count,next.draw_frames);
 return 1;
 bad:fclose(f);if(log)fprintf(log,"BOW_DATA ready=0 error=invalid_packet path=%s\n",path);return 0;
}
static unsigned duration(const BowState *s){float n=s->definition->duration*s->definition->frames[s->keyframe].duration_factor*(1-s->attack_speed);return n<1?1:(unsigned)n;}
int bow_start(BowState *s,const BowDefinition *d,float speed,int facing,int ammo){
 if(s->active||!d||!d->ready||ammo<1||!isfinite(speed))return 0;
 unsigned serial=s->serial+1;memset(s,0,sizeof(*s));s->serial=serial;s->definition=d;s->active=1;s->attack_speed=speed;s->facing=facing<0?-1:1;s->timer=duration(s);return 1;
}
float bow_projectile_speed(const BowState *s){
 if(!s->active||!s->definition)return 0;
 if(s->definition->velocity>=0)return s->definition->velocity;
 if(!s->keyframe)return 0.1f;
 return 2.0f*((float)(s->keyframe+1)-(float)s->timer/(float)duration(s))+2.0f;
}
void bow_projectile_origin(const BowState *s,float cx,float bottom,float *x,float *y){
 const BowFrame *k=&s->definition->frames[s->keyframe];float c=cosf(s->angle),t=sinf(s->angle),flip=s->facing<0?-1.0f:1.0f;
 *x=cx+flip*(k->aim_x+c*k->projectile_x-t*k->projectile_y);*y=bottom+k->aim_y+t*k->projectile_x+c*k->projectile_y;
}
int bow_step(BowState *s,int held){
 if(!s->active)return 0;
 if(s->keyframe<s->definition->draw_frames&&!held){s->keyframe=s->definition->draw_frames;s->timer=duration(s);return 1;}
 if(s->timer)s->timer--;
 if(s->timer)return 0;
 if(s->keyframe+1==s->definition->draw_frames&&held)return 0;
 s->keyframe++;
 if(s->keyframe>=s->definition->count){s->active=0;return 0;}
 s->timer=duration(s);return 0;
}
static int arrow_solid(const World *w,float x,float y){
 int left=(int)floorf((x-1)/16),right=(int)floorf((x+0.999f)/16),top=(int)floorf((y-1)/16),bottom=(int)floorf((y+0.999f)/16);
 for(int ty=top;ty<=bottom;ty++)for(int tx=left;tx<=right;tx++)if(world_solid(w,tx,ty))return 1;
 return 0;
}
static void sweep(NativeArrow *a,const World *w,float dx,float dy){
 int n=(int)ceilf(fmaxf(fabsf(dx),fabsf(dy))*2);if(n<1)return;float sx=dx/n,sy=dy/n;
 for(int i=0;i<n;i++){
  float x=a->x+sx,y=a->y+sy;
  if(arrow_solid(w,x,y)){a->stuck=1;a->vx=a->vy=0;a->life=120;return;}
  a->x=x;a->y=y;
 }
}
int arrow_launch(ArrowPool *p,const World *w,float cx,float cy,float x,float y,float vx,float vy,float damage){
 if(!isfinite(cx)||!isfinite(cy)||!isfinite(x)||!isfinite(y)||!isfinite(vx)||!isfinite(vy)||fabsf(vx)>64||fabsf(vy)>64||fabsf(x-cx)>128||fabsf(y-cy)>128||!bounded(damage,0,1))return 0;
 for(unsigned i=0;i<ARROW_LIMIT;i++)if(!p->arrows[i].active){
  NativeArrow a;memset(&a,0,sizeof(a));a.active=1;a.serial=p->fired+1;a.x=cx;a.y=cy;a.vx=vx;a.vy=vy;a.facing=vx<0?-1:1;a.angle=atan2f(vy,fabsf(vx));a.damage_factor=damage;
  if(arrow_solid(w,cx,cy)){a.stuck=1;a.vx=a.vy=0;a.life=120;}else sweep(&a,w,x-cx,y-cy);
  p->arrows[i]=a;p->fired++;if(a.stuck)p->impacts++;return 1;
 }
 return 0;
}
void arrows_step(ArrowPool *p,const World *w){
 for(unsigned i=0;i<ARROW_LIMIT;i++){
  NativeArrow *a=&p->arrows[i];if(!a->active)continue;
  if(a->stuck){if(a->life)a->life--;if(!a->life){a->active=0;p->expired++;}continue;}
  a->vx*=0.99f;a->vy+=0.16f;a->facing=a->vx<0?-1:1;a->angle=atan2f(a->vy,fabsf(a->vx));
  sweep(a,w,a->vx,a->vy);if(a->stuck)p->impacts++;
 }
}
