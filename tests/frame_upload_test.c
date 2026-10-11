#include "frame_upload.h"
#include <assert.h>
int main(void){
 uint32_t source[24]={0},history[2][18]={{0}},display[2][24]={{0}};
 assert(upload_changed_rows(display[0],source,history[0],6,3,8,1)==3);
 assert(upload_changed_rows(display[0],source,history[0],6,3,8,0)==0);
 source[9]=123;
 assert(upload_changed_rows(display[0],source,history[0],6,3,8,0)==1);
 assert(display[0][9]==123&&display[0][7]==0);
 assert(upload_changed_rows(display[1],source,history[1],6,3,8,1)==3);
 source[9]=0;source[17]=456;
 assert(upload_changed_rows(display[0],source,history[0],6,3,8,0)==2);
 assert(upload_changed_rows(display[1],source,history[1],6,3,8,0)==2);
 assert(display[0][9]==0&&display[1][17]==456);
 return 0;
}
