/* Real OpenGL context, kept hidden: checks ABI and guest-memory round trips. */
#include "win32.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern const ApiEntry win32_gl[];
uint8_t *guest_base;
static uint32_t top = 0x7000;
uint32_t guest_strdup(const char *s) {
    uint32_t p = top; top += (uint32_t)strlen(s) + 1;
    assert(top < 0x10000); strcpy(STR(p), s); return p;
}
static uint32_t call(const char *name, unsigned words) {
    const ApiEntry *e = win32_gl;
    while (e->name && strcmp(e->name, name)) ++e;
    assert(e->name); Cpu c = {0}; c.esp.d = 0x100;
    e->fn(&c); assert(c.esp.d == 0x104 + 4*words); return c.eax.d;
}
#define A(n, v) wr32(0x104 + (n)*4, v)
static void af(unsigned n, float v) { memcpy(G(0x104+n*4), &v, 4); }
static void ad(unsigned n, double v) { memcpy(G(0x104+n*4), &v, 8); }
int main(void) {
    guest_base = calloc(1, 0x10000); assert(guest_base);
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_Window *window = SDL_CreateWindow("Host GL test", 0, 0, 32, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window); SDL_GLContext ctx = SDL_GL_CreateContext(window); assert(ctx);
    A(0, GL_VERSION); uint32_t version = call("glGetString", 1); assert(version >= 0x7000);
    assert(!strcmp(STR(version), (const char *)glGetString(GL_VERSION)));
    uint32_t used = top; assert(call("glGetString", 1) == version && used == top);
    A(0, 0xffffffff); assert(call("glGetString", 1) == 0); call("glGetError", 0);
    A(0, 2); A(1, 3); A(2, 16); A(3, 18); call("glViewport", 4);
    A(0, GL_VIEWPORT); A(1, 0x1000); call("glGetIntegerv", 2);
    assert(rd32(0x1000) == 2 && rd32(0x1004) == 3 && rd32(0x1008) == 16 && rd32(0x100c) == 18);
    A(0, GL_BLEND); call("glEnable", 1); assert(call("glIsEnabled", 1)); call("glDisable", 1); assert(!call("glIsEnabled", 1));
    A(0, GL_PROJECTION); call("glMatrixMode", 1); call("glLoadIdentity", 0);
    ad(0, -2.0); ad(2, 6.0); ad(4, -4.0); ad(6, 4.0); ad(8, -1.0); ad(10, 3.0); call("glOrtho", 12);
    double matrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, matrix);
    assert(matrix[0] == .25 && matrix[5] == .25 && matrix[10] == -.5 && matrix[12] == -.5 && matrix[14] == -.5);
    A(0, GL_MODELVIEW); call("glMatrixMode", 1); call("glLoadIdentity", 0);
    float identity[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; memcpy(G(0x1800), identity, sizeof identity);
    A(0, 0x1800); call("glLoadMatrixf", 1);
    A(0, 1); A(1, 0x2000); call("glGenTextures", 2); uint32_t texture = rd32(0x2000); assert(texture);
    A(0, GL_TEXTURE_2D); A(1, texture); call("glBindTexture", 2);
    const unsigned char pixels[16] = {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    memcpy(G(0x2100), pixels, 16);
    A(0, GL_TEXTURE_2D); A(1, 0); A(2, GL_RGBA); A(3, 2); A(4, 2); A(5, 0); A(6, GL_RGBA); A(7, GL_UNSIGNED_BYTE); A(8, 0x2100); call("glTexImage2D", 9);
    A(0, GL_TEXTURE_2D); A(1, 0); A(2, GL_RGBA); A(3, GL_UNSIGNED_BYTE); A(4, 0x2200); call("glGetTexImage", 5);
    assert(!memcmp(G(0x2200), pixels, 16));
    memset(G(0x2300), 77, 4);
    A(0, GL_TEXTURE_2D); A(1, 0); A(2, 1); A(3, 0); A(4, 1); A(5, 1); A(6, GL_RGBA); A(7, GL_UNSIGNED_BYTE); A(8, 0x2300); call("glTexSubImage2D", 9);
    A(0, GL_TEXTURE_2D); A(1, 0); A(2, GL_RGBA); A(3, GL_UNSIGNED_BYTE); A(4, 0x2200); call("glGetTexImage", 5);
    assert(rd32(0x2204) == 0x4d4d4d4d);
    float vertices[6] = {-1,-1, 1,-1, 0,1}; memcpy(G(0x3000), vertices, sizeof vertices);
    A(0, 2); A(1, GL_FLOAT); A(2, 0); A(3, 0x3000); call("glVertexPointer", 4);
    void *pointer; glGetPointerv(GL_VERTEX_ARRAY_POINTER, &pointer); assert(pointer == G(0x3000));
    A(0, 4); A(1, GL_UNSIGNED_BYTE); A(2, 0); A(3, 0x2100); call("glColorPointer", 4);
    glGetPointerv(GL_COLOR_ARRAY_POINTER, &pointer); assert(pointer == G(0x2100));
    A(0, 2); A(1, GL_FLOAT); A(2, 0); A(3, 0x3000); call("glTexCoordPointer", 4);
    glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &pointer); assert(pointer == G(0x3000));
    A(0, GL_VERTEX_ARRAY); call("glEnableClientState", 1);
    wr8(0x3100, 0); wr8(0x3101, 1); wr8(0x3102, 2);
    A(0, GL_TRIANGLES); A(1, 3); A(2, GL_UNSIGNED_BYTE); A(3, 0x3100); call("glDrawElements", 4);
    A(0, GL_TRIANGLES); A(1, 0); A(2, 3); call("glDrawArrays", 3);
    af(0, .25f); af(1, .5f); af(2, .75f); af(3, 1.f); call("glClearColor", 4);
    float color[4]; glGetFloatv(GL_COLOR_CLEAR_VALUE, color); assert(color[0] == .25f && color[1] == .5f && color[2] == .75f);
    A(0, GL_COLOR_BUFFER_BIT); call("glClear", 1); call("glFlush", 0);
    assert(call("glGetError", 0) == GL_NO_ERROR);
    A(0, 1); A(1, 0x2000); call("glDeleteTextures", 2); assert(!glIsTexture(texture));
    printf("gl: ABI, doubles, strings, guest arrays and texture round trips OK (%s)\n", STR(version));
    SDL_GL_DeleteContext(ctx); SDL_DestroyWindow(window); SDL_Quit(); free(guest_base);
}
