/* Original diagnostic only. No recovered Vagante gameplay. */
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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
int main(void){
    const unsigned bytes=1024*544*4;
    SceUID block=sceKernelAllocMemBlock("VaganteDiagnostic",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,(bytes+0x3ffff)&~0x3ffff,NULL);
    if(block<0)return 1;
    if(sceKernelGetMemBlockBase(block,(void **)&frame)<0){sceKernelFreeMemBlock(block);return 1;}
    for(unsigned i=0;i<bytes/4;i++)frame[i]=0xff20170f;
    text(48,48,"VAGANTE2VITA",0xff80ddff);
    text(48,100,"TECHNICAL DIAGNOSTIC",0xffffffff);
    text(48,152,"GAME LOGIC NOT IMPLEMENTED",0xff80b0ff);
    FILE *f=fopen("ux0:data/Vagante2Vita/data.vra","rb");
    if(f){
        unsigned char h[2]={0};size_t n=fread(h,1,2,f);fclose(f);
        int valid=n==2 && (h[0]&15)==8 && (h[0]>>4)<=7 && (((unsigned)h[0]*256+h[1])%31)==0;
        text(48,232,valid?"DATA VRA ZLIB HEADER FOUND":"DATA VRA HEADER UNEXPECTED",valid?0xff80ff80:0xff8080ff);
        text(48,276,"ARCHIVE CONTENT NOT VERIFIED",0xffffffff);
    }else{text(48,232,"DATA VRA NOT FOUND",0xff8080ff);}
    text(48,354,"PLACE DATA VRA IN",0xffffffff);
    text(48,394,"UX0 DATA VAGANTE2VITA",0xffffffff);
    text(48,466,"PRESS START TO EXIT",0xffaaaaaa);
    SceDisplayFrameBuf fb;memset(&fb,0,sizeof(fb));fb.size=sizeof(fb);fb.base=frame;fb.pitch=1024;
    fb.pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;fb.width=960;fb.height=544;
    if(sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME)<0){sceKernelFreeMemBlock(block);return 1;}
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    for(;;){SceCtrlData pad;memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);if(pad.buttons&SCE_CTRL_START)break;sceDisplayWaitVblankStart();}
    sceDisplaySetFrameBuf(NULL,SCE_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
    sceKernelFreeMemBlock(block);sceKernelExitProcess(0);return 0;
}
