#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "win32.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
_Thread_local GuestThread guest_thread_state;
void vagante_glTexImage2D(Cpu *,GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid *);
void vagante_glDeleteTextures(GLsizei,const GLuint *);
void vagante_glGetTexImage(Cpu *,GLenum,GLint,GLenum,GLenum,GLvoid *);
static jmp_buf fail;
_Noreturn void guest_fault(Cpu *c,uint32_t address,const char *what) {
    (void)c;(void)address;(void)what; longjmp(fail,1);
}
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_Window *window=SDL_CreateWindow("Vita GL readback test",0,0,32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    assert(window); SDL_GLContext ctx=SDL_GL_CreateContext(window); assert(ctx);
    PFNGLGENFRAMEBUFFERSPROC gen=(void*)SDL_GL_GetProcAddress("glGenFramebuffers");
    PFNGLBINDFRAMEBUFFERPROC bind=(void*)SDL_GL_GetProcAddress("glBindFramebuffer");
    PFNGLDELETEFRAMEBUFFERSPROC del=(void*)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    assert(gen && bind && del);
    GLuint old; gen(1,&old); bind(GL_FRAMEBUFFER,old);
    GLuint texture; glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    Cpu c={0};
    unsigned char input[24]={1,2,3,255, 4,5,6,255, 7,8,9,255, 10,11,12,255, 13,14,15,255, 16,17,18,255};
    unsigned char reference[24], result[26]; memset(result,0xcd,sizeof result);
    vagante_glTexImage2D(&c,GL_TEXTURE_2D,0,GL_RGBA,3,2,0,GL_RGBA,GL_UNSIGNED_BYTE,input);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,reference);
    vagante_glGetTexImage(&c,GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,result+1);
    assert(!memcmp(result+1,reference,24) && !memcmp(result+1,input,24));
    assert(result[0]==0xcd && result[25]==0xcd);
    GLint restored; glGetIntegerv(GL_FRAMEBUFFER_BINDING,&restored); assert(restored==(GLint)old);
    /* Resize metadata must follow glTexImage2D; no stale larger readback. */
    vagante_glTexImage2D(&c,GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,input);
    memset(result,0xcd,sizeof result);
    vagante_glGetTexImage(&c,GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,result);
    assert(!memcmp(result,input,4) && result[4]==0xcd);
    if (!setjmp(fail)) { vagante_glGetTexImage(&c,GL_TEXTURE_2D,1,GL_RGBA,GL_UNSIGNED_BYTE,result); assert(0); }
    vagante_glDeleteTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    if (!setjmp(fail)) { vagante_glGetTexImage(&c,GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,result); assert(0); }
    glDeleteTextures(1,&texture); bind(GL_FRAMEBUFFER,0); del(1,&old);
    assert(glGetError()==GL_NO_ERROR);
    SDL_GL_DeleteContext(ctx); SDL_DestroyWindow(window); SDL_Quit();
    puts("vita GL compatibility: real FBO readback, resize, binding restore, bounds and failure paths OK");
}
