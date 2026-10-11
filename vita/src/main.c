/* Native title reconstruction; gameplay remains pending. */
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <psp2/kernel/threadmgr.h>
#include "resources.h"
#include "scene.h"
unsigned int _newlib_heap_size_user = 16 * 1024 * 1024;

/* Five-column uppercase diagnostic font, seven bits per column. */
static const uint8_t glyphs[36][5]={
 {126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {7,8,112,8,7},{97,81,73,69,67},
 {62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
 {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
 {54,73,73,73,54},{6,73,73,41,30}
};
static uint32_t *frame;
static void text(int x,int y,const char *s,uint32_t color){
    for(;*s;s++,x+=18){
        int g=(*s>='A'&&*s<='Z')?*s-'A':((*s>='0'&&*s<='9')?26+*s-'0':-1);
        if(g<0)continue;
        for(int c=0;c<5;c++)for(int r=0;r<7;r++)if(glyphs[g][c]&(1<<r))
            for(int dy=0;dy<3;dy++)for(int dx=0;dx<3;dx++){
                int px=x+c*3+dx,py=y+r*3+dy;
                if(px>=0&&px<960&&py>=0&&py<544)frame[py*1024+px]=color;
            }
    }
}
static uint32_t background[640*360];
static void present_background(void){
 for(int y=0;y<544;y++)for(int x=0;x<960;x++)frame[y*1024+x]=(y<2||y>=542)?0xff000000:background[((y-2)*2/3)*640+x*2/3];
}
int main(void){
 const unsigned bytes=1024*544*4;
 SceUID block=sceKernelAllocMemBlock("VaganteTitle",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,(bytes+0x3ffff)&~0x3ffff,NULL);
 if(block<0)return 1;
 if(sceKernelGetMemBlockBase(block,(void **)&frame)<0){sceKernelFreeMemBlock(block);return 1;}
 memset(frame,0,bytes);
 text(48,100,"LOADING AND VERIFYING DATA VRA",0xffffffff);
 SceDisplayFrameBuf fb;memset(&fb,0,sizeof(fb));fb.size=sizeof(fb);fb.base=frame;fb.pitch=1024;
 fb.pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;fb.width=960;fb.height=544;
 if(sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME)<0){sceKernelFreeMemBlock(block);return 1;}
 FILE *log=fopen("ux0:data/Vagante2Vita/runtime.log","w");
 uint64_t start=sceKernelGetSystemTimeWide();
 int loaded=load_resources("ux0:data/Vagante2Vita/data.vra",log);
 if(log){fprintf(log,"load_us=%llu\n",(unsigned long long)(sceKernelGetSystemTimeWide()-start));fflush(log);}
 if(loaded)draw_scene(background);
 int index=0,panel=0,dirty=1;unsigned previous=0;
 const char *labels[]={"NEW GAME","PLAY ONLINE","OPTIONS","EXIT GAME"};
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 for(;;){
  SceCtrlData pad;memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);
  unsigned pressed=pad.buttons&~previous;previous=pad.buttons;
  if((pad.buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT))break;
  if(!loaded&&(pressed&SCE_CTRL_START))break;
  if(loaded){
   if(panel){if(pressed&(SCE_CTRL_CIRCLE|SCE_CTRL_CROSS)){panel=0;dirty=1;}}
   else{
    if((pressed&SCE_CTRL_UP)&&index>0){index--;dirty=1;}
    if((pressed&SCE_CTRL_DOWN)&&index<3){index++;dirty=1;}
    if(pressed&SCE_CTRL_CROSS){if(index==3)break;panel=index+1;dirty=1;}
   }
  }
  if(dirty){
   start=sceKernelGetSystemTimeWide();
   sceDisplaySetFrameBuf(NULL,SCE_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
   if(loaded){
    present_background();
    if(panel){
     for(int y=270;y<450;y++)for(int x=80;x<880;x++)frame[y*1024+x]=0xff20170f;
     text(110,295,panel==1?"CHARACTER SELECTION PENDING":panel==2?"ONLINE PLAY PENDING":"OPTIONS RECOVERY PENDING",0xff80b0ff);
     text(110,355,"PRESS CIRCLE TO RETURN",0xffffffff);
    }else for(int i=0;i<4;i++)text(360,290+38*i,labels[i],i==index?0xff80ddff:0xffdddddd);
    text(18,515,"TITLE PREVIEW  GAMEPLAY PENDING",0xffaaaaaa);
   }else{
    memset(frame,0,bytes);text(48,100,"ARCHIVE LOAD FAILED",0xff8080ff);
    text(48,180,"CHECK DATA VRA AND RUNTIME LOG",0xffffffff);text(48,260,"PRESS START TO EXIT",0xffffffff);
   }
   sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
   if(log){fprintf(log,"render_us=%llu cursor=%d panel=%d\n",(unsigned long long)(sceKernelGetSystemTimeWide()-start),index,panel);fflush(log);}
   dirty=0;
  }
  sceDisplayWaitVblankStart();
 }
 if(log)fclose(log);
 free_resources();
 sceDisplaySetFrameBuf(NULL,SCE_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
 sceKernelFreeMemBlock(block);sceKernelExitProcess(0);return 0;
}
