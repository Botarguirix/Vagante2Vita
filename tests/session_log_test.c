#include "session_log.h"
#include <assert.h>
#include <string.h>
int main(void){
 char first[256],second[256],content[32]={0};
 FILE *a=open_session_log("session-log-fixtures",first,sizeof(first));assert(a);
 fputs("original session",a);fclose(a);
 FILE *b=open_session_log("session-log-fixtures",second,sizeof(second));assert(b);
 assert(strcmp(first,second));fputs("new session",b);fclose(b);
 a=fopen(first,"r");assert(a);assert(fread(content,1,16,a)==16);fclose(a);
 assert(!strcmp(content,"original session"));
 assert(!open_session_log("session-log-fixtures",second,2));
 return 0;
}
