/* Native reconstruction of observed tile flags. No Windows layouts retained. */
#include "world.h"
#include <string.h>
static uint16_t read16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t read32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
const WorldTile *world_tile(const World *w,int x,int y){
 if(x<0||y<0||x>=w->width||y>=w->height)return NULL;
 return &w->tiles[y*w->width+x];
}
int world_solid(const World *w,int x,int y){
 const WorldTile *t=world_tile(w,x,y);if(t)return t->solid;
 if((w->open_sky&&y<0)||(w->open_left&&x<0)||(w->open_right&&x>=w->width))return 0;
 return 1;
}
void world_collision_flags(World *w){
 for(int i=0;i<w->width*w->height;i++){
  WorldTile *t=&w->tiles[i];t->solid=t->type==1||t->type==7;t->platform=t->type==6;t->faces=0;
 }
 for(int y=0;y<w->height;y++)for(int x=0;x<w->width;x++){
  WorldTile *t=&w->tiles[y*w->width+x];
  if(t->solid){
   if(!world_solid(w,x-1,y))t->faces|=WORLD_LEFT;
   if(!world_solid(w,x,y-1))t->faces|=WORLD_UP;
   if(!world_solid(w,x+1,y))t->faces|=WORLD_RIGHT;
   if(!world_solid(w,x,y+1))t->faces|=WORLD_DOWN;
  }else if(t->platform)t->faces=WORLD_UP;
 }
}
int world_load(World *w,const char *path,FILE *log){
 unsigned char h[16],record[12];World next;memset(&next,0,sizeof(next));
 FILE *f=fopen(path,"rb");if(!f){if(log)fprintf(log,"WORLD error=open path=%s\n",path);return 0;}
 if(fread(h,1,sizeof(h),f)!=sizeof(h)||memcmp(h,"VWI1",4)||read16(h+4)!=WORLD_WIDTH||read16(h+6)!=WORLD_HEIGHT||read32(h+8)!=0||read32(h+12)!=WORLD_TILE_COUNT)goto invalid;
 next.width=WORLD_WIDTH;next.height=WORLD_HEIGHT;
 for(int i=0;i<WORLD_TILE_COUNT;i++){
  if(fread(record,1,12,f)!=12||record[0]>12||record[1]!=0)goto invalid;
  next.tiles[i].type=(int8_t)record[0];
  for(int layer=0;layer<5;layer++){
   uint16_t raw=read16(record+2+2*layer);int value=(raw&32768)?(int)raw-65536:(int)raw;
   const int limits[5]={1024,1024,704,512,512};
   if(value>=limits[layer])goto invalid;
   next.tiles[i].layers[layer]=(int16_t)value;
  }
 }
 if(fgetc(f)!=EOF||ferror(f))goto invalid;
 fclose(f);next.entrance_x=14;next.entrance_y=20;next.goal_x=18;next.goal_y=20;
 /* generateIntro changes the goal after generateIntermissionLevel flags. */
 world_collision_flags(&next);world_auto_tiles(&next);next.tiles[next.goal_y*next.width+next.goal_x].type=11;
 next.ready=1;*w=next;
 if(log)fprintf(log,"WORLD ready=1 width=40 height=24 group=4 entrance=14,20 goal=18,20 tile_fields=6 entities_pending=1\n");
 return 1;
 invalid:fclose(f);if(log)fprintf(log,"WORLD error=invalid_packet path=%s\n",path);return 0;
}
int starting_stats(int player_class,StartingStats *stats){
 static const int base[3][7]={{3,2,5,1,0,3,0},{1,4,3,1,0,3,1},{0,2,1,1,0,10,0}};
 if(player_class<0||player_class>=3||!stats)return 0;
 const int *v=base[player_class];
 *stats=(StartingStats){v[0],v[1],v[2],v[3],v[4],v[5],v[6]};return 1;
}

static int tile_bank(int value){
 if(value<0||value%32<16)return -1;
 return value/64*64+value%32-value%8;
}
static int tile_variant(int bank,unsigned faces){return bank+(int)faces+(faces>=8?24:0);}
void world_auto_tiles(World *w){
 const int dx[4]={-1,0,1,0},dy[4]={0,-1,0,1};
 const unsigned bits[4]={1,2,4,8};
 const int layers[4]={0,1,3,4};
 /* The observed routine traverses columns first and updates in place. */
 for(int x=0;x<w->width;x++)for(int y=0;y<w->height;y++){
  WorldTile *t=&w->tiles[y*w->width+x];
  for(int n=0;n<4;n++){
   int layer=layers[n],value=t->layers[layer],bank=tile_bank(value);
   if(bank<0)continue;
   unsigned edges=0;
   for(int side=0;side<4;side++){
    const WorldTile *other=world_tile(w,x+dx[side],y+dy[side]);
    if(!other)continue;
    int ob=tile_bank(other->layers[layer]);
    if(bank==ob)continue;
    /* Farground vegetation families have an ordered overlap rule. */
    if(layer==1&&(bank==720||bank==784||bank==848)&&(ob==720||ob==784||ob==848)&&bank>=ob)continue;
    edges|=bits[side];
   }
   t->layers[layer]=(int16_t)tile_variant(bank,edges);
  }
  int value=t->layers[2],bank=tile_bank(value);unsigned edges=0;
  if(t->type==1&&value%32<24){
   for(int side=0;side<4;side++){
    const WorldTile *other=world_tile(w,x+dx[side],y+dy[side]);
    if(other&&(other->type!=1||other->layers[2]%32>=24))edges|=bits[side];
   }
   t->layers[2]=(int16_t)tile_variant(bank,edges);
  }else if(bank>=0){
   for(int side=0;side<4;side++){
    const WorldTile *other=world_tile(w,x+dx[side],y+dy[side]);
    if(other&&tile_bank(other->layers[2])!=bank)edges|=bits[side];
   }
   t->layers[2]=(int16_t)tile_variant(bank,edges);
  }
  if(t->type==10||t->type==11)t->layers[3]=-1;
 }
}
