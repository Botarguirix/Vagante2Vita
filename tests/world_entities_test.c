#include "world.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static void write(FILE *f,const unsigned char *p,size_t n){rewind(f);assert(fwrite(p,1,n,f)==n);assert(fflush(f)==0);}
static void put16(unsigned char *p,unsigned value){p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);}
static void put_float(unsigned char *p,float value){memcpy(p,&value,sizeof(value));}
int main(void){
 float thief_width,thief_height;world_enemy_body(84,&thief_width,&thief_height);
 assert(thief_width==7.0f&&thief_height==12.0f);
 /* Ghidra confirms Thief's 1..2 damage and 60-frame attack cooldown. The
    runtime's brief weapon window is an explicit approximation. */
 World thief_ai;memset(&thief_ai,0,sizeof(thief_ai));thief_ai.entity_count=1;
 WorldEntity *thief=&thief_ai.entities[0];thief->kind=24;thief->subtype=84;thief->hp=4;
 thief->x_q8=100*256;thief->y_q8=100*256;
 world_enemies_step(&thief_ai,110,100,6,12,NULL);
 assert(thief->thief_swing_frames==8&&thief->thief_attack_cooldown==60);
 unsigned thief_damage=world_enemies_contact_damage(&thief_ai,110,100,6,12);
 assert(thief_damage>=1&&thief_damage<=2&&thief->thief_hit);
 assert(world_enemies_contact_damage(&thief_ai,110,100,6,12)==0);
 for(unsigned tick=0;tick<59;tick++)world_enemies_step(&thief_ai,110,100,6,12,NULL);
 assert(thief->thief_attack_cooldown==1);
 world_enemies_step(&thief_ai,200,100,6,12,NULL);assert(thief->thief_attack_cooldown==0);
 world_enemies_step(&thief_ai,110,100,6,12,NULL);assert(thief->thief_swing_frames==8);
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
 /* Bat::step's recovered charge cycle: 30-tick tell, 20-tick lunge and
    60-tick cooldown, with a direct-flight approximation for PathManager. */
 World bat_ai;memset(&bat_ai,0,sizeof(bat_ai));bat_ai.width=40;bat_ai.height=24;bat_ai.entity_count=1;
 WorldEntity *bat=&bat_ai.entities[0];bat->kind=24;bat->subtype=10;bat->hp=10;
 bat->x_q8=100*256;bat->y_q8=100*256;
 world_enemies_step(&bat_ai,125,100,6,12,NULL);assert(bat->bat_charge_prep==30);
 for(unsigned tick=0;tick<30;tick++)world_enemies_step(&bat_ai,125,100,6,12,NULL);
 assert(bat->bat_charge_prep==0&&bat->bat_charge_timer==20);
 assert(bat->vx>3.9f&&bat->vx<=4.0f);
 for(unsigned tick=0;tick<20;tick++)world_enemies_step(&bat_ai,125,100,6,12,NULL);
 assert(bat->bat_charge_timer==0&&bat->bat_charge_cooldown==60);
 assert(bat->vx>1.9f&&bat->vx<2.1f);
 for(unsigned tick=0;tick<60;tick++)world_enemies_step(&bat_ai,300,100,6,12,NULL);
 assert(bat->bat_charge_cooldown==0);
 /* Normal Slime: Ghidra recovers its 60-tick lunge tell, speed 7, 25-tick
    active interval, 3..5 damage and 150-tick cooldown. */
 World slime_ai;memset(&slime_ai,0,sizeof(slime_ai));slime_ai.width=40;slime_ai.height=24;slime_ai.entity_count=1;
 for(int tx=0;tx<40;tx++)slime_ai.tiles[7*40+tx].solid=1;
 WorldEntity *slime=&slime_ai.entities[0];slime->kind=24;slime->subtype=12;slime->hp=10;
 slime->x_q8=100*256;slime->y_q8=105*256;slime->slime_jump_timer=120;slime->slime_lunge_timer=120;slime->slime_jump_initialized=1;
 for(unsigned tick=0;tick<500&&!slime->slime_lunge_prep;tick++)
  world_enemies_step(&slime_ai,120,102.5f,6,12,NULL);
 assert(slime->slime_lunge_prep==60);
 for(unsigned tick=0;tick<60;tick++)world_enemies_step(&slime_ai,120,102.5f,6,12,NULL);
 assert(slime->slime_lunge_frames==25&&slime->vx==7.0f);
 slime->x_q8=115*256;slime->y_q8=105*256;
 unsigned slime_damage=world_enemies_contact_damage(&slime_ai,120,102.5f,6,12);
 assert(slime_damage>=3&&slime_damage<=5&&slime->slime_lunge_hit);
 assert(world_enemies_contact_damage(&slime_ai,120,102.5f,6,12)==0);
 for(unsigned tick=0;tick<25;tick++)world_enemies_step(&slime_ai,120,102.5f,6,12,NULL);
 assert(!slime->slime_lunge_frames&&slime->slime_lunge_cooldown==150);
 /* Invalid velocities must not become enormous/undefined movement loop
    counts in the shared collision sweep. */
 World invalid_motion;memset(&invalid_motion,0,sizeof(invalid_motion));
 invalid_motion.width=40;invalid_motion.height=24;invalid_motion.entity_count=1;
 WorldEntity *invalid_slime=&invalid_motion.entities[0];invalid_slime->kind=24;
 invalid_slime->subtype=12;invalid_slime->hp=10;invalid_slime->x_q8=100*256;
 invalid_slime->y_q8=105*256;invalid_slime->vx=1.0e30f;invalid_slime->vy=NAN;
 world_enemies_step(&invalid_motion,300,100,6,12,NULL);
 assert(invalid_slime->vx==0.0f&&isfinite(invalid_slime->vy));
 assert(invalid_slime->x_q8==100*256);
 /* Periodic hop uses recovered timer/velocity ranges and lands on the floor. */
 World slime_jump;memset(&slime_jump,0,sizeof(slime_jump));slime_jump.ready=1;
 slime_jump.width=40;slime_jump.height=24;slime_jump.entity_count=1;
 for(int tx=0;tx<40;tx++)slime_jump.tiles[7*40+tx].solid=1;
 WorldEntity *jumper=&slime_jump.entities[0];jumper->kind=24;jumper->subtype=12;
 jumper->hp=10;jumper->x_q8=100*256;jumper->y_q8=105*256;jumper->slime_jump_timer=1;
 jumper->slime_jump_initialized=1;
 world_enemies_step(&slime_jump,300,100,6,12,NULL);
 assert(jumper->vy<0.0f&&jumper->vy>=-3.6f&&jumper->vy<=-1.0f);
 assert(jumper->slime_jump_timer>=79&&jumper->slime_jump_timer<=119);
 int slime_bounced=0;
 for(unsigned tick=0;tick<100&&!slime_bounced;tick++){
  float previous_vy=jumper->vy;world_enemies_step(&slime_jump,300,100,6,12,NULL);
  slime_bounced=previous_vy>1.0f&&jumper->vy<0.0f;
 }
 assert(slime_bounced&&jumper->slime_bounces>0);
 /* Spider runs the recovered 20-frame tell, launches with the native
    angle/speed ranges, and stays attack-active until its tile landing. */
 World spider_ai;memset(&spider_ai,0,sizeof(spider_ai));
 spider_ai.ready=1;spider_ai.width=40;spider_ai.height=24;spider_ai.entity_count=1;
 for(int tx=0;tx<40;tx++)spider_ai.tiles[7*40+tx].solid=1;
 WorldEntity *active_spider=&spider_ai.entities[0];active_spider->kind=24;
 active_spider->subtype=14;active_spider->hp=10;active_spider->x_q8=100*256;
 active_spider->y_q8=104*256;
 world_enemies_step(&spider_ai,130,100,6,12,NULL);
 assert(active_spider->spider_jump_tell==20&&!active_spider->spider_jumping);
 for(unsigned tick=0;tick<20;tick++)world_enemies_step(&spider_ai,130,100,6,12,NULL);
 assert(active_spider->spider_jump_tell==0&&active_spider->spider_jumping);
 assert(active_spider->vx>.2f&&active_spider->vx<2.0f&&active_spider->vy<0.0f);
 active_spider->x_q8=130*256;active_spider->y_q8=100*256;
 unsigned active_spider_damage=world_enemies_contact_damage(&spider_ai,130,100,6,12);
 assert(active_spider_damage>=5&&active_spider_damage<=10);
 for(unsigned tick=0;tick<100&&active_spider->spider_jumping;tick++)
  world_enemies_step(&spider_ai,200,200,6,12,NULL);
 assert(!active_spider->spider_jumping&&active_spider->spider_recovery==30);
 assert(active_spider->y_q8==104*256);
 /* Spider::doCollision gates jump contact by speed and per-target cooldown. */
 World spider_contact;memset(&spider_contact,0,sizeof(spider_contact));
 spider_contact.entity_count=1;WorldEntity *spider_enemy=&spider_contact.entities[0];
 spider_enemy->kind=24;spider_enemy->subtype=14;spider_enemy->hp=10;
 spider_enemy->x_q8=100*256;spider_enemy->y_q8=100*256;spider_enemy->vx=.2f;
 assert(world_enemies_contact_damage(&spider_contact,100,100,1,1)==0);
 spider_enemy->spider_jumping=1;spider_enemy->vx=.199f;
 assert(world_enemies_contact_damage(&spider_contact,100,100,1,1)==0);
 spider_enemy->vx=-.2f;
 unsigned spider_damage=world_enemies_contact_damage(&spider_contact,100,100,1,1);
 assert(spider_damage>=5&&spider_damage<=10&&spider_enemy->contact_cooldown==30);
 assert(world_enemies_contact_damage(&spider_contact,100,100,1,1)==0);
 for(unsigned tick=0;tick<30;tick++)world_enemies_step(&spider_contact,200,200,1,1,NULL);
 spider_enemy->x_q8=100*256;spider_enemy->y_q8=100*256;spider_enemy->vx=.2f;spider_enemy->spider_jumping=1;
 assert(world_enemies_contact_damage(&spider_contact,100,100,1,1)>=5);
 /* Web-ball reaches a player, applies a timed snare, and is blocked by terrain. */
 World web_world;memset(&web_world,0,sizeof(web_world));web_world.width=40;web_world.height=24;web_world.entity_count=1;
 WorldEntity *web_spider=&web_world.entities[0];web_spider->kind=24;web_spider->subtype=14;
 web_spider->hp=10;web_spider->x_q8=100*256;web_spider->y_q8=100*256;
 web_spider->projectile_active=1;web_spider->spider_web_frames=90;
 web_spider->spider_web_x_q8=104*256;web_spider->spider_web_y_q8=100*256;
 web_spider->spider_web_vx=4;web_spider->spider_web_vy=0;
 assert(world_spider_web_step(&web_world,111,100,8,12)==7);
 assert(!web_spider->projectile_active&&web_spider->spider_web_hit&&web_spider->spider_webbed==90);
 web_spider->spider_webbed=0;web_spider->spider_web_cooldown=60;
 web_spider->projectile_active=1;web_spider->spider_web_hit=0;web_spider->spider_web_frames=90;
 web_spider->spider_web_x_q8=104*256;web_spider->spider_web_y_q8=100*256;
 web_spider->spider_web_vx=4;web_spider->spider_web_vy=0;
 web_world.tiles[6*40+6].solid=1;
 assert(world_spider_web_step(&web_world,300,100,8,12)==0);
 assert(!web_spider->projectile_active&&!web_spider->spider_web_hit);
 /* Lurker uses the observed 16-frame tell and 4..16 contact damage, then
    waits through its local recovery approximation before another bite. */
 World lurker_world;memset(&lurker_world,0,sizeof(lurker_world));
 lurker_world.width=40;lurker_world.height=24;lurker_world.entity_count=1;
 for(int tx=0;tx<40;tx++)lurker_world.tiles[7*40+tx].solid=1;
 WorldEntity *lurker=&lurker_world.entities[0];lurker->kind=24;lurker->subtype=11;
 lurker->hp=10;lurker->x_q8=100*256;lurker->y_q8=104*256;
 world_enemies_step(&lurker_world,101,104,6,12,NULL);assert(lurker->lurker_windup==16);
 for(unsigned tick=0;tick<16;tick++)world_enemies_step(&lurker_world,101,104,6,12,NULL);
 unsigned lurker_damage=world_enemies_contact_damage(&lurker_world,101,104,6,12);
 assert(lurker_damage>=4&&lurker_damage<=16&&lurker->lurker_hit&&lurker->lurker_recovery>0);
 assert(world_enemies_contact_damage(&lurker_world,101,104,6,12)==0);
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
 unsigned char spider[]={'V','W','E','3',1,0,16,0,24,14,0,0,0,16,0,0,0,32,0,0,0,1,0,0};
 assert(world_load_enemies(&w,path,NULL));
 write(f,spider,sizeof(spider));assert(world_load_enemies(&w,path,NULL));
 assert(w.entities[0].kind==24&&w.entities[0].subtype==14);
 spider[9]=84;write(f,spider,sizeof(spider));assert(world_load_enemies(&w,path,NULL));
 assert(w.entities[0].kind==24&&w.entities[0].subtype==84);
 before=w;spider[9]=13;write(f,spider,sizeof(spider));
 assert(!world_load_enemies(&w,path,NULL));assert(!memcmp(&w,&before,sizeof(w)));
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
 World boss_world;memset(&boss_world,0,sizeof(boss_world));boss_world.ready=1;boss_world.width=40;boss_world.height=24;
 unsigned char boss[]={'V','W','E','3',1,0,16,0,WORLD_KIND_BOSS,WORLD_BOSS_GOBLIN_KING,0,0,0,16,0,0,0,32,0,0,0,7,0,0};
 write(f,boss,sizeof(boss));assert(world_load_enemies(&boss_world,path,NULL));
 assert(boss_world.entity_count==1&&boss_world.entities[0].kind==WORLD_KIND_BOSS&&
        boss_world.entities[0].subtype==WORLD_BOSS_GOBLIN_KING&&boss_world.entities[0].hp==WORLD_GOBLIN_KING_HP);
 float boss_w,boss_h;world_enemy_body(WORLD_BOSS_GOBLIN_KING,&boss_w,&boss_h);
 assert(boss_w==14.0f&&boss_h==12.0f);
 unsigned char dragon_packet[]={'V','W','E','3',1,0,16,0,WORLD_KIND_BOSS,WORLD_BOSS_DRAGON,0,0,0,16,0,0,0,32,0,0,0,8,0,0};
 World dragon_world;memset(&dragon_world,0,sizeof(dragon_world));dragon_world.ready=1;dragon_world.width=40;dragon_world.height=24;
 write(f,dragon_packet,sizeof(dragon_packet));assert(world_load_enemies(&dragon_world,path,NULL));
 assert(dragon_world.entity_count==1&&dragon_world.entities[0].hp==WORLD_DRAGON_HP);
 dragon_world.bosses_defeated_mask=3;
 write(f,dragon_packet,sizeof(dragon_packet));assert(world_load_enemies(&dragon_world,path,NULL));
 assert(dragon_world.bosses_defeated_mask==0&&dragon_world.bosses_loaded==1&&dragon_world.bosses_defeated==0);
 unsigned char worm_packet[]={'V','W','E','3',1,0,16,0,WORLD_KIND_BOSS,WORLD_BOSS_WORM,0,0,0,16,0,0,0,32,0,0,0,9,0,0};
 World worm_world;memset(&worm_world,0,sizeof(worm_world));worm_world.ready=1;worm_world.width=128;worm_world.height=24;worm_world.bosses_loaded=1;
 write(f,worm_packet,sizeof(worm_packet));assert(world_load_enemies(&worm_world,path,NULL));
 assert(worm_world.entity_count==1&&worm_world.entities[0].subtype==WORLD_BOSS_WORM&&worm_world.entities[0].hp==WORLD_WORM_HP);
 world_enemy_body(WORLD_BOSS_WORM,&boss_w,&boss_h);assert(boss_w==8.0f&&boss_h==8.0f);
 for(int tx=0;tx<128;tx++){worm_world.tiles[3*128+tx].platform=1;worm_world.tiles[4*128+tx].solid=1;}
 world_enemies_step(&worm_world,1800,32,8,12,NULL);
 assert(worm_world.entities[0].worm_initialized&&worm_world.entities[0].worm_segment_count==WORLD_WORM_SEGMENTS);
 int32_t worm_old_x=worm_world.entities[0].x_q8;
 for(unsigned tick=0;tick<420;tick++)world_enemies_step(&worm_world,1800,32,8,12,NULL);
 assert(worm_world.entities[0].x_q8!=worm_old_x);
 assert(worm_world.entities[0].y_q8==40*256);
 float worm_initial_x,worm_initial_y;world_worm_path_sample(&worm_world,&worm_world.entities[0],0,&worm_initial_x,&worm_initial_y);
 float worm_tail_x,worm_tail_y,worm_head_x,worm_head_y;
 world_worm_path_sample(&worm_world,&worm_world.entities[0],WORLD_WORM_SEGMENTS-1u,&worm_tail_x,&worm_tail_y);
 world_worm_path_sample(&worm_world,&worm_world.entities[0],0,&worm_head_x,&worm_head_y);
 assert(fabsf((worm_head_x-worm_tail_x)-(WORLD_WORM_SEGMENTS-1u)*24.0f)<2.0f);
 world_worm_path_sample(&worm_world,&worm_world.entities[0],8,&worm_tail_x,&worm_tail_y);
 unsigned worm_contact_damage=world_enemies_contact_damage(&worm_world,worm_tail_x,worm_tail_y,8,12);
 assert(worm_contact_damage>=WORLD_WORM_DAMAGE-1u&&worm_contact_damage<=WORLD_WORM_DAMAGE+1u);
 assert(worm_world.entities[0].contact_cooldown==30);
 assert(world_enemies_contact_damage(&worm_world,worm_tail_x,worm_tail_y,8,12)==0);
 /* BossWorm::runAI creates type-10 shots at 2.5 px/tick and the projectile
    setup classifies them as poison. The current damage/aim spread are still
    explicitly provisional while the Ghidra constructor arguments are traced. */
 World worm_spit_world;memset(&worm_spit_world,0,sizeof(worm_spit_world));
 worm_spit_world.width=128;worm_spit_world.height=24;worm_spit_world.entity_count=1;
 worm_spit_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_WORM,
     .x_q8=100*256,.y_q8=72*256,.hp=WORLD_WORM_HP,.worm_initialized=1,
     .worm_segment_count=WORLD_WORM_SEGMENTS};
 for(int tx=0;tx<128;tx++){worm_spit_world.tiles[5*128+tx].platform=1;worm_spit_world.tiles[6*128+tx].solid=1;}
 for(unsigned tick=0;tick<24;tick++)world_enemies_step(&worm_spit_world,190,64,8,12,NULL);
 assert(worm_spit_world.entity_count==2&&worm_spit_world.entities[1].kind==WORLD_KIND_WORM_SPIT);
 WorldEntity *worm_shot=&worm_spit_world.entities[1];
 assert(worm_shot->variant==10&&worm_shot->subtype==4&&worm_shot->cooldown==176);
 assert(fabsf(sqrtf(worm_shot->vx*worm_shot->vx+worm_shot->vy*worm_shot->vy)-
              WORLD_WORM_SPIT_SPEED)<.01f);
 int32_t worm_shot_x=worm_shot->x_q8;world_enemies_step(&worm_spit_world,190,64,8,12,NULL);
 assert(worm_shot->x_q8!=worm_shot_x);
 World worm_spit_hit;memset(&worm_spit_hit,0,sizeof(worm_spit_hit));worm_spit_hit.entity_count=1;
 worm_spit_hit.entities[0]=(WorldEntity){.kind=WORLD_KIND_WORM_SPIT,.subtype=4,.variant=10,
     .x_q8=50*256,.y_q8=50*256,.hp=1.0f};
 assert(world_enemies_contact_damage(&worm_spit_hit,48,48,8,12)==WORLD_WORM_SPIT_DAMAGE);
 assert(worm_spit_hit.entities[0].hp==0.0f&&!worm_spit_hit.entities[0].projectile_active);
 dragon_world.entity_count=1;dragon_world.entities[0].x_q8=100*256;dragon_world.entities[0].y_q8=100*256;
 world_enemies_step(&dragon_world,180,100,8,12,NULL);
 assert(dragon_world.entities[0].dragon_initialized&&dragon_world.entities[0].dragon_action==WORLD_DRAGON_ACTION_IDLE);
 for(unsigned tick=0;tick<31;tick++)world_enemies_step(&dragon_world,180,100,8,12,NULL);
 assert(dragon_world.entities[0].dragon_action==WORLD_DRAGON_ACTION_MOVE_PLAYER);
 world_enemies_step(&dragon_world,180,100,8,12,NULL);
 assert(dragon_world.entities[0].x_q8>100*256&&dragon_world.entities[0].vx>0.0f&&dragon_world.entities[0].vx<=1.5f);
 world_enemy_body(WORLD_BOSS_DRAGON,&boss_w,&boss_h);assert(boss_w==13.0f&&boss_h==13.0f);
 WorldEntity *king=&boss_world.entities[0];
 world_enemies_step(&boss_world,32,32,8,12,NULL);
 assert(king->boss_charge_active&&king->boss_facing_right&&king->boss_charge_timer==35);
 for(unsigned tick=0;tick<34;tick++)world_enemies_step(&boss_world,32,32,8,12,NULL);
 assert(king->boss_charge_timer==1&&king->vx==0.0f);
 world_enemies_step(&boss_world,32,32,8,12,NULL);
 assert(king->boss_charge_timer==0&&fabsf(king->vx-9.3f)<.001f);
 int32_t charge_start_x=king->x_q8;
 world_enemies_step(&boss_world,32,32,8,12,NULL);
 assert(king->x_q8>charge_start_x&&king->vx<9.3f&&king->boss_charge_active);
 for(unsigned tick=0;tick<80&&king->boss_charge_active;tick++)world_enemies_step(&boss_world,32,32,8,12,NULL);
 assert(!king->boss_charge_active&&king->boss_charge_cooldown>0&&king->vx==0.0f);
 /* initializeStats writes base HP 70 and damage 8 when Monster's default
    scale factors are 1 and newGamePlus is false. One melee serial hits once. */
 World boss_melee_world;memset(&boss_melee_world,0,sizeof(boss_melee_world));
 boss_melee_world.width=40;boss_melee_world.height=24;boss_melee_world.entity_count=1;
 boss_melee_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_GOBLIN_KING,
     .x_q8=20*256,.y_q8=16*256,.hp=WORLD_GOBLIN_KING_HP};
 MeleeDefinition boss_melee_def;memset(&boss_melee_def,0,sizeof(boss_melee_def));
 boss_melee_def.frame_count=1;boss_melee_def.shape_count=1;boss_melee_def.frames[0].enabled=1;
 boss_melee_def.shapes[0].enabled=1;boss_melee_def.shapes[0].count=4;
 boss_melee_def.shapes[0].damage_min=10;boss_melee_def.shapes[0].damage_max=11;
 boss_melee_def.shapes[0].vertices[0]=(MeleePoint){0,-20};
 boss_melee_def.shapes[0].vertices[1]=(MeleePoint){20,-20};
 boss_melee_def.shapes[0].vertices[2]=(MeleePoint){20,0};
 boss_melee_def.shapes[0].vertices[3]=(MeleePoint){0,0};
 MeleeState boss_melee={.definition=&boss_melee_def,.active=1,.serial=1,.facing=1};
 world_enemies_step(&boss_melee_world,20,16,8,12,&boss_melee);
 assert(fabsf(boss_melee_world.entities[0].hp-(WORLD_GOBLIN_KING_HP-10.5f))<.001f);
 world_enemies_step(&boss_melee_world,20,16,8,12,&boss_melee);
 assert(fabsf(boss_melee_world.entities[0].hp-(WORLD_GOBLIN_KING_HP-10.5f))<.001f);
 boss_melee.serial++;
 world_enemies_step(&boss_melee_world,20,16,8,12,&boss_melee);
 assert(fabsf(boss_melee_world.entities[0].hp-(WORLD_GOBLIN_KING_HP-21.0f))<.001f);
 /* BossDragon's constructor body is 13x13. This attack touches only the
    bottom strip that the previous hardcoded 14x12 Goblin King box missed. */
 World dragon_box_world;memset(&dragon_box_world,0,sizeof(dragon_box_world));
 dragon_box_world.width=40;dragon_box_world.height=24;dragon_box_world.entity_count=1;
 dragon_box_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
     .x_q8=24*256,.y_q8=(int32_t)(28.5f*256),.hp=WORLD_DRAGON_HP};
 MeleeDefinition dragon_box_def;memset(&dragon_box_def,0,sizeof(dragon_box_def));
 dragon_box_def.frame_count=1;dragon_box_def.shape_count=1;dragon_box_def.frames[0].enabled=1;
 dragon_box_def.shapes[0].enabled=1;dragon_box_def.shapes[0].count=4;
 dragon_box_def.shapes[0].damage_min=2.5f;dragon_box_def.shapes[0].damage_max=2.5f;
 dragon_box_def.shapes[0].vertices[0]=(MeleePoint){0,-.8f};
 dragon_box_def.shapes[0].vertices[1]=(MeleePoint){1,-.8f};
 dragon_box_def.shapes[0].vertices[2]=(MeleePoint){1,-.6f};
 dragon_box_def.shapes[0].vertices[3]=(MeleePoint){0,-.6f};
 MeleeState dragon_box_attack={.definition=&dragon_box_def,.active=1,.serial=1,.facing=1};
 world_enemies_step(&dragon_box_world,20,30,8,12,&dragon_box_attack);
 assert(fabsf(dragon_box_world.entities[0].hp-(WORLD_DRAGON_HP-2.5f))<.001f);
 World boss_death_world;memset(&boss_death_world,0,sizeof(boss_death_world));
 boss_death_world.width=40;boss_death_world.height=24;boss_death_world.entity_count=1;
 boss_death_world.bosses_loaded=1;
 boss_death_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_GOBLIN_KING,
     .x_q8=20*256,.y_q8=16*256,.hp=1.0f};
 MeleeDefinition lethal_def;memset(&lethal_def,0,sizeof(lethal_def));
 lethal_def.frame_count=1;lethal_def.shape_count=1;lethal_def.frames[0].enabled=1;
 lethal_def.shapes[0].enabled=1;lethal_def.shapes[0].count=4;
 lethal_def.shapes[0].damage_min=2;lethal_def.shapes[0].damage_max=2;
 lethal_def.shapes[0].vertices[0]=(MeleePoint){0,-20};
 lethal_def.shapes[0].vertices[1]=(MeleePoint){20,-20};
 lethal_def.shapes[0].vertices[2]=(MeleePoint){20,0};
 lethal_def.shapes[0].vertices[3]=(MeleePoint){0,0};
 MeleeState lethal_attack={.definition=&lethal_def,.active=1,.serial=1,.facing=1};
 World worm_segment_hit;memset(&worm_segment_hit,0,sizeof(worm_segment_hit));worm_segment_hit.width=40;worm_segment_hit.height=24;worm_segment_hit.entity_count=1;
 worm_segment_hit.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_WORM,.x_q8=50*256,.y_q8=16*256,.hp=10.0f,.worm_initialized=1,.worm_segment_count=WORLD_WORM_SEGMENTS};
 world_enemies_step(&worm_segment_hit,0,16,8,12,NULL);
 MeleeDefinition worm_melee_def;memset(&worm_melee_def,0,sizeof(worm_melee_def));worm_melee_def.frame_count=1;worm_melee_def.shape_count=1;worm_melee_def.frames[0].enabled=1;
 worm_melee_def.shapes[0].enabled=1;worm_melee_def.shapes[0].count=4;worm_melee_def.shapes[0].damage_min=2;worm_melee_def.shapes[0].damage_max=2;
 worm_melee_def.shapes[0].vertices[0]=(MeleePoint){0,-20};worm_melee_def.shapes[0].vertices[1]=(MeleePoint){20,-20};
 worm_melee_def.shapes[0].vertices[2]=(MeleePoint){20,0};worm_melee_def.shapes[0].vertices[3]=(MeleePoint){0,0};
 MeleeState worm_segment_attack={.definition=&worm_melee_def,.active=1,.serial=1,.facing=1};
 world_enemies_step(&worm_segment_hit,26,16,8,12,&worm_segment_attack);assert(worm_segment_hit.entities[0].hp==8.0f);
 World worm_death_world;memset(&worm_death_world,0,sizeof(worm_death_world));worm_death_world.width=40;worm_death_world.height=24;worm_death_world.entity_count=1;worm_death_world.bosses_loaded=1;
 worm_death_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_WORM,.x_q8=20*256,.y_q8=16*256,.hp=1.0f};
 world_enemies_step(&worm_death_world,20,16,8,12,&lethal_attack);
 assert(worm_death_world.entities[0].hp==0&&worm_death_world.bosses_defeated==1&&worm_death_world.bosses_defeated_mask==4u);
 assert(worm_death_world.entities[0].boss_reward_spawned&&worm_death_world.entities[1].kind==16);
 world_enemies_step(&boss_death_world,20,16,8,12,&lethal_attack);
 assert(boss_death_world.entities[0].hp==0&&boss_death_world.entities[0].boss_reward_spawned);
 assert(boss_death_world.entity_count==2&&boss_death_world.bosses_defeated==1);
 assert(boss_death_world.entities[1].kind==16&&boss_death_world.entities[1].boss_reward);
 world_enemies_step(&boss_death_world,20,16,8,12,&lethal_attack);
 assert(boss_death_world.entity_count==2&&boss_death_world.bosses_defeated==1);
 World dragon_death_world;memset(&dragon_death_world,0,sizeof(dragon_death_world));
 dragon_death_world.width=40;dragon_death_world.height=24;dragon_death_world.entity_count=1;
 dragon_death_world.bosses_loaded=1;
 dragon_death_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
     .x_q8=20*256,.y_q8=16*256,.hp=1.0f};
 world_enemies_step(&dragon_death_world,20,16,8,12,&lethal_attack);
 assert(dragon_death_world.entities[0].hp==0&&dragon_death_world.entities[0].boss_reward_spawned);
 assert(dragon_death_world.entity_count==2&&dragon_death_world.bosses_defeated==1&&dragon_death_world.bosses_defeated_mask==(1u<<(WORLD_BOSS_DRAGON-WORLD_BOSS_GOBLIN_KING)));
 assert(dragon_death_world.entities[1].kind==16&&dragon_death_world.entities[1].boss_reward);
 World dragon_move_world;memset(&dragon_move_world,0,sizeof(dragon_move_world));
 dragon_move_world.width=40;dragon_move_world.height=24;dragon_move_world.entity_count=1;
 dragon_move_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
     .x_q8=100*256,.y_q8=100*256,.hp=WORLD_DRAGON_HP};
 world_enemies_step(&dragon_move_world,180,100,8,12,NULL);
 for(unsigned tick=0;tick<31;tick++)world_enemies_step(&dragon_move_world,180,100,8,12,NULL);
 assert(dragon_move_world.entities[0].dragon_action==WORLD_DRAGON_ACTION_MOVE_PLAYER);
 world_enemies_step(&dragon_move_world,180,100,8,12,NULL);
 assert(dragon_move_world.entities[0].x_q8>100*256);
 assert(dragon_move_world.entities[0].vx>0.0f&&dragon_move_world.entities[0].vx<=1.5f);
 assert(world_enemies_contact_damage(&dragon_move_world,
     dragon_move_world.entities[0].x_q8/256.0f,dragon_move_world.entities[0].y_q8/256.0f,8,12)==WORLD_DRAGON_DAMAGE);
 World sight_world;memset(&sight_world,0,sizeof(sight_world));
 sight_world.width=40;sight_world.height=24;
 assert(world_line_of_sight(&sight_world,56,56,154,56));
 sight_world.tiles[3*40+6].solid=1;
 assert(!world_line_of_sight(&sight_world,56,56,154,56));
 sight_world.tiles[3*40+6].solid=0;
 assert(world_line_of_sight(&sight_world,56,56,154,56));
 World dragon_fire_world;memset(&dragon_fire_world,0,sizeof(dragon_fire_world));
 dragon_fire_world.width=40;dragon_fire_world.height=24;dragon_fire_world.entity_count=1;
 dragon_fire_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
     .x_q8=50*256,.y_q8=50*256,.hp=WORLD_DRAGON_HP};
 dragon_fire_world.tiles[3*40+6].solid=1;
 dragon_fire_world.entities[0].dragon_action=WORLD_DRAGON_ACTION_SHOOT_PLAYER_ONCE;
 dragon_fire_world.entities[0].dragon_initialized=1;
 dragon_fire_world.entities[0].dragon_action_timer=20;
 for(unsigned tick=0;tick<21;tick++)world_enemies_step(&dragon_fire_world,150,50,8,12,NULL);
 assert(!dragon_fire_world.entities[0].dragon_attack_active);
 dragon_fire_world.tiles[3*40+6].solid=0;
 dragon_fire_world.tiles[3*40+3].solid=0;dragon_fire_world.tiles[3*40+4].solid=0;dragon_fire_world.tiles[3*40+5].solid=0;dragon_fire_world.tiles[3*40+6].solid=0;dragon_fire_world.tiles[3*40+7].solid=0;dragon_fire_world.tiles[3*40+8].solid=0;
 dragon_fire_world.entities[0].x_q8=100*256;dragon_fire_world.entities[0].y_q8=50*256;dragon_fire_world.entities[0].dragon_action=WORLD_DRAGON_ACTION_SHOOT_PLAYER_ONCE;
 dragon_fire_world.entities[0].vx=dragon_fire_world.entities[0].vy=0;
 dragon_fire_world.entities[0].dragon_action_timer=1;
 world_enemies_step(&dragon_fire_world,150,50,8,12,NULL);
 assert(dragon_fire_world.entities[0].dragon_attack_active&&dragon_fire_world.entities[0].dragon_attack_timer==55);
 for(unsigned tick=0;tick<55;tick++)world_enemies_step(&dragon_fire_world,150,50,8,12,NULL);
 assert(dragon_fire_world.entity_count==2&&dragon_fire_world.entities[1].kind==WORLD_KIND_DRAGON_FIREBALL);
 assert(!dragon_fire_world.entities[0].dragon_attack_active&&
        dragon_fire_world.entities[0].dragon_attack_shots_remaining==0);
 dragon_fire_world.entities[1].x_q8=150*256;dragon_fire_world.entities[1].y_q8=50*256;
 assert(world_enemies_contact_damage(&dragon_fire_world,150,50,8,12)==WORLD_DRAGON_FIREBALL_DAMAGE);
 assert(dragon_fire_world.entities[1].hp==0.0f);
 World dragon_burst_world;memset(&dragon_burst_world,0,sizeof(dragon_burst_world));
 dragon_burst_world.width=40;dragon_burst_world.height=24;dragon_burst_world.entity_count=1;
 dragon_burst_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
     .x_q8=100*256,.y_q8=50*256,.hp=WORLD_DRAGON_HP,.dragon_initialized=1,
     .dragon_action=WORLD_DRAGON_ACTION_SHOOT_PLAYER,.dragon_action_timer=1};
 world_enemies_step(&dragon_burst_world,150,50,8,12,NULL);
 assert(dragon_burst_world.entities[0].dragon_attack_active&&
        dragon_burst_world.entities[0].dragon_attack_shots_remaining==3);
 for(unsigned tick=0;tick<55;tick++)world_enemies_step(&dragon_burst_world,150,50,8,12,NULL);
 assert(dragon_burst_world.entity_count==2&&
        dragon_burst_world.entities[0].dragon_attack_shots_remaining==2);
 for(unsigned tick=0;tick<55;tick++)world_enemies_step(&dragon_burst_world,150,50,8,12,NULL);
 assert(dragon_burst_world.entity_count==3&&
        dragon_burst_world.entities[0].dragon_attack_shots_remaining==1);
 for(unsigned tick=0;tick<55;tick++)world_enemies_step(&dragon_burst_world,150,50,8,12,NULL);
 assert(dragon_burst_world.entity_count==4&&
        !dragon_burst_world.entities[0].dragon_attack_active&&
        dragon_burst_world.entities[0].dragon_attack_shots_remaining==0);
 for(unsigned shot=1;shot<=3;shot++)
  assert(dragon_burst_world.entities[shot].kind==WORLD_KIND_DRAGON_FIREBALL);
 assert(world_enemies_contact_damage(&dragon_move_world,
     dragon_move_world.entities[0].x_q8/256.0f,dragon_move_world.entities[0].y_q8/256.0f,8,12)==0);
 for(unsigned tick=0;tick<30;tick++)
  world_enemies_step(&dragon_move_world,
      dragon_move_world.entities[0].x_q8/256.0f,dragon_move_world.entities[0].y_q8/256.0f,8,12,NULL);
 assert(dragon_move_world.entities[0].contact_cooldown==0);
 assert(world_enemies_contact_damage(&dragon_move_world,
     dragon_move_world.entities[0].x_q8/256.0f,dragon_move_world.entities[0].y_q8/256.0f,8,12)==WORLD_DRAGON_DAMAGE);
 World boss_contact_world;memset(&boss_contact_world,0,sizeof(boss_contact_world));
 boss_contact_world.entity_count=1;WorldEntity *charging=&boss_contact_world.entities[0];
 *charging=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_GOBLIN_KING,
     .x_q8=100*256,.y_q8=100*256,.hp=WORLD_GOBLIN_KING_HP,.boss_charge_active=1};
 assert(world_enemies_contact_damage(&boss_contact_world,100,100,8,12)==WORLD_GOBLIN_KING_DAMAGE);
 assert(charging->boss_charge_hit);
 assert(world_enemies_contact_damage(&boss_contact_world,100,100,8,12)==0);
 WorldEntity retreating={.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_GOBLIN_KING,
                         .x_q8=80*256,.y_q8=80*256,.hp=100};
 World retreat_world;memset(&retreat_world,0,sizeof(retreat_world));
 retreat_world.width=40;retreat_world.height=24;retreat_world.entity_count=1;
 retreat_world.entities[0]=retreating;
 world_enemies_step(&retreat_world,40,80,8,12,NULL);
 assert(retreat_world.entities[0].boss_charge_active&&
        !retreat_world.entities[0].boss_facing_right&&retreat_world.entities[0].boss_charge_timer==75);
 World boss_before=boss_world;boss[9]=52;write(f,boss,sizeof(boss));
 assert(!world_load_enemies(&boss_world,path,NULL));assert(!memcmp(&boss_world,&boss_before,sizeof(boss_world)));
 fclose(f);assert(remove(path)==0);return 0;
}
