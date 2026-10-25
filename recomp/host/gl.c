/* OpenGL 1.1 imports only; WGL/context ownership remains in the window host. */
#include "win32.h"
#include <GL/gl.h>
#include <pthread.h>
#define API(name) static void api_##name(Cpu *c)
#define I(n) ((GLint)ARG(c, n))
#define F(n) rdf32(c->esp.d + 4u + 4u * (n))
#define D(n) rdf64(c->esp.d + 4u + 4u * (n))
#define P(n) (ARG(c, n) ? (void *)G(ARG(c, n)) : NULL)
#define DONE(n) ret_stdcall(c, 0, n)
_Static_assert(sizeof(GLint) == 4 && sizeof(GLuint) == 4 && sizeof(GLfloat) == 4 && sizeof(GLdouble) == 8, "OpenGL guest ABI");
API(glBindTexture) { glBindTexture(ARG(c, 0), ARG(c, 1)); DONE(2); }
API(glBlendFunc) { glBlendFunc(ARG(c, 0), ARG(c, 1)); DONE(2); }
API(glClear) { glClear(ARG(c, 0)); DONE(1); }
API(glClearColor) { glClearColor(F(0), F(1), F(2), F(3)); DONE(4); }
API(glColorPointer) { glColorPointer(I(0), ARG(c, 1), I(2), P(3)); DONE(4); }
API(glCopyTexSubImage2D) { glCopyTexSubImage2D(ARG(c, 0), I(1), I(2), I(3), I(4), I(5), I(6), I(7)); DONE(8); }
API(glDeleteTextures) { glDeleteTextures(I(0), P(1)); DONE(2); }
API(glDisable) { glDisable(ARG(c, 0)); DONE(1); }
API(glDrawArrays) { glDrawArrays(ARG(c, 0), I(1), I(2)); DONE(3); }
API(glDrawElements) { glDrawElements(ARG(c, 0), I(1), ARG(c, 2), P(3)); DONE(4); }
API(glEnable) { glEnable(ARG(c, 0)); DONE(1); }
API(glEnableClientState) { glEnableClientState(ARG(c, 0)); DONE(1); }
API(glFlush) { glFlush(); DONE(0); }
API(glGenTextures) { glGenTextures(I(0), P(1)); DONE(2); }
API(glGetError) { ret_stdcall(c, glGetError(), 0); }
API(glGetIntegerv) { glGetIntegerv(ARG(c, 0), P(1)); DONE(2); }
API(glGetString) {
    /* Host strings are never exposed as truncated guest addresses. Cache by
       enum once a context returns a string; a NULL result is not cached. */
    static struct { GLenum name; uint32_t guest; } strings[16];
    static unsigned count;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    GLenum name = ARG(c, 0);
    uint32_t result = 0;
    pthread_mutex_lock(&lock);
    for (unsigned i = 0; i < count; ++i)
        if (strings[i].name == name) { result = strings[i].guest; break; }
    if (!result) {
        const GLubyte *text = glGetString(name);
        if (text && count < sizeof strings / sizeof strings[0]) {
            result = guest_strdup((const char *)text);
            if (result) { strings[count].name = name; strings[count++].guest = result; }
        }
    }
    pthread_mutex_unlock(&lock);
    ret_stdcall(c, result, 1);
}
API(glGetTexImage) { glGetTexImage(ARG(c, 0), I(1), ARG(c, 2), ARG(c, 3), P(4)); DONE(5); }
API(glIsEnabled) { ret_stdcall(c, glIsEnabled(ARG(c, 0)), 1); }
API(glLoadIdentity) { glLoadIdentity(); DONE(0); }
API(glLoadMatrixf) { glLoadMatrixf(P(0)); DONE(1); }
API(glMatrixMode) { glMatrixMode(ARG(c, 0)); DONE(1); }
/* Six doubles occupy TWELVE guest stack words, not six. */
API(glOrtho) { glOrtho(D(0), D(2), D(4), D(6), D(8), D(10)); DONE(12); }
API(glPopAttrib) { glPopAttrib(); DONE(0); }
API(glPushAttrib) { glPushAttrib(ARG(c, 0)); DONE(1); }
API(glScissor) { glScissor(I(0), I(1), I(2), I(3)); DONE(4); }
API(glTexCoordPointer) { glTexCoordPointer(I(0), ARG(c, 1), I(2), P(3)); DONE(4); }
API(glTexImage2D) { glTexImage2D(ARG(c, 0), I(1), I(2), I(3), I(4), I(5), ARG(c, 6), ARG(c, 7), P(8)); DONE(9); }
API(glTexParameteri) { glTexParameteri(ARG(c, 0), ARG(c, 1), I(2)); DONE(3); }
API(glTexSubImage2D) { glTexSubImage2D(ARG(c, 0), I(1), I(2), I(3), I(4), I(5), ARG(c, 6), ARG(c, 7), P(8)); DONE(9); }
API(glVertexPointer) { glVertexPointer(I(0), ARG(c, 1), I(2), P(3)); DONE(4); }
API(glViewport) { glViewport(I(0), I(1), I(2), I(3)); DONE(4); }
#define E(name) {#name, api_##name}
const ApiEntry win32_gl[] = {
    E(glBindTexture), E(glBlendFunc), E(glClear), E(glClearColor), E(glColorPointer),
    E(glCopyTexSubImage2D), E(glDeleteTextures), E(glDisable), E(glDrawArrays), E(glDrawElements),
    E(glEnable), E(glEnableClientState), E(glFlush), E(glGenTextures), E(glGetError),
    E(glGetIntegerv), E(glGetString), E(glGetTexImage), E(glIsEnabled), E(glLoadIdentity),
    E(glLoadMatrixf), E(glMatrixMode), E(glOrtho), E(glPopAttrib), E(glPushAttrib), E(glScissor),
    E(glTexCoordPointer), E(glTexImage2D), E(glTexParameteri), E(glTexSubImage2D),
    E(glVertexPointer), E(glViewport), {NULL, NULL},
};
