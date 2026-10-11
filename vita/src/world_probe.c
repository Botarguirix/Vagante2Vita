#include "world.h"
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
 if(argc!=3)return 2;World w;memset(&w,0,sizeof(w));if(!world_load(&w,argv[1],stderr))return 1;
 FILE *f=fopen(argv[2],"wx");if(!f)return 1;
 fprintf(f,"x,y,type,background,farground,midground,foreground,closeground,solid,platform,faces\n");
 for(int y=0;y<w.height;y++)for(int x=0;x<w.width;x++){
  const WorldTile *t=world_tile(&w,x,y);
  fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u\n",x,y,t->type,t->layers[0],t->layers[1],t->layers[2],t->layers[3],t->layers[4],t->solid,t->platform,t->faces);
 }
 return fclose(f)?1:0;
}
