#include "world.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
static World world,before;
static void u16(FILE *f,unsigned n){fputc(n&255,f);fputc((n>>8)&255,f);}
static void u32(FILE *f,unsigned n){u16(f,n);u16(f,n>>16);}
static void packet(const char *path,unsigned w,unsigned h,unsigned flags,unsigned count,unsigned ex,int blocked,int trailing){
 FILE *f=fopen(path,"wb");assert(f);fwrite("VWI2",1,4,f);u16(f,w);u16(f,h);u32(f,flags);u32(f,count);
 u16(f,ex);u16(f,1);u16(f,70);u16(f,38);u32(f,0);u32(f,0);
 for(unsigned i=0;i<count;i++){fputc(blocked&&i==w+ex?1:0,f);fputc(0,f);for(int j=0;j<5;j++)u16(f,65535);}
 if(trailing){fputc(1,f);}
 assert(!fclose(f));
}
int main(void){
 const char *path="world-extent-fixture.vwi";packet(path,80,40,0,3200,2,0,0);
 assert(world_load(&world,path,stdout));assert(world.width==80&&world.height==40&&world.entrance_x==2&&world.goal_x==70);
 assert(world_tile(&world,79,39)==&world.tiles[3199]);assert(!world_tile(&world,80,39));
 WorldCamera c=world_camera(&world,100,100);assert(c.x==0&&c.y==0);
 c=world_camera(&world,700,350);assert(c.x==380&&c.y==170);
 c=world_camera(&world,99999,99999);assert(c.x==640&&c.y==280);
 c=world_camera(&world,701,351);assert(c.x==380&&c.y==170);
 c=world_camera(&world,NAN,3);assert(c.x==0&&c.y==0);
 before=world;
 packet(path,80,40,8,3200,2,0,0);assert(!world_load(&world,path,NULL));assert(!memcmp(&world,&before,sizeof(world)));
 packet(path,80,40,0,3199,2,0,0);assert(!world_load(&world,path,NULL));
 packet(path,80,40,0,3200,80,0,0);assert(!world_load(&world,path,NULL));
 packet(path,80,40,0,3200,2,1,0);assert(!world_load(&world,path,NULL));
 packet(path,80,40,0,3200,2,0,1);assert(!world_load(&world,path,NULL));
 packet(path,128,128,0,16384,2,0,0);assert(!world_load(&world,path,NULL));
 assert(!memcmp(&world,&before,sizeof(world)));
 FILE *f=fopen(path,"wb");assert(f);fwrite("VWI2",1,4,f);fclose(f);assert(!world_load(&world,path,NULL));
 assert(!memcmp(&world,&before,sizeof(world)));remove(path);return 0;
}
