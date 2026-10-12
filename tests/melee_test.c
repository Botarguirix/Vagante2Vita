#include "melee.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void put16(FILE *f,unsigned v){unsigned char b[2]={v&255,(v>>8)&255};assert(fwrite(b,1,2,f)==2);}
static void putf(FILE *f,float value){uint32_t v;memcpy(&v,&value,4);unsigned char b[4]={v&255,(v>>8)&255,(v>>16)&255,v>>24};assert(fwrite(b,1,4,f)==4);}
static void packet(const char *path){
 FILE *f=fopen(path,"wb");assert(f);assert(fwrite("VWA1",1,4,f)==4);put16(f,1);put16(f,1);putf(f,0);putf(f,0);
 put16(f,49);put16(f,0);put16(f,2);put16(f,0);putf(f,0);putf(f,0);put16(f,1);put16(f,1);
 put16(f,0);put16(f,1);put16(f,1);put16(f,0);
 putf(f,0);putf(f,0);putf(f,0);putf(f,1);putf(f,1);putf(f,1);
 put16(f,3);put16(f,1);putf(f,1);putf(f,2);putf(f,0);
 putf(f,0);putf(f,0);putf(f,4);putf(f,0);putf(f,0);putf(f,4);assert(fclose(f)==0);
}
int main(void){
 const char *path="melee-packet-test.bin";packet(path);MeleeDatabase db;memset(&db,0,sizeof(db));
 assert(melee_load(&db,path,NULL)&&db.ready&&db.count==1&&melee_find(&db,49)&&!melee_find(&db,79));
 MeleeDatabase old=db;FILE *f=fopen(path,"r+b");assert(f);assert(fseek(f,24,SEEK_SET)==0);putf(f,NAN);assert(fclose(f)==0);
 assert(!melee_load(&db,path,NULL)&&!memcmp(&db,&old,sizeof(db)));
 packet(path);f=fopen(path,"ab");assert(f);assert(fputc(0,f)!=EOF);assert(fclose(f)==0);assert(!melee_load(&db,path,NULL));
 f=fopen(path,"wb");assert(f);assert(fwrite("VWA1",1,4,f)==4);assert(fclose(f)==0);assert(!melee_load(&db,path,NULL));
 packet(path);f=fopen(path,"r+b");assert(f);assert(fseek(f,6,SEEK_SET)==0);put16(f,4);assert(fclose(f)==0);assert(!melee_load(&db,path,NULL));
 assert(!memcmp(&db,&old,sizeof(db)));assert(remove(path)==0);

 MeleeDefinition d;memset(&d,0,sizeof(d));d.duration=2;d.frame_count=2;d.shape_count=1;
 d.frames[0].duration_factor=d.frames[1].duration_factor=1;d.frames[1].enabled=1;d.frames[1].x=2;d.frames[1].y=-4;
 d.shapes[0].enabled=1;d.shapes[0].count=4;
 d.shapes[0].vertices[0]=(MeleePoint){0,0};d.shapes[0].vertices[1]=(MeleePoint){4,0};d.shapes[0].vertices[2]=(MeleePoint){4,4};d.shapes[0].vertices[3]=(MeleePoint){0,4};
 MeleeState s;memset(&s,0,sizeof(s));assert(melee_start(&s,&d,0,1));assert(!melee_start(&s,&d,0,1));
 assert(!melee_hits_rect(&s,10,20,13,17,1,1));melee_step(&s,0);assert(s.keyframe==0);melee_step(&s,0);assert(s.keyframe==1);
 assert(melee_hits_rect(&s,10,20,13,17,1,1));assert(!melee_hits_rect(&s,10,20,5,17,1,1));
 s.facing=-1;assert(melee_hits_rect(&s,10,20,5,17,1,1));assert(!melee_hits_rect(&s,10,20,13,17,1,1));
 s.facing=1;d.frames[1].degrees=90;assert(melee_hits_rect(&s,10,20,9,17,1,1));assert(!melee_hits_rect(&s,10,20,13,17,1,1));
 melee_step(&s,0);melee_step(&s,0);assert(!s.active&&!melee_hits_rect(&s,10,20,9,17,1,1));
 d.frames[1].charge=1;d.flags=32;assert(melee_start(&s,&d,0,-1));
 for(int i=0;i<50;i++)melee_step(&s,1);
 assert(s.active&&s.keyframe==1&&s.charge_ticks>40);
 melee_step(&s,0);assert(!s.active);assert(melee_start(&s,&d,0.5f,1)&&s.timer==1);
 MeleeState before=s;assert(!melee_start(&s,NULL,0,1));assert(!memcmp(&before,&s,sizeof(s)));
 return 0;
}
