#include "player_stats.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void close_to(float a,float b){assert(fabsf(a-b)<0.0001f);}
int main(void){
 const int hp[]={100,80,60};const float melee[]={3,2.5f,1.5f},ranged[]={1.75f,2.25f,1};
 for(int cls=0;cls<3;cls++){
  StartingStats s;DerivedStats d;StartingLoadout l;
  assert(starting_stats(cls,&s));player_derive_stats(&s,&d);assert(player_starting_loadout(cls,&l));
  assert(d.max_hp==hp[cls]);close_to(d.move_speed,1.5f);close_to(d.melee_base,melee[cls]);close_to(d.ranged_base,ranged[cls]);
  close_to(d.max_air,cls==0?100:cls==1?92:84);close_to(d.damage_taken_multiplier,1);
  assert(l.item_count==(cls==1?3u:1u));assert(l.items[0].type==(cls==0?49:cls==1?45:79));
  if(cls==1){assert(l.items[1].type==93&&l.items[2].type==3&&l.items[2].quantity==30);assert(l.skill_count==0);assert(d.evasion>0.046f&&d.evasion<0.048f);assert(d.critical>d.evasion);}
  else {close_to(d.evasion,0);close_to(d.critical,0);}
  if(cls==2){assert(l.skill_count==2&&l.skills[0]==11&&l.skills[1]==10);}
 }
 StartingLoadout before,after;memset(&before,0xa5,sizeof(before));after=before;
 assert(!player_starting_loadout(-1,&after)&&!memcmp(&before,&after,sizeof(after)));
 assert(!player_starting_loadout(3,&after)&&!memcmp(&before,&after,sizeof(after)));
 StartingStats negative={-100,-100,-10,0,20,0,-1};DerivedStats d;player_derive_stats(&negative,&d);
 assert(d.max_hp==1);close_to(d.melee_base,1);close_to(d.ranged_base,0);close_to(d.damage_taken_multiplier,0);close_to(d.evasion,0);close_to(d.critical,0);
 StartingStats lucky={0,0,0,0,0,0,1000};player_derive_stats(&lucky,&d);close_to(d.evasion,1.0f/3);close_to(d.critical,0.5f);
 return 0;
}
