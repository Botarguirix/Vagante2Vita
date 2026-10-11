#include "resources.h"
#include "scene.h"
#include <stdlib.h>
int main(int argc,char **argv){
 if(argc!=3)return 2;
 if(!load_resources(argv[1],stdout))return 1;
 uint32_t *p=malloc(640*360*4);if(!p){free_resources();return 1;}
 draw_scene(p);FILE *f=fopen(argv[2],"wb");if(!f){free(p);free_resources();return 1;}
 fprintf(f,"P6\n640 360\n255\n");for(int i=0;i<640*360;i++){unsigned char rgb[3]={p[i]&255,(p[i]>>8)&255,(p[i]>>16)&255};fwrite(rgb,1,3,f);}
 fclose(f);free(p);free_resources();return 0;
}
