#pragma once
#include <cstddef>
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLubyte;
typedef void GLvoid;
typedef unsigned char GLboolean;
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#define GL_RGBA 0x1908
#define GL_RGBA8 0x8058
#define GL_LINEAR 0x2601
#define GL_MIN_FILTER 0x2801
#define GL_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE0 0x84C0
#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_BLEND 0x0BE2
#define GL_DEPTH_TEST 0x0B71
#define GL_UNSIGNED_BYTE 0x1401
#define GL_VIEWPORT 0x0BA2
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLE_STRIP 0x0005
inline void glGenTextures(GLsizei, GLuint*) {}
inline void glDeleteTextures(GLsizei, const GLuint*) {}
inline void glBindTexture(GLenum, GLuint) {}
inline void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
inline void glTexParameteri(GLenum, GLenum, GLint) {}
inline void glActiveTexture(GLenum) {}
inline void glGenFramebuffers(GLsizei, GLuint*) {}
inline void glDeleteFramebuffers(GLsizei, const GLuint*) {}
inline void glBindFramebuffer(GLenum, GLuint) {}
inline void glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint) {}
inline GLenum glCheckFramebufferStatus(GLenum) { return GL_FRAMEBUFFER_COMPLETE; }
inline GLuint glCreateShader(GLenum) { return 1; }
inline void glShaderSource(GLuint, GLsizei, const char* const*, const GLint*) {}
inline void glCompileShader(GLuint) {}
inline void glGetShaderiv(GLuint, GLenum, GLint* p) { if (p) *p = 1; }
inline void glDeleteShader(GLuint) {}
inline GLuint glCreateProgram() { return 1; }
inline void glAttachShader(GLuint, GLuint) {}
inline void glLinkProgram(GLuint) {}
inline void glGetProgramiv(GLuint, GLenum, GLint* p) { if (p) *p = 1; }
inline void glDeleteProgram(GLuint) {}
inline void glUseProgram(GLuint) {}
inline GLint glGetUniformLocation(GLuint, const char*) { return 0; }
inline void glUniform1i(GLint, GLint) {}
inline void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat*) {}
inline void glGenVertexArrays(GLsizei, GLuint*) {}
inline void glBindVertexArray(GLuint) {}
inline void glDrawArrays(GLenum, GLint, GLsizei) {}
inline void glGetIntegerv(GLenum, GLint* p) { if (p) *p = 0; }
inline void glViewport(GLint, GLint, GLsizei, GLsizei) {}
inline void glEnable(GLenum) {}
inline void glDisable(GLenum) {}
inline GLenum glGetError() { return 0; }
