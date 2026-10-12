#include "melee.h"
#include <math.h>
#include <string.h>
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static float f32(const unsigned char *p){uint32_t u=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;float f;memcpy(&f,&u,4);return f;}
static int finite_range(float v,float lo,float hi){return isfinite(v)&&v>=lo&&v<=hi;}
int melee_load(MeleeDatabase *db,const char *path,FILE *log){
 FILE *f=fopen(path,"rb");if(!f){if(log)fprintf(log,"MELEE_DATA ready=0 error=open path=%s\n",path);return 0;}
 unsigned char h[16],r[32];MeleeDatabase next;memset(&next,0,sizeof(next));
 if(fread(h,1,16,f)!=16||memcmp(h,"VWA1",4)||u16(h+4)!=1||u16(h+6)<1||u16(h+6)>MELEE_WEAPONS)goto bad;
 for(unsigned i=8;i<16;i++)if(h[i])goto bad;
 next.count=u16(h+6);
 for(unsigned n=0;n<next.count;n++){
  if(fread(r,1,20,f)!=20)goto bad;
  MeleeDefinition *d=&next.weapons[n];
  d->type=u16(r);d->sprite=u16(r+2);d->duration=u16(r+4);d->flags=u16(r+6);
  d->anchor_x=f32(r+8);d->anchor_y=f32(r+12);d->frame_count=u16(r+16);d->shape_count=u16(r+18);
  if((d->type!=49&&d->type!=45&&d->type!=79)||d->sprite>=144||d->duration<1||d->duration>120||!d->frame_count||d->frame_count>MELEE_FRAMES||d->shape_count>MELEE_SHAPES||!finite_range(d->anchor_x,0,1)||!finite_range(d->anchor_y,0,1))goto bad;
  for(unsigned j=0;j<n;j++)if(next.weapons[j].type==d->type)goto bad;
  for(unsigned i=0;i<d->frame_count;i++){
   if(fread(r,1,32,f)!=32||u16(r+2)<1||u16(r+2)>120||r[4]>1||r[5]>1||u16(r+6))goto bad;
   MeleeFrame *k=&d->frames[i];k->sprite=u16(r);k->enabled=r[4];k->charge=r[5];
   k->x=f32(r+8);k->y=f32(r+12);k->degrees=f32(r+16);k->duration_factor=f32(r+20);k->scale_x=f32(r+24);k->scale_y=f32(r+28);
   if(k->sprite>=9||!finite_range(k->x,-64,64)||!finite_range(k->y,-64,64)||!finite_range(k->degrees,-360,360)||!finite_range(k->duration_factor,0.001f,8)||!finite_range(k->scale_x,0.1f,4)||!finite_range(k->scale_y,0.1f,4))goto bad;
  }
  for(unsigned i=0;i<d->shape_count;i++){
   if(fread(r,1,16,f)!=16||u16(r)<3||u16(r)>MELEE_VERTICES||u16(r+2)>1)goto bad;
   MeleeShape *s=&d->shapes[i];s->count=u16(r);s->enabled=u16(r+2);s->damage_min=f32(r+4);s->damage_max=f32(r+8);s->knockback=f32(r+12);
   if(!finite_range(s->damage_min,0,100)||!finite_range(s->damage_max,s->damage_min,100)||!finite_range(s->knockback,0,10))goto bad;
   for(unsigned j=0;j<s->count;j++){
    if(fread(r,1,8,f)!=8)goto bad;
    s->vertices[j]=(MeleePoint){f32(r),f32(r+4)};
    if(!finite_range(s->vertices[j].x,-64,64)||!finite_range(s->vertices[j].y,-64,64))goto bad;
   }
  }
 }
 if(fgetc(f)!=EOF||ferror(f))goto bad;
 fclose(f);next.ready=1;*db=next;
 if(log)fprintf(log,"MELEE_DATA ready=1 weapons=%u script=player-stab damage_application_pending=1\n",next.count);
 return 1;
 bad:fclose(f);if(log)fprintf(log,"MELEE_DATA ready=0 error=invalid_packet path=%s\n",path);return 0;
}
const MeleeDefinition *melee_find(const MeleeDatabase *db,unsigned type){if(db->ready)for(unsigned i=0;i<db->count;i++)if(db->weapons[i].type==type)return &db->weapons[i];return NULL;}
static unsigned duration(const MeleeState *s){float t=s->definition->duration*s->definition->frames[s->keyframe].duration_factor*(1.0f-s->attack_speed);return t<1?1:(unsigned)t;}
int melee_start(MeleeState *s,const MeleeDefinition *d,float speed,int facing){
 if(s->active||!d||!d->frame_count||!isfinite(speed))return 0;
 unsigned serial=s->serial+1;memset(s,0,sizeof(*s));s->serial=serial;s->definition=d;s->active=1;s->facing=facing<0?-1:1;s->attack_speed=speed;s->timer=duration(s);return 1;
}
const MeleeFrame *melee_frame(const MeleeState *s){return s->active&&s->definition&&s->keyframe<s->definition->frame_count?&s->definition->frames[s->keyframe]:NULL;}
void melee_step(MeleeState *s,int held){
 const MeleeFrame *k=melee_frame(s);if(!k)return;s->elapsed++;if(s->timer)s->timer--;
 if(s->timer)return;
 if(k->charge&&(s->definition->flags&32)&&held){s->charge_ticks++;return;}
 s->keyframe++;if(s->keyframe>=s->definition->frame_count){s->active=0;return;}s->timer=duration(s);
}
unsigned melee_polygon(const MeleeState *s,unsigned shape,float cx,float bottom,MeleePoint *points){
 const MeleeFrame *k=melee_frame(s);if(!k||!k->enabled||shape>=s->definition->shape_count)return 0;
 const MeleeShape *p=&s->definition->shapes[shape];if(!p->enabled)return 0;
 float a=k->degrees*0.0174532925199433f,c=cosf(a),t=sinf(a),flip=s->facing<0?-1.0f:1.0f;
 for(unsigned i=0;i<p->count;i++){float x=p->vertices[i].x,y=p->vertices[i].y;points[i]=(MeleePoint){cx+flip*(k->x+c*x-t*y),bottom+k->y+t*x+c*y};}return p->count;
}
static int contained(const MeleePoint *p,unsigned n,float x,float y){int inside=0;for(unsigned i=0,j=n-1;i<n;j=i++){if((p[i].y>y)!=(p[j].y>y)&&x<(p[j].x-p[i].x)*(y-p[i].y)/(p[j].y-p[i].y)+p[i].x)inside=!inside;}return inside;}
static int segment(MeleePoint a,MeleePoint b,float x,float y,float w,float h){
 float delta[2]={b.x-a.x,b.y-a.y},origin[2]={a.x,a.y},lo[2]={x,y},hi[2]={x+w,y+h},first=0,last=1;
 for(int i=0;i<2;i++){
  if(fabsf(delta[i])<0.000001f){if(origin[i]<lo[i]||origin[i]>hi[i])return 0;}
  else {float t0=(lo[i]-origin[i])/delta[i],t1=(hi[i]-origin[i])/delta[i];if(t0>t1){float t=t0;t0=t1;t1=t;}
   if(t0>first)first=t0;
   if(t1<last)last=t1;
   if(first>last)return 0;}
 }return 1;
}
int melee_hits_rect(const MeleeState *s,float cx,float bottom,float x,float y,float w,float h){
 if(w<=0||h<=0||!melee_frame(s))return 0;
 for(unsigned j=0;j<s->definition->shape_count;j++){
  MeleePoint p[MELEE_VERTICES];unsigned n=melee_polygon(s,j,cx,bottom,p);if(!n)continue;
  if(contained(p,n,x,y)||contained(p,n,x+w,y)||contained(p,n,x,y+h)||contained(p,n,x+w,y+h))return 1;
  for(unsigned i=0;i<n;i++)if(segment(p[i],p[(i+1)%n],x,y,w,h))return 1;
 }return 0;
}
