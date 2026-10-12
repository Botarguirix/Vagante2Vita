#include "world.h"
#include <assert.h>
#include <string.h>
static void write(FILE *f,const unsigned char *p,size_t n){rewind(f);assert(fwrite(p,1,n,f)==n);assert(fflush(f)==0);}
int main(void){
 World w;memset(&w,0,sizeof(w));w.ready=1;w.width=40;w.height=24;
 char path[]="entity-test-packet.bin";FILE *f=fopen(path,"wb");assert(f);
 unsigned char p[]={ 'V','W','E','1',1,0,0,0,1,3,233,0,55,1,0,0};
 write(f,p,sizeof(p));assert(world_load_entities(&w,path,NULL));assert(w.entity_count==1&&w.entities[0].x==233&&w.entities[0].y==311);
 World before=w;p[9]=99;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));assert(!memcmp(&w,&before,sizeof(w)));
 p[9]=3;p[4]=65;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));
 p[4]=2;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));
 p[4]=1;p[10]=255;p[11]=255;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));
 fclose(f);assert(remove(path)==0);return 0;
}
