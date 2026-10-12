#include "bow.h"
#include "world.h"
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
static void put16(FILE *f,unsigned v){unsigned char b[2]={v&255,(v>>8)&255};assert(fwrite(b,1,2,f)==2);}
static void putf(FILE *f,float v){uint32_t u;memcpy(&u,&v,4);unsigned char b[4]={u&255,(u>>8)&255,(u>>16)&255,u>>24};assert(fwrite(b,1,4,f)==4);}
static void packet(const char *path){
 FILE *f=fopen(path,"wb");assert(f);assert(fwrite("VWB1",1,4,f)==4);
 put16(f,1);put16(f,3);put16(f,2);put16(f,25);put16(f,2);put16(f,0);
 const float values[]={0.35f,0.29f,0.6f,0.48f,-1,3,7};for(int i=0;i<7;i++)putf(f,values[i]);
 for(int i=0;i<3;i++){put16(f,i);put16(f,1);const float k[]={1,0,0,0,1,-6,12,0,1,1};for(int j=0;j<10;j++)putf(f,k[j]);}
 assert(fclose(f)==0);
}
int main(void){
 const char *path="bow-packet-test.bin";packet(path);BowDefinition d;memset(&d,0,sizeof(d));assert(bow_load(&d,path,NULL));BowDefinition old=d;
 FILE *f=fopen(path,"r+b");assert(f);assert(fseek(f,32,SEEK_SET)==0);putf(f,NAN);assert(fclose(f)==0);assert(!bow_load(&d,path,NULL)&&!memcmp(&old,&d,sizeof(d)));
 packet(path);f=fopen(path,"ab");assert(f);assert(fputc(0,f)!=EOF);assert(fclose(f)==0);assert(!bow_load(&d,path,NULL));
 f=fopen(path,"wb");assert(f);assert(fwrite("VWB1",1,4,f)==4);assert(fclose(f)==0);assert(!bow_load(&d,path,NULL));assert(remove(path)==0);
 BowState s;memset(&s,0,sizeof(s));assert(!bow_start(&s,&d,0,1,0));assert(bow_start(&s,&d,0,1,1));assert(!bow_start(&s,&d,0,1,1));assert(fabsf(bow_projectile_speed(&s)-0.1f)<0.0001f);
 bow_step(&s,1);bow_step(&s,1);assert(s.keyframe==1&&s.timer==2&&fabsf(bow_projectile_speed(&s)-4)<0.001f);
 for(int i=0;i<40;i++)assert(!bow_step(&s,1));assert(s.keyframe==1&&s.timer==0&&fabsf(bow_projectile_speed(&s)-6)<0.001f);
 float x,y;bow_projectile_origin(&s,100,100,&x,&y);assert(fabsf(x-113)<0.001f&&fabsf(y-94)<0.001f);
 s.facing=-1;bow_projectile_origin(&s,100,100,&x,&y);assert(fabsf(x-87)<0.001f);
 assert(bow_step(&s,0)==1);for(int i=0;i<10;i++)assert(!bow_step(&s,0));assert(!s.active);
 World w;memset(&w,0,sizeof(w));w.ready=1;w.width=40;w.height=24;
 for(int ty=0;ty<24;ty++)w.tiles[ty*40+10].type=1;world_collision_flags(&w);
 ArrowPool p;memset(&p,0,sizeof(p));assert(arrow_launch(&p,&w,90,100,100,100,40,0,1));
 arrows_step(&p,&w);assert(p.arrows[0].active&&!p.arrows[0].stuck);arrows_step(&p,&w);
 assert(p.arrows[0].stuck&&p.arrows[0].x+1<=160.01f&&p.impacts==1);
 for(int i=0;i<120;i++)arrows_step(&p,&w);assert(!p.arrows[0].active&&p.expired==1);
 memset(&p,0,sizeof(p));assert(arrow_launch(&p,&w,157,100,170,100,10,0,1));assert(p.arrows[0].stuck&&p.arrows[0].x+1<=160.01f);
 memset(&p,0,sizeof(p));for(int i=0;i<ARROW_LIMIT;i++)assert(arrow_launch(&p,&w,90,100,100,100,1,0,1));
 ArrowPool before=p;assert(!arrow_launch(&p,&w,90,100,100,100,1,0,1)&&!memcmp(&before,&p,sizeof(p)));
 assert(!arrow_launch(&p,&w,90,100,NAN,100,1,0,1));
 return 0;
}
