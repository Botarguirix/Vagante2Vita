#include "player.h"
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
static float route_distance(const NativePlayer *player,int tx,int ty){
 float px=player->x+player->w*0.5f,py=player->y+player->h*0.5f;
 const WorldTile *tile=NULL;
 (void)tile;
 return fabsf(px-(tx*16.0f+8.0f))+fabsf(py-(ty*16.0f+8.0f));
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
 static const unsigned actions[]={0,PLAYER_LEFT,PLAYER_RIGHT,PLAYER_UP,PLAYER_DOWN,
  PLAYER_LEFT|PLAYER_JUMP,PLAYER_RIGHT|PLAYER_JUMP,PLAYER_DOWN|PLAYER_JUMP,
  PLAYER_UP|PLAYER_LEFT,PLAYER_UP|PLAYER_RIGHT};
 float current=route_distance(player,tx,ty),best=1e30f;unsigned selected=0;
 const WorldTile *goal_tile=world_tile(world,tx,ty);
 int platform_goal=goal_tile&&goal_tile->platform;
 for(unsigned i=0;i<sizeof(actions)/sizeof(actions[0]);i++){
  NativePlayer trial=*player;World trial_world=*world;float closest=1e30f;
  for(unsigned tick=0;tick<12;tick++){
   player_step(&trial,&trial_world,actions[i]);
   if(trial.dead||player_overlaps_solid(&trial,&trial_world))break;
   float d=route_distance(&trial,tx,ty);
   if(platform_goal){float feet=trial.y+trial.h;float target=ty*16.0f;
    d=fabsf((trial.x+trial.w*0.5f)-(tx*16.0f+8.0f))+fabsf(feet-target)*2.0f;
   }
   if(d<closest)closest=d;
  }
  float score=closest;
  if(closest>=current-0.1f)score+=28.0f;
  if(actions[i]&(PLAYER_LEFT|PLAYER_RIGHT))score+=0.01f;
  if(score<best){best=score;selected=actions[i];}
 }
 if(selected==0&&fabsf((tx*16.0f+8.0f)-(player->x+player->w*0.5f))>12.0f)
  selected=(tx*16.0f+8.0f)>(player->x+player->w*0.5f)?PLAYER_RIGHT:PLAYER_LEFT;
 return selected;
}

