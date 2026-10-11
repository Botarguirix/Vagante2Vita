#pragma once
#include <stdint.h>
#include <stdio.h>
typedef struct { const char *name; unsigned char *rgba; int w,h; } Texture;
extern Texture textures[10];
int load_resources(const char *path, FILE *log);
void free_resources(void);
