#include "resources.h"
#include "miniz.h"
#include <stdlib.h>
#include <string.h>
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
Texture textures[10]={
 {"gfx/scenes/intro/sky.png",0,0,0},
 {"gfx/scenes/intro/sky_stars.png",0,0,0},
 {"gfx/scenes/intro/moon.png",0,0,0},
 {"gfx/scenes/intro/clouds.png",0,0,0},
 {"gfx/scenes/intro/mountains.png",0,0,0},
 {"gfx/scenes/intro/midground_treeline_1.png",0,0,0},
 {"gfx/scenes/intro/ground.png",0,0,0},
 {"gfx/scenes/intro/foreground_treeline.png",0,0,0},
 {"gfx/scenes/intro/caravan.png",0,0,0},
 {"gfx/gui/title.png",0,0,0}
};
typedef struct { FILE *f; mz_stream z; unsigned char in[32768],out[32768]; size_t pos,len; int end; } Reader;
static int fill(Reader *r){
 if(r->end)return 0;
 for(;;){
  if(!r->z.avail_in){r->z.avail_in=(unsigned)fread(r->in,1,sizeof(r->in),r->f);r->z.next_in=r->in;if(!r->z.avail_in)return -1;}
  r->z.next_out=r->out;r->z.avail_out=sizeof(r->out);
  int status=mz_inflate(&r->z,MZ_NO_FLUSH);
  r->len=sizeof(r->out)-r->z.avail_out;r->pos=0;
  if(status==MZ_STREAM_END){r->end=1;if(r->z.avail_in||fgetc(r->f)!=EOF)return -1;}
  else if(status!=MZ_OK)return -1;
  if(r->z.total_out>256u*1024u*1024u)return -1;
  if(r->len)return 1;
  if(r->end)return 0;
 }
}
static int read_bytes(Reader *r,void *dst,size_t n){
 unsigned char *p=dst;
 while(n){if(r->pos==r->len&&fill(r)!=1)return 0;size_t k=r->len-r->pos;if(k>n)k=n;
  if(p){memcpy(p,r->out+r->pos,k);p+=k;}r->pos+=k;n-=k;}
 return 1;
}
static uint32_t le32(const unsigned char *b){return b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
void free_resources(void){for(int i=0;i<10;i++){stbi_image_free(textures[i].rgba);textures[i].rgba=0;}}
int load_resources(const char *path,FILE *log){
 Reader *r=calloc(1,sizeof(*r));unsigned char b[6];unsigned records=0,declared=0;int ok=0;
 if(!r)return 0;
 r->f=fopen(path,"rb");if(!r->f){free(r);return 0;}
 if(mz_inflateInit(&r->z)!=MZ_OK){fclose(r->f);free(r);return 0;}
 if(!read_bytes(r,b,6)||le32(b)!=8)goto done;
 declared=b[4]|b[5]<<8;
 for(;;){
  if(r->pos==r->len){int status=fill(r);if(status==0)break;if(status<0)goto done;}
  if(++records>10000||!read_bytes(r,b,4))goto done;
  uint32_t n=le32(b);char name[1025];
  if(!n||n>1024||!read_bytes(r,name,n))goto done;
  name[n]=0;
  if(memchr(name,0,n)||!read_bytes(r,b,4))goto done;
  uint32_t size=le32(b);if(size>128u*1024u*1024u)goto done;
  int target=-1;for(int i=0;i<10;i++)if(!strcmp(name,textures[i].name))target=i;
  if(target<0){if(!read_bytes(r,NULL,size))goto done;continue;}
  if(!size||size>4u*1024u*1024u||textures[target].rgba)goto done;
  unsigned char *encoded=malloc(size);if(!encoded)goto done;
  if(!read_bytes(r,encoded,size)){free(encoded);goto done;}
  int w,h,c;
  if(!stbi_info_from_memory(encoded,(int)size,&w,&h,&c)||w<1||h<1||w>1024||h>1024){free(encoded);goto done;}
  Texture *t=&textures[target];t->rgba=stbi_load_from_memory(encoded,(int)size,&t->w,&t->h,&c,4);free(encoded);
  if(!t->rgba)goto done;
 }
 if(records<declared||records-declared>3)goto done;
 for(int i=0;i<10;i++)if(!textures[i].rgba)goto done;
 ok=1;
done:
 if(log){fprintf(log,"archive_ok=%d records=%u declared=%u inflated=%lu\n",ok,records,declared,r->z.total_out);for(int i=0;i<10;i++)fprintf(log,"%s %dx%d loaded=%d\n",textures[i].name,textures[i].w,textures[i].h,textures[i].rgba!=NULL);fflush(log);}
 mz_inflateEnd(&r->z);fclose(r->f);free(r);if(!ok)free_resources();return ok;
}
