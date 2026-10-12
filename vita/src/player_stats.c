/* Native base initialization, recovered from PlayerCustomizer and updateStats.
   No background, item bonuses, upgraded affinity or weapon damage inferred. */
#include "player_stats.h"
#include <math.h>
#include <string.h>
void player_derive_stats(const StartingStats *s,DerivedStats *d){
 memset(d,0,sizeof(*d));
 d->max_hp=(int)fmaxf(1.0f,50.0f+10.0f*(float)s->vitality);
 d->move_speed=1.4f+0.1f*(float)s->speed;
 d->melee_base=1.0f+fmaxf(0.0f,0.5f*s->strength+0.25f*s->dexterity);
 d->ranged_base=fmaxf(0.0f,0.5f*s->dexterity+0.25f*s->strength);
 d->attack_speed_bonus=0.0025f*s->speed+0.01f*s->dexterity;
 d->damage_taken_multiplier=fmaxf(0.0f,1.0f-0.1f*s->stamina);
 if(s->luck>=0){
  float common=log1pf((float)s->luck)/log1pf(0.18f)*0.01f;
  d->evasion=fminf(1.0f/3.0f,common+0.005f*s->luck);
  d->critical=fminf(0.5f,common+0.0075f*s->luck);
 }
 d->max_air=80.0f+4.0f*s->vitality;
}
int player_starting_loadout(int cls,StartingLoadout *out){
 if(cls<0||cls>=3)return 0;
 StartingLoadout next;memset(&next,0,sizeof(next));
 if(cls==0){next.items[0]=(StartingItem){49,1,START_MELEE};next.item_count=1;}
 else if(cls==1){
  next.items[0]=(StartingItem){45,1,START_MELEE};next.items[1]=(StartingItem){93,1,START_RANGED};
  next.items[2]=(StartingItem){3,30,START_AMMO};next.item_count=3;
 }else{
  next.items[0]=(StartingItem){79,1,START_MELEE};next.item_count=1;
  next.skills[0]=11;next.skills[1]=10;next.skill_count=2;
 }
 *out=next;return 1;
}
const char *starting_item_name(int type){
 switch(type){case 49:return "SWORD";case 45:return "DAGGER";case 93:return "BOW";case 79:return "ROD";case 3:return "ARROW";default:return "UNKNOWN";}
}
const char *starting_skill_name(int type){return type==11?"FROST NOVA":type==10?"ELEC LANCE":"UNKNOWN";}
