#ifndef FRAME_UPLOAD_H
#define FRAME_UPLOAD_H
#include <stdint.h>
#include <string.h>
/* Compare cached RAM copies, never read slow display memory. Each display
   buffer has its own history so alternating frames restore old sprites/HUD. */
static inline unsigned upload_changed_rows(uint32_t *display,const uint32_t *source,
 uint32_t *history,unsigned width,unsigned height,unsigned pitch,int force){
 unsigned rows=0;
 for(unsigned y=0;y<height;y++){
  const uint32_t *row=source+y*pitch;uint32_t *old=history+y*width;
  if(force||memcmp(old,row,width*sizeof(*row))){
   memcpy(display+y*pitch,row,width*sizeof(*row));
   memcpy(old,row,width*sizeof(*row));rows++;
  }
 }
 return rows;
}
#endif
