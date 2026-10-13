#include "session_log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
FILE *open_session_log(const char *directory,char *path,size_t capacity){
 if(mkdir(directory,0777)<0&&errno!=EEXIST)return NULL;
 for(unsigned session=1;session<=999999;session++){
  int n=snprintf(path,capacity,"%s/session-%06u.log",directory,session);
  if(n<0||(size_t)n>=capacity)return NULL;
  /* Some emulator I/O backends ignore O_EXCL. Keep the atomic open flag on
     hardware, and skip existing names explicitly before opening. */
  if(access(path,F_OK)==0)continue;
  if(errno!=ENOENT)return NULL;
  int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0666);
  if(fd<0){if(errno==EEXIST)continue;return NULL;}
  FILE *log=fdopen(fd,"w");if(!log){close(fd);return NULL;}
  return log;
 }
 return NULL;
}
