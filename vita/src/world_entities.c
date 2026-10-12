#include "world.h"
#include <string.h>
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
int world_load_entities(World *w,const char *path,FILE *log){
 unsigned char h[8],r[8];WorldEntity next[WORLD_ENTITY_LIMIT];
 FILE *f=fopen(path,"rb");if(!f){if(log)fprintf(log,"WORLD_ENTITIES ready=0 error=open path=%s\n",path);return 0;}
 if(!w->ready||fread(h,1,8,f)!=8||memcmp(h,"VWE1",4)||u16(h+6)!=0)goto bad;
 unsigned count=u16(h+4);if(count>WORLD_ENTITY_LIMIT)goto bad;
 for(unsigned i=0;i<count;i++){
  if(fread(r,1,8,f)!=8||u16(r+6)!=0)goto bad;
  unsigned x=u16(r+2),y=u16(r+4);
  if(x>=(unsigned)w->width*16||y>=(unsigned)w->height*16)goto bad;
  if(r[0]==1){if(r[1]!=3&&r[1]!=11&&r[1]!=12&&r[1]!=13&&r[1]!=15)goto bad;}
  else if(r[0]!=2||r[1]!=0)goto bad;
  next[i]=(WorldEntity){r[0],r[1],(int16_t)x,(int16_t)y};
 }
 if(fgetc(f)!=EOF||ferror(f))goto bad;
 fclose(f);memcpy(w->entities,next,count*sizeof(*next));w->entity_count=count;
 if(log)fprintf(log,"WORLD_ENTITIES ready=1 count=%u marker_sprites=0 trophy_unlock_pending=1\n",count);
 return 1;
 bad:fclose(f);if(log)fprintf(log,"WORLD_ENTITIES ready=0 error=invalid_packet path=%s\n",path);return 0;
}
