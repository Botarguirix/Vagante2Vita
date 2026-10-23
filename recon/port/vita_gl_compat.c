/* OpenGL entry points SFML references that vitaGL does not implement.
 * Push/PopClientAttrib only save/restore client state around RenderTarget::pushGLStates;
 * glGetTexImage backs Texture::copyToImage (screenshots), which the port does not need yet. */
#if defined(__vita__)
#include <vitaGL.h>

void glPushClientAttrib(GLbitfield mask) { (void)mask; }
void glPopClientAttrib(void) {}
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels)
{
    (void)target; (void)level; (void)format; (void)type; (void)pixels;
}
#endif
