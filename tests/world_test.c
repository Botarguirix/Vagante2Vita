#include "world.h"
#include <assert.h>
#include <string.h>
int main(int argc,char **argv){
 World w;memset(&w,0,sizeof(w));w.width=3;w.height=3;
 w.tiles[4].type=1;w.tiles[7].type=6;world_collision_flags(&w);
 assert(w.tiles[4].solid&&w.tiles[4].faces==15);
 assert(!w.tiles[7].solid&&w.tiles[7].platform&&w.tiles[7].faces==WORLD_UP);
 w.tiles[3].type=1;world_collision_flags(&w);assert(!(w.tiles[4].faces&WORLD_LEFT));
 assert(world_solid(&w,-1,0));w.open_left=1;assert(!world_solid(&w,-1,0));
 assert(world_solid(&w,0,-1));w.open_sky=1;assert(!world_solid(&w,0,-1));
 World a;memset(&a,0,sizeof(a));a.width=3;a.height=3;
 for(int i=0;i<9;i++)for(int layer=0;layer<5;layer++)a.tiles[i].layers[layer]=16;
 a.tiles[4].layers[0]=80;world_auto_tiles(&a);assert(a.tiles[4].layers[0]==119);
 assert(a.tiles[0].layers[0]==16);assert(a.tiles[1].layers[0]==48);
 StartingStats stats;assert(starting_stats(0,&stats));assert(stats.vitality==5&&stats.intelligence==3);
 assert(starting_stats(2,&stats)&&stats.intelligence==10);assert(!starting_stats(3,&stats));
 if(argc==3){World before=w;assert(!world_load(&w,argv[1],stdout));assert(!memcmp(&w,&before,sizeof(w)));}
 if(argc==2){assert(world_load(&w,argv[1],stdout));assert(w.ready&&w.width==40&&w.height==24);
 assert(w.tiles[20*40+18].type==11);World before=w;
 assert(!world_load(&w,"does-not-exist.vwi",stdout));assert(!memcmp(&w,&before,sizeof(w)));}
 return 0;
}
