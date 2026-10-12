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
#include "native_ui.h"
static World world;
static NativePlayer player;
unsigned int _newlib_heap_size_user = 16 * 1024 * 1024;

static uint32_t *frame;
static void text(int x,int y,const char *s,uint32_t color){native_text(frame,x,y,s,color);}
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
 if(b&SCE_CTRL_SQUARE)result|=PLAYER_ATTACK;
 if(b&SCE_CTRL_RTRIGGER)result|=PLAYER_RANGE;
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
 if(log){fprintf(log,"LOG path=%s\n",log_path);fprintf(log,"BOOT version=" VAGANTE_VERSION " iteration=" VAGANTE_ITERATION " heap_bytes=%u framebuffer_bytes=%u renderer=software logical=320x180 target_draw_hz=60 direct_row_scaler=1 profile=lightweight scenery_cached=1 prepared_colors=0 particles=0\n",_newlib_heap_size_user,bytes*2);fprintf(log,"PENDING exact_intro_tween day_night_tints audio unlocks saves procedural_generation combat skills inventory online options\n");fflush(log);}
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
 unsigned previous=0;int buffer_index=0,paused=0,character=0,character_page=0,display_error=0,reduction=2,lightweight=1;
 const char *labels[]={"NEW GAME","PLAY ONLINE","OPTIONS","EXIT GAME"};
 uint64_t last_tick=sceKernelGetSystemTimeWide(),next_draw=last_tick,accumulator=0;
 uint64_t state_log_time=last_tick;
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 for(;;){
  SceCtrlData pad;memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);
  unsigned pressed=pad.buttons&~previous;previous=pad.buttons;
  if((pad.buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT))break;
  if(!loaded&&(pressed&SCE_CTRL_START))break;
  if(menu.screen==MENU_WORLD&&!character&&(pressed&SCE_CTRL_START)){
   paused=!paused;accumulator=0;player.previous=player_buttons(&pad);
   if(log){fprintf(log,"PAUSE active=%d tick=%u\n",paused,player.ticks);fflush(log);}
  }
  if(menu.screen!=MENU_WORLD&&(pressed&SCE_CTRL_LTRIGGER)){reduction=reduction==2?1:2;if(log){fprintf(log,"PROFILE logical_width=%d logical_height=%d\n",640/reduction,360/reduction);fflush(log);}}
  if(menu.screen!=MENU_WORLD&&(pressed&SCE_CTRL_RTRIGGER)){lightweight=(lightweight+2)%3;scene_invalidate_cache();if(log){fprintf(log,"QUALITY profile=%d scenery_cached=%d prepared_colors=%d particles=%d\n",lightweight,lightweight==1,lightweight==2,lightweight==1?0:lightweight==2?4:12);fflush(log);}}
  MenuScreen old_screen=menu.screen;int old_cursor=menu.cursor,old_class=menu.player_class;
  if(loaded){
   if(menu.screen==MENU_TITLE&&menu.cursor==3&&(pressed&SCE_CTRL_CROSS))break;
   if(menu.screen==MENU_WORLD){
    int was_character=character;
    if(pressed&SCE_CTRL_SELECT){character=!character;character_page=0;}
    if(character){
     if(pressed&SCE_CTRL_LEFT)character_page=(character_page+2)%3;
     if(pressed&SCE_CTRL_RIGHT)character_page=(character_page+1)%3;
     if(pressed&SCE_CTRL_CIRCLE)character=0;
    }
    if(was_character||character){
     player.previous=player_buttons(&pad);accumulator=0;
     if(log&&((pressed&(SCE_CTRL_SELECT|SCE_CTRL_CIRCLE|SCE_CTRL_LEFT|SCE_CTRL_RIGHT))!=0)){
      fprintf(log,"CHARACTER_PANEL open=%d page=%d tick=%u\n",character,character_page,player.ticks);fflush(log);
     }
    }
    if(!paused&&!was_character&&!character&&(pressed&SCE_CTRL_CIRCLE)){
     int reset=player_spawn(&player,&world,menu.player_class);player.previous=player_buttons(&pad);
     if(log)fprintf(log,"PLAYER_RESET complete=%d class=%d\n",reset,menu.player_class);
    }
    if(pressed&SCE_CTRL_TRIANGLE){menu.screen=MENU_CLASS;paused=0;character=0;}
   }else menu_input(&menu,mapped_buttons(pad.buttons));
   if(old_screen==MENU_CLASS&&menu.screen==MENU_BLOCKED){
    uint64_t world_start=sceKernelGetSystemTimeWide();
    scene_free_cache();
    const char *directories[]={"ux0:data/Vagante2Vita/world"};
    int world_ready=0;
    for(unsigned source=0;source<1&&!world_ready;source++){
     char packet[128];snprintf(packet,sizeof(packet),"%s/intro-world.vwi",directories[source]);
     world_ready=world_load(&world,packet,log)&&load_world_visuals(directories[source],menu.player_class,log)&&player_spawn(&player,&world,menu.player_class);
     if(log)fprintf(log,"WORLD_SOURCE directory=%s ready=%d\n",directories[source],world_ready);
    }
    if(world_ready){
     melee_load(&world.melee,"ux0:data/Vagante2Vita/world/intro-melee.vwa",log);
     bow_load(&world.bow,"ux0:data/Vagante2Vita/world/intro-bow.vwb",log);
     if(menu.player_class==1&&world.bow.ready)player.bow.definition=&world.bow;
     player.melee.definition=melee_find(&world.melee,(unsigned)player.loadout.items[0].type);
     world_load_entities(&world,"ux0:data/Vagante2Vita/world/intro-entities.vwe",log);
     menu.screen=MENU_WORLD;character=0;player.previous=player_buttons(&pad);
     if(log)fprintf(log,"WORLD_START complete=1 class=%d load_us=%llu hp=%d\n",menu.player_class,(unsigned long long)(sceKernelGetSystemTimeWide()-world_start),player.hp);
     if(log){
      fprintf(log,"PLAYER_BASE hp=%d move_speed=%.3f melee_base=%.3f ranged_base=%.3f evasion=%.6f critical=%.6f max_air=%.3f items=%u skills=%u\n",player.max_hp,player.derived.move_speed,player.derived.melee_base,player.derived.ranged_base,player.derived.evasion,player.derived.critical,player.derived.max_air,player.loadout.item_count,player.loadout.skill_count);
      for(unsigned n=0;n<player.loadout.item_count;n++)fprintf(log,"START_ITEM type=%d name=%s quantity=%d role=%d\n",player.loadout.items[n].type,starting_item_name(player.loadout.items[n].type),player.loadout.items[n].quantity,player.loadout.items[n].role);
      for(unsigned n=0;n<player.loadout.skill_count;n++)fprintf(log,"START_SKILL type=%d name=%s execution_pending=1\n",player.loadout.skills[n],starting_skill_name(player.loadout.skills[n]));
     }
     /* Loading is not simulation time; do not advance the player during I/O. */
     last_tick=sceKernelGetSystemTimeWide();accumulator=0;
    }else if(log)fprintf(log,"WORLD_START complete=0 class=%d check_world_data=1\n",menu.player_class);
   }
  }
  uint64_t now=sceKernelGetSystemTimeWide(),delta=now-last_tick;last_tick=now;
  if(delta>250000){if(log)fprintf(log,"WARN slow_loop_us=%llu\n",(unsigned long long)delta);delta=250000;}
  accumulator+=delta;
  unsigned old_arrows=player.arrows.fired,old_impacts=player.arrows.impacts,old_bow_frame=player.bow.keyframe,old_bow_active=player.bow.active;
  unsigned old_attack_serial=player.melee.serial,old_attack_frame=player.melee.keyframe,old_attack_active=player.melee.active;
  PlayerMotion old_motion=player.motion;unsigned old_input=player.previous;
  while(accumulator>=16667){if(loaded){menu_tick(&menu);if(menu.screen==MENU_WORLD&&!paused&&!character)player_step(&player,&world,player_buttons(&pad));}accumulator-=16667;}
  if(log&&menu.screen==MENU_WORLD&&(old_motion!=player.motion||old_input!=player.previous)){
   fprintf(log,"PLAYER_EVENT tick=%u motion=%s input=%u x=%.3f y=%.3f vx=%.3f vy=%.3f contacts=%u\n",player.ticks,player_motion_name(player.motion),player.previous,player.x,player.y,player.vx,player.vy,player.contacts);
  }
  if(log&&menu.screen==MENU_WORLD&&!paused&&!character){
   if(old_arrows!=player.arrows.fired||old_impacts!=player.arrows.impacts||old_bow_frame!=player.bow.keyframe||old_bow_active!=player.bow.active)
    fprintf(log,"BOW_EVENT active=%u keyframe=%u timer=%u fired=%u terrain_hits=%u expired=%u\n",player.bow.active,player.bow.keyframe,player.bow.timer,player.arrows.fired,player.arrows.impacts,player.arrows.expired);
   if((player.previous&PLAYER_RANGE)&&!(old_input&PLAYER_RANGE)&&!player.bow.active)fprintf(log,"BOW_BLOCKED missing_bow_or_data_or_ammo_or_state=1\n");

   if(old_attack_serial!=player.melee.serial||old_attack_frame!=player.melee.keyframe||old_attack_active!=player.melee.active){
    const MeleeFrame *key=melee_frame(&player.melee);
    fprintf(log,"MELEE_EVENT serial=%u active=%u keyframe=%u timer=%u shape_active=%u charge_ticks=%u type=%u\n",player.melee.serial,player.melee.active,player.melee.keyframe,player.melee.timer,key?key->enabled:0,player.melee.charge_ticks,player.melee.definition?player.melee.definition->type:0);
   }
   if((player.previous&PLAYER_ATTACK)&&!(old_input&PLAYER_ATTACK)&&!player.melee.definition)fprintf(log,"MELEE_BLOCKED reason=missing_external_data\n");
  }
  if(old_screen!=menu.screen||old_cursor!=menu.cursor||old_class!=menu.player_class)log_state(log,&menu,pad.buttons);
  if(now>=next_draw){
   do{next_draw+=16667;}while(next_draw<=now);buffer_index^=1;frame=buffers+buffer_index*(bytes/4);
   if(loaded){
    if(menu.screen==MENU_WORLD)draw_world(background,&world,&player,reduction);
    else draw_scene_profile(background,&menu,reduction,lightweight);
    present_background(reduction);
    if(menu.screen==MENU_WORLD){
     char info[64];snprintf(info,sizeof(info),"HP %d OF %d  STR %d DEX %d VIT %d",player.hp,player.max_hp,player.stats.strength,player.stats.dexterity,player.stats.vitality);text(18,42,info,0xff80ddff);
     snprintf(info,sizeof(info),"SPD %d STA %d INT %d LUK %d",player.stats.speed,player.stats.stamina,player.stats.intelligence,player.stats.luck);text(18,75,info,0xffffffff);
     text(18,475,paused?"PAUSED START TO RESUME":"SQUARE MELEE R BOW X JUMP",0xffffffff);
     text(18,515,"SELECT CHARACTER O RESET TRIANGLE RETURN",0xffaaaaaa);
    }
    if(menu.screen==MENU_WORLD&&character)draw_character_panel(frame,&player,character_page);
    if(menu.screen==MENU_TITLE)for(int i=0;i<4;i++)text(360,290+38*i,labels[i],i==menu.cursor?0xff80ddff:0xffdddddd);
    if(menu.screen==MENU_CLASS){text(225,49,"CHOOSE CLASS",0xff80ddff);text(195,425,"LEFT RIGHT TO SELECT",0xffffffff);text(195,461,"X CONFIRM  CIRCLE BACK",0xffffffff);}
    if(menu.screen==MENU_BLOCKED||menu.screen==MENU_NOTICE){
     for(int y=260;y<485;y++)for(int x=65;x<895;x++)frame[y*1024+x]=0xff20170f;
     text(95,285,menu.screen==MENU_BLOCKED?"CLASS SELECTED":menu.notice==1?"ONLINE PLAY PENDING":"OPTIONS RECOVERY PENDING",0xff80ddff);
     if(menu.screen==MENU_BLOCKED){text(95,337,"CHECK UX0 DATA VAGANTE2VITA WORLD",0xff80b0ff);text(95,379,"BLOCKER RECORDED IN LOG",0xffffffff);}
     text(95,435,"CIRCLE TO RETURN",0xffffffff);
    }
    if(menu.screen==MENU_TITLE)text(18,480,lightweight==2?"R SWITCH TO LIGHTWEIGHT":lightweight==1?"R SWITCH TO FULL ANIMATION":"R SWITCH TO PREPARED ANIMATION",0xffaaaaaa);
    if(menu.screen!=MENU_WORLD)text(18,515,log?"ITERATION " VAGANTE_ITERATION "  GAMEPLAY PENDING":"ERROR LOG FILE COULD NOT OPEN",log?0xffaaaaaa:0xff8080ff);
   }else{
    memset(frame,0,bytes);text(48,100,"ARCHIVE LOAD FAILED",0xff8080ff);
    text(48,180,"CHECK DATA VRA AND RUNTIME LOG",0xffffffff);text(48,260,"PRESS START TO EXIT",0xffffffff);
   }
   fb.base=frame;int status=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
   if(status<0){display_error=status;if(log)fprintf(log,"ERROR display code=%d\n",status);break;}
  }
  now=sceKernelGetSystemTimeWide();
  if(now-state_log_time>=2000000){
   if(log&&menu.screen==MENU_WORLD){fprintf(log,"PLAYER time_us=%llu class=%d motion=%s x=%.3f y=%.3f vx=%.3f vy=%.3f ground=%d ladder=%d duck=%d contacts=%u hanging=%d hp=%d max_hp=%d input=%u ticks=%u\n",(unsigned long long)now,player.player_class,player_motion_name(player.motion),player.x,player.y,player.vx,player.vy,player.on_ground,player.on_ladder,player.ducking,player.contacts,player.hanging,player.hp,player.max_hp,player.previous,player.ticks);fflush(log);}
   state_log_time=now;
  }
  sceDisplayWaitVblankStart();
 }
 if(log){fprintf(log,"EXIT normal=%d screen=%s display_error=%d\n",display_error==0,menu_screen_name(menu.screen),display_error);fclose(log);}
 scene_free_cache();free_world_visuals();free_resources();sceDisplaySetFrameBuf(NULL,SCE_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
 sceKernelFreeMemBlock(block);sceKernelExitProcess(0);return 0;
}
