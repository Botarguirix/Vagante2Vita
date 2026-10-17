#include "world.h"
#include <string.h>

#define LOOT_UNIQUE 1u

static uint16_t read16(const unsigned char *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t read32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int read_exact(FILE *f,unsigned char *p,size_t n){return fread(p,1,n,f)==n;}
static LootPoolRange *range_for(LootPoolRange *ranges,unsigned count,uint8_t id){
 for(unsigned i=0;i<count;i++)if(ranges[i].id==id)return &ranges[i];
 return NULL;
}
int world_load_loot_from_file(World *w,FILE *f,FILE *log){
 if(!w||!f)return 0;
 unsigned char h[14],b[16];WorldLoot next;memset(&next,0,sizeof(next));
 if(!read_exact(f,h,sizeof(h))||memcmp(h,"VLT1",4)||read16(h+4)!=2)goto bad;
 unsigned direct_count=read16(h+6),row_count=read16(h+8),nested_count=read16(h+10),edge_count=read16(h+12);
 if(!direct_count||direct_count>WORLD_LOOT_POOL_LIMIT||row_count>WORLD_LOOT_ROW_LIMIT||
    !nested_count||nested_count>WORLD_LOOT_POOL_LIMIT||edge_count>WORLD_LOOT_EDGE_LIMIT)goto bad;
 next.direct_count=(uint8_t)direct_count;next.row_count=(uint16_t)row_count;
 next.nested_count=(uint8_t)nested_count;next.edge_count=(uint16_t)edge_count;
 for(unsigned i=0;i<direct_count;i++){
  if(!read_exact(f,b,6))goto bad;
  LootPoolRange *r=&next.direct[i];r->id=b[0];r->first=read16(b+2);r->count=read16(b+4);
  if(b[1]||r->id>=WORLD_LOOT_POOL_LIMIT||!r->count||r->first+r->count>row_count)goto bad;
  for(unsigned j=0;j<i;j++)if(next.direct[j].id==r->id)goto bad;
 }
 for(unsigned i=0;i<row_count;i++){
  if(!read_exact(f,b,sizeof(b)))goto bad;
  LootRarityRow *r=&next.rows[i];r->min_level=read32(b);r->max_level=read32(b+4);
  r->weight=read16(b+8);r->item_type=read16(b+10);r->flags=b[12];r->atlas=b[13];r->sprite=read16(b+14);
  if(r->min_level>r->max_level||r->max_level>99999||!r->weight||(r->flags&~LOOT_UNIQUE)||r->atlas>6)goto bad;
 }
 for(unsigned i=0;i<nested_count;i++){
  if(!read_exact(f,b,6))goto bad;
  LootPoolRange *r=&next.nested[i];r->id=b[0];r->first=read16(b+2);r->count=read16(b+4);
  if(b[1]||r->id>=WORLD_LOOT_POOL_LIMIT||!r->count||r->first+r->count>edge_count)goto bad;
  for(unsigned j=0;j<i;j++)if(next.nested[j].id==r->id)goto bad;
 }
 for(unsigned i=0;i<edge_count;i++){
  if(!read_exact(f,b,6))goto bad;
  next.edges[i].child=b[0];next.edges[i].weight_milli=read32(b+2);
  if(b[1]||next.edges[i].child>=WORLD_LOOT_POOL_LIMIT||!next.edges[i].weight_milli)goto bad;
 }
 if(fgetc(f)!=EOF||ferror(f)||!range_for(next.nested,next.nested_count,23))goto bad;
 for(unsigned i=0;i<edge_count;i++){
  unsigned child=next.edges[i].child;
  if(!range_for(next.direct,next.direct_count,(uint8_t)child)&&
     !range_for(next.nested,next.nested_count,(uint8_t)child))goto bad;
 }
 next.ready=1;w->loot=next;
 if(log)fprintf(log,"WORLD_LOOT ready=1 direct_pools=%u rarity_rows=%u nested_pools=%u edges=%u format=VLT1 version=2\n",
                direct_count,row_count,nested_count,edge_count);
 return 1;
bad:
	if(log)fprintf(log,"WORLD_LOOT ready=0 error=invalid_packet\n");
	return 0;
}
int world_load_loot(World *w,const char *path,FILE *log){
 if(!w||!path)return 0;
 FILE *f=fopen(path,"rb");if(!f){if(log)fprintf(log,"WORLD_LOOT ready=0 error=open path=%s\n",path);return 0;}
 int ok=world_load_loot_from_file(w,f,log);fclose(f);
 if(!ok&&log)fprintf(log,"WORLD_LOOT path=%s\n",path);
 return ok;
}

static uint32_t random32(WorldLoot *loot){
 uint32_t x=loot->rng_state;if(!x)x=0x9e3779b9u;
 x^=x<<13;x^=x>>17;x^=x<<5;loot->rng_state=x;return x;
}
static uint32_t bounded(WorldLoot *loot,uint32_t bound){
 if(!bound)return 0;
 uint32_t threshold=(uint32_t)(-bound)%bound,x;
 do{x=random32(loot);}while(x<threshold);
 return x%bound;
}
static int unique_spawned(const WorldLoot *loot,uint16_t type){
 for(unsigned i=0;i<loot->spawned_unique_count;i++)if(loot->spawned_unique[i]==type)return 1;
 return 0;
}
static LootPoolRange *find_range(LootPoolRange *ranges,unsigned count,uint8_t id){
 return range_for(ranges,count,id);
}
static int roll_pool(WorldLoot *loot,uint8_t id,uint32_t level,unsigned depth,
                     uint16_t *type,uint8_t *atlas,uint16_t *sprite){
 if(depth>WORLD_LOOT_POOL_LIMIT)return 0;
 LootPoolRange *nested=find_range(loot->nested,loot->nested_count,id);
 if(nested){
  uint8_t tried[WORLD_LOOT_EDGE_LIMIT];memset(tried,0,sizeof(tried));
  for(unsigned attempt=0;attempt<nested->count;attempt++){
   uint64_t sum=0;
   for(unsigned i=0;i<nested->count;i++)if(!tried[i])sum+=loot->edges[nested->first+i].weight_milli;
   if(!sum||sum>0xffffffffu)return 0;
   uint32_t choice=bounded(loot,(uint32_t)sum);unsigned selected=0;
   for(;selected<nested->count;selected++)if(!tried[selected]){
    uint32_t weight=loot->edges[nested->first+selected].weight_milli;
		if(choice<weight)break;
		choice-=weight;
   }
   if(selected>=nested->count)return 0;
   uint8_t child=loot->edges[nested->first+selected].child;
   if(roll_pool(loot,child,level,depth+1,type,atlas,sprite))return 1;
   tried[selected]=1;
  }
  return 0;
 }
 LootPoolRange *direct=find_range(loot->direct,loot->direct_count,id);
 if(!direct)return 0;
 uint64_t sum=0;
 for(unsigned i=0;i<direct->count;i++){
  LootRarityRow *r=&loot->rows[direct->first+i];
  if(r->min_level<=level&&level<=r->max_level&&(!(r->flags&LOOT_UNIQUE)||!unique_spawned(loot,r->item_type)))sum+=r->weight;
 }
 if(!sum||sum>0xffffffffu)return 0;
 uint32_t choice=bounded(loot,(uint32_t)sum);
 for(unsigned i=0;i<direct->count;i++){
  LootRarityRow *r=&loot->rows[direct->first+i];
  if(r->min_level>level||level>r->max_level||((r->flags&LOOT_UNIQUE)&&unique_spawned(loot,r->item_type)))continue;
  if(choice<r->weight){*type=r->item_type;*atlas=r->atlas;*sprite=r->sprite;return 1;}
  choice-=r->weight;
 }
 return 0;
}
int world_spawn_chest_loot(World *w,WorldEntity *chest,int level,uint32_t seed,uint16_t *item_type){
 if(!w||!chest||!w->loot.ready||w->entity_count>=WORLD_ENTITY_LIMIT)return 0;
 w->loot.rng_state=seed?seed:0x6d2b79f5u;
 uint16_t type=0,sprite=0xffff;uint8_t atlas=0;
 if(!roll_pool(&w->loot,23,(uint32_t)level,0,&type,&atlas,&sprite))return 0;
 WorldEntity drop;memset(&drop,0,sizeof(drop));drop.kind=25;drop.item_type=type;drop.item_atlas=atlas;drop.item_sprite=sprite;
 drop.x_q8=chest->x_q8+4*256;drop.y_q8=chest->y_q8-12*256;
 drop.x=(int16_t)(drop.x_q8/256);drop.y=(int16_t)(drop.y_q8/256);drop.source_index=chest->source_index;
 w->entities[w->entity_count++]=drop;
 for(unsigned i=0;i<w->loot.row_count;i++)if(w->loot.rows[i].item_type==type&&
     (w->loot.rows[i].flags&LOOT_UNIQUE)&&!unique_spawned(&w->loot,type)){
  if(w->loot.spawned_unique_count<32)w->loot.spawned_unique[w->loot.spawned_unique_count++]=type;
  break;
 }
 if(item_type)*item_type=type;
 return 1;
}
