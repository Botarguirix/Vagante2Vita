#include "resources.h"
#include "scene.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
 uint32_t *low=malloc(320*180*4);if(!low)goto done;
 draw_scene_resolution(low,&m,2);
 for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(low[y*320+x]!=p[(y*2)*640+x*2]){free(low);goto done;}
 clock_t begin=clock();for(int i=0;i<20;i++)draw_scene_resolution(p,&m,1);
 double high_us=(double)(clock()-begin)*1000000/CLOCKS_PER_SEC/20;
 begin=clock();for(int i=0;i<20;i++)draw_scene_resolution(low,&m,2);
 double low_us=(double)(clock()-begin)*1000000/CLOCKS_PER_SEC/20;
 printf("BENCH host_only=1 high_avg_us=%.0f low_avg_us=%.0f low_matches_decimated_high=1\n",high_us,low_us);
 free(low);
 m.screen=MENU_CLASS;m.player_class=1;draw_animated_scene(p,&m);
 snprintf(path,sizeof(path),"%s.classes.ppm",argv[2]);if(!save(path,p))goto done;
 puts("RENDER animation_changed=1 class_preview_written=1");ok=1;
 done:free(p);free(first);free_resources();return ok?0:1;
}
