/* vitaGL lacks desktop texture readback. SFML copyToImage uses level-0 RGBA8;
   attach that texture to a temporary FBO, read it, and restore the old binding. */
#include "win32.h"
#ifdef VAGANTE_GL_COMPAT_TEST
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#define glGenFramebuffers ((PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers"))
#define glBindFramebuffer ((PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer"))
#define glFramebufferTexture2D ((PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D"))
#define glCheckFramebufferStatus ((PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus"))
#define glDeleteFramebuffers ((PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers"))
#else
#include <vitaGL.h>
#endif
#include <stdlib.h>
struct TextureSize { GLuint name; GLsizei width, height; struct TextureSize *next; };
static struct TextureSize *sizes;
void vagante_glTexImage2D(Cpu *c, GLenum target, GLint level, GLint internal, GLsizei width, GLsizei height,
                         GLint border, GLenum format, GLenum type, const GLvoid *data) {
    glTexImage2D(target, level, internal, width, height, border, format, type, data);
    if (target != GL_TEXTURE_2D || level != 0 || width <= 0 || height <= 0) return;
    GLint name = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &name);
    struct TextureSize *p = sizes;
    for (; p && p->name != (GLuint)name; p = p->next) {}
    if (!p) {
        p = malloc(sizeof *p);
        if (!p) guest_fault(c, guest_current_target, "Vita texture metadata allocation failed");
        p->name = name; p->next = sizes; sizes = p;
    }
    p->width = width; p->height = height;
}
void vagante_glDeleteTextures(GLsizei n, const GLuint *textures) {
    for (GLsizei i=0; i<n; ++i) {
        struct TextureSize **link = &sizes;
        while (*link && (*link)->name != textures[i]) link = &(*link)->next;
        if (*link) { struct TextureSize *p = *link; *link = p->next; free(p); }
    }
    glDeleteTextures(n, textures);
}
void vagante_glGetTexImage(Cpu *c, GLenum target, GLint level, GLenum format, GLenum type, GLvoid *data) {
    if (target != GL_TEXTURE_2D || level != 0 || format != GL_RGBA || type != GL_UNSIGNED_BYTE || !data)
        guest_fault(c, guest_current_target, "Vita glGetTexImage supports only 2D level 0 RGBA/UNSIGNED_BYTE");
    GLint name = 0, old = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &name);
    struct TextureSize *p = sizes;
    for (; p && p->name != (GLuint)name; p=p->next) {}
    if (!p) guest_fault(c, guest_current_target, "Vita glGetTexImage: missing texture dimensions");
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old);
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    if (!framebuffer) guest_fault(c, guest_current_target, "Vita texture readback framebuffer allocation failed");
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, name, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) glReadPixels(0, 0, p->width, p->height, format, type, data);
    glBindFramebuffer(GL_FRAMEBUFFER, old);
    glDeleteFramebuffers(1, &framebuffer);
    if (status != GL_FRAMEBUFFER_COMPLETE) guest_fault(c, guest_current_target, "Vita texture readback framebuffer is incomplete");
}
