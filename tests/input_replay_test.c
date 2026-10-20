#include "input_replay.h"
#include <assert.h>
#include <string.h>
static void packet(const char *path,unsigned mask,unsigned ticks,int extra){
 unsigned char b[16]={'V','T','S','1',1,0,0,0,mask&255,(mask>>8)&255,(mask>>16)&255,(mask>>24)&255,ticks&255,(ticks>>8)&255,0,0};
 FILE *f=fopen(path,"wb");assert(f);fwrite(b,1,16,f);if(extra){fputc(1,f);}fclose(f);
}
static void packet_two_steps(const char *path){
 unsigned char b[24]={'V','T','S','1',2,0,0,0,
  0x00,0x40,0,0,2,0,0,0,
  0x20,0,0,0,3,0,0,0};
 FILE *f=fopen(path,"wb");assert(f);assert(fwrite(b,1,sizeof(b),f)==sizeof(b));fclose(f);
}
int main(void){
 InputReplay r={0},before;const char *path="input-replay-fixture.vts";
 packet(path,0x4000,6,0);assert(replay_load(&r,path,NULL));assert(replay_buttons(&r)==0x4000);assert(!r.start_intro);
 replay_advance(&r,0);assert(!r.done);replay_advance(&r,5);assert(!r.done);replay_advance(&r,1);assert(r.done&&!r.active&&!replay_buttons(&r));
 before=r;packet(path,0x4000,0,0);assert(!replay_load(&r,path,NULL));assert(!memcmp(&r,&before,sizeof(r)));
 packet(path,0x4000,601,0);assert(!replay_load(&r,path,NULL));
 packet(path,0x4,6,0);assert(!replay_load(&r,path,NULL));
 packet_two_steps(path);assert(replay_load(&r,path,NULL));
 assert(replay_buttons(&r)==0x4000);replay_advance(&r,1);
 assert(!r.done&&r.index==0&&r.elapsed==1&&replay_buttons(&r)==0x4000);
 replay_advance(&r,1);assert(!r.done&&r.index==1&&r.elapsed==0);
 assert(replay_buttons(&r)==0x20);replay_advance(&r,2);
 assert(!r.done&&r.index==1&&r.elapsed==2&&replay_buttons(&r)==0x20);
 replay_advance(&r,1);assert(r.done&&!r.active&&!replay_buttons(&r));
 assert(replay_load(&r,path,NULL));replay_advance(&r,3);
 assert(!r.done&&r.index==1&&r.elapsed==1&&replay_buttons(&r)==0x20);
 replay_advance(&r,2);assert(r.done&&!r.active);
 before=r;
 packet(path,0x4000,6,1);assert(!replay_load(&r,path,NULL));
 packet(path,0x4000,6,0);FILE *flags=fopen(path,"r+b");assert(flags);assert(fseek(flags,6,SEEK_SET)==0);fputc(1,flags);fclose(flags);
 assert(replay_load(&r,path,NULL));assert(r.start_intro&&!r.stop_after_first_floor);before=r;
 flags=fopen(path,"r+b");assert(flags);assert(fseek(flags,6,SEEK_SET)==0);fputc(2,flags);fclose(flags);
 assert(replay_load(&r,path,NULL));assert(!r.start_intro&&r.stop_after_first_floor);
 flags=fopen(path,"r+b");assert(flags);assert(fseek(flags,6,SEEK_SET)==0);fputc(4,flags);fclose(flags);
 assert(replay_load(&r,path,NULL));assert(!r.start_intro&&!r.stop_after_first_floor&&r.fast_forward);before=r;
 flags=fopen(path,"r+b");assert(flags);assert(fseek(flags,6,SEEK_SET)==0);fputc(7,flags);fclose(flags);
 assert(replay_load(&r,path,NULL));assert(r.start_intro&&r.stop_after_first_floor&&r.fast_forward);before=r;
 flags=fopen(path,"r+b");assert(flags);assert(fseek(flags,6,SEEK_SET)==0);fputc(8,flags);fclose(flags);
 assert(!replay_load(&r,path,NULL));assert(!memcmp(&r,&before,sizeof(r)));
 FILE *f=fopen(path,"wb");fwrite("VTS1",1,4,f);fclose(f);assert(!replay_load(&r,path,NULL));
 assert(!memcmp(&r,&before,sizeof(r)));
 f=fopen(path,"wb");assert(f);fwrite("VTS1",1,4,f);unsigned char header[4]={1,2,0,0};fwrite(header,1,4,f);
 for(unsigned i=0;i<513;i++){unsigned char step[8]={0x00,0x40,0,0,1,0,0,0};fwrite(step,1,8,f);}fclose(f);
 assert(replay_load(&r,path,NULL));assert(r.count==513);assert(r.steps[512].buttons==0x4000);remove(path);return 0;
}