int main(int argc,char **argv){
 if(argc!=4&&(argc!=6||strcmp(argv[4],"--trace-out"))){fprintf(stderr,"usage: act-one-route-probe WORLD.VWI ENTITIES.VWE SEED [--trace-out TRACE.VTR]\n");return 2;}
 const char *trace_path=argc==6?argv[5]:NULL;
 char *end=NULL;unsigned long seed=strtoul(argv[3],&end,10);
 if(!end||*end||seed>0xfffffffful){fprintf(stderr,"invalid seed\n");return 2;}
 World world;memset(&world,0,sizeof(world));
 if(!world_load(&world,argv[1],stdout)||!world_load_entities(&world,argv[2],stdout))return 2;
 unsigned route[WORLD_CAPACITY],route_count=0;
 if(!build_tile_route(&world,route,&route_count)){fprintf(stderr,"ACT_ONE_ROUTE no_tile_route=1\n");return 2;}
 NativePlayer player;if(!player_spawn(&player,&world,0))return 2;
 int reached=world_goal_touched(&world,player.x,player.y,player.w,player.h);
 unsigned ticks=0,jumps=0,waypoint=route_count>1?1:0,stuck=0,waypoint_ticks=0;
 int target_x=-1,target_y=-1,stage_x=-1,stage_y=-1,stage_waypoint=-1;
 float last_x=player.x,last_y=player.y;
 const unsigned max_ticks=180000,waypoint_limit=7200;
 uint16_t *controller_trace=trace_path?calloc(max_ticks,sizeof(*controller_trace)):NULL;
 if(trace_path&&!controller_trace){fprintf(stderr,"route probe cannot allocate controller trace\n");return 2;}
 while(!reached&&ticks<max_ticks){
  unsigned prior_waypoint=waypoint;
  while(waypoint<route_count){
   int wx=(int)(route[waypoint]%(unsigned)world.width),wy=(int)(route[waypoint]/(unsigned)world.width);
   float tx=wx*16.0f+8.0f,ty=wy*16.0f+8.0f;
   const WorldTile *waypoint_tile=world_tile(&world,wx,wy);
   int at_tile=fabsf(tx-(player.x+player.w*0.5f))<7.0f&&fabsf(ty-(player.y+player.h*0.5f))<9.0f;
   int at_platform=waypoint_tile&&waypoint_tile->platform&&fabsf((player.y+player.h)-wy*16.0f)<9.0f;
   if(at_tile||at_platform)waypoint++;
   else break;
  }
  if(waypoint!=prior_waypoint){waypoint_ticks=0;stuck=0;}
  else waypoint_ticks++;
  unsigned target=waypoint<route_count?route[waypoint]:route[route_count-1];
  int tx=(int)(target%(unsigned)world.width),ty=(int)(target/(unsigned)world.width);
  float px=player.x+player.w*0.5f,py=player.y+player.h*0.5f;
  if(stage_waypoint!=(int)waypoint){stage_x=-1;stage_y=-1;stage_waypoint=(int)waypoint;}
  if(stage_x>=0&&fabsf((stage_x*16.0f+8.0f)-px)<7.0f&&fabsf((stage_y*16.0f+8.0f)-py)<9.0f)stage_x=stage_y=-1;
  if(stage_x<0&&player.on_ground&&ty*16.0f+8.0f<py-36.0f){
   if(find_jump_stage(&world,tx,ty,px,py,&stage_x,&stage_y))stage_waypoint=(int)waypoint;
  }
  if(stage_x>=0){tx=stage_x;ty=stage_y;}
  target_x=tx;target_y=ty;
  float gx=tx*16.0f+8.0f,gy=ty*16.0f+8.0f;
  float dx=gx-px,dy=gy-py;
  unsigned input=choose_route_input(&player,&world,tx,ty);
  int blocked=(dx>0&&(player.contacts&WORLD_RIGHT))||(dx<0&&(player.contacts&WORLD_LEFT));
  if(player.on_ground&&blocked&&(input&(PLAYER_LEFT|PLAYER_RIGHT))){
   if(!(player.previous&PLAYER_JUMP)){input|=PLAYER_JUMP;jumps++;}
  }else if(!player.on_ground&&player.vy<0.0f){
   const WorldTile *goal_tile=world_tile(&world,tx,ty);
   if(!goal_tile||!goal_tile->platform||player.y+player.h>ty*16.0f+0.5f)input|=PLAYER_JUMP;
  }
  else if(player.on_ground&&dy>14.0f){
   const WorldTile *feet=world_tile(&world,(int)floorf(px/16.0f),(int)floorf((player.y+player.h+0.01f)/16.0f));
   if(feet&&feet->platform){input|=PLAYER_DOWN;if(!(player.previous&PLAYER_JUMP)){input|=PLAYER_JUMP;jumps++;}}
  }
  player_step(&player,&world,input);ticks++;
  if(controller_trace)controller_trace[ticks-1]=controller_buttons(input);
  if(player_overlaps_solid(&player,&world)||!isfinite(player.x)||!isfinite(player.y)){
   fprintf(stderr,"route probe detected invalid player state at tick=%u\n",ticks);return 3;
  }
  if(fabsf(player.x-last_x)<0.01f&&fabsf(player.y-last_y)<0.01f)stuck++;else stuck=0;
  last_x=player.x;last_y=player.y;
  if(stuck==120){
   fprintf(stderr,"route probe waiting tick=%u waypoint=%u/%u x=%.2f y=%.2f target=%d,%d ground=%d ladder=%d input=%u contacts=%u vx=%.2f vy=%.2f stage=%d,%d\n",ticks,waypoint,route_count,player.x,player.y,target_x,target_y,player.on_ground,player.on_ladder,player.previous,player.contacts,player.vx,player.vy,stage_x,stage_y);
  }
  if(stuck>360){
   fprintf(stderr,"route probe stuck tick=%u waypoint=%u/%u x=%.2f y=%.2f ground=%d hanging=%d contacts=%u vy=%.2f\n",ticks,waypoint,route_count,player.x,player.y,player.on_ground,player.hanging,player.contacts,player.vy);return 4;
  }
  if(waypoint_ticks>=waypoint_limit){
   float d=route_distance(&player,tx,ty);
   fprintf(stderr,"route probe waypoint timeout tick=%u waypoint=%u/%u target=%d,%d distance=%.2f player=%.2f,%.2f ground=%d ladder=%d hanging=%d contacts=%u input=%u vx=%.2f vy=%.2f\n",ticks,waypoint,route_count,tx,ty,d,player.x,player.y,player.on_ground,player.on_ladder,player.hanging,player.contacts,player.previous,player.vx,player.vy);return 5;
  }
  reached=world_goal_touched(&world,player.x,player.y,player.w,player.h);
 }
 printf("ACT_ONE_ROUTE seed=%lu tiles=%u waypoint=%u ticks=%u jumps=%u reached_exit=%d x=%.2f y=%.2f target=%d,%d ground=%d contacts=%u vx=%.2f vy=%.2f dead=%d motion=%s\n",
        seed,route_count,waypoint,ticks,jumps,reached,player.x,player.y,target_x,target_y,player.on_ground,player.contacts,player.vx,player.vy,player.dead,player_motion_name(player.motion));
 if(reached&&trace_path&&!write_controller_trace(trace_path,controller_trace,ticks)){
  fprintf(stderr,"route probe reached exit but could not write controller trace: %s\n",trace_path);free(controller_trace);return 6;
 }
 free(controller_trace);
 return reached?0:1;
}
