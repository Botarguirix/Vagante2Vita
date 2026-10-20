#include "frame_scaler.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void){
 static uint32_t high[640*360],low[320*180],frame[1024*544];
 for(int y=0;y<360;y++)for(int x=0;x<640;x++)
  high[y*640+x]=(uint32_t)(1+y*640+x);
 for(int y=0;y<180;y++)for(int x=0;x<320;x++)
  low[y*320+x]=high[(y*2)*640+x*2];

 frame_scale_960x544(high,1,frame,1024);
 for(int y=0;y<544;y++)for(int x=0;x<1024;x++){
  uint32_t expected=0;
  if(y>=2&&y<542&&x<960){
   int sy=(y-2)*2/3,sx=x*2/3;
   expected=high[sy*640+sx];
  }
  assert(frame[y*1024+x]==expected);
 }

 frame_scale_960x544(low,2,frame,1024);
 for(int y=0;y<544;y++)for(int x=0;x<1024;x++){
  uint32_t expected=0;
  if(y>=2&&y<542&&x<960)expected=low[(y-2)/3*320+x/3];
  assert(frame[y*1024+x]==expected);
 }
 puts("frame-scaler: complete 960x540 coverage, centered 640x360 3:2 and 320x180 3x mappings pass");
 return 0;
}
