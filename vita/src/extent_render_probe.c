#include "world_visuals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static World world,previous;
int main(int argc,char **argv){
 if(argc!=5)return 2;
 NativePlayer p;uint32_t *hi=malloc(640*360*4),*lo=malloc(320*180*4);int ok=0;
 char old_packet[512];snprintf(old_packet,sizeof(old_packet),"%s/intro-world.vwi",argv[4]);
 if(!hi||!lo||!world_load(&previous,old_packet,stdout)||!load_world_visuals(argv[4],1,&previous,stdout)||!world_load(&world,argv[1],stdout)||!load_world_visuals(argv[2],1,&world,stdout)||!player_spawn(&p,&world,1))goto done;
 if(world.width!=80||world.height!=40)goto done;
 const int positions[4][2]={{20,20},{650,350},{1000,500},{1240,600}};
 for(unsigned i=0;i<4;i++){
  p.x=(float)positions[i][0];p.y=(float)positions[i][1];p.ticks=i*7;
  WorldCamera c=world_camera(&world,p.x+p.w*0.5f,p.y);
  draw_world(hi,&world,&p,1);draw_world(lo,&world,&p,2);
  uint32_t expected=0xff400000u|(unsigned)(c.x/16*3)|(unsigned)(c.y/16*5)<<8;
  if(hi[0]!=expected)goto done;
  for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(lo[y*320+x]!=hi[y*2*640+x*2])goto done;
  printf("WORLD_EXTENT_RENDER fixture_only=1 camera=%d,%d terrain_sample=1 resolution_match=1\n",c.x,c.y);
 }
 if(!player_spawn(&p,&world,1))goto done;
 for(int i=0;i<120;i++)player_step(&p,&world,PLAYER_RIGHT);
 if(player_overlaps_solid(&p,&world)||p.x<=1124||!p.on_ground)goto done;
 draw_world(hi,&world,&p,1);
 FILE *f=fopen(argv[3],"wb");if(!f)goto done;fprintf(f,"P6\n640 360\n255\n");
 for(int i=0;i<640*360;i++){unsigned char rgb[3]={hi[i]&255,(hi[i]>>8)&255,(hi[i]>>16)&255};fwrite(rgb,1,3,f);}if(fclose(f))goto done;
 printf("WORLD_EXTENT_MOVEMENT fixture_only=1 x=%.3f grounded=1 solid_overlap=0\n",p.x);
 world.width=79;if(load_world_visuals(argv[2],1,&world,stdout))goto done;
 printf("WORLD_EXTENT_VALIDATION directory_change=1 texture_dimension_mismatch_rejected=1\n");ok=1;
 done:free(hi);free(lo);free_world_visuals();return ok?0:1;
}
