#include "menu.h"
#include <assert.h>

int main(void){
 Menu title={0};
 menu_input(&title,INPUT_UP);assert(title.cursor==0);
 menu_input(&title,0);menu_input(&title,INPUT_DOWN);assert(title.cursor==1);
 menu_input(&title,INPUT_DOWN);assert(title.cursor==1);
 menu_input(&title,0);menu_input(&title,INPUT_UP);menu_input(&title,0);
 menu_input(&title,INPUT_ACCEPT);assert(title.screen==MENU_FADE);
 for(int i=0;i<89;i++)menu_tick(&title);assert(title.screen==MENU_FADE);
 menu_tick(&title);assert(title.screen==MENU_CLASS);

 /* Circle remains a back action on the class screen. */
 Menu class_menu={.screen=MENU_CLASS};
 menu_input(&class_menu,INPUT_BACK);assert(class_menu.screen==MENU_TITLE);

 /* Confirm opens the existing data notice; Circle returns to class. */
 class_menu=(Menu){.screen=MENU_CLASS};
 menu_input(&class_menu,INPUT_ACCEPT);assert(class_menu.screen==MENU_BLOCKED);
 menu_input(&class_menu,0);menu_input(&class_menu,INPUT_BACK);
 assert(class_menu.screen==MENU_CLASS);
 menu_input(&class_menu,0);menu_input(&class_menu,INPUT_LEFT);
 assert(class_menu.player_class==2);
 menu_input(&class_menu,0);menu_input(&class_menu,INPUT_RIGHT);
 assert(class_menu.player_class==0);
 menu_input(&class_menu,0);menu_input(&class_menu,INPUT_BACK);
 assert(class_menu.screen==MENU_TITLE);

 Menu complete={.screen=MENU_COMPLETE};
 assert(menu_screen_name(complete.screen)[0]=='A');
 menu_input(&complete,INPUT_ACCEPT);assert(complete.screen==MENU_CLASS);
 menu_input(&complete,0);menu_input(&complete,INPUT_BACK);
 assert(complete.screen==MENU_TITLE);
 return 0;
}
