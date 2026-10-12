#include "native_ui.h"
#include <stdlib.h>
#include <string.h>
static int save(const char *path,const uint32_t *p){
 FILE *f=fopen(path,"wb");if(!f)return 0;fprintf(f,"P6\n960 544\n255\n");
 for(int y=0;y<544;y++)for(int x=0;x<960;x++){uint32_t v=p[y*1024+x];unsigned char b[3]={v&255,(v>>8)&255,(v>>16)&255};if(fwrite(b,1,3,f)!=3){fclose(f);return 0;}}
 return fclose(f)==0;
}
int main(int argc,char **argv){
 if(argc!=2)return 2;uint32_t *p=malloc(1024*544*4);if(!p)return 1;
 World w;memset(&w,0,sizeof(w));w.ready=1;w.width=40;w.height=24;w.entrance_x=14;w.entrance_y=20;
 for(int cls=0;cls<3;cls++){
  NativePlayer player;if(!player_spawn(&player,&w,cls)){free(p);return 1;}
  for(int page=0;page<3;page++){
   for(int i=0;i<1024*544;i++)p[i]=0xff302820;
   draw_character_panel(p,&player,page);
   char path[512];snprintf(path,sizeof(path),"%s/class-%d-page-%d.ppm",argv[1],cls,page);
   if(!save(path,p)){free(p);return 1;}
  }
 }
 free(p);puts("CHARACTER_PREVIEW native_render=1 classes=3 pages=3");return 0;
}
