#include "world.h"
#include "player.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void u16(FILE *f,unsigned v){fputc((int)v&255,f);fputc((int)(v>>8)&255,f);}
static void u32(FILE *f,unsigned v){u16(f,v&65535);u16(f,v>>16);}
static void packet(FILE *f){
 fwrite("VLT1",1,4,f);u16(f,2);u16(f,1);u16(f,2);u16(f,1);u16(f,1);
 fputc(15,f);fputc(0,f);u16(f,0);u16(f,2);
 u32(f,0);u32(f,0);u16(f,1);u16(f,49);fputc(1,f);fputc(1,f);u16(f,12);
 u32(f,0);u32(f,10);u16(f,5);u16(f,50);fputc(0,f);fputc(0,f);u16(f,0xffff);
 fputc(23,f);fputc(0,f);u16(f,0);u16(f,1);fputc(15,f);fputc(0,f);u32(f,1000);
}
int main(void){
 FILE *f=tmpfile();assert(f);packet(f);rewind(f);
 World w;memset(&w,0,sizeof(w));assert(world_load_loot_from_file(&w,f,NULL));fclose(f);
 assert(w.loot.ready&&w.loot.row_count==2&&w.loot.nested_count==1);
 WorldEntity chest={.kind=16,.x_q8=100*256,.y_q8=100*256};uint16_t type=0;
 assert(world_spawn_chest_loot(&w,&chest,0,123,&type));assert(type==49&&w.entity_count==1);
 assert(w.entities[0].item_sprite==12&&w.entities[0].item_atlas==1);
 assert(w.loot.spawned_unique_count==1&&w.loot.spawned_unique[0]==49);
 WorldEntity boss={.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_GOBLIN_KING,
                   .x_q8=220*256,.y_q8=100*256,.hp=0,.source_index=7};
 assert(world_spawn_boss_reward_chest(&w,&boss));
 assert(boss.boss_reward_spawned&&w.entity_count==2);
 assert(w.entities[1].kind==16&&w.entities[1].boss_reward&&
        w.entities[1].source_index==boss.source_index);
 assert(!world_spawn_boss_reward_chest(&w,&boss)&&w.entity_count==2);
 WorldEntity dragon={.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
                     .x_q8=240*256,.y_q8=100*256,.hp=0,.source_index=8};
 assert(world_spawn_boss_reward_chest(&w,&dragon));
 assert(w.entities[2].kind==16&&w.entities[2].boss_reward&&dragon.boss_reward_spawned);
 assert(world_spawn_chest_loot(&w,&w.entities[1],0,123,&type)&&type==50);
 assert(w.entity_count==4);
 assert(!world_spawn_chest_loot(&w,&chest,11,123,&type));
 /* Interaction opens a chest and creates a drop; a later UP collects it. */
 memset(&w,0,sizeof(w));w.width=40;w.height=24;w.ready=1;w.entrance_x=14;w.entrance_y=20;
 for(int x=0;x<40;x++)w.tiles[21*40+x].type=1;
 world_collision_flags(&w);w.loot.ready=1;w.loot.enabled=1;w.loot.direct_count=1;w.loot.row_count=1;
 w.loot.nested_count=1;w.loot.edge_count=1;w.loot.direct[0]=(LootPoolRange){15,0,1};
 w.loot.rows[0]=(LootRarityRow){0,10,1,49,0,1,12};w.loot.nested[0]=(LootPoolRange){23,0,1};
 w.loot.edges[0]=(LootPoolEdge){15,1000};NativePlayer p;assert(player_spawn(&p,&w,0));
 w.entities[0]=(WorldEntity){.kind=16,.x_q8=(int32_t)((p.x+2)*256),.y_q8=(int32_t)((p.y+4)*256)};w.entity_count=1;
 assert(player_interact_chests(&p,&w)==1&&p.chests_opened==1&&w.entity_count==2);
 p.x=(float)(w.entities[1].x_q8/256);p.y=(float)(w.entities[1].y_q8/256);
 assert(player_interact_chests(&p,&w)==0&&p.items_collected==1&&p.loadout.item_count==2);
 assert((w.entities[1].flags&WORLD_ENTITY_PICKED_UP)!=0);
 return 0;
}
