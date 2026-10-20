#ifndef VAGANTE_FRAME_SCALER_H
#define VAGANTE_FRAME_SCALER_H

#include <stdint.h>
#include <string.h>

/* Scale the logical scene into the centered 960x540 visible area of the Vita's
   960x544 framebuffer. The high profile uses exact nearest-neighbour 3:2
   scaling; the lightweight profile uses an exact 3x scale. */
static inline void frame_scale_960x544(const uint32_t *source,int reduction,
                                       uint32_t *destination,int pitch){
 uint32_t row[960];
 memset(destination,0,(size_t)544*pitch*sizeof(*destination));
 if(reduction==2){
  for(int y=0;y<180;y++){
   uint32_t *dst=destination+(size_t)(2+y*3)*pitch;
   const uint32_t *src=source+(size_t)y*320;
   for(int x=0;x<320;x++)row[x*3]=row[x*3+1]=row[x*3+2]=src[x];
   memcpy(dst,row,sizeof(row));memcpy(dst+pitch,row,sizeof(row));memcpy(dst+2*pitch,row,sizeof(row));
  }
  return;
 }
 for(int y=0;y<360;y+=2){
  const uint32_t *src0=source+(size_t)y*640;
  const uint32_t *src1=src0+640;
  for(int x=0;x<320;x++){
   row[x*3]=src0[x*2];row[x*3+1]=src0[x*2];row[x*3+2]=src0[x*2+1];
  }
  uint32_t *dst=destination+(size_t)(2+(y/2)*3)*pitch;
  memcpy(dst,row,sizeof(row));memcpy(dst+pitch,row,sizeof(row));
  for(int x=0;x<320;x++){
   row[x*3]=src1[x*2];row[x*3+1]=src1[x*2];row[x*3+2]=src1[x*2+1];
  }
  memcpy(dst+2*pitch,row,sizeof(row));
 }
}

#endif
