#include "player.h"
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned tile_index(const World *world,int x,int y){return (unsigned)(y*world->width+x);}
static int jump_edge_reachable(const World *world,int sx,int sy,int tx,int ty){
 NativePlayer initial;World safe_world=*world;safe_world.entity_count=0;
 if(!player_spawn(&initial,&safe_world,0))return 0;
 initial.x=sx*16.0f+4.0f;initial.y=sy*16.0f+2.0f;
 initial.vx=initial.vy=0;initial.on_ground=1;initial.on_ladder=0;
 initial.previous=0;initial.ticks=0;
 if(player_overlaps_solid(&initial,&safe_world))return 0;
 for(int direction=-1;direction<=1;direction+=2){
  for(unsigned jump_hold=6;jump_hold<=18;jump_hold+=4){
   for(unsigned run_hold=8;run_hold<=40;run_hold+=4){
    NativePlayer trial=initial;World trial_world=safe_world;int landed=0;
    for(unsigned tick=0;tick<72;tick++){
     unsigned input=tick<run_hold?(direction<0?PLAYER_LEFT:PLAYER_RIGHT):0;
     if(tick<jump_hold)input|=PLAYER_JUMP;
     player_step(&trial,&trial_world,input);
     if(trial.dead||player_overlaps_solid(&trial,&trial_world))break;
     if(tick>jump_hold&&trial.on_ground){
      float cx=trial.x+trial.w*0.5f,cy=trial.y+trial.h*0.5f;
      if(fabsf(cx-(tx*16.0f+8.0f))<7.0f&&fabsf(cy-(ty*16.0f+8.0f))<9.0f)landed=1;
      break;
     }
    }
    if(landed)return 1;
   }
  }
 }
 return 0;
}
static int build_tile_route(const World *world,unsigned *route,unsigned *route_count){
 if(world->goal_y>=world->entrance_y){
  int parent[WORLD_CAPACITY],queue[WORLD_CAPACITY],head=0,tail=0;
  unsigned count=0;
  unsigned cells=(unsigned)(world->width*world->height);
  for(unsigned i=0;i<cells;i++)parent[i]=-2;
  unsigned start=tile_index(world,world->entrance_x,world->entrance_y);
  unsigned goal=tile_index(world,world->goal_x,world->goal_y);
  parent[start]=-1;queue[tail++]=(int)start;
  const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
  while(head<tail&&parent[goal]==-2){
   unsigned current=(unsigned)queue[head++];
   int x=(int)(current%(unsigned)world->width),y=(int)(current/(unsigned)world->width);
   for(int d=0;d<4;d++){
    int nx=x+dx[d],ny=y+dy[d];
    if(nx<0||ny<0||nx>=world->width||ny>=world->height||world_solid(world,nx,ny))continue;
    unsigned next=tile_index(world,nx,ny);if(parent[next]!=-2)continue;
    parent[next]=(int)current;queue[tail++]=(int)next;
   }
  }
  if(parent[goal]==-2)return 0;
  for(int node=(int)goal;node>=0;node=parent[node]){if(count>=cells)return 0;route[count++]=(unsigned)node;}
  for(unsigned i=0;i<count/2;i++){unsigned swap=route[i];route[i]=route[count-1-i];route[count-1-i]=swap;}
  *route_count=count;return count>0;
 }
 int parent[WORLD_CAPACITY],distance[WORLD_CAPACITY],visited[WORLD_CAPACITY];
 unsigned count=0;
 unsigned cells=(unsigned)(world->width*world->height);
 for(unsigned i=0;i<cells;i++){parent[i]=-2;distance[i]=0x3fffffff;visited[i]=0;}
 unsigned start=tile_index(world,world->entrance_x,world->entrance_y);
 unsigned goal=tile_index(world,world->goal_x,world->goal_y);
 parent[start]=-1;distance[start]=0;
 const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
 for(unsigned iteration=0;iteration<cells;iteration++){
  int selected=-1,best=0x3fffffff;
  for(unsigned i=0;i<cells;i++)if(!visited[i]&&distance[i]<best){selected=(int)i;best=distance[i];}
  if(selected<0||selected==(int)goal)break;
  unsigned current=(unsigned)selected;visited[current]=1;
  int x=(int)(current%(unsigned)world->width),y=(int)(current/(unsigned)world->width);
  for(int d=0;d<4;d++){
   int nx=x+dx[d],ny=y+dy[d];if(nx<0||ny<0||nx>=world->width||ny>=world->height||world_solid(world,nx,ny))continue;
   unsigned next=tile_index(world,nx,ny);if(visited[next])continue;
   int step=1;
   if(dy[d]<0&&world->goal_y<world->entrance_y){
    const WorldTile *from=world_tile(world,x,y),*to=world_tile(world,nx,ny);
    if(from->type!=3&&to->type!=3)continue;
   }
   if(distance[current]+step<distance[next]){distance[next]=distance[current]+step;parent[next]=(int)current;}
  }
  const WorldTile *standing=world_tile(world,x,y),*floor_tile=world_tile(world,x,y+1);
  if(standing&&standing->type!=3&&floor_tile&&(floor_tile->solid||floor_tile->platform)){
   for(int rise=1;rise<=2;rise++)for(int run=-5;run<=5;run++){
    if(!run&&rise==0)continue;
    int nx=x+run,ny=y-rise;
    if(nx<0||ny<1||nx>=world->width||world_solid(world,nx,ny)||world_solid(world,nx,ny-1))continue;
    const WorldTile *landing=world_tile(world,nx,ny+1);
    if(!landing||(!landing->solid&&!landing->platform))continue;
    if(!jump_edge_reachable(world,x,y,nx,ny))continue;
    unsigned next=tile_index(world,nx,ny);if(visited[next])continue;
    int jump_cost=4+abs(run)+rise*2;
    if(distance[current]+jump_cost<distance[next]){
     distance[next]=distance[current]+jump_cost;parent[next]=(int)current;
    }
   }
  }
 }
 if(parent[goal]==-2)return 0;
 for(int node=(int)goal;node>=0;node=parent[node]){if(count>=cells)return 0;route[count++]=(unsigned)node;}
 for(unsigned i=0;i<count/2;i++){unsigned swap=route[i];route[i]=route[count-1-i];route[count-1-i]=swap;}
 *route_count=count;return count>0;
}
static int build_tile_route_between(const World *source,int sx,int sy,int gx,int gy,
                                    unsigned *route,unsigned *route_count){
 World copy=*source;copy.entrance_x=sx;copy.entrance_y=sy;copy.goal_x=gx;copy.goal_y=gy;
 return build_tile_route(&copy,route,route_count);
}
static int build_worm_staging_route(const World *world,int sx,int sy,int bx,int by,
                                    unsigned *route,unsigned *route_count,
                                    int *stage_x,int *stage_y){
 int best_score=INT_MAX,best=0;
 unsigned candidate_route[WORLD_CAPACITY];
 for(int radius=4;radius<=7;radius++)for(int dy=-2;dy<=2;dy++){
  for(int side=-1;side<=1;side+=2){
   int x=bx+side*radius,y=by+dy;
   const WorldTile *cell=world_tile(world,x,y),*head=world_tile(world,x,y-1),*support=world_tile(world,x,y+1);
   if(!cell||!head||!support||cell->solid||cell->platform||head->solid||
      (!support->solid&&!support->platform))continue;
   unsigned candidate_count=0;
   if(!build_tile_route_between(world,sx,sy,x,y,candidate_route,&candidate_count)||
      candidate_count>=WORLD_CAPACITY)continue;
   int score=(int)candidate_count*16+abs(dy)*3+(side<0?0:1);
   if(score<best_score){
    memcpy(route,candidate_route,candidate_count*sizeof(*route));
    *route_count=candidate_count;*stage_x=x;*stage_y=y;best_score=score;best=1;
   }
  }
 }
 return best;
}
static int find_jump_stage(const World *world,int tx,int ty,float px,float py,int *sx,int *sy){
 int best=0,best_score=0;
 const WorldTile *goal=world_tile(world,tx,ty);
 if(goal&&goal->platform)ty--;
 float goal_x=tx*16.0f+8.0f,goal_y=ty*16.0f+8.0f;
 float current_distance=fabsf(goal_x-px)+fabsf(goal_y-py);
 for(int y=ty;y<=ty+8&&y<world->height-1;y++)for(int x=tx-8;x<=tx+8;x++){
  const WorldTile *cell=world_tile(world,x,y),*above=world_tile(world,x,y-1),*support=world_tile(world,x,y+1);
  if(!cell||!above||!support||cell->solid||above->solid||(!support->solid&&!support->platform))continue;
  float stage_x=x*16.0f+8.0f,stage_y=y*16.0f+8.0f;
  float horizontal=fabsf(stage_x-px),vertical=fabsf(stage_y-py);
  float candidate_distance=fabsf(stage_x-goal_x)+fabsf(stage_y-goal_y);
  if(horizontal>64.0f||vertical>38.0f||stage_y>=py-8.0f||candidate_distance>=current_distance-8.0f)continue;
  int score=(int)(candidate_distance*4.0f+horizontal+vertical*0.5f);
  if(!best||score<best_score){*sx=x;*sy=y;best_score=score;best=1;}
 }
 return best;
}
static int find_drop_stage(const World *world,int tx,int ty,float px,float py,int *sx,int *sy){
 int best=0,best_score=0;
 float gx=tx*16.0f+8.0f,gy=ty*16.0f+8.0f;
 float current=fabsf(gx-px)+fabsf(gy-py);
 for(int y=ty;y<=ty+8&&y<world->height-1;y++)for(int x=tx-8;x<=tx+8;x++){
  const WorldTile *cell=world_tile(world,x,y),*above=world_tile(world,x,y-1),*support=world_tile(world,x,y+1);
  if(!cell||!above||!support||cell->solid||cell->platform||above->solid||
     (!support->solid&&!support->platform))continue;
  float cx=x*16.0f+8.0f,cy=y*16.0f+8.0f;
  float horizontal=fabsf(cx-px),vertical=cy-py;
  float candidate=fabsf(cx-gx)+fabsf(cy-gy);
  if(horizontal>80.0f||vertical<8.0f||cy>gy||candidate>=current)continue;
  int score=(int)(candidate*4.0f+horizontal+vertical*.5f);
  if(!best||score<best_score){*sx=x;*sy=y;best_score=score;best=1;}
 }
 return best;
}
static float route_distance(const NativePlayer *player,int tx,int ty){
 float px=player->x+player->w*0.5f,py=player->y+player->h*0.5f;
 const WorldTile *tile=NULL;
 (void)tile;
 return fabsf(px-(tx*16.0f+8.0f))+fabsf(py-(ty*16.0f+8.0f));
}
static int nearby_melee_enemy_direction(const World *world,const NativePlayer *player){
 float px=player->x+player->w*0.5f,py=player->y+player->h*0.5f;
 float best=1e30f;int direction=0;
 for(unsigned i=0;i<world->entity_count;i++){
  const WorldEntity *enemy=&world->entities[i];
  if(enemy->hp<=0.0f)continue;
  if(enemy->kind==24){
   float width,height;world_enemy_body(enemy->subtype,&width,&height);
   float ex=enemy->x_q8/256.0f+width*0.5f,ey=enemy->y_q8/256.0f+height*0.5f;
   float horizontal=fmaxf(0.0f,fabsf(ex-px)-(width+player->w)*0.5f);
   float vertical=fmaxf(0.0f,fabsf(ey-py)-(height+player->h)*0.5f);
   if(horizontal>20.0f||vertical>18.0f)continue;
   float score=horizontal+vertical*2.0f;
   if(score<best){best=score;direction=ex>=px?1:-1;}
  }else if(enemy->kind==WORLD_KIND_BOSS&&enemy->subtype==WORLD_BOSS_WORM){
   for(unsigned segment=0;segment<=WORLD_WORM_SEGMENTS;segment++){
    float sx=enemy->x_q8/256.0f,sy=enemy->y_q8/256.0f;
    if(segment)world_worm_path_sample(world,enemy,segment-1u,&sx,&sy);
    float ex=sx+4.0f,ey=sy+4.0f;
    float horizontal=fmaxf(0.0f,fabsf(ex-px)-(4.0f+player->w)*0.5f);
    float vertical=fmaxf(0.0f,fabsf(ey-py)-(4.0f+player->h)*0.5f);
    if(horizontal>20.0f||vertical>18.0f)continue;
    float score=horizontal+vertical*2.0f;
    if(score<best){best=score;direction=ex>=px?1:-1;}
   }
  }
 }
 return direction;
}
static uint16_t controller_buttons(unsigned input){
 uint16_t buttons=0;
 if(input&PLAYER_UP)buttons|=0x0010;
 if(input&PLAYER_RIGHT)buttons|=0x0020;
 if(input&PLAYER_DOWN)buttons|=0x0040;
 if(input&PLAYER_LEFT)buttons|=0x0080;
 if(input&PLAYER_SLOW)buttons|=0x0100;
 if(input&PLAYER_RANGE)buttons|=0x0200;
 if(input&PLAYER_ATTACK)buttons|=0x8000;
 if(input&PLAYER_JUMP)buttons|=0x4000;
 return buttons;
}
static int write_controller_trace(const char *path,const uint16_t *trace,unsigned ticks){
 FILE *file=fopen(path,"wb");if(!file)return 0;
 unsigned char header[8]={'V','T','R','1',(unsigned char)ticks,(unsigned char)(ticks>>8),(unsigned char)(ticks>>16),(unsigned char)(ticks>>24)};
 int ok=fwrite(header,1,sizeof(header),file)==sizeof(header);
 for(unsigned i=0;ok&&i<ticks;i++){
  unsigned char record[2]={(unsigned char)trace[i],(unsigned char)(trace[i]>>8)};
  ok=fwrite(record,1,sizeof(record),file)==sizeof(record);
 }
 if(fclose(file)!=0)ok=0;
 return ok;
}
static unsigned choose_route_input(const NativePlayer *player,const World *world,int tx,int ty){
static const unsigned actions[]={0,PLAYER_LEFT,PLAYER_RIGHT,PLAYER_UP,PLAYER_DOWN,PLAYER_JUMP,
  PLAYER_LEFT|PLAYER_JUMP,PLAYER_RIGHT|PLAYER_JUMP,PLAYER_DOWN|PLAYER_JUMP,
  PLAYER_UP|PLAYER_LEFT,PLAYER_UP|PLAYER_RIGHT};
 float current=route_distance(player,tx,ty),best=1e30f;unsigned selected=0;
 const WorldTile *goal_tile=world_tile(world,tx,ty);
 int platform_goal=goal_tile&&goal_tile->platform;
 unsigned action_count=(unsigned)(sizeof(actions)/sizeof(actions[0]));
 for(unsigned i=0;i<action_count;i++){
  unsigned action=actions[i];
  NativePlayer trial=*player;World trial_world=*world;float closest=1e30f;
  /* Candidate simulations may move existing Worm spits, but must not create
     new attacks or advance the global articulated-path cache. */
  for(unsigned entity=0;entity<trial_world.entity_count;entity++)
   if(trial_world.entities[entity].kind==WORLD_KIND_BOSS&&
      trial_world.entities[entity].subtype==WORLD_BOSS_WORM)
    trial_world.entities[entity].hp=0.0f;
  for(unsigned tick=0;tick<12;tick++){
   player_step(&trial,&trial_world,action);
   if(trial.dead||player_overlaps_solid(&trial,&trial_world))break;
   float d=route_distance(&trial,tx,ty);
   if(platform_goal){float feet=trial.y+trial.h;float target=ty*16.0f;
    d=fabsf((trial.x+trial.w*0.5f)-(tx*16.0f+8.0f))+fabsf(feet-target)*2.0f;
   }
   if(d<closest)closest=d;
  }
  float score=closest+(float)(player->hp-trial.hp)*24.0f;
  if(trial.dead)score+=1000.0f;
  if(closest>=current-0.1f)score+=28.0f;
  if(action&(PLAYER_LEFT|PLAYER_RIGHT))score+=0.01f;
  if(score<best){best=score;selected=action;}
 }
 if(selected==0&&fabsf((tx*16.0f+8.0f)-(player->x+player->w*0.5f))>12.0f)
  selected=(tx*16.0f+8.0f)>(player->x+player->w*0.5f)?PLAYER_RIGHT:PLAYER_LEFT;
 return selected;
}
static int worm_spit_threat_near_player(const World *world,const NativePlayer *player){
 float px=player->x+player->w*.5f,py=player->y+player->h*.5f;
 for(unsigned i=0;i<world->entity_count;i++){
  const WorldEntity *shot=&world->entities[i];
  if(shot->kind!=WORLD_KIND_WORM_SPIT||shot->hp<=0.0f)continue;
  float dx=px-(shot->x_q8/256.0f),dy=py-(shot->y_q8/256.0f);
  float distance2=dx*dx+dy*dy;
  if(distance2>240.0f*240.0f||distance2<0.001f)continue;
  if(dx*shot->vx+dy*shot->vy>0.0f)return 1;
 }
 return 0;
}
int main(int argc,char **argv){
 const char *trace_path=NULL,*input_trace_path=NULL,*position_log_path=NULL,*boss_packet=NULL,*melee_packet=NULL,*loot_packet=NULL,*attack_packet=NULL;
 int boss_test_class=0,boss_class_override=-1,start_hp_override=-1,start_arrows_override=-1;
 for(int i=4;i<argc;){
  if(!strcmp(argv[i],"--trace-out")&&i+1<argc&&!trace_path){trace_path=argv[i+1];i+=2;}
  else if(!strcmp(argv[i],"--trace-in")&&i+1<argc&&!input_trace_path){input_trace_path=argv[i+1];i+=2;}
  else if(!strcmp(argv[i],"--position-log")&&i+1<argc&&!position_log_path){position_log_path=argv[i+1];i+=2;}
  else if(!strcmp(argv[i],"--boss-class")&&i+1<argc&&boss_class_override<0){
   char *class_end=NULL;long class_value=strtol(argv[i+1],&class_end,10);
   if(!class_end||*class_end||class_value<0||class_value>2){fprintf(stderr,"invalid boss player class\n");return 2;}
   boss_class_override=(int)class_value;i+=2;
  }
  else if(!strcmp(argv[i],"--start-hp")&&i+1<argc&&start_hp_override<0){
   char *value_end=NULL;long value=strtol(argv[i+1],&value_end,10);
   if(!value_end||*value_end||value<1||value>300){fprintf(stderr,"invalid starting hp\n");return 2;}
   start_hp_override=(int)value;i+=2;
  }
  else if(!strcmp(argv[i],"--start-arrows")&&i+1<argc&&start_arrows_override<0){
   char *value_end=NULL;long value=strtol(argv[i+1],&value_end,10);
   if(!value_end||*value_end||value<0||value>999){fprintf(stderr,"invalid starting arrows\n");return 2;}
   start_arrows_override=(int)value;i+=2;
  }
  else if(!strcmp(argv[i],"--boss")&&i+4<argc&&!boss_packet){boss_packet=argv[i+1];melee_packet=argv[i+2];loot_packet=argv[i+3];attack_packet=argv[i+4];i+=5;}
  else {fprintf(stderr,"usage: act-one-route-probe WORLD.VWI ENTITIES.VWE SEED [--trace-in INPUT.VTR] [--trace-out TRACE.VTR] [--position-log POSITIONS.CSV] [--boss-class 0|1|2] [--start-hp N] [--start-arrows N] [--boss ENEMIES.VWE MELEE.VWA LOOT.VLT ATTACK.VGA]\n");return 2;}
 }
 uint16_t *input_trace=NULL;unsigned input_trace_count=0;
 if(input_trace_path){
  FILE *file=fopen(input_trace_path,"rb");unsigned char header[8];
  if(!file||fread(header,1,sizeof(header),file)!=sizeof(header)||memcmp(header,"VTR1",4)){
   if(file)fclose(file);fprintf(stderr,"route probe cannot read VTR1 input trace: %s\n",input_trace_path);return 2;
  }
  input_trace_count=(unsigned)header[4]|(unsigned)header[5]<<8|(unsigned)header[6]<<16|(unsigned)header[7]<<24;
  if(!input_trace_count||input_trace_count>36000){fclose(file);fprintf(stderr,"route probe input trace frame count is invalid\n");return 2;}
  input_trace=calloc(input_trace_count,sizeof(*input_trace));
  if(!input_trace){fclose(file);fprintf(stderr,"route probe cannot allocate input trace\n");return 2;}
  for(unsigned frame=0;frame<input_trace_count;frame++){
   unsigned char record[2];
   if(fread(record,1,sizeof(record),file)!=sizeof(record)){free(input_trace);fclose(file);fprintf(stderr,"route probe input trace is truncated\n");return 2;}
   input_trace[frame]=(uint16_t)(record[0]|(unsigned)record[1]<<8);
   if(input_trace[frame]&~0xf3f9u){free(input_trace);fclose(file);fprintf(stderr,"route probe input trace contains unsupported buttons\n");return 2;}
  }
  if(fgetc(file)!=EOF){free(input_trace);fclose(file);fprintf(stderr,"route probe input trace has trailing data\n");return 2;}
  fclose(file);
 }
 char *end=NULL;unsigned long seed=strtoul(argv[3],&end,10);
 if(!end||*end||seed>0xfffffffful){fprintf(stderr,"invalid seed\n");return 2;}
 World world;memset(&world,0,sizeof(world));
 if(!world_load(&world,argv[1],stdout)||!world_load_entities(&world,argv[2],stdout))return 2;
 char probe_bow_path[1024];
 const char *world_separator=strrchr(argv[1],'/');
 const char *windows_separator=strrchr(argv[1],'\\');
 if(!world_separator|| (windows_separator&&windows_separator>world_separator))world_separator=windows_separator;
 size_t world_directory_length=world_separator?(size_t)(world_separator-argv[1]):0;
 if(world_directory_length+15>=sizeof(probe_bow_path))return 2;
 memcpy(probe_bow_path,argv[1],world_directory_length);
 snprintf(probe_bow_path+world_directory_length,sizeof(probe_bow_path)-world_directory_length,"/intro-bow.vwb");
 FILE *probe_bow_file=fopen(probe_bow_path,"rb");
 if(!probe_bow_file){
  if(world_directory_length+19<sizeof(probe_bow_path)){
   snprintf(probe_bow_path,sizeof(probe_bow_path),"%.*s/../intro-bow.vwb",
            (int)world_directory_length,argv[1]);
  }
 }else fclose(probe_bow_file);
 (void)bow_load(&world.bow,probe_bow_path,stdout);
 unsigned route[WORLD_CAPACITY],route_count=0;
 unsigned boss_waypoint=0;WorldEntity *boss=NULL;
 if(boss_packet){
  if(!world_load_enemies(&world,boss_packet,stdout)||!melee_load(&world.melee,melee_packet,stdout)||
     !world_load_loot(&world,loot_packet,stdout)||!world_load_goblin_attack(&world,attack_packet,stdout))return 2;
  world.loot.enabled=1;
  for(unsigned i=0;i<world.entity_count;i++)if(world.entities[i].kind==WORLD_KIND_BOSS&&(world.entities[i].subtype==WORLD_BOSS_GOBLIN_KING||world.entities[i].subtype==WORLD_BOSS_DRAGON||world.entities[i].subtype==WORLD_BOSS_WORM)){boss=&world.entities[i];break;}
  if(!boss){fprintf(stderr,"ACT_ONE_BOSS missing_supported_boss=1\n");return 2;}
  if(boss->subtype==WORLD_BOSS_DRAGON||boss->subtype==WORLD_BOSS_WORM)boss_test_class=1;
  if(boss_class_override>=0)boss_test_class=boss_class_override;
  NativePlayer spawn_probe;if(!player_spawn(&spawn_probe,&world,boss_test_class))return 2;
  int sx=(int)floorf((spawn_probe.x+spawn_probe.w*0.5f)/16.0f),sy=(int)floorf((spawn_probe.y+spawn_probe.h*0.5f)/16.0f);
  int bx=(int)floorf((boss->x_q8/256.0f)/16.0f),by=(int)floorf((boss->y_q8/256.0f)/16.0f);
  if(boss->subtype==WORLD_BOSS_WORM){
   /* Stop at supported ground beside the Worm, rather than routing through
      its body/under the obstructing ledge. The combat controller can then
      keep ranged spacing and seek a clear firing lane. */
   unsigned approach[WORLD_CAPACITY],approach_count=0,exit_route[WORLD_CAPACITY],exit_count=0;
   int stage_x=-1,stage_y=-1;
   int approach_ready=build_worm_staging_route(&world,sx,sy,bx,by,approach,&approach_count,&stage_x,&stage_y);
   int exit_ready=approach_ready&&build_tile_route_between(&world,bx,by,world.goal_x,world.goal_y,exit_route,&exit_count);
   if(!approach_ready||!exit_ready||approach_count+exit_count-1>WORLD_CAPACITY){
    fprintf(stderr,"ACT_ONE_BOSS no_campaign_route=1 approach=%d exit=%d staging=%d,%d anchor=%d,%d entrance=%d,%d goal=%d,%d\n",
            approach_ready,exit_ready,stage_x,stage_y,bx,by,sx,sy,world.goal_x,world.goal_y);return 2;}
   memcpy(route,approach,approach_count*sizeof(*route));
   memcpy(route+approach_count,exit_route+1,(exit_count-1)*sizeof(*route));
   route_count=approach_count+exit_count-1;boss_waypoint=approach_count-1;
   printf("ACT_ONE_BOSS staging_tile=%d,%d distance_tiles=%d\n",stage_x,stage_y,abs(stage_x-bx)+abs(stage_y-by));fflush(stdout);
  }else{
   unsigned approach[WORLD_CAPACITY],approach_count=0,exit_route[WORLD_CAPACITY],exit_count=0;
   int approach_ready=build_tile_route_between(&world,sx,sy,bx,by,approach,&approach_count);
   int exit_ready=approach_ready&&build_tile_route_between(&world,bx,by,world.goal_x,world.goal_y,exit_route,&exit_count);
   if(!approach_ready||!exit_ready||approach_count+exit_count-1>WORLD_CAPACITY){
    fprintf(stderr,"ACT_ONE_BOSS no_campaign_route=1 approach=%d exit=%d anchor=%d,%d entrance=%d,%d goal=%d,%d\n",
            approach_ready,exit_ready,bx,by,sx,sy,world.goal_x,world.goal_y);return 2;}
   memcpy(route,approach,approach_count*sizeof(*route));
   memcpy(route+approach_count,exit_route+1,(exit_count-1)*sizeof(*route));
   route_count=approach_count+exit_count-1;boss_waypoint=approach_count-1;
  }
 }
 NativePlayer player;if(!player_spawn(&player,&world,boss_test_class))return 2;
 if(start_hp_override>0){if(start_hp_override>player.max_hp){fprintf(stderr,"starting hp exceeds class maximum\n");return 2;}player.hp=start_hp_override;}
 if(start_arrows_override>=0){int found=0;for(unsigned item=0;item<player.loadout.item_count;item++)if(player.loadout.items[item].type==3){player.loadout.items[item].quantity=(uint16_t)start_arrows_override;found=1;break;}if(!found){fprintf(stderr,"starting arrows requested for a class without arrow ammunition\n");return 2;}}
 unsigned starting_ammo=0;for(unsigned item=0;item<player.loadout.item_count;item++)if(player.loadout.items[item].type==3)starting_ammo+=player.loadout.items[item].quantity;
 printf("ACT_ONE_PLAYER class=%d hp=%d/%d bow=%u arrows=%u\n",player.player_class,player.hp,player.max_hp,player.bow.definition!=NULL,starting_ammo);fflush(stdout);
 if(!boss){int sx=(int)floorf((player.x+player.w*0.5f)/16.0f),sy=(int)floorf((player.y+player.h*0.5f)/16.0f);
  if(!build_tile_route_between(&world,sx,sy,world.goal_x,world.goal_y,route,&route_count)){fprintf(stderr,"ACT_ONE_ROUTE no_tile_route=1 start=%d,%d\n",sx,sy);return 2;}}
 int reached=world_goal_touched(&world,player.x,player.y,player.w,player.h);
 unsigned ticks=0,jumps=0,waypoint=route_count>1?1:0,stuck=0,waypoint_ticks=0;
 int route_replanned_after_boss=0;
 int target_x=-1,target_y=-1,stage_x=-1,stage_y=-1,stage_waypoint=-1;
 int drop_stage_pending=0,drop_stage_active=0;
 float last_x=player.x,last_y=player.y;
 const unsigned max_ticks=180000,waypoint_limit=7200;
 unsigned common_enemy_start=0,common_enemy_deaths=0;
 for(unsigned i=0;i<world.entity_count;i++)if(world.entities[i].kind==24&&world.entities[i].hp>0.0f)common_enemy_start++;
 uint16_t *controller_trace=trace_path?calloc(max_ticks,sizeof(*controller_trace)):NULL;
 if(trace_path&&!controller_trace){fprintf(stderr,"route probe cannot allocate controller trace\n");return 2;}
 FILE *position_log=position_log_path?fopen(position_log_path,"w"):NULL;
 if(position_log_path&&!position_log){fprintf(stderr,"route probe cannot open position log: %s\n",position_log_path);free(controller_trace);return 2;}
 if(position_log)fprintf(position_log,"tick,x,y,hp,ground,ladder,waypoint,input,contacts,target_x,target_y,vx,vy,melee_active,bow_active,nearby_enemy,stage_x,stage_y,drop_pending,drop_active,tile_x,tile_y,solid\n");
 while(!reached&&ticks<max_ticks){
  if(boss&&boss->hp<=0.0f&&!route_replanned_after_boss){
   int player_tile_x=(int)floorf((player.x+player.w*0.5f)/16.0f);
   int player_tile_y=(int)floorf((player.y+player.h*0.5f)/16.0f);
   int route_rejoined=0;
   if(boss->subtype==WORLD_BOSS_WORM){
    unsigned first=boss_waypoint>12?boss_waypoint-12:0,best=first;
    int nearest=INT_MAX;
    for(unsigned i=first;i<route_count;i++){
     int rx=(int)(route[i]%(unsigned)world.width),ry=(int)(route[i]/(unsigned)world.width);
     int distance=abs(rx-player_tile_x)+abs(ry-player_tile_y);
     if(distance<nearest){nearest=distance;best=i;}
    }
    if(nearest<=8){
     waypoint=best;waypoint_ticks=0;stuck=0;
     stage_x=stage_y=-1;stage_waypoint=-1;route_rejoined=1;
     fprintf(stdout,"ACT_ONE_BOSS exit_route_rejoined=1 start=%d,%d waypoint=%u distance=%d tiles=%u\n",
             player_tile_x,player_tile_y,waypoint,nearest,route_count);fflush(stdout);
    }
   }
   if(!route_rejoined){
    unsigned exit_route[WORLD_CAPACITY],exit_count=0;
    if(build_tile_route_between(&world,player_tile_x,player_tile_y,
                               world.goal_x,world.goal_y,exit_route,&exit_count)){
     memcpy(route,exit_route,exit_count*sizeof(*route));route_count=exit_count;
     waypoint=0;boss_waypoint=0;waypoint_ticks=0;stuck=0;
     stage_x=stage_y=-1;stage_waypoint=-1;
     fprintf(stdout,"ACT_ONE_BOSS exit_route_replanned=1 start=%d,%d tiles=%u\n",
             player_tile_x,player_tile_y,route_count);fflush(stdout);
     fprintf(stdout,"ACT_ONE_BOSS exit_route=");
     for(unsigned route_index=0;route_index<route_count;route_index++){
      unsigned route_tile=route[route_index];
      fprintf(stdout,"%s%u,%u",route_index?";":"",
              route_tile%(unsigned)world.width,route_tile/(unsigned)world.width);
     }
     fputc('\n',stdout);fflush(stdout);
    }else{
     fprintf(stderr,"ACT_ONE_BOSS exit_route_replanned=0 start=%d,%d goal=%d,%d\n",
             player_tile_x,player_tile_y,world.goal_x,world.goal_y);fflush(stderr);
    }
   }
   route_replanned_after_boss=1;
  }
  if(boss&&boss->hp>0.0f&&waypoint>boss_waypoint)waypoint=boss_waypoint;
  unsigned prior_waypoint=waypoint;
  while(waypoint<route_count&&(!boss||boss->hp<=0.0f||
        boss->subtype==WORLD_BOSS_WORM||waypoint!=boss_waypoint)){
   int wx=(int)(route[waypoint]%(unsigned)world.width),wy=(int)(route[waypoint]/(unsigned)world.width);
   float tx=wx*16.0f+8.0f,ty=wy*16.0f+8.0f;
   const WorldTile *waypoint_tile=world_tile(&world,wx,wy);
   int boss_blocks_route=boss&&boss->hp>0.0f&&
     boss->subtype!=WORLD_BOSS_WORM&&waypoint==boss_waypoint;
   int at_tile=!boss_blocks_route&&fabsf(tx-(player.x+player.w*0.5f))<7.0f&&fabsf(ty-(player.y+player.h*0.5f))<9.0f;
   int at_platform=waypoint_tile&&waypoint_tile->platform&&fabsf((player.y+player.h)-wy*16.0f)<9.0f;
   if(at_tile||at_platform)waypoint++;
   else break;
  }
  if(waypoint!=prior_waypoint){waypoint_ticks=0;stuck=0;}
  else waypoint_ticks++;
  unsigned target=waypoint<route_count?route[waypoint]:route[route_count-1];
  int tx=(int)(target%(unsigned)world.width),ty=(int)(target/(unsigned)world.width);
  float px=player.x+player.w*0.5f,py=player.y+player.h*0.5f;
  if(stage_waypoint!=(int)waypoint){stage_x=-1;stage_y=-1;drop_stage_pending=0;drop_stage_active=0;stage_waypoint=(int)waypoint;}
  if(stage_x>=0&&!drop_stage_active&&
     fabsf((stage_x*16.0f+8.0f)-px)<7.0f&&
     (drop_stage_pending?fabsf((stage_y*16.0f+8.0f)-py)<14.0f:
       fabsf((stage_y*16.0f+8.0f)-py)<9.0f)){
   if(drop_stage_pending)drop_stage_active=1;
   else stage_x=stage_y=-1;
   drop_stage_pending=0;
  }
  if(drop_stage_active&&player.on_ground&&player.y+player.h>stage_y*16.0f+14.0f){
   stage_x=stage_y=-1;drop_stage_pending=0;drop_stage_active=0;
  }
  if(stage_x<0&&player.on_ground&&ty*16.0f+8.0f<py-36.0f){
   drop_stage_pending=0;
   if(find_jump_stage(&world,tx,ty,px,py,&stage_x,&stage_y))stage_waypoint=(int)waypoint;
  }
  if((!boss||boss->hp<=0.0f||boss->subtype==WORLD_BOSS_WORM)&&stage_x<0&&
     player.on_ground&&ty*16.0f+8.0f>py+14.0f){
   if(find_drop_stage(&world,tx,ty,px,py,&stage_x,&stage_y)){
    if(boss&&boss->hp>0.0f&&boss->subtype==WORLD_BOSS_WORM){
     /* A geometrically valid lower waypoint can sit directly under a solid
        overhang. Move the stage to the nearest clear vertical shaft so the
        player leaves the ledge instead of repeatedly pressing DOWN+JUMP on
        solid ground. */
     int feet_row=(int)floorf((player.y+player.h+0.01f)/16.0f);
     int current_column=(int)floorf(px/16.0f),best_column=stage_x;
     int best_distance=world.width+1;
     for(int column=current_column-1;column<=current_column+1;column++){
      int distance=abs(column-stage_x);
      if(distance>=best_distance)continue;
      int clear=1;
      for(int row=feet_row;row<=stage_y;row++){
       const WorldTile *tile=world_tile(&world,column,row);
       if(!tile||tile->solid||tile->platform){clear=0;break;}
      }
      if(clear){best_column=column;best_distance=distance;}
     }
     stage_x=best_column;
    }
    stage_waypoint=(int)waypoint;drop_stage_pending=1;drop_stage_active=0;
   }
  }
  if(stage_x>=0){tx=stage_x;ty=stage_y;}
  target_x=tx;target_y=ty;
  float gx=tx*16.0f+8.0f,gy=ty*16.0f+8.0f;
  float dx=gx-px,dy=gy-py;
  unsigned input=0;
  int ranged_boss_battle=boss&&boss->hp>0.0f&&waypoint==boss_waypoint&&
      ((boss->subtype==WORLD_BOSS_DRAGON&&route_distance(&player,tx,ty)<128.0f)||
      boss->subtype==WORLD_BOSS_WORM);
  if(ranged_boss_battle){
   float boss_w,boss_h;world_enemy_body(boss->subtype,&boss_w,&boss_h);
   float boss_x=boss->x_q8/256.0f+boss_w*.5f,boss_y=boss->y_q8/256.0f+boss_h*.5f;
   float player_x=player.x+player.w*.5f,player_y=player.y+player.h*.5f;
   float horizontal= boss_x-player_x;
   float distance=hypotf(horizontal,boss_y-player_y);
   /* The movement code also sets facing, so face the target while a bow shot
      is being drawn/released and only retreat between shots. */
   int facing_input=boss_x>player_x?PLAYER_RIGHT:PLAYER_LEFT;
   int retreat_input=boss_x>player_x?PLAYER_LEFT:PLAYER_RIGHT;
   float desired_distance=boss->subtype==WORLD_BOSS_DRAGON?176.0f:96.0f;
   float minimum_distance=boss->subtype==WORLD_BOSS_DRAGON?136.0f:72.0f;
   const WorldTile *player_feet=world_tile(&world,(int)floorf(player_x/16.0f),
     (int)floorf((player.y+player.h+0.01f)/16.0f));
   int reposition_above_worm=boss->subtype==WORLD_BOSS_WORM&&
      boss_y>player_y+24.0f&&fabsf(horizontal)<48.0f&&
      ty*16.0f+8.0f>player_y+8.0f;
   int drop_for_worm=boss->subtype==WORLD_BOSS_WORM&&player.on_ground&&
     player_feet&&player_feet->platform&&boss_y>player_y+12.0f;
   if(reposition_above_worm){
    /* The floor-1 Worm stands below a solid ledge. Backing away from its head
       can pin the player on the upper edge; follow the lower route waypoint
       until the player can fall to the Worm's level. */
    if(fabsf(dx)>2.0f)input=dx>0.0f?PLAYER_RIGHT:PLAYER_LEFT;
    else input=choose_route_input(&player,&world,tx,ty);
    if(player.on_ground&&!(player.previous&PLAYER_JUMP)){input|=PLAYER_JUMP;jumps++;}
    if(player.bow.definition&&distance<160.0f){
     if(player.facing!=(facing_input==PLAYER_RIGHT?1:-1)){
      input&=~(PLAYER_LEFT|PLAYER_RIGHT);
      input|=facing_input;
     }
     if(player.bow.active||ticks%150u<70u)input|=PLAYER_RANGE;
     if(boss_y>player_y+12.0f)input|=PLAYER_DOWN;
    }
   }else if(drop_for_worm){
    /* A platform otherwise intercepts every horizontal arrow before it can
       reach the lower Worm. Drop first, then draw from the lower floor. */
    input=PLAYER_DOWN;
    if(!(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
   }else{
   int melee_dragon=boss->subtype==WORLD_BOSS_DRAGON&&!player.bow.definition;
   if(melee_dragon){
    if(!player.melee.active&&distance<48.0f&&ticks%24u<8u)
     input|=facing_input|PLAYER_ATTACK;
    else if(!player.melee.active&&fabsf(horizontal)<14.0f)input|=retreat_input;
    else if(fabsf(horizontal)>24.0f)input|=facing_input;
    if((boss->dragon_attack_active||distance<38.0f)&&player.on_ground&&
       !(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
   }else{
    int bow_shot_active=player.bow.active;
   unsigned ammo_count=0;
   for(unsigned item=0;item<player.loadout.item_count;item++){
    if(player.loadout.items[item].type==3)ammo_count=player.loadout.items[item].quantity;
   }
    if(bow_shot_active){
     input|=PLAYER_SLOW;
     if(distance<42.0f&&player.on_ground&&!(player.previous&PLAYER_JUMP)&&ticks%30u==0u)
      input|=PLAYER_JUMP;
    }
   else if(boss->subtype==WORLD_BOSS_WORM&&ammo_count==0){
    float melee_reach=(boss_w+player.w)*.5f+8.0f;
    if(fabsf(horizontal)>melee_reach)input|=facing_input;
    else if(ticks%36u<10u)input|=facing_input|PLAYER_ATTACK;
    if(player.on_ground&&!(player.previous&PLAYER_JUMP)&&
       worm_spit_threat_near_player(&world,&player))input|=PLAYER_JUMP;
   }
    else if(boss->subtype==WORLD_BOSS_DRAGON&&distance<48.0f){
     if(!player.melee.active&&ticks%24u<8u){
      input&=~(PLAYER_LEFT|PLAYER_RIGHT);
      input|=facing_input|PLAYER_ATTACK;
     }else if(!player.melee.active&&fabsf(horizontal)<14.0f)input|=retreat_input;
     else if(!player.melee.active&&fabsf(horizontal)>24.0f)input|=facing_input;
    }
    else{
     if(fabsf(horizontal)>desired_distance)input|=facing_input;
     else if(fabsf(horizontal)<minimum_distance)input|=retreat_input;
    }
    if(player.hanging&&!(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
    if((boss->dragon_attack_active||distance<48.0f)&&player.on_ground&&
       !(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
    if(player.bow.active){
     if(player.bow.keyframe!=player.bow.definition->draw_frames-1)input|=PLAYER_RANGE;
     float predicted_y=boss_y+boss->vy*18.0f;
     if(predicted_y<player_y-20.0f)input|=PLAYER_UP;
     else if(predicted_y>player_y+12.0f)input|=PLAYER_DOWN;
    }else if(!(boss->subtype==WORLD_BOSS_DRAGON&&distance<48.0f)&&ticks%150u<70u){
     input|=PLAYER_RANGE;
    }
   }
   }
  }else if(boss&&boss->hp>0.0f&&waypoint==boss_waypoint&&
     route_distance(&player,tx,ty)<20.0f){
   float boss_w,boss_h;world_enemy_body(boss->subtype,&boss_w,&boss_h);
   float boss_x=boss->x_q8/256.0f+boss_w*.5f;
   float player_x=player.x+player.w*.5f;
   if(boss->subtype==WORLD_BOSS_DRAGON){
    float boss_y=boss->y_q8/256.0f+boss_h*.5f;
    float player_y=player.y+player.h*.5f;
    float distance=hypotf(boss_x-player_x,boss_y-player_y);
    if(distance<100.0f){input|=boss_x>=player_x?PLAYER_LEFT:PLAYER_RIGHT;}
    else if(distance>160.0f){input|=boss_x>=player_x?PLAYER_RIGHT:PLAYER_LEFT;}
    if(fabsf(player_y-boss_y)>48.0f&&player.on_ground&&!(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
    if(ticks%42<18)input|=PLAYER_RANGE;
   }else{
   if(fabsf(player_x-boss_x)>2.0f)input|=player_x<boss_x?PLAYER_RIGHT:PLAYER_LEFT;
   float boss_center_y=boss->y_q8/256.0f+boss_h*.5f;
   float player_center_y=player.y+player.h*.5f;
   if(boss->subtype==WORLD_BOSS_WORM&&player.on_ground&&
      player_center_y<boss_center_y-8.0f){
    const WorldTile *feet=world_tile(&world,(int)floorf(player_x/16.0f),
      (int)floorf((player.y+player.h+0.01f)/16.0f));
    if(feet&&feet->platform){input|=PLAYER_DOWN;if(!(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;}
   }
   if(player.on_ground&&player_center_y>boss_center_y+7.0f&&!(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
   if(ticks%30<6)input|=PLAYER_ATTACK;
   }
   }else if(!ranged_boss_battle)input=choose_route_input(&player,&world,tx,ty);
  int blocked=(dx>0&&(player.contacts&WORLD_RIGHT))||(dx<0&&(player.contacts&WORLD_LEFT));
  if(player.on_ground&&blocked&&(input&(PLAYER_LEFT|PLAYER_RIGHT))){
   if(!(player.previous&PLAYER_JUMP)){input|=PLAYER_JUMP;jumps++;}
  }else if(!player.on_ground&&player.vy<0.0f){
   const WorldTile *goal_tile=world_tile(&world,tx,ty);
   if(!goal_tile||!goal_tile->platform||player.y+player.h>ty*16.0f+0.5f)input|=PLAYER_JUMP;
  }
  else if(player.on_ground&&dy>14.0f){
   const WorldTile *feet=world_tile(&world,(int)floorf(px/16.0f),(int)floorf((player.y+player.h+0.01f)/16.0f));
   if(feet&&feet->platform){
    /* Native physics only drops through a one-way platform on a fresh
       DOWN+JUMP edge. Keep the pair together; DOWN alone merely ducks. */
    input|=PLAYER_DOWN;
    if(!(player.previous&PLAYER_JUMP)){input|=PLAYER_JUMP;jumps++;}
   }
  }
  int nearby_enemy=0;
  if(boss&&boss->subtype==WORLD_BOSS_WORM&&boss->hp>0.0f){
   nearby_enemy=nearby_melee_enemy_direction(&world,&player);
   if(nearby_enemy){
    int ranged=player.bow.definition&&!player.melee.active&&
      !player.hanging&&!player.on_ladder&&
      !(boss&&boss->subtype==WORLD_BOSS_WORM);
    if(ranged){
     input&=~(PLAYER_LEFT|PLAYER_RIGHT);
     input&=~PLAYER_ATTACK;
     float worm_w,worm_h;world_enemy_body(boss->subtype,&worm_w,&worm_h);
     float worm_x=boss->x_q8/256.0f+worm_w*.5f;
     float player_x=player.x+player.w*.5f;
     float horizontal=worm_x-player_x;
     float worm_y=boss->y_q8/256.0f+worm_h*.5f;
     float player_y=player.y+player.h*.5f;
     float vertical=fmaxf(0.0f,worm_y-player_y);
     float shooting_gap=fminf(136.0f,fmaxf(64.0f,vertical*.70f));
     int worm_direction=worm_x>=player_x?1:-1;
     if(player.bow.active){
      if(player.facing!=worm_direction)
       input|=worm_direction>0?PLAYER_RIGHT:PLAYER_LEFT;
      input|=PLAYER_SLOW;
     }else if(fabsf(horizontal)<shooting_gap){
      input|=horizontal>=0.0f?PLAYER_LEFT:PLAYER_RIGHT;
      input&=~PLAYER_RANGE;
     }else{
      if(player.facing!=worm_direction)
       input|=worm_direction>0?PLAYER_RIGHT:PLAYER_LEFT;
      if(ticks%150u<70u)input|=PLAYER_RANGE;
      else input&=~PLAYER_RANGE;
     }
     if(player.bow.active&&worm_y>player_y+12.0f){
      input|=PLAYER_DOWN;
      const WorldTile *feet=world_tile(&world,(int)floorf((player.x+player.w*.5f)/16.0f),
        (int)floorf((player.y+player.h-0.01f)/16.0f));
      if(player.on_ground&&feet&&feet->platform&&!(player.previous&PLAYER_JUMP))
       input|=PLAYER_JUMP;
     }
    }else{
     input&=~PLAYER_RANGE;
     if(!player.melee.active&&!player.bow.active&&!player.hanging&&!player.on_ladder){
      input&=~PLAYER_ATTACK;
      if(ticks%5u==0u)input|=PLAYER_ATTACK;
     }
     if(!player.bow.active&&!player.melee.active){
      input&=~(PLAYER_LEFT|PLAYER_RIGHT);
      input|=nearby_enemy>0?PLAYER_RIGHT:PLAYER_LEFT;
     }else if(player.melee.active){
      input&=~(PLAYER_LEFT|PLAYER_RIGHT);
     }
    }
    if(nearby_enemy&&player.on_ground&&ticks%30u==0u&&
       !(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
   }
  }
  else if(boss&&boss->subtype==WORLD_BOSS_DRAGON){
   nearby_enemy=nearby_melee_enemy_direction(&world,&player);
   if(nearby_enemy&&player.on_ground&&ticks%12u==0u&&
      !(player.previous&PLAYER_JUMP)&&!drop_stage_active)input|=PLAYER_JUMP;
  }
  if(boss&&boss->subtype==WORLD_BOSS_WORM&&boss->hp>0.0f&&
     worm_spit_threat_near_player(&world,&player)){
   /* Do not discard a charged bow shot while evading an incoming spit. The
      prior override replaced the whole combat command with route navigation,
      repeatedly cancelling the draw and leaving the Worm almost untouched. */
   float player_x=player.x+player.w*.5f,player_y=player.y+player.h*.5f;
   float worm_x=boss->x_q8/256.0f+4.0f,worm_y=boss->y_q8/256.0f+4.0f;
   if(player.bow.active&&fabsf(worm_x-player_x)<160.0f&&fabsf(worm_y-player_y)<128.0f){
    input&=~(PLAYER_LEFT|PLAYER_RIGHT|PLAYER_UP|PLAYER_DOWN);
    input|=PLAYER_SLOW;
    if(player.on_ground&&!(player.previous&PLAYER_JUMP))input|=PLAYER_JUMP;
   }
  }
  if(boss&&boss->subtype==WORLD_BOSS_WORM&&boss->hp>0.0f&&
     waypoint<boss_waypoint&&!drop_stage_active&&player.on_ground&&
     fabsf(dy)<10.0f&&fabsf(dx)>8.0f){
   input=dx>0.0f?PLAYER_RIGHT:PLAYER_LEFT;
  }
  /* Preserve a fresh DOWN+JUMP edge needed by player_step to fall through
     one-way platforms; spit dodging must not replace that navigation input. */
  if(drop_stage_active)input=0;
  if(input_trace){
   if(ticks>=input_trace_count){fprintf(stderr,"route probe input trace ended before reaching exit tick=%u boss_hp=%.1f\n",ticks,boss?boss->hp:0.0f);free(input_trace);return 8;}
   uint16_t buttons=input_trace[ticks];input=0;
   if(buttons&0x0010)input|=PLAYER_UP;
   if(buttons&0x0020)input|=PLAYER_RIGHT;
   if(buttons&0x0040)input|=PLAYER_DOWN;
   if(buttons&0x0080)input|=PLAYER_LEFT;
   if(buttons&0x0100)input|=PLAYER_SLOW;
   if(buttons&0x0200)input|=PLAYER_RANGE;
   if(buttons&0x4000)input|=PLAYER_JUMP;
   if(buttons&0x8000)input|=PLAYER_ATTACK;
  }
  player_step(&player,&world,input);ticks++;
  if(boss&&boss->subtype==WORLD_BOSS_WORM&&boss->hp>0.0f){
   unsigned alive=0;for(unsigned i=0;i<world.entity_count;i++)if(world.entities[i].kind==24&&world.entities[i].hp>0.0f)alive++;
   unsigned deaths=common_enemy_start-alive;
   if(deaths>common_enemy_deaths){common_enemy_deaths=deaths;fprintf(stdout,"ACT_ONE_COMMON_ENEMY deaths=%u alive=%u tick=%u\n",deaths,alive,ticks);fflush(stdout);}
  }
  if(position_log&&ticks%12u==0u)fprintf(position_log,"%u,%.2f,%.2f,%d,%d,%d,%u,%u,%u,%d,%d,%.2f,%.2f,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d\n",
    ticks,player.x,player.y,player.hp,player.on_ground,player.on_ladder,waypoint,input,player.contacts,
    target_x,target_y,player.vx,player.vy,player.melee.active,player.bow.active,nearby_enemy,
    stage_x,stage_y,drop_stage_pending,drop_stage_active,
    (int)floorf((player.x+player.w*.5f)/16.0f),
    (int)floorf((player.y+player.h*.5f)/16.0f),
    player_overlaps_solid(&player,&world));
  if(controller_trace)controller_trace[ticks-1]=controller_buttons(input);
  if(player.dead){
   fprintf(stderr,"route probe player died tick=%u waypoint=%u/%u cause=%s hp=%d input=%u x=%.2f y=%.2f\n",ticks,waypoint,route_count,player_death_cause_name((PlayerDeathCause)player.death_cause),player.hp,input,player.x,player.y);
   return 7;
  }
  if(player_overlaps_solid(&player,&world)||!isfinite(player.x)||!isfinite(player.y)){
   fprintf(stderr,"route probe detected invalid player state at tick=%u\n",ticks);return 3;
  }
  /* A stationary player is expected while the boss is within melee range. */
  if(boss&&boss->hp>0.0f&&waypoint>=boss_waypoint)stuck=0;
  else if(fabsf(player.x-last_x)<0.01f&&fabsf(player.y-last_y)<0.01f)stuck++;else stuck=0;
  last_x=player.x;last_y=player.y;
  if(stuck==120){
   fprintf(stderr,"route probe waiting tick=%u waypoint=%u/%u x=%.2f y=%.2f target=%d,%d ground=%d ladder=%d hp=%d dead=%d input=%u contacts=%u vx=%.2f vy=%.2f stage=%d,%d\n",ticks,waypoint,route_count,player.x,player.y,target_x,target_y,player.on_ground,player.on_ladder,player.hp,player.dead,player.previous,player.contacts,player.vx,player.vy,stage_x,stage_y);
  }
  if(stuck>360){
   fprintf(stderr,"route probe stuck tick=%u waypoint=%u/%u x=%.2f y=%.2f ground=%d hanging=%d hp=%d dead=%d contacts=%u vy=%.2f\n",ticks,waypoint,route_count,player.x,player.y,player.on_ground,player.hanging,player.hp,player.dead,player.contacts,player.vy);return 4;
  }
  if(waypoint_ticks>=waypoint_limit){
   float d=route_distance(&player,tx,ty);
   fprintf(stderr,"route probe waypoint timeout tick=%u waypoint=%u/%u boss_waypoint=%u boss_hp=%.1f boss_subtype=%u target=%d,%d distance=%.2f player=%.2f,%.2f ground=%d ladder=%d hanging=%d contacts=%u input=%u vx=%.2f vy=%.2f stage=%d,%d stage_waypoint=%d waypoint_ticks=%u\n",ticks,waypoint,route_count,boss_waypoint,boss?boss->hp:0.0f,boss?boss->subtype:0,tx,ty,d,player.x,player.y,player.on_ground,player.on_ladder,player.hanging,player.contacts,player.previous,player.vx,player.vy,stage_x,stage_y,stage_waypoint,waypoint_ticks);return 5;
  }
  if(boss&&boss->hp>0.0f&&waypoint>=boss_waypoint&&ticks%300==0)
   {fprintf(stdout,"ACT_ONE_BOSS fight ticks=%u subtype=%u hp=%.1f player_hp=%d player=%.1f,%.1f boss=%.1f,%.1f bow=%u/%u arrows=%u hits=%u terrain_hits=%u fireball=%u\n",ticks,boss->subtype,boss->hp,player.hp,player.x,player.y,boss->x_q8/256.0f,boss->y_q8/256.0f,player.bow.active,player.bow.keyframe,player.arrows.fired,player.arrows.enemy_hits,player.arrows.impacts,boss->dragon_attack_active);fflush(stdout);}
  reached=boss&&boss->hp>0.0f?0:world_goal_touched(&world,player.x,player.y,player.w,player.h);
 }
 if(boss_packet)printf("ACT_ONE_BOSS result defeated=%d hp=%.1f player_hp=%d ticks=%u waypoint=%u/%u\n",boss&&boss->hp<=0.0f,boss?boss->hp:0.0f,player.hp,ticks,waypoint,route_count);
 if(position_log)fclose(position_log);
 printf("ACT_ONE_ROUTE seed=%lu tiles=%u waypoint=%u ticks=%u jumps=%u reached_exit=%d x=%.2f y=%.2f target=%d,%d ground=%d contacts=%u vx=%.2f vy=%.2f dead=%d motion=%s\n",
        seed,route_count,waypoint,ticks,jumps,reached,player.x,player.y,target_x,target_y,player.on_ground,player.contacts,player.vx,player.vy,player.dead,player_motion_name(player.motion));
 if(reached&&trace_path&&!write_controller_trace(trace_path,controller_trace,ticks)){
  fprintf(stderr,"route probe reached exit but could not write controller trace: %s\n",trace_path);free(controller_trace);return 6;
 }
 free(controller_trace);
 free(input_trace);
 return reached&&(!boss_packet||!boss||boss->hp<=0.0f)?0:1;
}
