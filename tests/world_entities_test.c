#include "world.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static void write(FILE *f,const unsigned char *p,size_t n){rewind(f);assert(fwrite(p,1,n,f)==n);assert(fflush(f)==0);}
static void put16(unsigned char *p,unsigned value){p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);}
static void put_float(unsigned char *p,float value){memcpy(p,&value,sizeof(value));}
int main(void){
 World w;memset(&w,0,sizeof(w));w.ready=1;w.width=40;w.height=24;
 char path[]="entity-test-packet.bin";FILE *f=fopen(path,"wb");assert(f);
 unsigned char p[]={ 'V','W','E','1',1,0,0,0,1,3,233,0,55,1,0,0};
 write(f,p,sizeof(p));assert(world_load_entities(&w,path,NULL));assert(w.entity_count==1&&w.entities[0].x==233&&w.entities[0].y==311);
 World before=w;p[9]=99;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));assert(!memcmp(&w,&before,sizeof(w)));

 /* ACT_ONE constructors set subtype-specific Monster body dimensions. Test
    exact touching edges as well as a small overlap for contact collisions. */
 const unsigned char enemy_types[]={9,10,11,12};
 const float enemy_widths[]={7.0f,9.0f,10.0f,10.0f};
 const float enemy_heights[]={12.0f,10.0f,10.0f,7.0f};
 for(unsigned n=0;n<4;n++){
  World contact_world;memset(&contact_world,0,sizeof(contact_world));
  contact_world.entity_count=1;WorldEntity *enemy=&contact_world.entities[0];
  enemy->kind=24;enemy->subtype=enemy_types[n];enemy->hp=2;
  enemy->x_q8=100*256;enemy->y_q8=100*256;
  assert(world_enemies_contact_damage(&contact_world,100+enemy_widths[n],100,1,1)==0);
  assert(world_enemies_contact_damage(&contact_world,100,100+enemy_heights[n],1,1)==0);
  unsigned contact_damage=world_enemies_contact_damage(&contact_world,
                               100+enemy_widths[n]-.5f,100+enemy_heights[n]-.5f,1,1);
  if(enemy_types[n]==10){assert(contact_damage>=1&&contact_damage<=6);assert(enemy->contact_cooldown==30);}
  else {assert(contact_damage==0);assert(enemy->contact_cooldown==0);}
 }
 /* Goblin uses the original 16-frame tell and goblin-stab polygon, not body
    contact. The script's 0..1 shape multiplier scales its 5..9 damage roll. */
 World goblin_world;memset(&goblin_world,0,sizeof(goblin_world));
 goblin_world.entity_count=1;goblin_world.goblin_attack.ready=1;
 goblin_world.goblin_attack.frame_count=5;goblin_world.goblin_attack.damage_min=5;
 goblin_world.goblin_attack.damage_max=9;
 static const unsigned frame_ticks[5]={9,1,1,6,1};
 for(unsigned i=0;i<5;i++){
  GoblinAttackFrame *frame=&goblin_world.goblin_attack.frames[i];frame->duration=(uint16_t)frame_ticks[i];
  frame->enabled=(uint8_t)(i>=1&&i<=3);frame->vertex_count=4;frame->origin_x=0;frame->origin_y=-6;
  frame->damage_min=0;frame->damage_max=1;
  frame->vertices[0]=(GoblinAttackPoint){0,0};frame->vertices[1]=(GoblinAttackPoint){10,0};
  frame->vertices[2]=(GoblinAttackPoint){9,3};frame->vertices[3]=(GoblinAttackPoint){0,3};
 }
 WorldEntity *goblin=&goblin_world.entities[0];goblin->kind=24;goblin->subtype=9;goblin->hp=2;
 goblin->x_q8=100*256;goblin->y_q8=100*256;goblin->goblin_facing_right=1;
 goblin->goblin_swing_frame=10;goblin->goblin_attack_damage=5;
 unsigned goblin_damage=world_enemies_contact_damage(&goblin_world,105,106,6,4);
 assert(goblin_damage==3&&goblin->goblin_hit);
 assert(world_enemies_contact_damage(&goblin_world,105,106,6,4)==0);
 goblin->goblin_hit=0;goblin->goblin_swing_frame=1;
 assert(world_enemies_contact_damage(&goblin_world,105,106,6,4)==0);

 /* VGA1 loader validates the exact script timing and leaves old state intact
    if a later package is malformed. */
 unsigned char goblin_attack_packet[16+5*(24+4*8)]={0};memcpy(goblin_attack_packet,"VGA1",4);
 put16(goblin_attack_packet+4,1);put16(goblin_attack_packet+6,5);put16(goblin_attack_packet+8,16);
 put16(goblin_attack_packet+10,40);put16(goblin_attack_packet+12,5);put16(goblin_attack_packet+14,9);
 size_t attack_offset=16;
 for(unsigned i=0;i<5;i++){
  put16(goblin_attack_packet+attack_offset,frame_ticks[i]);goblin_attack_packet[attack_offset+2]=(unsigned char)(i>=1&&i<=3);
  goblin_attack_packet[attack_offset+3]=4;put_float(goblin_attack_packet+attack_offset+4,0);
  put_float(goblin_attack_packet+attack_offset+8,-6);put_float(goblin_attack_packet+attack_offset+12,0);
  put_float(goblin_attack_packet+attack_offset+16,0);put_float(goblin_attack_packet+attack_offset+20,1);
  attack_offset+=24;
  const float points[8]={0,0,10,0,9,3,0,3};
  for(unsigned j=0;j<8;j++){put_float(goblin_attack_packet+attack_offset,points[j]);attack_offset+=4;}
 }
 FILE *attack_file=fopen("goblin-attack-test.vga","wb");assert(attack_file);
 assert(fwrite(goblin_attack_packet,1,sizeof(goblin_attack_packet),attack_file)==sizeof(goblin_attack_packet));
 fclose(attack_file);World attack_world;memset(&attack_world,0,sizeof(attack_world));
 assert(world_load_goblin_attack(&attack_world,"goblin-attack-test.vga",NULL));
 assert(attack_world.goblin_attack.ready&&attack_world.goblin_attack.windup==16&&
        attack_world.goblin_attack.cooldown==40&&attack_world.goblin_attack.frames[3].duration==6);
 goblin_attack_packet[8]=15;attack_file=fopen("goblin-attack-test.vga","wb");assert(attack_file);
 assert(fwrite(goblin_attack_packet,1,sizeof(goblin_attack_packet),attack_file)==sizeof(goblin_attack_packet));fclose(attack_file);
 GoblinAttackData valid_attack=attack_world.goblin_attack;
 assert(!world_load_goblin_attack(&attack_world,"goblin-attack-test.vga",NULL));
 assert(!memcmp(&valid_attack,&attack_world.goblin_attack,sizeof(valid_attack)));
 remove("goblin-attack-test.vga");
 memset(&goblin_world,0,sizeof(goblin_world));goblin_world.width=40;goblin_world.height=24;
 goblin_world.goblin_attack=valid_attack;goblin_world.entity_count=1;
 for(int tx=0;tx<40;tx++)goblin_world.tiles[7*40+tx].solid=1;
 goblin=&goblin_world.entities[0];goblin->kind=24;goblin->subtype=9;goblin->hp=2;
 goblin->x_q8=100*256;goblin->y_q8=100*256;
 unsigned first_goblin_hit=0;unsigned first_goblin_tick=0;
 for(unsigned tick=0;tick<80;tick++){
  world_enemies_step(&goblin_world,110,100,6,12,NULL);
  unsigned hit=world_enemies_contact_damage(&goblin_world,110,100,6,12);
  if(hit&&!first_goblin_hit){first_goblin_hit=hit;first_goblin_tick=tick;}
 }
 assert(first_goblin_hit>=3&&first_goblin_hit<=5&&first_goblin_tick>=27);

 p[9]=3;p[4]=65;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));
 p[4]=2;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));
 p[4]=1;p[10]=255;p[11]=255;write(f,p,sizeof(p));assert(!world_load_entities(&w,path,NULL));
 unsigned char v2[]={'V','W','E','2',1,0,16,0,17,0,0,0,0x34,0x12,0,0,0x56,0x34,0,0,0xde,0xbc,0x9a,0x78};
 write(f,v2,sizeof(v2));assert(world_load_entities(&w,path,NULL));
 assert(w.entity_count==1&&w.entities[0].kind==17&&w.entities[0].x_q8==0x1234&&w.entities[0].y_q8==0x3456);
 FILE *entity_log=tmpfile();assert(entity_log);
 assert(world_load_entities(&w,path,entity_log));rewind(entity_log);
 char entity_line[160];assert(fgets(entity_line,sizeof(entity_line),entity_log));
 assert(strstr(entity_line,"behavior_pending=0"));fclose(entity_log);
 assert(w.entities[0].x==0x12&&w.entities[0].y==0x34&&w.entities[0].source_index==0x789abcdeu);
 before=w;v2[9]=4;write(f,v2,sizeof(v2));assert(!world_load_entities(&w,path,NULL));assert(!memcmp(&w,&before,sizeof(w)));
 unsigned char opened[]={'V','W','E','2',1,0,16,0,16,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 write(f,opened,sizeof(opened));assert(world_load_entities(&w,path,NULL));assert(w.entities[0].kind==16&&(w.entities[0].flags&WORLD_ENTITY_OPENED));
 opened[8]=17;opened[9]=1;write(f,opened,sizeof(opened));assert(!world_load_entities(&w,path,NULL));
 v2[9]=0;v2[6]=15;write(f,v2,sizeof(v2));assert(!world_load_entities(&w,path,NULL));
 unsigned char enemies[]={'V','W','E','3',1,0,16,0,24,9,0,0,0,16,0,0,0,32,0,0,0,0,0,0};
 write(f,enemies,sizeof(enemies));assert(world_load_enemies(&w,path,NULL));assert(w.entity_count==2&&w.entities[0].kind==24&&w.entities[0].subtype==9&&w.entities[0].hp==2.0f&&w.entities[1].kind==16);
 assert(world_load_enemies(&w,path,NULL));assert(w.entity_count==2);
 unsigned char object[]={'V','W','E','2',1,0,16,0,17,0,0,0,0x34,0x12,0,0,0x56,0x34,0,0,0xde,0xbc,0x9a,0x78};
 write(f,object,sizeof(object));assert(world_load_entities(&w,path,NULL));assert(w.entity_count==2&&w.entities[0].kind==17&&w.entities[1].kind==24);
 unsigned char facing_trap[]={'V','W','E','2',1,0,16,0,18,6,0,0,0,0,0,0,0,0,0,0,1,0,0,0};
 write(f,facing_trap,sizeof(facing_trap));assert(world_load_entities(&w,path,NULL));
 assert(w.entities[0].kind==18&&(w.entities[0].flags&WORLD_ENTITY_FACING_LEFT)&&
        (w.entities[0].flags&WORLD_ENTITY_CAN_RELOAD));
 before=w;facing_trap[9]=8;write(f,facing_trap,sizeof(facing_trap));
 assert(!world_load_entities(&w,path,NULL));assert(!memcmp(&w,&before,sizeof(w)));
 unsigned char facing_blocker[]={'V','W','E','2',1,0,16,0,23,2,0,0,0,0,0,0,0,0,0,0,1,0,0,0};
 write(f,facing_blocker,sizeof(facing_blocker));assert(world_load_entities(&w,path,NULL));
 assert(w.entities[0].kind==23&&w.entities[0].flags==WORLD_ENTITY_FACING_LEFT);
 before=w;facing_blocker[9]=4;write(f,facing_blocker,sizeof(facing_blocker));
 assert(!world_load_entities(&w,path,NULL));assert(!memcmp(&w,&before,sizeof(w)));
 unsigned char breakable_wall[]={'V','W','E','2',1,0,16,0,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 write(f,breakable_wall,sizeof(breakable_wall));entity_log=tmpfile();assert(entity_log);
 assert(world_load_entities(&w,path,entity_log));rewind(entity_log);
 assert(fgets(entity_line,sizeof(entity_line),entity_log));
 assert(strstr(entity_line,"behavior_pending=0"));fclose(entity_log);
 assert(w.entities[0].kind==21&&w.entities[0].hp==14.0f);
 unsigned char unsupported_behavior[]={'V','W','E','2',1,0,16,0,22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 write(f,unsupported_behavior,sizeof(unsupported_behavior));entity_log=tmpfile();assert(entity_log);
 assert(world_load_entities(&w,path,entity_log));rewind(entity_log);
 assert(fgets(entity_line,sizeof(entity_line),entity_log));
 assert(strstr(entity_line,"behavior_pending=0"));fclose(entity_log);
 unsigned char standalone_boulder[]={'V','W','E','2',1,0,16,0,22,0,0,0,0,0xa0,0,0,0,0x80,0,0,4,0,0,0};
 World standalone_world;memset(&standalone_world,0,sizeof(standalone_world));standalone_world.ready=1;standalone_world.width=40;standalone_world.height=24;
 standalone_world.tiles[10*40+10].type=1;standalone_world.tiles[10*40+10].solid=1;
 write(f,standalone_boulder,sizeof(standalone_boulder));assert(world_load_entities(&standalone_world,path,NULL));
 WorldEntity *standalone=&standalone_world.entities[0];
 assert(standalone->kind==22&&standalone->boulder_present&&!standalone->boulder_released);
 assert(standalone->boulder_x_q8==160*256&&standalone->boulder_y_q8==128*256&&standalone->cooldown==120);
 for(int tick=0;tick<120;tick++){world_boulders_step(&standalone_world);assert(!standalone->boulder_released);}
 world_boulders_step(&standalone_world);assert(standalone->boulder_released&&standalone->boulder_vy==.32f&&standalone->boulder_y_q8==128*256);
 world_boulders_step(&standalone_world);assert(standalone->boulder_y_q8>128*256&&!standalone->boulder_landed);
 for(int tick=0;tick<180;tick++)world_boulders_step(&standalone_world);
 assert(standalone->boulder_landed&&standalone->boulder_y_q8==9*16*256&&standalone->boulder_vy==0);
 FILE *boulder_log=tmpfile();assert(boulder_log&&world_load_entities(&standalone_world,path,boulder_log));rewind(boulder_log);
 assert(fgets(entity_line,sizeof(entity_line),boulder_log)&&strstr(entity_line,"behavior_pending=0"));fclose(boulder_log);
 unsigned char boulder_trap[]={'V','W','E','2',1,0,16,0,20,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0};
 write(f,boulder_trap,sizeof(boulder_trap));assert(world_load_entities(&w,path,NULL));
 assert(w.entities[0].kind==20&&w.entities[0].boulder_plate_down&&!w.entities[0].boulder_initialized);
 World boulder_world;memset(&boulder_world,0,sizeof(boulder_world));boulder_world.width=40;boulder_world.height=24;
 WorldEntity *boulder_trap_entity=&boulder_world.entities[0];boulder_world.entity_count=1;
 boulder_trap_entity->kind=20;boulder_trap_entity->x_q8=100*256;boulder_trap_entity->y_q8=100*256;
 for(int tick=0;tick<60;tick++)assert(world_boulder_traps_step(&boulder_world,0,0,6,12)==0);
 assert(boulder_trap_entity->boulder_plate_down&&boulder_trap_entity->cooldown==1);
 assert(world_boulder_traps_step(&boulder_world,0,0,6,12)==0);
 assert(!boulder_trap_entity->boulder_plate_down&&boulder_trap_entity->cooldown==0);
 assert(world_boulder_traps_step(&boulder_world,101,101,6,12)==1);
 assert(boulder_trap_entity->boulder_plate_down&&boulder_trap_entity->boulder_triggered);
 assert(world_boulder_traps_step(&boulder_world,101,101,6,12)==0);
 assert(world_boulder_traps_step(&boulder_world,0,0,6,12)==0&&boulder_trap_entity->boulder_plate_down);
 World held_plate_world;memset(&held_plate_world,0,sizeof(held_plate_world));held_plate_world.width=40;held_plate_world.height=24;
 WorldEntity *held_plate=&held_plate_world.entities[0];held_plate_world.entity_count=1;held_plate->kind=20;
 held_plate->x_q8=100*256;held_plate->y_q8=100*256;
 for(int tick=0;tick<61;tick++)assert(world_boulder_traps_step(&held_plate_world,101,101,6,12)==0);
 assert(held_plate->boulder_plate_down&&!held_plate->boulder_triggered&&held_plate->cooldown==0);
 assert(world_boulder_traps_step(&held_plate_world,0,0,6,12)==0&&!held_plate->boulder_plate_down);
 assert(world_boulder_traps_step(&held_plate_world,101,101,6,12)==1&&held_plate->boulder_triggered);
 assert(held_plate->boulder_release_frames==0&&!held_plate->boulder_release_ready);
 assert(world_boulder_traps_step(&held_plate_world,0,0,6,12)==0);
 for(int tick=0;tick<4;tick++){
  assert(world_boulder_traps_step(&held_plate_world,0,0,6,12)==0);
  assert(held_plate->boulder_release_frames==(uint8_t)(tick+2)&&!held_plate->boulder_release_ready);
 }
 assert(world_boulder_traps_step(&held_plate_world,0,0,6,12)==0);
 assert(held_plate->boulder_release_frames==6&&held_plate->boulder_release_ready);
 assert(world_boulder_traps_step(&held_plate_world,101,101,6,12)==0);
 World boulder_spawn_world;memset(&boulder_spawn_world,0,sizeof(boulder_spawn_world));
 boulder_spawn_world.ready=1;boulder_spawn_world.width=40;boulder_spawn_world.height=24;
 boulder_spawn_world.tiles[7*40+10].type=1;boulder_spawn_world.tiles[7*40+10].solid=1;
 unsigned char anchored_boulder[]={'V','W','E','2',1,0,16,0,20,0,0,0,0,0xa0,0,0,0,0xa0,0,0,3,0,0,0};
 write(f,anchored_boulder,sizeof(anchored_boulder));
 assert(world_load_entities(&boulder_spawn_world,path,NULL));
 assert(boulder_spawn_world.entity_count==1&&boulder_spawn_world.entities[0].kind==20);
 assert(boulder_spawn_world.entities[0].boulder_present);
 assert(boulder_spawn_world.entities[0].boulder_x_q8==160*256);
 assert(boulder_spawn_world.entities[0].boulder_y_q8==7*16*256);
 World falling_world;memset(&falling_world,0,sizeof(falling_world));falling_world.ready=1;falling_world.width=40;falling_world.height=24;
 falling_world.tiles[10*40+10].type=1;falling_world.tiles[10*40+10].solid=1;
 WorldEntity *falling=&falling_world.entities[0];falling_world.entity_count=1;falling->kind=20;falling->boulder_present=1;
 falling->boulder_released=1;falling->boulder_x_q8=160*256;falling->boulder_y_q8=7*16*256;falling->boulder_vy=.32f;
 world_boulders_step(&falling_world);assert(falling->boulder_y_q8>7*16*256&&!falling->boulder_landed);
 for(int tick=0;tick<180;tick++)world_boulders_step(&falling_world);
 assert(falling->boulder_landed&&falling->boulder_y_q8==9*16*256&&falling->boulder_vy==0);
 int32_t landed_y=falling->boulder_y_q8;world_boulders_step(&falling_world);assert(falling->boulder_y_q8==landed_y);
 World trap_world;memset(&trap_world,0,sizeof(trap_world));trap_world.ready=1;trap_world.width=40;trap_world.height=24;
 WorldEntity *trap=&trap_world.entities[0];trap_world.entity_count=1;trap->kind=18;trap->flags=WORLD_ENTITY_CAN_RELOAD;
 trap->x_q8=100*256;trap->y_q8=100*256;
 assert(world_dart_traps_step(&trap_world,130,106,6,6)==0&&trap->projectile_active);
 assert(trap->projectile_x_q8==116*256&&trap->projectile_y_q8==106*256);
 assert(world_dart_traps_step(&trap_world,130,106,6,6)==0&&trap->projectile_active);
 assert(trap->projectile_x_q8==121*256);
 assert(world_dart_traps_step(&trap_world,130,106,6,6)==1&&!trap->projectile_active);
 assert(trap->cooldown==130&&!trap->fired_once);
 w.entities[1].x_q8=24*256;w.entities[1].y_q8=16*256;w.entities[1].hp=1;
 WorldTile *floor=&w.tiles[2*w.width+1];floor->solid=1;floor=&w.tiles[2*w.width+2];floor->solid=1;floor=&w.tiles[2*w.width+3];floor->solid=1;
 MeleeDefinition *definition=calloc(1,sizeof(*definition));assert(definition);definition->shape_count=1;definition->frame_count=1;definition->frames[0].enabled=1;definition->shapes[0].enabled=1;definition->shapes[0].count=4;definition->shapes[0].damage_min=1;definition->shapes[0].damage_max=1;
 definition->shapes[0].vertices[0]=(MeleePoint){-8,-8};definition->shapes[0].vertices[1]=(MeleePoint){8,-8};
 definition->shapes[0].vertices[2]=(MeleePoint){8,8};definition->shapes[0].vertices[3]=(MeleePoint){-8,8};
 MeleeState attack={.definition=definition,.active=1,.serial=1};
 World wall_world;memset(&wall_world,0,sizeof(wall_world));wall_world.ready=1;wall_world.width=40;wall_world.height=24;
 wall_world.entity_count=2;
 WorldEntity *wall=&wall_world.entities[0],*joined_wall=&wall_world.entities[1];
 wall->kind=21;wall->hp=1.0f;wall->x_q8=5*16*256-26;wall->y_q8=5*16*256-26;
 joined_wall->kind=21;joined_wall->hp=14.0f;joined_wall->x_q8=6*16*256-26;joined_wall->y_q8=5*16*256-26;
 wall_world.tiles[5*40+5].type=1;wall_world.tiles[5*40+6].type=1;
 world_collision_flags(&wall_world);
 assert(world_breakable_walls_step(&wall_world,84,68,8,12,&attack)==2);
 assert(wall->hp==0.0f&&joined_wall->hp==0.0f);
 assert(wall->wall_tile_destroyed&&joined_wall->wall_tile_destroyed);
 assert(wall_world.tiles[5*40+5].type==0&&wall_world.tiles[5*40+6].type==0);
 assert(!world_solid(&wall_world,5,5)&&!world_solid(&wall_world,6,5));
 w.entities[1].hp=3;
 world_enemies_step(&w,20,16,8,12,&attack);assert(w.entities[1].hp==2&&w.entities[1].cooldown==12);
 attack.serial++;world_enemies_step(&w,20,16,8,12,&attack);assert(w.entities[1].hp==2&&w.entities[1].cooldown==11);
 for(int i=0;i<11;i++)world_enemies_step(&w,20,16,8,12,&attack);
 assert(w.entities[1].hp==1&&w.entities[1].cooldown==12);
 for(int i=0;i<12;i++)world_enemies_step(&w,20,16,8,12,&attack);
 assert(w.entities[1].hp==1&&w.entities[1].cooldown==0);
 attack.serial++;world_enemies_step(&w,20,16,8,12,&attack);assert(w.entities[1].hp==0);
 free(definition);
 fclose(f);assert(remove(path)==0);return 0;
}
