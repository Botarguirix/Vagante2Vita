#include "player.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static World flat(void){World w;memset(&w,0,sizeof(w));w.width=40;w.height=24;w.ready=1;w.entrance_x=14;w.entrance_y=20;
 for(int x=0;x<40;x++)w.tiles[21*40+x].type=1;
 world_collision_flags(&w);return w;}
int main(int argc,char **argv){
 World w=flat();NativePlayer p;assert(player_spawn(&p,&w,0));
 /* A floor transition keeps health, attributes, inventory and current-world bindings. */
 w.melee.ready=1;w.melee.count=1;w.melee.weapons[0].type=45;w.bow.ready=1;
 NativePlayer previous,next;assert(player_spawn(&previous,&w,1));assert(player_spawn(&next,&w,1));
 previous.hp=47;previous.loadout.items[2].quantity=8;previous.stats.dexterity+=2;previous.derived.move_speed+=0.25f;previous.chests_opened=2;
 assert(player_carry_campaign_state(&next,&previous,&w));
 assert(next.hp==47&&next.max_hp==previous.max_hp&&next.stats.dexterity==previous.stats.dexterity);
 assert(fabsf(next.derived.move_speed-previous.derived.move_speed)<0.001f&&next.loadout.items[2].quantity==8&&next.chests_opened==2);
 assert(next.melee.definition==&w.melee.weapons[0]&&next.bow.definition==&w.bow&&!next.bow.active&&!next.arrows.fired);
 NativePlayer wrong_class=previous;wrong_class.player_class=0;assert(!player_carry_campaign_state(&next,&wrong_class,&w));
 w.melee.ready=0;w.melee.count=0;w.bow.ready=0;
 w.goal_x=18;w.goal_y=20;
 assert(!world_goal_touched(&w,p.x,p.y,p.w,p.h));
 assert(world_goal_touched(&w,18*16.0f+4,20*16.0f+4,8,12));
 assert(!world_goal_touched(&w,NAN,0,8,12));
  for(int i=0;i<5;i++){player_step(&p,&w,0);}assert(p.on_ground&&p.hp==100);
 float x=p.x;for(int i=0;i<60;i++){player_step(&p,&w,PLAYER_RIGHT);assert(!player_overlaps_solid(&p,&w));}assert(p.x>x+70);
 float y=p.y;player_step(&p,&w,PLAYER_JUMP);assert(p.vy<0);
 float top=p.y;for(int i=0;i<120;i++){player_step(&p,&w,PLAYER_JUMP);if(p.y<top)top=p.y;}assert(top<y-25&&p.on_ground);
  for(int i=0;i<20;i++){player_step(&p,&w,PLAYER_JUMP);}assert(p.on_ground);
 player_step(&p,&w,PLAYER_DOWN);assert(p.ducking&&p.h==6);player_step(&p,&w,0);assert(!p.ducking&&p.h==12);

 /* UP while touching a chest opens it once and persists its visual state. */
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=16,.variant=0,.x_q8=(int32_t)((p.x+2)*256),.y_q8=(int32_t)((p.y+4)*256)};w.entity_count=1;
 player_step(&p,&w,PLAYER_UP);
 assert((w.entities[0].flags&WORLD_ENTITY_OPENED)&&p.chests_opened==1);
 player_step(&p,&w,PLAYER_UP);assert(p.chests_opened==1);
 player_step(&p,&w,0);player_step(&p,&w,PLAYER_UP);assert(p.chests_opened==1);

 /* A floor-local loot table creates a visible drop and UP collects it. */
 w=flat();assert(player_spawn(&p,&w,0));w.loot.ready=1;w.loot.enabled=1;
 w.loot.direct_count=1;w.loot.row_count=1;w.loot.nested_count=1;w.loot.edge_count=1;
 w.loot.direct[0]=(LootPoolRange){15,0,1};w.loot.rows[0]=(LootRarityRow){0,10,1,49,0,1,12};
 w.loot.nested[0]=(LootPoolRange){23,0,1};w.loot.edges[0]=(LootPoolEdge){15,1000};
 w.entities[0]=(WorldEntity){.kind=16,.x_q8=(int32_t)((p.x+2)*256),.y_q8=(int32_t)((p.y+4)*256)};w.entity_count=1;
 player_step(&p,&w,PLAYER_UP);assert(p.chests_opened==1&&w.entity_count==2&&w.entities[1].kind==25&&w.entities[1].item_type==49);
 p.previous=0;p.x=(float)(w.entities[1].x_q8/256);p.y=(float)(w.entities[1].y_q8/256);player_step(&p,&w,PLAYER_UP);
 assert(p.items_collected==1&&p.loadout.item_count==2&&p.loadout.items[1].type==49&&(w.entities[1].flags&WORLD_ENTITY_PICKED_UP));

 /* Ghidra-observed spikes activate on tile type 11 and hit on descent > 0.4. */
 w=flat();w.tiles[21*40+14].type=11;w.tiles[22*40+14].type=1;world_collision_flags(&w);
 w.entities[0]=(WorldEntity){.kind=17,.x_q8=14*16*256,.y_q8=21*16*256};w.entity_count=1;
 assert(player_spawn(&p,&w,0));
 player_step(&p,&w,0);assert(!p.dead&&p.hp==p.max_hp);
 player_step(&p,&w,0);assert(!p.dead&&p.hp==p.max_hp);
  for(int i=0;i<12&&!p.dead;i++){player_step(&p,&w,0);}assert(p.dead&&p.hp==0&&p.death_cause==PLAYER_DEATH_SPIKES);
 player_step(&p,&w,0);assert(p.dead&&p.hp==0);
 /* Entity alone is inactive unless its supporting tile is the spike tile. */
 w=flat();w.tiles[22*40+14].type=1;world_collision_flags(&w);
 w.entities[0]=(WorldEntity){.kind=17,.x_q8=14*16*256,.y_q8=21*16*256};w.entity_count=1;
 assert(player_spawn(&p,&w,0));for(int i=0;i<30;i++)player_step(&p,&w,0);assert(!p.dead&&p.hp==p.max_hp);
 /* An upward collision does not meet the native falling-speed guard. */
 w=flat();w.tiles[21*40+14].type=11;w.tiles[22*40+14].type=1;world_collision_flags(&w);
 w.entities[0]=(WorldEntity){.kind=17,.x_q8=14*16*256,.y_q8=21*16*256};w.entity_count=1;
 assert(player_spawn(&p,&w,0));p.vy=-2.0f;
 player_step(&p,&w,PLAYER_JUMP);assert(!p.dead&&p.hp==p.max_hp);

 /* Bat uses Ghidra's 1..6 damage range and 30-frame per-unit hurt gate.
    Goblin contact alone does not trigger its separate weapon attack. */
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=24,.subtype=10,.x_q8=(int32_t)(p.x*256),.y_q8=(int32_t)(p.y*256),.hp=2};w.entity_count=1;
 player_step(&p,&w,0);int bat_damage=p.max_hp-p.hp;assert(bat_damage>=1&&bat_damage<=6&&w.entities[0].contact_cooldown==30);
 for(int i=0;i<29;i++)player_step(&p,&w,0);
 assert(p.hp==p.max_hp-bat_damage&&w.entities[0].contact_cooldown==1);
 int hp_before_second_bat_hit=p.hp;player_step(&p,&w,0);
 int second_bat_damage=hp_before_second_bat_hit-p.hp;
 assert(second_bat_damage>=(int)ceilf(p.derived.damage_taken_multiplier)&&
        second_bat_damage<=(int)ceilf(6.0f*p.derived.damage_taken_multiplier)&&
        w.entities[0].contact_cooldown==30);
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=24,.subtype=9,.x_q8=(int32_t)(p.x*256),.y_q8=(int32_t)(p.y*256),.hp=2};w.entity_count=1;
 player_step(&p,&w,0);assert(p.hp==p.max_hp&&w.entities[0].contact_cooldown==0);
 w=flat();assert(player_spawn(&p,&w,0));p.hp=1;
 w.entities[0]=(WorldEntity){.kind=24,.subtype=10,.x_q8=(int32_t)(p.x*256),.y_q8=(int32_t)(p.y*256),.hp=2};w.entity_count=1;
 player_step(&p,&w,0);assert(p.dead&&p.hp==0&&p.death_cause==PLAYER_DEATH_ENEMY);

 /* Square Boulder contact is gated by the original 2*g speed threshold and
    one-hit-per-unit collision tracking. */
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=20,.boulder_present=1,.boulder_released=1,
  .boulder_x_q8=(int32_t)(p.x*256),.boulder_y_q8=(int32_t)(p.y*256),.boulder_vy=.32f};w.entity_count=1;
 assert(!player_boulder_contact(&p,&w)&&p.hp==p.max_hp);
 w.entities[0].boulder_vy=.48f;int hp_before_boulder=p.hp;
 assert(player_boulder_contact(&p,&w)&&p.hp<hp_before_boulder&&w.entities[0].boulder_player_hit);
 int hp_after_boulder=p.hp;assert(!player_boulder_contact(&p,&w)&&p.hp==hp_after_boulder);
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=22,.boulder_present=1,.boulder_released=1,
  .boulder_x_q8=(int32_t)(p.x*256),.boulder_y_q8=(int32_t)(p.y*256),.boulder_vy=.48f};w.entity_count=1;
 hp_before_boulder=p.hp;assert(player_boulder_contact(&p,&w)&&p.hp<hp_before_boulder&&w.entities[0].boulder_player_hit);
 w=flat();assert(player_spawn(&p,&w,0));p.hp=20;
 w.entities[0]=(WorldEntity){.kind=20,.boulder_present=1,.boulder_released=1,.boulder_vy=1.0f,
  .boulder_x_q8=(int32_t)(p.x*256),.boulder_y_q8=(int32_t)(p.y*256)};w.entity_count=1;
 assert(player_boulder_contact(&p,&w)&&p.dead&&p.hp==0&&p.death_cause==PLAYER_DEATH_BOULDER);
 assert(!strcmp(player_death_cause_name(PLAYER_DEATH_BOULDER),"boulder"));

 /* DartTrap fires an ArrowTrap horizontally, damages once, and obeys terrain. */
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=18,.flags=WORLD_ENTITY_FACING_LEFT,
  .x_q8=270*256,.y_q8=324*256};w.entity_count=1;
 for(int i=0;i<8&&!p.dead;i++)player_step(&p,&w,0);
 assert(p.hp==p.max_hp-5&&!p.dead&&!w.entities[0].projectile_active&&w.entities[0].fired_once);
 assert(player_death_cause_name(PLAYER_DEATH_DART_TRAP)[0]=='d');
 for(int i=0;i<30;i++)player_step(&p,&w,0);
 assert(p.hp==p.max_hp-5);
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=18,.flags=WORLD_ENTITY_FACING_LEFT|WORLD_ENTITY_CAN_RELOAD,
  .x_q8=270*256,.y_q8=324*256};w.entity_count=1;
 for(int i=0;i<8;i++)player_step(&p,&w,0);
 assert(p.hp==p.max_hp-5&&!w.entities[0].projectile_active&&w.entities[0].cooldown==127);
 for(int i=0;i<126;i++)player_step(&p,&w,0);
 assert(!w.entities[0].projectile_active&&w.entities[0].cooldown==1);
 player_step(&p,&w,0);assert(w.entities[0].projectile_active&&w.entities[0].cooldown==0);
 w=flat();assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=18,.flags=WORLD_ENTITY_FACING_LEFT,
  .x_q8=270*256,.y_q8=324*256};w.entity_count=1;
 w.tiles[20*40+15].type=1;world_collision_flags(&w);
 for(int i=0;i<20;i++)player_step(&p,&w,0);
 assert(p.hp==p.max_hp&&!w.entities[0].projectile_active&&w.entities[0].fired_once);

 w=flat();assert(player_spawn(&p,&w,1));
  for(int ywall=18;ywall<21;ywall++){w.tiles[ywall*40+20].type=1;}world_collision_flags(&w);
  for(int i=0;i<200;i++){player_step(&p,&w,PLAYER_RIGHT);}assert(p.x+p.w<=320.001f&&!player_overlaps_solid(&p,&w));
 /* A subpixel overlap at a wall edge must be snapped clear so gravity can
    carry the player below the wall instead of pinning its vertical motion. */
 w=flat();for(int tx=0;tx<40;tx++)w.tiles[21*40+tx].type=0;
 for(int ywall=18;ywall<22;ywall++)w.tiles[ywall*40+20].type=1;
 for(int tx=0;tx<40;tx++)w.tiles[23*40+tx].type=1;
 world_collision_flags(&w);assert(player_spawn(&p,&w,0));
 p.x=312.0005f;p.y=330.5f;p.on_ground=0;p.vy=1.76f;
 player_step(&p,&w,PLAYER_RIGHT);assert(!player_overlaps_solid(&p,&w));
 for(int i=0;i<90;i++){player_step(&p,&w,PLAYER_RIGHT);assert(!player_overlaps_solid(&p,&w));}
 assert(p.x>320.0f&&p.on_ground);
 w=flat();for(int tx=0;tx<40;tx++){w.tiles[21*40+tx].type=6;w.tiles[23*40+tx].type=1;}world_collision_flags(&w);
 assert(player_spawn(&p,&w,2));for(int i=0;i<5;i++)player_step(&p,&w,0);assert(p.on_ground&&p.hp==60);
 for(int i=0;i<180;i++){player_step(&p,&w,0);assert(p.on_ground);assert(fabsf(p.y+p.h-336.0f)<0.001f);}
 player_step(&p,&w,PLAYER_DOWN|PLAYER_JUMP);for(int i=0;i<60;i++)player_step(&p,&w,0);assert(p.on_ground&&p.y>340);
 w=flat();w.tiles[19*40+14].type=3;w.tiles[20*40+14].type=3;world_collision_flags(&w);
 assert(player_spawn(&p,&w,0));for(int i=0;i<5;i++)player_step(&p,&w,0);y=p.y;
  for(int i=0;i<10;i++){player_step(&p,&w,PLAYER_UP);}assert(p.on_ladder&&p.y<y-10);
 player_step(&p,&w,PLAYER_JUMP);assert(!p.on_ladder&&p.vy<0);
 /* Ledge grab, blocked mantle, clear mantle, release and both orientations. */
 w=flat();w.tiles[18*40+20].type=1;world_collision_flags(&w);
 assert(player_spawn(&p,&w,0));p.x=312;p.y=288;p.on_ground=0;p.vy=0;
 player_step(&p,&w,PLAYER_RIGHT);assert(p.hanging&&p.motion==PLAYER_HANG);
  for(int i=0;i<30;i++){player_step(&p,&w,0);}assert(p.hanging&&p.vy==0);
 w.tiles[17*40+20].type=1;world_collision_flags(&w);
 player_step(&p,&w,PLAYER_UP);assert(p.hanging&&!player_overlaps_solid(&p,&w));
 w.tiles[17*40+20].type=0;world_collision_flags(&w);
 player_step(&p,&w,PLAYER_UP);assert(!p.hanging&&p.on_ground&&p.x==320&&p.y==276);
 assert(player_spawn(&p,&w,0));p.x=336;p.y=288;p.on_ground=0;
 player_step(&p,&w,PLAYER_LEFT);assert(p.hanging&&p.facing==-1);
 player_step(&p,&w,PLAYER_DOWN);assert(!p.hanging&&p.hang_cooldown);
 assert(player_spawn(&p,&w,0));p.x=312;p.y=288;p.on_ground=0;
 player_step(&p,&w,PLAYER_RIGHT);assert(p.hanging);
 player_step(&p,&w,PLAYER_JUMP);assert(!p.hanging&&p.vy<0&&p.vx<0);
 if(argc==2){assert(world_load(&w,argv[1],stdout));assert(player_spawn(&p,&w,0));
  for(int i=0;i<300;i++){unsigned input=i<130?PLAYER_RIGHT:PLAYER_LEFT;if(i==50||i==180)input|=PLAYER_JUMP;player_step(&p,&w,input);assert(isfinite(p.x)&&isfinite(p.y)&&!player_overlaps_solid(&p,&w));}
 }
 return 0;
}
