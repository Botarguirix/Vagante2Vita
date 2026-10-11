/* Native title reconstruction; gameplay remains pending. */
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <psp2/kernel/threadmgr.h>
#include "resources.h"
#include "scene.h"
#include "session_log.h"
#include "world_visuals.h"
static World world;
static NativePlayer player;
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
static void present_background(int reduction){
 uint32_t row[960];
 memset(frame,0,2*1024*4);memset(frame+542*1024,0,2*1024*4);
 if(reduction==2){
  for(int y=0;y<180;y++){
   uint32_t *dst=frame+(2+y*3)*1024;const uint32_t *src=background+y*320;
   for(int x=0;x<320;x++)row[x*3]=row[x*3+1]=row[x*3+2]=src[x];
   memcpy(dst,row,sizeof(row));memcpy(dst+1024,row,sizeof(row));memcpy(dst+2048,row,sizeof(row));
  }
  return;
 }
 for(int y=0;y<360;y++){
  uint32_t *dst=frame+(2+(y*3+1)/2)*1024;const uint32_t *src=background+y*640;
  for(int x=0;x<320;x++){row[x*3]=src[x*2];row[x*3+1]=src[x*2];row[x*3+2]=src[x*2+1];}
  memcpy(dst,row,sizeof(row));if(!(y&1))memcpy(dst+1024,row,sizeof(row));
 }
}
static unsigned mapped_buttons(unsigned b){
 unsigned result=0;
 if(b&SCE_CTRL_UP)result|=INPUT_UP;
 if(b&SCE_CTRL_DOWN)result|=INPUT_DOWN;
 if(b&SCE_CTRL_LEFT)result|=INPUT_LEFT;
 if(b&SCE_CTRL_RIGHT)result|=INPUT_RIGHT;
 if(b&SCE_CTRL_CROSS)result|=INPUT_ACCEPT;
 if(b&SCE_CTRL_CIRCLE)result|=INPUT_BACK;
 return result;
}
static unsigned player_buttons(const SceCtrlData *pad){
 unsigned b=pad->buttons,result=0;
 if((b&SCE_CTRL_LEFT)||pad->lx<64)result|=PLAYER_LEFT;
 if((b&SCE_CTRL_RIGHT)||pad->lx>192)result|=PLAYER_RIGHT;
 if(b&SCE_CTRL_UP)result|=PLAYER_UP;
 if(b&SCE_CTRL_DOWN)result|=PLAYER_DOWN;
 if(b&SCE_CTRL_CROSS)result|=PLAYER_JUMP;
 if(b&SCE_CTRL_LTRIGGER)result|=PLAYER_SLOW;
 return result;
}
static void log_state(FILE *log,const Menu *m,unsigned b){
 if(!log)return;
 fprintf(log,"EVENT time_us=%llu screen=%s cursor=%d class=%d buttons=%08x tick=%llu\n",(unsigned long long)sceKernelGetSystemTimeWide(),menu_screen_name(m->screen),m->cursor,m->player_class,b,(unsigned long long)m->total_ticks);
 if(m->screen==MENU_BLOCKED)fprintf(log,"BLOCKER subsystem=world_start missing=world_data_or_visuals_or_spawn reconstruction_pending=1 selected_class=%d\n",m->player_class);
 fflush(log);
}
int main(void){
 const unsigned bytes=1024*544*4;
 mkdir("ux0:data/Vagante2Vita/logs",0777);
 char log_path[256]={0};
 FILE *log=open_session_log("ux0:data/Vagante2Vita/logs/iteration" VAGANTE_ITERATION,log_path,sizeof(log_path));
 if(log){fprintf(log,"LOG path=%s\n",log_path);fprintf(log,"BOOT version=" VAGANTE_VERSION " iteration=" VAGANTE_ITERATION " heap_bytes=%u framebuffer_bytes=%u renderer=software logical=320x180 target_draw_hz=60 direct_row_scaler=1 lightweight=1 scenery_cached=1 particles=0\n",_newlib_heap_size_user,bytes*2);fprintf(log,"PENDING exact_intro_tween day_night_tints audio unlocks saves procedural_generation combat skills inventory online options\n");fflush(log);}
 SceUID block=sceKernelAllocMemBlock("VaganteMenu",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,(bytes*2+0x3ffff)&~0x3ffff,NULL);
 if(block<0){if(log){fprintf(log,"ERROR framebuffer_alloc code=%d\n",block);fclose(log);}return 1;}
 uint32_t *buffers;
 int base_status=sceKernelGetMemBlockBase(block,(void **)&buffers);
 if(base_status<0){if(log){fprintf(log,"ERROR framebuffer_base code=%d\n",base_status);fclose(log);}sceKernelFreeMemBlock(block);return 1;}
 frame=buffers;memset(buffers,0,bytes*2);
 text(48,100,"LOADING AND VERIFYING DATA VRA",0xffffffff);
 SceDisplayFrameBuf fb;memset(&fb,0,sizeof(fb));fb.size=sizeof(fb);fb.base=frame;fb.pitch=1024;
 fb.pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;fb.width=960;fb.height=544;
 int display_status=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
 if(display_status<0){if(log){fprintf(log,"ERROR display_initial code=%d\n",display_status);fclose(log);}sceKernelFreeMemBlock(block);return 1;}
 uint64_t start=sceKernelGetSystemTimeWide();
 FILE *cache=fopen("ux0:data/Vagante2Vita/menu-only.vra","rb");
 int loaded=0;
 if(cache){fclose(cache);if(log)fprintf(log,"CACHE candidate=menu-only.vra\n");loaded=load_resources("ux0:data/Vagante2Vita/menu-only.vra",log);if(!loaded&&log)fprintf(log,"WARN menu_cache_invalid fallback=data.vra\n");}
 if(!loaded){if(log)fprintf(log,"SOURCE archive=data.vra\n");loaded=load_resources("ux0:data/Vagante2Vita/data.vra",log);}
 else if(log)fprintf(log,"SOURCE archive=menu-only.vra scope=menu_only origin_not_rehashed_on_vita\n");
 if(log){fprintf(log,"LOAD complete=%d load_us=%llu\n",loaded,(unsigned long long)(sceKernelGetSystemTimeWide()-start));fflush(log);}
 Menu menu={0};log_state(log,&menu,0);
 unsigned previous=0;int buffer_index=0,stats=0,display_error=0,reduction=2,lightweight=1;
 const char *labels[]={"NEW GAME","PLAY ONLINE","OPTIONS","EXIT GAME"};
 uint64_t last_tick=sceKernelGetSystemTimeWide(),last_draw=0,next_draw=last_tick,accumulator=0;
 uint64_t window_start=last_tick,cpu_sum=0,cpu_max=0,compose_sum=0,scale_sum=0,upload_sum=0;unsigned draws=0,logic_steps=0,uploaded_rows=0;uint64_t logic_us=0,interval_max=0;float fps=0;
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 for(;;){
  SceCtrlData pad;memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);
  unsigned pressed=pad.buttons&~previous;previous=pad.buttons;
  if((pad.buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT))break;
  if(!loaded&&(pressed&SCE_CTRL_START))break;
  if(pressed&SCE_CTRL_SELECT)stats=!stats;
  if(menu.screen!=MENU_WORLD&&(pressed&SCE_CTRL_LTRIGGER)){reduction=reduction==2?1:2;window_start=sceKernelGetSystemTimeWide();draws=0;cpu_sum=cpu_max=compose_sum=scale_sum=0;if(log){fprintf(log,"PROFILE logical_width=%d logical_height=%d\n",640/reduction,360/reduction);fflush(log);}}
  if(menu.screen!=MENU_WORLD&&(pressed&SCE_CTRL_RTRIGGER)){lightweight=!lightweight;scene_invalidate_cache();if(log){fprintf(log,"QUALITY lightweight=%d scenery_cached=%d particles=%d\n",lightweight,lightweight,!lightweight);fflush(log);}}
  MenuScreen old_screen=menu.screen;int old_cursor=menu.cursor,old_class=menu.player_class;
  if(loaded){
   if(menu.screen==MENU_TITLE&&menu.cursor==3&&(pressed&SCE_CTRL_CROSS))break;
   if(menu.screen==MENU_WORLD){
    if(pressed&SCE_CTRL_CIRCLE){
     int reset=player_spawn(&player,&world,menu.player_class);player.previous=player_buttons(&pad);
     if(log)fprintf(log,"PLAYER_RESET complete=%d class=%d\n",reset,menu.player_class);
    }
    if(pressed&SCE_CTRL_TRIANGLE)menu.screen=MENU_CLASS;
   }else menu_input(&menu,mapped_buttons(pad.buttons));
   if(old_screen==MENU_CLASS&&menu.screen==MENU_BLOCKED){
    uint64_t world_start=sceKernelGetSystemTimeWide();
    const char *directories[]={"ux0:data/Vagante2Vita/world","app0:world"};
    int world_ready=0;
    for(unsigned source=0;source<2&&!world_ready;source++){
     char packet[128];snprintf(packet,sizeof(packet),"%s/intro-world.vwi",directories[source]);
     world_ready=world_load(&world,packet,log)&&load_world_visuals(directories[source],menu.player_class,log)&&player_spawn(&player,&world,menu.player_class);
     if(log)fprintf(log,"WORLD_SOURCE directory=%s ready=%d\n",directories[source],world_ready);
    }
    if(world_ready){
     menu.screen=MENU_WORLD;player.previous=player_buttons(&pad);
     if(log)fprintf(log,"WORLD_START complete=1 class=%d load_us=%llu hp=%d\n",menu.player_class,(unsigned long long)(sceKernelGetSystemTimeWide()-world_start),player.hp);
     /* Loading is not simulation time; do not advance the player during I/O. */
     last_tick=sceKernelGetSystemTimeWide();accumulator=0;
    }else if(log)fprintf(log,"WORLD_START complete=0 class=%d check_world_data=1\n",menu.player_class);
   }
  }
  uint64_t now=sceKernelGetSystemTimeWide(),delta=now-last_tick;last_tick=now;
  if(delta>250000){if(log)fprintf(log,"WARN slow_loop_us=%llu\n",(unsigned long long)delta);delta=250000;}
  accumulator+=delta;
  PlayerMotion old_motion=player.motion;unsigned old_input=player.previous;
  uint64_t logic_start=sceKernelGetSystemTimeWide();
  while(accumulator>=16667){if(loaded){menu_tick(&menu);if(menu.screen==MENU_WORLD){player_step(&player,&world,player_buttons(&pad));logic_steps++;}}accumulator-=16667;}
  logic_us+=sceKernelGetSystemTimeWide()-logic_start;
  if(log&&menu.screen==MENU_WORLD&&(old_motion!=player.motion||old_input!=player.previous)){
   fprintf(log,"PLAYER_EVENT tick=%u motion=%s input=%u x=%.3f y=%.3f vx=%.3f vy=%.3f contacts=%u\n",player.ticks,player_motion_name(player.motion),player.previous,player.x,player.y,player.vx,player.vy,player.contacts);
  }
  if(old_screen!=menu.screen||old_cursor!=menu.cursor||old_class!=menu.player_class)log_state(log,&menu,pad.buttons);
  if(now>=next_draw){
   if(last_draw&&now-last_draw>interval_max)interval_max=now-last_draw;
   last_draw=now;do{next_draw+=16667;}while(next_draw<=now);start=sceKernelGetSystemTimeWide();buffer_index^=1;frame=buffers+buffer_index*(bytes/4);
   if(loaded){
    if(menu.screen==MENU_WORLD)draw_world(background,&world,&player,reduction);
    else draw_scene_profile(background,&menu,reduction,lightweight);
    uint64_t compose_end=sceKernelGetSystemTimeWide();compose_sum+=compose_end-start;
    present_background(reduction);scale_sum+=sceKernelGetSystemTimeWide()-compose_end;
    if(menu.screen==MENU_WORLD){
     char info[64];snprintf(info,sizeof(info),"HP %d OF %d  STR %d DEX %d VIT %d",player.hp,player.max_hp,player.stats.strength,player.stats.dexterity,player.stats.vitality);text(18,42,info,0xff80ddff);
     snprintf(info,sizeof(info),"SPD %d STA %d INT %d LUK %d",player.stats.speed,player.stats.stamina,player.stats.intelligence,player.stats.luck);text(18,75,info,0xffffffff);
     text(18,475,"X JUMP DOWN DUCK UP CLIMB",0xffffffff);
     text(18,515,"O RESET TRIANGLE RETURN " VAGANTE_ITERATION,0xffaaaaaa);
    }
    if(menu.screen==MENU_TITLE)for(int i=0;i<4;i++)text(360,290+38*i,labels[i],i==menu.cursor?0xff80ddff:0xffdddddd);
    if(menu.screen==MENU_CLASS){text(225,49,"CHOOSE CLASS",0xff80ddff);text(195,425,"LEFT RIGHT TO SELECT",0xffffffff);text(195,461,"X CONFIRM  CIRCLE BACK",0xffffffff);}
    if(menu.screen==MENU_BLOCKED||menu.screen==MENU_NOTICE){
     for(int y=260;y<485;y++)for(int x=65;x<895;x++)frame[y*1024+x]=0xff20170f;
     text(95,285,menu.screen==MENU_BLOCKED?"CLASS SELECTED":menu.notice==1?"ONLINE PLAY PENDING":"OPTIONS RECOVERY PENDING",0xff80ddff);
     if(menu.screen==MENU_BLOCKED){text(95,337,"WORLD DATA OR SPAWN FAILED",0xff80b0ff);text(95,379,"BLOCKER RECORDED IN LOG",0xffffffff);}
     text(95,435,"CIRCLE TO RETURN",0xffffffff);
    }
    if(menu.screen!=MENU_WORLD)text(18,515,log?"ITERATION " VAGANTE_ITERATION "  GAMEPLAY PENDING":"ERROR LOG FILE COULD NOT OPEN",log?0xffaaaaaa:0xff8080ff);
    if(stats){char info[48];snprintf(info,sizeof(info),"DRAW FPS %u  CPU US %u",(unsigned)fps,(unsigned)cpu_max);text(18,12,info,0xff80ff80);}
   }else{
    memset(frame,0,bytes);text(48,100,"ARCHIVE LOAD FAILED",0xff8080ff);
    text(48,180,"CHECK DATA VRA AND RUNTIME LOG",0xffffffff);text(48,260,"PRESS START TO EXIT",0xffffffff);
   }
   uint64_t cpu=sceKernelGetSystemTimeWide()-start;cpu_sum+=cpu;if(cpu>cpu_max)cpu_max=cpu;draws++;
   fb.base=frame;int status=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
   if(status<0){display_error=status;if(log)fprintf(log,"ERROR display code=%d\n",status);break;}
  }
  now=sceKernelGetSystemTimeWide();
  if(now-window_start>=2000000){
   fps=(float)draws*1000000.0f/(float)(now-window_start);
   if(log){fprintf(log,"PERF time_us=%llu screen=%s draw_fps=%.2f cpu_avg_us=%llu cpu_max_us=%llu frames=%u tick=%llu caravan_frame=%u logical_width=%d logical_height=%d compose_avg_us=%llu scale_avg_us=%llu lightweight=%d cache_rebuilds=%u frame_interval_max_us=%llu logic_steps=%u logic_total_us=%llu upload_rows=%u upload_avg_us=%llu\n",(unsigned long long)now,menu_screen_name(menu.screen),fps,(unsigned long long)(draws?cpu_sum/draws:0),(unsigned long long)cpu_max,draws,(unsigned long long)menu.total_ticks,(unsigned)(menu.total_ticks/7)%8,640/reduction,360/reduction,(unsigned long long)(draws?compose_sum/draws:0),(unsigned long long)(draws?scale_sum/draws:0),lightweight,scene_cache_rebuilds(),(unsigned long long)interval_max,logic_steps,(unsigned long long)logic_us,uploaded_rows,(unsigned long long)(draws?upload_sum/draws:0));fflush(log);}
   if(log&&menu.screen==MENU_WORLD){fprintf(log,"PLAYER time_us=%llu class=%d motion=%s x=%.3f y=%.3f vx=%.3f vy=%.3f ground=%d ladder=%d duck=%d contacts=%u hp=%d max_hp=%d input=%u ticks=%u\n",(unsigned long long)now,player.player_class,player_motion_name(player.motion),player.x,player.y,player.vx,player.vy,player.on_ground,player.on_ladder,player.ducking,player.contacts,player.hp,player.max_hp,player.previous,player.ticks);fflush(log);}
   window_start=now;draws=0;cpu_sum=0;cpu_max=0;compose_sum=0;scale_sum=0;interval_max=0;logic_steps=0;logic_us=0;uploaded_rows=0;upload_sum=0;
  }
  sceDisplayWaitVblankStart();
 }
 if(log){fprintf(log,"EXIT normal=%d screen=%s display_error=%d\n",display_error==0,menu_screen_name(menu.screen),display_error);fclose(log);}
 free_world_visuals();free_resources();sceDisplaySetFrameBuf(NULL,SCE_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
 sceKernelFreeMemBlock(block);sceKernelExitProcess(0);return 0;
}
