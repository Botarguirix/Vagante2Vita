#pragma once
#include <stdint.h>
#include <stdio.h>
typedef struct { const char *name; unsigned char *rgba; int w,h; } Texture;
#define TEXTURE_COUNT 22
extern Texture textures[TEXTURE_COUNT];
int load_resources(const char *path, FILE *log);
void free_resources(void);

int load_png_file(Texture *texture,const char *path,FILE *log);
void free_texture(Texture *texture);
