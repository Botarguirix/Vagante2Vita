#pragma once
#include <stdio.h>
#define REPLAY_LIMIT 2048
typedef struct {unsigned buttons,ticks;} ReplayStep;
typedef struct {ReplayStep steps[REPLAY_LIMIT];unsigned count,index,elapsed,active,done,start_intro,stop_after_first_floor,fast_forward;} InputReplay;
int replay_load(InputReplay *r,const char *path,FILE *log);
unsigned replay_buttons(const InputReplay *r);
void replay_advance(InputReplay *r,unsigned ticks);
