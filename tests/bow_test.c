#include "bow.h"
#include "world.h"
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
static void put16(FILE *f,unsigned v){unsigned char b[2]={v&255,(v>>8)&255};assert(fwrite(b,1,2,f)==2);}
static void putf(FILE *f,float v){uint32_t u;memcpy(&u,&v,4);unsigned char b[4]={u&255,(u>>8)&255,(u>>16)&255,u>>24};assert(fwrite(b,1,4,f)==4);}
static void packet(const char *path){
 FILE *f=fopen(path,"wb");assert(f);assert(fwrite("VWB1",1,4,f)==4);
 put16(f,1);put16(f,3);put16(f,2);put16(f,25);put16(f,2);put16(f,0);
 const float values[]={0.35f,0.29f,0.6f,0.48f,-1,3,7};for(int i=0;i<7;i++)putf(f,values[i]);
 for(int i=0;i<3;i++){put16(f,i);put16(f,1);const float k[]={1,0,0,0,1,-6,12,0,1,1};for(int j=0;j<10;j++)putf(f,k[j]);}
 assert(fclose(f)==0);
}
int main(void){
 const char *path="bow-packet-test.bin";packet(path);BowDefinition d;memset(&d,0,sizeof(d));assert(bow_load(&d,path,NULL));BowDefinition old=d;
 FILE *f=fopen(path,"r+b");assert(f);assert(fseek(f,32,SEEK_SET)==0);putf(f,NAN);assert(fclose(f)==0);assert(!bow_load(&d,path,NULL)&&!memcmp(&old,&d,sizeof(d)));
 packet(path);f=fopen(path,"ab");assert(f);assert(fputc(0,f)!=EOF);assert(fclose(f)==0);assert(!bow_load(&d,path,NULL));
 f=fopen(path,"wb");assert(f);assert(fwrite("VWB1",1,4,f)==4);assert(fclose(f)==0);assert(!bow_load(&d,path,NULL));assert(remove(path)==0);
 BowState s;memset(&s,0,sizeof(s));assert(!bow_start(&s,&d,0,1,0));assert(bow_start(&s,&d,0,1,1));assert(!bow_start(&s,&d,0,1,1));assert(fabsf(bow_projectile_speed(&s)-0.1f)<0.0001f);
 bow_step(&s,1);bow_step(&s,1);assert(s.keyframe==1&&s.timer==2&&fabsf(bow_projectile_speed(&s)-4)<0.001f);
 for(int i=0;i<40;i++){assert(!bow_step(&s,1));}assert(s.keyframe==1&&s.timer==0&&fabsf(bow_projectile_speed(&s)-6)<0.001f);
 float x,y;bow_projectile_origin(&s,100,100,&x,&y);assert(fabsf(x-113)<0.001f&&fabsf(y-94)<0.001f);
 s.facing=-1;bow_projectile_origin(&s,100,100,&x,&y);assert(fabsf(x-87)<0.001f);
 assert(bow_step(&s,0)==1);for(int i=0;i<10;i++)assert(!bow_step(&s,0));assert(!s.active);
 World w;memset(&w,0,sizeof(w));w.ready=1;w.width=40;w.height=24;
 for(int ty=0;ty<24;ty++){w.tiles[ty*40+10].type=1;}world_collision_flags(&w);
 ArrowPool p;memset(&p,0,sizeof(p));assert(arrow_launch(&p,&w,90,100,100,100,40,0,1));
 arrows_step(&p,&w);assert(p.arrows[0].active&&!p.arrows[0].stuck);arrows_step(&p,&w);
 assert(p.arrows[0].stuck&&p.arrows[0].x+1<=160.01f&&p.impacts==1);
 for(int i=0;i<120;i++){arrows_step(&p,&w);}assert(!p.arrows[0].active&&p.expired==1);
 memset(&p,0,sizeof(p));assert(arrow_launch(&p,&w,157,100,170,100,10,0,1));assert(p.arrows[0].stuck&&p.arrows[0].x+1<=160.01f);
 memset(&p,0,sizeof(p));w.entity_count=1;w.entities[0]=(WorldEntity){.kind=24,.x_q8=104*256,.y_q8=94*256,.hp=2};
 assert(arrow_launch(&p,&w,90,100,90,100,20,0,1));arrows_step(&p,&w);
 assert(p.enemy_hits==1&&p.enemy_kills==0&&fabsf(w.entities[0].hp-1)<0.001f&&!p.arrows[0].active);
 assert(arrow_launch(&p,&w,90,100,90,100,20,0,1));arrows_step(&p,&w);
 assert(p.enemy_hits==2&&p.enemy_kills==1&&w.entities[0].hp==0&&!p.arrows[1].active);
 arrows_step(&p,&w);assert(p.enemy_hits==2&&p.enemy_kills==1);
 memset(&p,0,sizeof(p));w.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,
  .subtype=WORLD_BOSS_GOBLIN_KING,.x_q8=200*256,.y_q8=100*256,.hp=WORLD_GOBLIN_KING_HP};
 assert(arrow_launch(&p,&w,190,106,205,106,10,0,1));
 assert(p.enemy_hits==1&&p.enemy_kills==0&&fabsf(w.entities[0].hp-(WORLD_GOBLIN_KING_HP-1))<.001f);
 assert(!p.arrows[0].active);
 w.entities[0].hp=70;w.entities[0].boss_reward_spawned=0;w.entity_count=1;w.bosses_loaded=1;
 memset(&p,0,sizeof(p));
 assert(arrow_launch(&p,&w,190,106,205,106,10,0,70));
 assert(p.enemy_hits==1&&p.enemy_kills==1&&w.entities[0].hp==0);
 assert(w.bosses_defeated==1&&w.bosses_defeated_mask==(1u<<(WORLD_BOSS_GOBLIN_KING-WORLD_BOSS_GOBLIN_KING))&&w.entity_count==2&&w.entities[0].boss_reward_spawned);
 assert(w.entities[1].kind==16&&w.entities[1].boss_reward);
 memset(&p,0,sizeof(p));w.entity_count=1;w.bosses_loaded=1;w.bosses_defeated=0;
 w.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_DRAGON,
  .x_q8=200*256,.y_q8=100*256,.hp=WORLD_DRAGON_HP};
 assert(w.entities[0].subtype==WORLD_BOSS_DRAGON); assert(arrow_launch(&p,&w,190,106,205,106,10,0,WORLD_DRAGON_HP));
 assert(p.enemy_kills==1&&w.entities[0].hp==0&&w.bosses_defeated==1);
 assert(w.entity_count==2&&w.entities[1].kind==16&&w.entities[1].boss_reward);
 memset(&p,0,sizeof(p));w.entity_count=1;w.bosses_loaded=1;w.bosses_defeated=0;
 w.bosses_defeated_mask=0;
 w.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_WORM,
  .x_q8=200*256,.y_q8=100*256,.hp=WORLD_WORM_HP};
 assert(arrow_launch(&p,&w,190,104,205,104,10,0,1));
 assert(p.enemy_hits==1&&p.enemy_kills==0&&fabsf(w.entities[0].hp-(WORLD_WORM_HP-1))<.001f);
 assert(!p.arrows[0].active);
 w.entities[0].hp=1;w.entities[0].boss_reward_spawned=0;memset(&p,0,sizeof(p));
 assert(arrow_launch(&p,&w,190,104,205,104,10,0,1));
 assert(p.enemy_hits==1&&p.enemy_kills==1&&w.entities[0].hp==0);
 assert(w.bosses_defeated==1&&w.bosses_defeated_mask==(1u<<(WORLD_BOSS_WORM-WORLD_BOSS_GOBLIN_KING)));
 assert(w.entity_count==2&&w.entities[0].boss_reward_spawned&&w.entities[1].kind==16&&w.entities[1].boss_reward);
 World worm_world;memset(&worm_world,0,sizeof(worm_world));worm_world.ready=1;worm_world.width=128;worm_world.height=24;
 worm_world.entity_count=1;worm_world.bosses_loaded=1;
 worm_world.entities[0]=(WorldEntity){.kind=WORLD_KIND_BOSS,.subtype=WORLD_BOSS_WORM,
  .x_q8=16*256,.y_q8=32*256,.hp=1.0f};
 for(int tx=0;tx<128;tx++){worm_world.tiles[3*128+tx].platform=1;worm_world.tiles[4*128+tx].solid=1;}
 for(unsigned tick=0;tick<420;tick++)world_enemies_step(&worm_world,1800,32,8,12,NULL);
 float worm_tail_x,worm_tail_y,worm_head_x,worm_head_y;
 world_worm_path_sample(&worm_world,&worm_world.entities[0],WORLD_WORM_SEGMENTS-1u,&worm_tail_x,&worm_tail_y);
 world_worm_path_sample(&worm_world,&worm_world.entities[0],0,&worm_head_x,&worm_head_y);
 assert(fabsf(worm_head_x-worm_tail_x)>400.0f);
 memset(&p,0,sizeof(p));
 assert(arrow_launch(&p,&worm_world,worm_tail_x-12.0f,worm_tail_y+4.0f,
                     worm_tail_x,worm_tail_y+4.0f,12.0f,0.0f,1.0f));
 assert(p.enemy_hits==1&&p.enemy_kills==1&&worm_world.entities[0].hp==0.0f);
 assert(worm_world.bosses_defeated==1&&worm_world.bosses_defeated_mask==(1u<<(WORLD_BOSS_WORM-WORLD_BOSS_GOBLIN_KING)));
 assert(worm_world.entities[0].boss_reward_spawned&&worm_world.entity_count==2&&worm_world.entities[1].boss_reward);
 const uint8_t enemy_types[]={9,10,11,12};
 const float enemy_widths[]={7,9,10,10},enemy_heights[]={12,10,10,7};
 for(unsigned i=0;i<4;i++){
  float enemy_w,enemy_h;world_enemy_body(enemy_types[i],&enemy_w,&enemy_h);
  assert(enemy_w==enemy_widths[i]&&enemy_h==enemy_heights[i]);
  memset(&p,0,sizeof(p));w.entities[0]=(WorldEntity){.kind=24,.subtype=enemy_types[i],.x_q8=200*256,.y_q8=100*256,.hp=2};
  float center_y=100+enemy_h*0.5f;
  assert(arrow_launch(&p,&w,200+enemy_w+1,center_y,200+enemy_w+1,center_y,0,0,1));
  assert(p.enemy_hits==0&&p.arrows[0].active);
  memset(&p,0,sizeof(p));
  assert(arrow_launch(&p,&w,200+enemy_w+0.5f,center_y,200+enemy_w+0.5f,center_y,0,0,1));
  assert(p.enemy_hits==1&&!p.arrows[0].active);
  memset(&p,0,sizeof(p));
  assert(arrow_launch(&p,&w,200+enemy_w*0.5f,100+enemy_h+1,200+enemy_w*0.5f,100+enemy_h+1,0,0,1));
  assert(p.enemy_hits==0&&p.arrows[0].active);
  memset(&p,0,sizeof(p));
  assert(arrow_launch(&p,&w,200+enemy_w*0.5f,100+enemy_h+0.5f,200+enemy_w*0.5f,100+enemy_h+0.5f,0,0,1));
  assert(p.enemy_hits==1&&!p.arrows[0].active);
 }
 w.entity_count=0;
 memset(&p,0,sizeof(p));for(int i=0;i<ARROW_LIMIT;i++)assert(arrow_launch(&p,&w,90,100,100,100,1,0,1));
 ArrowPool before=p;assert(!arrow_launch(&p,&w,90,100,100,100,1,0,1)&&!memcmp(&before,&p,sizeof(p)));
 assert(!arrow_launch(&p,&w,90,100,NAN,100,1,0,1));
 return 0;
}
