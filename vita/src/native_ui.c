#include "native_ui.h"
#include <stdio.h>
#include <stddef.h>
/* Five-column uppercase diagnostic font, seven bits per column. */
static const uint8_t glyphs[36][5]={
 {126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {7,8,112,8,7},{97,81,73,69,67},
 {62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
 {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
 {54,73,73,73,54},{6,73,73,41,30}
};
void native_text(uint32_t *frame,int x,int y,const char *s,uint32_t color){
    for(;*s;s++,x+=18){
        int g=(*s>='A'&&*s<='Z')?*s-'A':((*s>='0'&&*s<='9')?26+*s-'0':-1);
        static const uint8_t dot[5]={0,96,96,0,0},minus[5]={8,8,8,8,8};
        const uint8_t *shape=g>=0?glyphs[g]:*s=='.'?dot:*s=='-'?minus:NULL;
        if(!shape)continue;
        for(int c=0;c<5;c++)for(int r=0;r<7;r++)if(shape[c]&(1<<r))
            for(int dy=0;dy<3;dy++)for(int dx=0;dx<3;dx++){
                int px=x+c*3+dx,py=y+r*3+dy;
                if(px>=0&&px<960&&py>=0&&py<544)frame[py*1024+px]=color;
            }
    }
}

void draw_character_panel(uint32_t *frame,const NativePlayer *player,int page){

     for(int y=112;y<453;y++)for(int x=90;x<870;x++)frame[y*1024+x]=0xff20170f;
     native_text(frame,120,130,"CHARACTER",0xff80ddff);char line[64];
     if(page==0){
      native_text(frame,120,168,"BASE STATS",0xffffffff);
      snprintf(line,sizeof(line),"MELEE BASE %.2f RANGED BASE %.2f",player->derived.melee_base,player->derived.ranged_base);native_text(frame,120,207,line,0xffffffff);
      snprintf(line,sizeof(line),"ATTACK SPEED BONUS %.2f PCT",player->derived.attack_speed_bonus*100);native_text(frame,120,244,line,0xffffffff);
      snprintf(line,sizeof(line),"EVASION %.2f PCT CRITICAL %.2f PCT",player->derived.evasion*100,player->derived.critical*100);native_text(frame,120,281,line,0xffffffff);
      snprintf(line,sizeof(line),"AIR %.0f DAMAGE TAKEN %.2f",player->derived.max_air,player->derived.damage_taken_multiplier);native_text(frame,120,318,line,0xffffffff);
     }else if(page==1){
      native_text(frame,120,168,"STARTING EQUIPMENT",0xffffffff);
      const char *roles[]={"MELEE","RANGED","AMMO"};
      for(unsigned n=0;n<player->loadout.item_count;n++){
       const StartingItem *item=&player->loadout.items[n];snprintf(line,sizeof(line),"%s %s X %d",roles[item->role],starting_item_name(item->type),item->quantity);native_text(frame,120,207+38*n,line,0xffffffff);
      }
     }else{
      native_text(frame,120,168,"STARTING SKILLS",0xffffffff);
      if(!player->loadout.skill_count)native_text(frame,120,207,"NONE",0xffffffff);
      for(unsigned n=0;n<player->loadout.skill_count;n++)native_text(frame,120,207+38*n,starting_skill_name(player->loadout.skills[n]),0xffffffff);
     }
     native_text(frame,120,396,"LEFT RIGHT PAGES CIRCLE CLOSE",0xffaaaaaa);
    
}
