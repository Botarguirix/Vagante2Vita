#include "world_visuals.h"
#include <stdlib.h>
#include <string.h>
static int save(const char *path,const uint32_t *pixels){
 FILE *f=fopen(path,"wb");if(!f)return 0;fprintf(f,"P6\n640 360\n255\n");
 for(int i=0;i<640*360;i++){unsigned char b[3]={pixels[i]&255,(pixels[i]>>8)&255,(pixels[i]>>16)&255};if(fwrite(b,1,3,f)!=3){fclose(f);return 0;}}
 return fclose(f)==0;
}
int main(int argc,char **argv){
 if(argc!=4)return 2;World world;NativePlayer player;memset(&world,0,sizeof(world));
 uint32_t *high=malloc(640*360*4),*first=malloc(640*360*4),*low=malloc(320*180*4);int ok=0;
 if(!high||!first||!low||!world_load(&world,argv[1],stdout))goto done;
 char entity_path[512];snprintf(entity_path,sizeof(entity_path),"%s/intro-entities.vwe",argv[2]);
 if(!world_load_entities(&world,entity_path,stdout)||world.entity_count!=22)goto done;
 snprintf(entity_path,sizeof(entity_path),"%s/intro-melee.vwa",argv[2]);
 if(!melee_load(&world.melee,entity_path,stdout)||world.melee.count!=3)goto done;
 snprintf(entity_path,sizeof(entity_path),"%s/intro-bow.vwb",argv[2]);
 if(!bow_load(&world.bow,entity_path,stdout))goto done;
 for(int cls=0;cls<3;cls++){
  if(!load_world_visuals(argv[2],cls,&world,stdout)||!player_spawn(&player,&world,cls))goto done;
  draw_world(first,&world,&player,1);
  unsigned count=world.entity_count;world.entity_count=0;
  draw_world(high,&world,&player,1);world.entity_count=count;
  if(!memcmp(first,high,640*360*4))goto done;
  NativePlayer still=player;still.motion=PLAYER_RISE;still.ticks=0;
  draw_world(first,&world,&still,1);still.ticks=8;draw_world(high,&world,&still,1);
  if(!memcmp(first,high,640*360*4))goto done;
  draw_world(first,&world,&player,1);
  printf("WORLD_DECOR class=%d visible_entities_change_image=1 sprite_animation_changed=1 entity_count=%u\n",cls,count);
  for(int i=0;i<90;i++)player_step(&player,&world,PLAYER_RIGHT|(i==20?PLAYER_JUMP:0));
  if(player_overlaps_solid(&player,&world))goto done;
  draw_world(high,&world,&player,1);draw_world(low,&world,&player,2);
  if(!memcmp(high,first,640*360*4))goto done;
  for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(low[y*320+x]!=high[y*2*640+x*2])goto done;
  unsigned active=0,hit=0;
  for(int step=0;step<80;step++){
   player_step(&player,&world,step<14?PLAYER_ATTACK:0);
   const MeleeFrame *key=melee_frame(&player.melee);if(!key)continue;
   active++;if(key->enabled)hit++;
   draw_world(high,&world,&player,1);draw_world(low,&world,&player,2);
   for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(low[y*320+x]!=high[y*2*640+x*2])goto done;
   if(cls==0&&key->enabled&&!save(argv[3],high))goto done;
  }
  if(!active||!hit||player.melee.active||player.melee.serial!=1)goto done;
  printf("WORLD_MELEE class=%d active_steps=%u hit_steps=%u serial=%u completed=1 resolution_match=1\n",cls,active,hit,player.melee.serial);
  if(cls==1){
   for(int n=0;n<90;n++){
    player_step(&player,&world,n<60?PLAYER_RANGE:0);
    draw_world(high,&world,&player,1);draw_world(low,&world,&player,2);
    for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(low[y*320+x]!=high[y*2*640+x*2])goto done;
    if(n==50||n==65){char image_path[512];snprintf(image_path,sizeof(image_path),n==50?"%s.charge.ppm":"%s.bow.ppm",argv[3]);if(!save(image_path,high))goto done;}
   }
   if(player.arrows.fired!=1||player.loadout.items[2].quantity!=29||player.bow.active)goto done;
   for(int n=0;n<90;n++){
    player_step(&player,&world,n<60?(PLAYER_RANGE|PLAYER_LEFT|PLAYER_UP):PLAYER_UP);
    draw_world(high,&world,&player,1);draw_world(low,&world,&player,2);
    for(int y=0;y<180;y++)for(int x=0;x<320;x++)if(low[y*320+x]!=high[y*2*640+x*2])goto done;
    if(n==65){char image_path[512];snprintf(image_path,sizeof(image_path),"%s.bow-left.ppm",argv[3]);if(!save(image_path,high))goto done;}
   }
   if(player.arrows.fired!=2||player.loadout.items[2].quantity!=28||player.bow.active||player_overlaps_solid(&player,&world))goto done;
   printf("WORLD_BOW class=1 fired=%u ammo=%d completed=1 left_diagonal=1 resolution_match=1\n",player.arrows.fired,player.loadout.items[2].quantity);
  }
  printf("WORLD_RUNTIME class=%d motion=%s x=%.3f y=%.3f hp=%d animation_changed=1 resolution_match=1\n",cls,player_motion_name(player.motion),player.x,player.y,player.hp);
 }
 ok=1;
 done:free(high);free(first);free(low);free_world_visuals();return ok?0:1;
}
