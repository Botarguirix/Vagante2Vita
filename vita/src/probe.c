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

 scene_invalidate_cache();m.total_ticks=0;draw_scene_profile(first,&m,2,1);
 unsigned builds=scene_cache_rebuilds();m.total_ticks=7;draw_scene_profile(low,&m,2,1);
 if(scene_cache_rebuilds()!=builds||!memcmp(first,low,320*180*4)){free(low);goto done;}
 memcpy(first,low,320*180*4);scene_invalidate_cache();draw_scene_profile(low,&m,2,1);
 if(memcmp(first,low,320*180*4)){free(low);goto done;}
 begin=clock();for(int i=0;i<200;i++){m.total_ticks++;draw_scene_profile(low,&m,2,1);}
 double lite_us=(double)(clock()-begin)*1000000/CLOCKS_PER_SEC/200;
 printf("BENCH host_only=1 lite_avg_us=%.0f cached_animation=1 no_ghosting=1\n",lite_us);
 m.screen=MENU_CLASS;m.player_class=0;draw_scene_profile(low,&m,2,1);builds=scene_cache_rebuilds();
 memcpy(first,low,320*180*4);m.player_class=1;draw_scene_profile(low,&m,2,1);
 if(scene_cache_rebuilds()!=builds+1||!memcmp(first,low,320*180*4)){free(low);goto done;}
 draw_scene_profile(p,&m,1,1);draw_scene_profile(low,&m,2,1);
 for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(low[y*320+x]!=p[(y*2)*640+x*2]){free(low);goto done;}
 snprintf(path,sizeof(path),"%s.lite-classes.ppm",argv[2]);if(!save(path,p)){free(low);goto done;}
 m.screen=MENU_FADE;m.ticks=89;draw_scene_profile(low,&m,2,1);builds=scene_cache_rebuilds();
 m.ticks=90;draw_scene_profile(low,&m,2,1);
 if(scene_cache_rebuilds()!=builds){free(low);goto done;}
 for(int i=0;i<320*180;i++)if(low[i]!=0xff000000){free(low);goto done;}
 puts("RENDER lite_class_invalidation=1 lite_resolution_match=1 fade_black=1");
 free(low);
 for(int resolution=1;resolution<=2;resolution++){
  for(int trial=0;trial<3;trial++)for(int mode=0;mode<3;mode++){
   m.screen=MENU_TITLE;m.total_ticks=0;draw_scene_profile(p,&m,resolution,mode);
   memcpy(first,p,(size_t)(640/resolution)*(360/resolution)*4);
   int changed=0;begin=clock();for(int n=0;n<24;n++){m.total_ticks=(uint64_t)(n+1)*7;draw_scene_profile(p,&m,resolution,mode);if(memcmp(p,first,(size_t)(640/resolution)*(360/resolution)*4))changed=1;}
   double elapsed=(double)(clock()-begin)*1000000/CLOCKS_PER_SEC/24;
   if(!changed)goto done;
   printf("PROFILE_BENCH host_only=1 trial=%d resolution=%d mode=%d avg_us=%.0f animation_changed=1\n",trial,resolution,mode,elapsed);
  }
 }
 for(int tick=0;tick<1200;tick+=137){
  m.total_ticks=(uint64_t)tick;draw_scene_profile(first,&m,1,2);
  draw_scene_prepared_reference(p,&m,1);
  if(memcmp(first,p,640*360*4))goto done;
 }
 puts("PREPARED_ANIMATION fast_layers_reference_match=1");
 m.total_ticks=100;draw_scene_profile(p,&m,1,2);
 uint32_t *check=malloc(320*180*4);if(!check)goto done;draw_scene_profile(check,&m,2,2);
 for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(check[y*320+x]!=p[y*2*640+x*2]){free(check);goto done;}
 free(check);puts("PREPARED_ANIMATION resolution_match=1");
 m.screen=MENU_CLASS;m.player_class=1;draw_animated_scene(p,&m);
 snprintf(path,sizeof(path),"%s.classes.ppm",argv[2]);if(!save(path,p))goto done;
 puts("RENDER animation_changed=1 class_preview_written=1");ok=1;
 done:free(p);free(first);scene_free_cache();free_resources();return ok?0:1;
}
