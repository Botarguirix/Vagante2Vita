/* VitaSDK supplies vitaGL.h rather than desktop GL/gl.h. */
#pragma once
#include <vitaGL.h>
void vagante_glTexImage2D(Cpu *, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void vagante_glDeleteTextures(GLsizei, const GLuint *);
void vagante_glGetTexImage(Cpu *, GLenum, GLint, GLenum, GLenum, GLvoid *);
#define glTexImage2D(...) vagante_glTexImage2D(c, __VA_ARGS__)
#define glDeleteTextures vagante_glDeleteTextures

#define glGetTexImage(...) vagante_glGetTexImage(c, __VA_ARGS__)
