#include "input_replay.h"
#include <string.h>
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static unsigned u32(const unsigned char *p){return u16(p)|u16(p+2)<<16;}
int replay_load(InputReplay *r,const char *path,FILE *log){
 FILE *f=fopen(path,"rb");if(!f)return 0;
 InputReplay next;memset(&next,0,sizeof(next));unsigned char b[8];unsigned total=0;
 if(fread(b,1,8,f)!=8||memcmp(b,"VTS1",4)||u16(b+6))goto bad;
 next.count=u16(b+4);if(!next.count||next.count>REPLAY_LIMIT)goto bad;
 for(unsigned i=0;i<next.count;i++){
  if(fread(b,1,8,f)!=8||u16(b+6))goto bad;
  unsigned mask=u32(b),ticks=u16(b+4);
  if((mask&~0x0000f3f9u)||!ticks||ticks>600)goto bad;
  next.steps[i]=(ReplayStep){mask,ticks};total+=ticks;if(total>36000)goto bad;
 }
 if(fgetc(f)!=EOF||ferror(f))goto bad;
 fclose(f);next.active=1;*r=next;
 if(log)fprintf(log,"AUTOTEST enabled=1 steps=%u scheduled_ticks=%u source=external_replay physical_controller_verified=0\n",next.count,total);
 return 1;
 bad:fclose(f);if(log)fprintf(log,"AUTOTEST enabled=0 error=invalid_replay\n");return 0;
}
unsigned replay_buttons(const InputReplay *r){return r->active&&!r->done?r->steps[r->index].buttons:0;}
void replay_advance(InputReplay *r,unsigned ticks){
 if(!r->active||r->done||!ticks)return;
 while(ticks&&r->active){
  unsigned remaining=r->steps[r->index].ticks-r->elapsed;
  if(ticks<remaining){r->elapsed+=ticks;return;}
  ticks-=remaining;r->elapsed=0;
  if(++r->index==r->count){r->done=1;r->active=0;}
 }
}
