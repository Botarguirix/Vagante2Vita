#include "input_replay.h"
#include <assert.h>
#include <string.h>
static void packet(const char *path,unsigned mask,unsigned ticks,int extra){
 unsigned char b[16]={'V','T','S','1',1,0,0,0,mask&255,(mask>>8)&255,(mask>>16)&255,(mask>>24)&255,ticks&255,(ticks>>8)&255,0,0};
 FILE *f=fopen(path,"wb");assert(f);fwrite(b,1,16,f);if(extra){fputc(1,f);}fclose(f);
}
int main(void){
 InputReplay r={0},before;const char *path="input-replay-fixture.vts";
 packet(path,0x4000,6,0);assert(replay_load(&r,path,NULL));assert(replay_buttons(&r)==0x4000);
 replay_advance(&r,0);assert(!r.done);replay_advance(&r,5);assert(!r.done);replay_advance(&r,1);assert(r.done&&!r.active&&!replay_buttons(&r));
 before=r;packet(path,0x4000,0,0);assert(!replay_load(&r,path,NULL));assert(!memcmp(&r,&before,sizeof(r)));
 packet(path,0x4000,601,0);assert(!replay_load(&r,path,NULL));
 packet(path,0x4,6,0);assert(!replay_load(&r,path,NULL));
 packet(path,0x4000,6,1);assert(!replay_load(&r,path,NULL));
 FILE *f=fopen(path,"wb");fwrite("VTS1",1,4,f);fclose(f);assert(!replay_load(&r,path,NULL));
 assert(!memcmp(&r,&before,sizeof(r)));remove(path);return 0;
}
