/* Vita homebrew runtime and ACT_ONE world loop. */
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
#include "input_replay.h"
#include "campaign_flow.h"
#include "frame_scaler.h"
static World world;
static NativePlayer player;
static int act_one_exit_reached;
static int act_one_boss_gate_reported;
static int act_one_floor_index=-1;
unsigned int _newlib_heap_size_user = 16 * 1024 * 1024;

static uint32_t *frame;
static void text(int x,int y,const char *s,uint32_t color){native_text(frame,x,y,s,color);}
static int load_act_one_campaign_floor(World *destination,NativePlayer *active_player,int player_class,unsigned floor_index,FILE *log,FILE *asset_log){
 if(!destination||!active_player||floor_index>2)return 0;
 const char *directory="ux0:data/Vagante2Vita/world/act-one";
 char world_path[192],entity_path[192],enemy_path[192],terrain_file[96],melee_path[192],bow_path[192];
 if(floor_index==0){
  snprintf(world_path,sizeof(world_path),"%s/act-one-world.vwi",directory);
  snprintf(entity_path,sizeof(entity_path),"%s/act-one-entities.vwe",directory);
  snprintf(enemy_path,sizeof(enemy_path),"%s/act-one-enemies.vwe",directory);
  strcpy(terrain_file,"intro-terrain.png");
 }else{
  snprintf(world_path,sizeof(world_path),"%s/floors/floor-%02u.vwi",directory,floor_index);
  snprintf(entity_path,sizeof(entity_path),"%s/floors/floor-%02u-entities.vwe",directory,floor_index);
  snprintf(enemy_path,sizeof(enemy_path),"%s/floors/floor-%02u-enemies.vwe",directory,floor_index);
  snprintf(terrain_file,sizeof(terrain_file),"floors/floor-%02u-terrain.png",floor_index);
 }
 snprintf(melee_path,sizeof(melee_path),"%s/intro-melee.vwa",directory);
 snprintf(bow_path,sizeof(bow_path),"%s/intro-bow.vwb",directory);
 World *candidate=calloc(1,sizeof(*candidate));if(!candidate)return 0;
 int ready=world_load(candidate,world_path,log)&&world_load_entities(candidate,entity_path,log)&&world_load_enemies(candidate,enemy_path,log);
 if(ready){char attack_path[192];snprintf(attack_path,sizeof(attack_path),"%s/act-one-goblin-attack.vga",directory);ready=world_load_goblin_attack(candidate,attack_path,log);}
 if(ready){char loot_path[192];snprintf(loot_path,sizeof(loot_path),"%s/item-loot.vlt",directory);ready=world_load_loot(candidate,loot_path,log);if(ready){candidate->loot.enabled=1;candidate->loot_level=(int)floor_index;}}
 NativePlayer next_player;
 if(ready){melee_load(&candidate->melee,melee_path,log);bow_load(&candidate->bow,bow_path,log);ready=load_world_visuals_for_terrain(directory,terrain_file,player_class,candidate,asset_log)&&player_spawn(&next_player,candidate,player_class)&&player_carry_campaign_state(&next_player,active_player,candidate);}
 if(ready){
  candidate->bosses_defeated_mask=destination->bosses_defeated_mask;
  *destination=*candidate;*active_player=next_player;
  active_player->melee.definition=melee_find(&destination->melee,(unsigned)active_player->loadout.items[0].type);
  if(player_class==1&&destination->bow.ready)active_player->bow.definition=&destination->bow;
  if(log)fprintf(log,"WORLD_FLOOR_READY floor_index=%u source=%s entities=%u hp=%d\n",floor_index,world_path,destination->entity_count,active_player->hp);
 }
 free(candidate);return ready;
}
static uint32_t background[640*360];
static void present_background(int reduction){
 frame_scale_960x544(background,reduction,frame,1024);
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
 FILE *session_log=open_session_log("ux0:data/Vagante2Vita/logs/iteration" VAGANTE_ITERATION,log_path,sizeof(log_path));
 FILE *log=session_log;
 if(log){fprintf(log,"LOG path=%s\n",log_path);fprintf(log,"BOOT version=" VAGANTE_VERSION " iteration=" VAGANTE_ITERATION " heap_bytes=%u framebuffer_bytes=%u renderer=software logical=640x360 output=960x540 target_draw_hz=60 direct_row_scaler=1 profile=lightweight scenery_cached=1 prepared_colors=0 particles=0\n",_newlib_heap_size_user,bytes*2);fprintf(log,"PENDING exact_intro_tween day_night_tints audio unlocks saves procedural_generation combat skills inventory online options\n");fflush(log);}
 InputReplay replay={0};
 const char *replay_path="ux0:data/Vagante2Vita/autotest.vts";
 if(replay_load(&replay,replay_path,log)){
  char consumed[300];snprintf(consumed,sizeof(consumed),"%s.replay",log_path);
  if(!log||rename(replay_path,consumed)){replay.active=0;if(log)fprintf(log,"AUTOTEST enabled=0 error=consume_failed\n");}
  else if(log)fprintf(log,"AUTOTEST playback=fixed_step catch_up=disabled start_intro=%u stop_after_first_floor=%u fast_forward=%u\n",replay.start_intro,replay.stop_after_first_floor,replay.fast_forward);
 }
 if(session_log&&replay.active&&replay.fast_forward){
  char event_path[300];snprintf(event_path,sizeof(event_path),"%s.events",log_path);
  FILE *events=fopen(event_path,"w");
  if(events){
   fprintf(events,"BOOT version=" VAGANTE_VERSION " iteration=" VAGANTE_ITERATION " fast_forward=1\n");
   fprintf(events,"AUTOTEST enabled=1\nAUTOTEST playback=fixed_step catch_up=disabled start_intro=%u stop_after_first_floor=%u fast_forward=1\n",replay.start_intro,replay.stop_after_first_floor);
   fflush(events);log=events;
  }
 }
 FILE *asset_log=replay.active&&replay.fast_forward?NULL:log;
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
 if(cache){fclose(cache);if(log)fprintf(log,"CACHE candidate=menu-only.vra\n");loaded=load_resources("ux0:data/Vagante2Vita/menu-only.vra",asset_log);if(!loaded&&log)fprintf(log,"WARN menu_cache_invalid fallback=data.vra\n");}
 if(!loaded){if(log)fprintf(log,"SOURCE archive=data.vra\n");loaded=load_resources("ux0:data/Vagante2Vita/data.vra",asset_log);}
 else if(log)fprintf(log,"SOURCE archive=menu-only.vra scope=menu_only origin_not_rehashed_on_vita\n");
 if(log){fprintf(log,"LOAD complete=%d load_us=%llu\n",loaded,(unsigned long long)(sceKernelGetSystemTimeWide()-start));fflush(log);}
 Menu menu={0};log_state(log,&menu,0);
 unsigned previous=0,replay_log_tick=0;int buffer_index=0,paused=0,character=0,character_page=0,display_error=0,reduction=1,lightweight=1;
 const char *labels[]={"NEW GAME","PLAY ONLINE","OPTIONS","EXIT GAME"};
 uint64_t last_tick=sceKernelGetSystemTimeWide(),next_draw=last_tick,accumulator=0;
 uint64_t state_log_time=last_tick;
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 unsigned replay_loop_heartbeat=0;
 int replay_logged_walk=0,replay_logged_jump=0,replay_logged_arrow=0;
 int replay_logged_attack[3]={0,0,0};
 for(;;){
  if(replay.done){if(log)fprintf(log,"AUTOTEST sequence_consumed=1 final_screen=%s assertions_pending_on_host=1\n",menu_screen_name(menu.screen));break;}
  unsigned heartbeat_loop=++replay_loop_heartbeat;
  int heartbeat_due=log&&replay.active&&!replay.fast_forward&&heartbeat_loop%60u==0u;
  if(heartbeat_due){fprintf(log,"AUTOTEST heartbeat stage=loop_begin loop=%u screen=%s player_ticks=%u step=%u elapsed=%u\n",heartbeat_loop,menu_screen_name(menu.screen),player.ticks,replay.index,replay.elapsed);fflush(log);}
  SceCtrlData pad;memset(&pad,0,sizeof(pad));pad.lx=pad.ly=pad.rx=pad.ry=128;
  sceCtrlPeekBufferPositive(0,&pad,1);
  if(replay.active){pad.buttons=replay_buttons(&replay);pad.lx=pad.ly=pad.rx=pad.ry=128;}
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
     player.previous=player_buttons(&pad);
     /* Keep replay/menu time moving while the character panel is open.
        Reset only on its edges so gameplay input cannot leak across it. */
     if(was_character!=character)accumulator=0;
     if(log&&((pressed&(SCE_CTRL_SELECT|SCE_CTRL_CIRCLE|SCE_CTRL_LEFT|SCE_CTRL_RIGHT))!=0)){
      fprintf(log,"CHARACTER_PANEL open=%d page=%d tick=%u\n",character,character_page,player.ticks);fflush(log);
     }
    }
    if(pressed&SCE_CTRL_TRIANGLE){menu.screen=MENU_CLASS;paused=0;character=0;}
   }else menu_input(&menu,mapped_buttons(pad.buttons));
   if(old_screen==MENU_CLASS&&menu.screen==MENU_BLOCKED){
    uint64_t world_start=sceKernelGetSystemTimeWide();
    scene_free_cache();
    const char *directories[]={"ux0:data/Vagante2Vita/world/act-one","ux0:data/Vagante2Vita/world"};
    const char *packets[]={"act-one-world.vwi","intro-world.vwi"};
    unsigned source_order[2]={1,0};
    if(replay.active&&!replay.start_intro){source_order[0]=0;source_order[1]=1;}
    const char *chosen_directory="ux0:data/Vagante2Vita/world";
    int world_ready=0,chosen_source=-1;
    for(unsigned attempt=0;attempt<2&&!world_ready;attempt++){
     unsigned source=source_order[attempt];
     char packet[192];snprintf(packet,sizeof(packet),"%s/%s",directories[source],packets[source]);
     world_ready=world_load(&world,packet,log);
     if(world_ready){
      char entity_packet[192];
      snprintf(entity_packet,sizeof(entity_packet),"%s/%s",directories[source],source==0?"act-one-entities.vwe":"intro-entities.vwe");
      int entities_ready=world_load_entities(&world,entity_packet,log);
      if(source==0&&!entities_ready)world_ready=0;
      if(source==0&&world_ready){char enemy_packet[192];snprintf(enemy_packet,sizeof(enemy_packet),"%s/act-one-enemies.vwe",directories[source]);if(!world_load_enemies(&world,enemy_packet,log))world_ready=0;}
      if(source==0&&world_ready){char attack_packet[192];snprintf(attack_packet,sizeof(attack_packet),"%s/act-one-goblin-attack.vga",directories[source]);if(!world_load_goblin_attack(&world,attack_packet,log))world_ready=0;}
      if(source==0&&world_ready){char loot_packet[192];snprintf(loot_packet,sizeof(loot_packet),"%s/item-loot.vlt",directories[source]);if(!world_load_loot(&world,loot_packet,log))world_ready=0;else world.loot.enabled=1;}
      if(world_ready)world_ready=load_world_visuals(directories[source],menu.player_class,&world,asset_log)&&player_spawn(&player,&world,menu.player_class);
      if(world_ready){chosen_directory=directories[source];chosen_source=(int)source;}
     }
     if(log)fprintf(log,"WORLD_SOURCE directory=%s ready=%d\n",directories[source],world_ready);
    }
    if(world_ready){
     char melee_path[192],bow_path[192];
     snprintf(melee_path,sizeof(melee_path),"%s/intro-melee.vwa",chosen_directory);
     snprintf(bow_path,sizeof(bow_path),"%s/intro-bow.vwb",chosen_directory);
     melee_load(&world.melee,melee_path,log);
     bow_load(&world.bow,bow_path,log);
     if(menu.player_class==1&&world.bow.ready)player.bow.definition=&world.bow;
     player.melee.definition=melee_find(&world.melee,(unsigned)player.loadout.items[0].type);
     act_one_floor_index=chosen_source==0?0:-1;
     menu.screen=MENU_WORLD;character=0;player.previous=player_buttons(&pad);
     if(log)fprintf(log,"WORLD_START complete=1 class=%d load_us=%llu hp=%d entities=%u\n",menu.player_class,(unsigned long long)(sceKernelGetSystemTimeWide()-world_start),player.hp,world.entity_count);
     if(log&&!replay.fast_forward){
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
  /* Replay traces are recorded one controller state per simulation tick.
     Fast-forward tests synthesize exactly one tick per outer loop and skip
     presentation; visible replays stay tied to wall-clock time. */
  if(replay.active&&replay.fast_forward)accumulator=16667;
  else if(replay.active&&accumulator>16667)accumulator=16667;
  unsigned old_arrows=player.arrows.fired,old_impacts=player.arrows.impacts,old_enemy_hits=player.arrows.enemy_hits,old_enemy_kills=player.arrows.enemy_kills,old_bow_frame=player.bow.keyframe,old_bow_active=player.bow.active;
  unsigned old_attack_serial=player.melee.serial,old_attack_frame=player.melee.keyframe,old_attack_active=player.melee.active;
  PlayerMotion old_motion=player.motion;unsigned old_input=player.previous;
  uint64_t replay_before=menu.total_ticks;
  while(accumulator>=16667){
   if(loaded){
    menu_tick(&menu);
    if(menu.screen==MENU_WORLD&&!paused&&!character){
     unsigned chests_before=player.chests_opened,items_before=player.items_collected,entities_before=world.entity_count;
     unsigned bosses_defeated_before=world.bosses_defeated;
     if(heartbeat_due){fprintf(log,"AUTOTEST heartbeat stage=player_step_begin loop=%u player_ticks=%u input=%u\n",heartbeat_loop,player.ticks,player_buttons(&pad));fflush(log);}
     player_step(&player,&world,player_buttons(&pad));
     if(heartbeat_due){fprintf(log,"AUTOTEST heartbeat stage=player_step_end loop=%u player_ticks=%u hp=%d\n",heartbeat_loop,player.ticks,player.hp);fflush(log);}
      for(unsigned i=0;i<world.entity_count;i++){
       WorldEntity *e=&world.entities[i];
       if(e->kind!=WORLD_KIND_BOSS)continue;
       if(log&&!replay.fast_forward&&player.ticks%120u==0u)fprintf(log,"WORLD_BOSS_STATE tick=%u subtype=%u hp=%.1f x=%.1f y=%.1f player=%.1f,%.1f player_hp=%d\n",player.ticks,e->subtype,e->hp,e->x_q8/256.0f,e->y_q8/256.0f,player.x,player.y,player.hp);
      }
     if(log&&world.bosses_defeated>bosses_defeated_before){
      unsigned rewards=0;for(unsigned i=0;i<world.entity_count;i++)rewards+=world.entities[i].kind==16&&world.entities[i].boss_reward;
      fprintf(log,"WORLD_BOSS_DEFEAT count=%u defeated_mask=%u reward_chests=%u reward_model=provisional_floor_plus_3 native_parity=0\n",world.bosses_defeated,world.bosses_defeated_mask,rewards);fflush(log);
     }
     if(log&&player.chests_opened>chests_before){unsigned drop=world.entity_count>entities_before?world.entity_count-1:0;fprintf(log,"WORLD_INTERACTION event=chest_opened opened=%u total=%u loot_type=%u spawned=%d\n",player.chests_opened-chests_before,player.chests_opened,world.entity_count>entities_before?world.entities[drop].item_type:0,world.entity_count>entities_before);}
     if(log&&player.items_collected>items_before)fprintf(log,"WORLD_LOOT event=collected total=%u type=%d\n",player.items_collected,player.loadout.items[player.loadout.item_count-1].type);
     unsigned boulder_triggers=world_boulder_traps_step(&world,player.x,player.y,player.w,player.h);
     world_boulders_step(&world);
     int hp_before_boulder=player.hp;
     int boulder_hit=player_boulder_contact(&player,&world);
     if(boulder_hit&&log){fprintf(log,"WORLD_INTERACTION event=boulder_hit_player damage=%d hp=%d dead=%d\n",hp_before_boulder-player.hp,player.hp,player.dead);fflush(log);}
     if(boulder_triggers&&log){fprintf(log,"WORLD_INTERACTION event=boulder_trap_triggered count=%u boulder_release_pending=1\n",boulder_triggers);fflush(log);}
     if(player.dead){
      if(log){fprintf(log,"WORLD_DEATH cause=%s class=%d tick=%u retry=class_select\n",player_death_cause_name((PlayerDeathCause)player.death_cause),player.player_class,player.ticks);fflush(log);}
      menu.screen=MENU_CLASS;character=0;paused=0;accumulator=0;
      if(replay.active){replay.active=0;replay.done=1;if(log){fprintf(log,"AUTOTEST failed=player_death cause=%s\n",player_death_cause_name((PlayerDeathCause)player.death_cause));fflush(log);}}
     }
     int at_world_exit=world_goal_touched(&world,player.x,player.y,player.w,player.h);
     CampaignExitAction exit_action=campaign_exit_action(act_one_floor_index,
       player.dead,at_world_exit,world.bosses_loaded,world.bosses_defeated,world.bosses_defeated_mask);
     if(exit_action==CAMPAIGN_EXIT_BLOCKED_BOSS){
      if(log&&!act_one_boss_gate_reported){
       fprintf(log,"WORLD_EXIT blocked=1 reason=%s bosses_loaded=%u bosses_defeated=%u\n",
         world.bosses_loaded?"boss_alive":"boss_missing",world.bosses_loaded,world.bosses_defeated);fflush(log);
      }
      act_one_boss_gate_reported=1;
     }
     if(exit_action==CAMPAIGN_EXIT_BLOCKED_ROSTER&&log&&!act_one_boss_gate_reported){
      fprintf(log,"WORLD_EXIT blocked=1 reason=boss_roster_incomplete defeated_mask=%u required_mask=7\n",world.bosses_defeated_mask);fflush(log);
      act_one_boss_gate_reported=1;
     }
     if(exit_action==CAMPAIGN_EXIT_ADVANCE_FLOOR||
        exit_action==CAMPAIGN_EXIT_ENTER_ACT_ONE||
        exit_action==CAMPAIGN_EXIT_COMPLETE){
      if(log){fprintf(log,"WORLD_EXIT reached=1 tick=%u x=%.3f y=%.3f goal=%d,%d next_floor_pending=1\n",player.ticks,player.x,player.y,world.goal_x,world.goal_y);fflush(log);}
      act_one_exit_reached=1;
      act_one_boss_gate_reported=0;
      if(exit_action==CAMPAIGN_EXIT_ADVANCE_FLOOR){
       unsigned next_floor=(unsigned)act_one_floor_index+1;
       int advanced=load_act_one_campaign_floor(&world,&player,menu.player_class,next_floor,log,asset_log);
       if(advanced){act_one_floor_index=(int)next_floor;act_one_exit_reached=0;if(log)fprintf(log,"WORLD_TRANSITION complete=1 source=act-one target=act-one mode=next-floor floor_index=%d\n",act_one_floor_index);if(replay.active&&replay.start_intro&&next_floor==1){replay.active=0;replay.done=1;if(log){fprintf(log,"AUTOTEST intro_entry_sequence_consumed=1\n");fflush(log);}}else if(replay.active&&replay.stop_after_first_floor&&next_floor==1){replay.active=0;replay.done=1;if(log){fprintf(log,"AUTOTEST act_one_exit_sequence_consumed=1\n");fflush(log);}}}
       else {menu.screen=MENU_BLOCKED;if(log)fprintf(log,"WORLD_TRANSITION complete=0 error=next_floor_load_failed floor_index=%u\n",next_floor);}
      }else if(exit_action==CAMPAIGN_EXIT_ENTER_ACT_ONE){
       int advanced=load_act_one_campaign_floor(&world,&player,menu.player_class,0,log,asset_log);
       if(advanced){act_one_floor_index=0;act_one_exit_reached=0;if(log)fprintf(log,"WORLD_TRANSITION complete=1 source=intro target=act-one mode=first-floor floor_index=0\n");}
       else {menu.screen=MENU_BLOCKED;if(log)fprintf(log,"WORLD_TRANSITION complete=0 error=act_one_data_unavailable floor_index=0\n");}
      }else if(exit_action==CAMPAIGN_EXIT_COMPLETE){
       menu.screen=MENU_COMPLETE;character=0;paused=0;accumulator=0;
       if(log)fprintf(log,"ACT_ONE_FLOORS complete=1 count=3 boss_defeated=1 defeated_mask=%u campaign_complete=1\n",world.bosses_defeated_mask);
      }
     }
    }
   }
   accumulator-=16667;
  }
  replay_advance(&replay,(unsigned)(menu.total_ticks-replay_before));
  if(heartbeat_due){fprintf(log,"AUTOTEST heartbeat stage=replay_advance_end loop=%u player_ticks=%u step=%u elapsed=%u\n",heartbeat_loop,player.ticks,replay.index,replay.elapsed);fflush(log);}
  if(log&&replay.active&&!replay.fast_forward&&menu.total_ticks-replay_log_tick>=600u){fprintf(log,"AUTOTEST progress step=%u/%u elapsed=%u floor=%d player_ticks=%u hp=%d x=%.1f y=%.1f\n",replay.index,replay.count,replay.elapsed,act_one_floor_index,player.ticks,player.hp,player.x,player.y);fflush(log);replay_log_tick=menu.total_ticks;}
  if(act_one_exit_reached&&act_one_floor_index==2&&replay.active){replay.active=0;replay.done=1;if(log){fprintf(log,"AUTOTEST act_one_exit_sequence_consumed=1\nAUTOTEST act_one_campaign_sequence_consumed=1\n");fflush(log);}}
  int sampled_replay=replay.active&&replay.fast_forward;
  int emit_player_event=!sampled_replay||
    (player.motion==PLAYER_WALK&&!replay_logged_walk)||
    (player.motion==PLAYER_RISE&&!replay_logged_jump);
  if(log&&emit_player_event&&menu.screen==MENU_WORLD&&(old_motion!=player.motion||old_input!=player.previous)){
   fprintf(log,"PLAYER_EVENT tick=%u motion=%s input=%u x=%.3f y=%.3f vx=%.3f vy=%.3f contacts=%u\n",player.ticks,player_motion_name(player.motion),player.previous,player.x,player.y,player.vx,player.vy,player.contacts);
   if(player.motion==PLAYER_WALK)replay_logged_walk=1;
   if(player.motion==PLAYER_RISE)replay_logged_jump=1;
  }
  if(log&&menu.screen==MENU_WORLD&&!paused&&!character){
   if((!sampled_replay||(!replay_logged_arrow&&player.arrows.fired>0))&&
      (old_arrows!=player.arrows.fired||old_impacts!=player.arrows.impacts||old_bow_frame!=player.bow.keyframe||old_bow_active!=player.bow.active)){
    fprintf(log,"BOW_EVENT active=%u keyframe=%u timer=%u fired=%u terrain_hits=%u expired=%u\n",player.bow.active,player.bow.keyframe,player.bow.timer,player.arrows.fired,player.arrows.impacts,player.arrows.expired);
    if(player.arrows.fired>0)replay_logged_arrow=1;
   }
   if(!sampled_replay&&(old_enemy_hits!=player.arrows.enemy_hits||old_enemy_kills!=player.arrows.enemy_kills))
    fprintf(log,"WORLD_COMBAT ranged_hits=%u ranged_kills=%u\n",player.arrows.enemy_hits,player.arrows.enemy_kills);
   if((player.previous&PLAYER_RANGE)&&!(old_input&PLAYER_RANGE)&&!player.bow.active)fprintf(log,"BOW_BLOCKED missing_bow_or_data_or_ammo_or_state=1\n");

   if((!sampled_replay||((unsigned)player.player_class<3u&&!replay_logged_attack[player.player_class]))&&
      (old_attack_serial!=player.melee.serial||old_attack_frame!=player.melee.keyframe||old_attack_active!=player.melee.active)){
    const MeleeFrame *key=melee_frame(&player.melee);
    if(!sampled_replay||(key&&key->enabled&&player.melee.serial==1u)){
     fprintf(log,"MELEE_EVENT serial=%u active=%u keyframe=%u timer=%u shape_active=%u charge_ticks=%u type=%u\n",player.melee.serial,player.melee.active,player.melee.keyframe,player.melee.timer,key?key->enabled:0,player.melee.charge_ticks,player.melee.definition?player.melee.definition->type:0);
     if(sampled_replay&&(unsigned)player.player_class<3u)replay_logged_attack[player.player_class]=1;
    }
   }
   if(!sampled_replay&&(player.previous&PLAYER_ATTACK)&&!(old_input&PLAYER_ATTACK)&&!player.melee.definition)fprintf(log,"MELEE_BLOCKED reason=missing_external_data\n");
  }
  if(old_screen!=menu.screen||old_cursor!=menu.cursor||old_class!=menu.player_class)log_state(log,&menu,pad.buttons);
  if(!(replay.active&&replay.fast_forward)&&now>=next_draw){
   do{next_draw+=16667;}while(next_draw<=now);
   buffer_index^=1;frame=buffers+buffer_index*(bytes/4);
   if(loaded){
    if(menu.screen==MENU_WORLD)draw_world(background,&world,&player,reduction);
    else draw_scene_profile(background,&menu,reduction,lightweight);
    present_background(reduction);
    if(menu.screen==MENU_WORLD){
     char info[64];snprintf(info,sizeof(info),"HP %d OF %d  STR %d DEX %d VIT %d",player.hp,player.max_hp,player.stats.strength,player.stats.dexterity,player.stats.vitality);text(18,42,info,0xff80ddff);
     snprintf(info,sizeof(info),"SPD %d STA %d INT %d LUK %d",player.stats.speed,player.stats.stamina,player.stats.intelligence,player.stats.luck);text(18,75,info,0xffffffff);
     text(18,475,paused?"PAUSED START TO RESUME":"WALK TO CAVE TO ENTER  NO CIRCLE",0xffffffff);
     text(18,495,"D-PAD/STICK MOVE  X JUMP  DOWN+X DROP",0xffffffff);
     text(18,515,"SELECT CHARACTER TRIANGLE RETURN",0xffaaaaaa);
    }
    if(menu.screen==MENU_WORLD&&character)draw_character_panel(frame,&player,character_page);
    if(menu.screen==MENU_TITLE)for(int i=0;i<4;i++)text(360,290+38*i,labels[i],i==menu.cursor?0xff80ddff:0xffdddddd);
    if(menu.screen==MENU_CLASS){text(225,49,"CHOOSE CLASS",0xff80ddff);text(195,425,"LEFT RIGHT TO SELECT",0xffffffff);text(195,461,"X CONFIRM  CIRCLE BACK",0xffffffff);}
    if(menu.screen==MENU_COMPLETE){
     for(int y=245;y<490;y++)for(int x=65;x<895;x++)frame[y*1024+x]=0xff20170f;
     text(255,290,"ACT ONE COMPLETE",0xff80ddff);
     text(207,347,"GOBLIN KING DEFEATED",0xffffffff);
     text(235,435,"X CONTINUE  CIRCLE TITLE",0xffffffff);
    }
    if(menu.screen==MENU_BLOCKED||menu.screen==MENU_NOTICE){
     for(int y=260;y<485;y++)for(int x=65;x<895;x++)frame[y*1024+x]=0xff20170f;
     text(95,285,menu.screen==MENU_BLOCKED?"CLASS SELECTED":menu.notice==1?"ONLINE PLAY PENDING":"OPTIONS RECOVERY PENDING",0xff80ddff);
     if(menu.screen==MENU_BLOCKED){text(95,337,"CHECK UX0 DATA VAGANTE2VITA WORLD",0xff80b0ff);text(95,379,"BLOCKER RECORDED IN LOG",0xffffffff);}
     text(95,435,"CIRCLE TO RETURN",0xffffffff);
    }
    if(menu.screen==MENU_TITLE)text(18,480,lightweight==2?"R SWITCH TO LIGHTWEIGHT":lightweight==1?"R SWITCH TO FULL ANIMATION":"R SWITCH TO PREPARED ANIMATION",0xffaaaaaa);
    if(menu.screen==MENU_TITLE)text(18,498,reduction==1?"L RESOLUTION 640x360":"L RESOLUTION 320x180",0xffaaaaaa);
    if(menu.screen!=MENU_WORLD&&menu.screen!=MENU_COMPLETE)text(18,515,log?"ITERATION " VAGANTE_ITERATION "  GAMEPLAY PENDING":"ERROR LOG FILE COULD NOT OPEN",log?0xffaaaaaa:0xff8080ff);
   }else{
    memset(frame,0,bytes);text(48,100,"ARCHIVE LOAD FAILED",0xff8080ff);
    text(48,180,"CHECK DATA VRA AND RUNTIME LOG",0xffffffff);text(48,260,"PRESS START TO EXIT",0xffffffff);
   }
   fb.base=frame;int status=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
   if(status<0){display_error=status;if(log)fprintf(log,"ERROR display code=%d\n",status);break;}
  }
  now=sceKernelGetSystemTimeWide();
  if(now-state_log_time>=2000000){
   if(log&&menu.screen==MENU_WORLD&&!sampled_replay){fprintf(log,"PLAYER time_us=%llu class=%d motion=%s x=%.3f y=%.3f vx=%.3f vy=%.3f ground=%d ladder=%d duck=%d contacts=%u hanging=%d hp=%d max_hp=%d input=%u ticks=%u\n",(unsigned long long)now,player.player_class,player_motion_name(player.motion),player.x,player.y,player.vx,player.vy,player.on_ground,player.on_ladder,player.ducking,player.contacts,player.hanging,player.hp,player.max_hp,player.previous,player.ticks);fflush(log);}
   state_log_time=now;
  }
  if(heartbeat_due){fprintf(log,"AUTOTEST heartbeat stage=vblank_begin loop=%u player_ticks=%u\n",heartbeat_loop,player.ticks);fflush(log);}
  if(replay.active){
   /* Vita3K can leave an unattended replay blocked in its vblank wait after
      repeated software-framebuffer submissions. Fixed-step replay already
      caps simulation to one tick per outer loop, so yield briefly instead. */
   sceKernelDelayThread(1000);
  }else sceDisplayWaitVblankStart();
  if(heartbeat_due){fprintf(log,"AUTOTEST heartbeat stage=vblank_end loop=%u player_ticks=%u\n",heartbeat_loop,player.ticks);fflush(log);}
 }
 if(log){fprintf(log,"EXIT normal=%d screen=%s display_error=%d\n",display_error==0,menu_screen_name(menu.screen),display_error);fclose(log);}
 if(session_log&&session_log!=log)fclose(session_log);
 scene_free_cache();free_world_visuals();free_resources();sceDisplaySetFrameBuf(NULL,SCE_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
 /* The process manager reclaims CDRAM at process exit. Keep the final
    framebuffer alive through display/emulator teardown. */
 sceKernelExitProcess(0);return 0;
}
