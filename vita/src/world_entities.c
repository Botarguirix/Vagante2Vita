#include "world.h"
#include <math.h>
#include <string.h>
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int32_t i32(const unsigned char *p){return (int32_t)u32(p);}
static float f32(const unsigned char *p){uint32_t bits=u32(p);float value;memcpy(&value,&bits,sizeof(value));return value;}
int world_load_entities(World *w,const char *path,FILE *log){
 unsigned char h[8],r[16];WorldEntity next[WORLD_ENTITY_LIMIT];
 FILE *f=fopen(path,"rb");if(!f){if(log)fprintf(log,"WORLD_ENTITIES ready=0 error=open path=%s\n",path);return 0;}
 if(!w->ready||fread(h,1,8,f)!=8)goto bad;
 int version=!memcmp(h,"VWE1",4)?1:!memcmp(h,"VWE2",4)?2:0;
 unsigned count=u16(h+4);if(!version||count>WORLD_ENTITY_LIMIT||(version==1&&u16(h+6))||(version==2&&u16(h+6)!=16))goto bad;
 for(unsigned i=0;i<count;i++){
  if(fread(r,1,version==1?8:16,f)!=(version==1?8:16))goto bad;
  if(version==1){
   unsigned x=u16(r+2),y=u16(r+4);
   if(u16(r+6)||x>=(unsigned)w->width*16||y>=(unsigned)w->height*16)goto bad;
   if(r[0]==1){if(r[1]!=3&&r[1]!=11&&r[1]!=12&&r[1]!=13&&r[1]!=15)goto bad;}
   else if(r[0]!=2||r[1]!=0)goto bad;
   next[i]=(WorldEntity){.kind=r[0],.subtype=r[1],.x=(int16_t)x,.y=(int16_t)y,.x_q8=(int32_t)(x<<8),.y_q8=(int32_t)(y<<8),.source_index=i};
  }else{
   unsigned kind=r[0],flags=r[1],variant=u16(r+2);int32_t x=i32(r+4),y=i32(r+8);uint32_t source=u32(r+12);
   if(kind<16||kind>23||x<0||y<0||x>=(int32_t)w->width*16*256||y>=(int32_t)w->height*16*256)goto bad;
   if((kind==16&&variant>1)||(kind==17&&variant!=0)||(kind==18&&variant!=0)||(kind==19&&(variant<1||variant>21))||(kind>=20&&variant!=0))goto bad;
   if((kind==16&&(flags&~WORLD_ENTITY_OPENED))||
      (kind==18&&(flags&~(WORLD_ENTITY_FACING_LEFT|WORLD_ENTITY_CAN_RELOAD)))||
      (kind==23&&(flags&~WORLD_ENTITY_FACING_LEFT))||
      (kind!=16&&kind!=18&&kind!=23&&flags))goto bad;
   next[i]=(WorldEntity){.kind=(uint8_t)kind,.subtype=0,.x=(int16_t)(x>>8),.y=(int16_t)(y>>8),.flags=(uint8_t)flags,.variant=(uint16_t)variant,.x_q8=x,.y_q8=y,.source_index=source,
      .hp=kind==21?14.0f:0.0f,
      .boulder_plate_down=(uint8_t)(kind==20)};
   /* BoulderTrap::setLevel checks its center tile, then scans upward to the
      first solid cell before constructing the linked square Boulder there. */
   if(kind==20){
    int trap_x=(x>>8)+8,trap_y=(y>>8)+8;
    int tile_x=trap_x/WORLD_TILE_SIZE,tile_y=trap_y/WORLD_TILE_SIZE;
    const WorldTile *anchor=world_tile(w,tile_x,tile_y);
    if(anchor&&anchor->type!=11&&!anchor->solid){
     while(tile_y>=0&&!world_solid(w,tile_x,tile_y))tile_y--;
     if(tile_y>=0){
      next[i].boulder_present=1;
      next[i].boulder_x_q8=(x>>8)*256;
      next[i].boulder_y_q8=tile_y*WORLD_TILE_SIZE*256;
     }
    }
   }
   /* Room files omit a Boulder subtype. Reconstruct the default shake type
      and its constructor's 120-tick release timer. */
   if(kind==22){
    next[i].boulder_present=1;
    next[i].boulder_x_q8=x;
    next[i].boulder_y_q8=y;
    next[i].cooldown=120;
   }
  }
 }
 if(fgetc(f)!=EOF||ferror(f))goto bad;
 fclose(f);
 unsigned kept=0;
 for(unsigned i=0;i<w->entity_count;i++)kept+=w->entities[i].kind==24;
 if(kept+count>WORLD_ENTITY_LIMIT)goto limit_closed;
 kept=0;
 for(unsigned i=0;i<w->entity_count;i++)if(w->entities[i].kind==24)next[count+kept++]=w->entities[i];
 memcpy(w->entities,next,(kept+count)*sizeof(*next));w->entity_count=kept+count;
 unsigned boulders=0;
 if(version==2)for(unsigned i=0;i<count;i++){
  boulders+=next[i].boulder_present!=0;
 }
 if(log)fprintf(log,"WORLD_ENTITIES ready=1 format=VWE%d count=%u boulders=%u behavior_pending=0\n",version,count,boulders);
 return 1;
limit_closed:if(log)fprintf(log,"WORLD_ENTITIES ready=0 error=capacity path=%s\n",path);return 0;
 bad:fclose(f);if(log)fprintf(log,"WORLD_ENTITIES ready=0 error=invalid_packet path=%s\n",path);return 0;
}
int world_load_enemies(World *w,const char *path,FILE *log){
 unsigned char h[8],r[16];WorldEntity next[WORLD_ENTITY_LIMIT];FILE *f=fopen(path,"rb");
 if(!f){if(log)fprintf(log,"WORLD_ENEMIES ready=0 error=open\n");return 0;}
 if(fread(h,1,8,f)!=8||memcmp(h,"VWE3",4)||u16(h+6)!=16||u16(h+4)>64)goto bad;
 unsigned count=u16(h+4);
 for(unsigned i=0;i<count;i++){
  if(fread(r,1,16,f)!=16||r[0]!=24||
     !((r[1]>=9&&r[1]<=12)||r[1]==14)||u16(r+2))goto bad;
  int32_t x=i32(r+4),y=i32(r+8);if(x<0||y<0||x>=(int32_t)w->width*16*256||y>=(int32_t)w->height*16*256)goto bad;
  next[i]=(WorldEntity){.kind=24,.subtype=r[1],.x=(int16_t)(x>>8),.y=(int16_t)(y>>8),.x_q8=x,.y_q8=y,.source_index=u32(r+12),.hp=2.0f};
 }
 if(fgetc(f)!=EOF)goto bad;
 fclose(f);
 unsigned kept=0;
 for(unsigned i=0;i<w->entity_count;i++)kept+=w->entities[i].kind!=24;
 if(kept+count>WORLD_ENTITY_LIMIT)goto limit_closed;
 kept=0;
 for(unsigned i=0;i<w->entity_count;i++)if(w->entities[i].kind!=24)next[count+kept++]=w->entities[i];
 memcpy(w->entities,next,(kept+count)*sizeof(*next));w->entity_count=kept+count;
 if(log)fprintf(log,"WORLD_ENEMIES ready=1 count=%u parity=provisional\n",count);
 return 1;
bad:fclose(f);if(log)fprintf(log,"WORLD_ENEMIES ready=0 error=invalid_packet\n");return 0;
limit_closed:if(log)fprintf(log,"WORLD_ENEMIES ready=0 error=capacity\n");return 0;
}
int world_load_goblin_attack(World *w,const char *path,FILE *log){
 unsigned char h[16],r[24],v[8];GoblinAttackData next={0};FILE *f=fopen(path,"rb");
 if(!f){if(log)fprintf(log,"WORLD_GOBLIN_ATTACK ready=0 error=open\n");return 0;}
 if(fread(h,1,sizeof(h),f)!=sizeof(h)||memcmp(h,"VGA1",4)||u16(h+4)!=1||
    u16(h+6)!=5||u16(h+8)!=16||u16(h+10)!=40||u16(h+12)!=5||u16(h+14)!=9)goto bad;
 next.frame_count=5;next.windup=u16(h+8);next.cooldown=u16(h+10);
 next.damage_min=u16(h+12);next.damage_max=u16(h+14);
 static const uint16_t durations[5]={9,1,1,6,1};
 for(unsigned i=0;i<next.frame_count;i++){
  GoblinAttackFrame *frame=&next.frames[i];
  if(fread(r,1,sizeof(r),f)!=sizeof(r))goto bad;
  frame->duration=u16(r);frame->enabled=r[2];frame->vertex_count=r[3];
  frame->origin_x=f32(r+4);frame->origin_y=f32(r+8);frame->origin_degrees=f32(r+12);
  frame->damage_min=f32(r+16);frame->damage_max=f32(r+20);
  if(frame->duration!=durations[i]||frame->enabled>1||frame->vertex_count<3||
     frame->vertex_count>WORLD_GOBLIN_ATTACK_VERTICES||!isfinite(frame->origin_x)||
     !isfinite(frame->origin_y)||!isfinite(frame->origin_degrees)||
     !isfinite(frame->damage_min)||!isfinite(frame->damage_max)||
     frame->damage_min<0||frame->damage_max<frame->damage_min||frame->damage_max>1)goto bad;
  for(unsigned j=0;j<frame->vertex_count;j++){
   if(fread(v,1,sizeof(v),f)!=sizeof(v))goto bad;
   frame->vertices[j]=(GoblinAttackPoint){f32(v),f32(v+4)};
   if(!isfinite(frame->vertices[j].x)||!isfinite(frame->vertices[j].y)||
      frame->vertices[j].x< -16||frame->vertices[j].x>16||
      frame->vertices[j].y< -16||frame->vertices[j].y>16)goto bad;
  }
 }
 if(fgetc(f)!=EOF||ferror(f))goto bad;
 fclose(f);next.ready=1;w->goblin_attack=next;
 if(log)fprintf(log,"WORLD_GOBLIN_ATTACK ready=1 windup=%u cooldown=%u damage=%u..%u frames=%u\n",
                next.windup,next.cooldown,next.damage_min,next.damage_max,next.frame_count);
 return 1;
bad:fclose(f);if(log)fprintf(log,"WORLD_GOBLIN_ATTACK ready=0 error=invalid_packet\n");return 0;
}
