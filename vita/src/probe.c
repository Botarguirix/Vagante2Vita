#include "resources.h"
#include "scene.h"
#include <stdlib.h>
#include <string.h>
static int save(const char *path,const uint32_t *p){
 FILE *f=fopen(path,"wb");if(!f)return 0;
 fprintf(f,"P6\n640 360\n255\n");for(int i=0;i<640*360;i++){unsigned char rgb[3]={p[i]&255,(p[i]>>8)&255,(p[i]>>16)&255};fwrite(rgb,1,3,f);}
 return fclose(f)==0;
}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 if(!load_resources(argv[1],stdout))return 1;
 uint32_t *p=malloc(640*360*4),*first=malloc(640*360*4);char path[4096];int ok=0;
 if(!p||!first)goto done;
 Menu m={0};draw_animated_scene(first,&m);if(!save(argv[2],first))goto done;
 m.total_ticks=240;draw_animated_scene(p,&m);
 if(!memcmp(p,first,640*360*4))goto done;
 snprintf(path,sizeof(path),"%s.animated.ppm",argv[2]);if(!save(path,p))goto done;
 m.screen=MENU_CLASS;m.player_class=1;draw_animated_scene(p,&m);
 snprintf(path,sizeof(path),"%s.classes.ppm",argv[2]);if(!save(path,p))goto done;
 puts("RENDER animation_changed=1 class_preview_written=1");ok=1;
 done:free(p);free(first);free_resources();return ok?0:1;
}
