#ifndef OPENGL_FUNCTIONS_H
#define OPENGL_FUNCTIONS_H

#include <GL/gl.h>
#ifdef __WIN32
#include <GL/wglext.h>
#endif
#include <GL/glext.h>
/*
  This header file contains get_gl_function
  which is implemented by a platform, and a whole
  lot of opengl function pointers.
*/
#define LOAD_GL( type, pfn_name ) (pfn_name = (type)get_gl_function( #pfn_name ))

typedef void(*PFNGLGENBUFFERS)( GLsizei, GLuint* );
typedef void(*PFNGLBINDBUFFER)( GLenum, GLuint );
typedef void(*PFNGLBUFFERDATA)( GLenum, GLsizeiptr, const void*, GLenum );
typedef void(*PFNGLDELETEBUFFERS)( GLsizei, GLuint* );

typedef void(*PFNGLGENVERTEXARRAYS)( GLsizei, GLuint* );
typedef void(*PFNGLDELETEVERTEXARRAYS)( GLsizei, const GLuint* );
typedef void(*PFNGLBINDVERTEXARRAY)( GLuint );

typedef void(*PFNGLVERTEXATTRIBPOINTER)( GLuint, GLint, GLenum, GLboolean, GLsizei, const void* );
typedef void(*PFNGLENABLEVERTEXATTRIBARRAY)( GLuint );

typedef void(*PFNGLDRAWARRAYS)( GLenum, GLint, GLsizei );

typedef GLuint(*PFNGLCREATEPROGRAM)( void );
typedef void(*PFNGLDELETEPROGRAM)( GLuint );

typedef GLuint(*PFNGLCREATESHADER)( GLenum );
typedef void(*PFNGLDELETESHADER)( GLuint );

typedef void(*PFNGLSHADERSOURCE)( GLuint, GLsizei, const GLchar**, const GLint* );

typedef void(*PFNGLATTACHSHADER)( GLuint, GLuint );
typedef void(*PFNGLDETACHSHADER)( GLuint, GLuint );

typedef void(*PFNGLUSEPROGRAM)( GLuint );
typedef void(*PFNGLCOMPILESHADER)( GLuint );
typedef void(*PFNGLLINKPROGRAM)( GLuint );

typedef GLint(*PFNGLGETUNIFORMLOCATION)( GLuint, const GLchar* );

typedef void(*PFNGLUNIFORM1F)( GLint, GLfloat );
typedef void(*PFNGLUNIFORM2F)( GLint, GLfloat, GLfloat );
typedef void(*PFNGLUNIFORM3F)( GLint, GLfloat, GLfloat, GLfloat );

typedef void(*PFNGLUNIFORM1I)( GLint, GLint );
typedef void(*PFNGLUNIFORM2I)( GLint, GLint, GLint );
typedef void(*PFNGLUNIFORM3I)( GLint, GLint, GLint, GLint );

typedef void(*PFNGLUNIFORMMATRIX)( GLint, GLsizei, GLboolean, const GLfloat* );

typedef void(*PFNGLGETPROGRAMIV)( GLuint, GLenum, GLint* );
typedef void(*PFNGLGETPROGRAMINFOLOG)( GLuint, GLsizei, GLsizei*, GLchar* );

typedef void(*PFNGLGETSHADERIV)( GLuint, GLenum, GLint* );
typedef void(*PFNGLGETSHADERINFOLOG)( GLuint, GLsizei, GLsizei*, GLchar* );

typedef void(*PFNGLGENERATEMIPMAP)( GLenum );

// NOTE(jerry): I should probably check this or something... I shouldn't need a #ifdef here.
#ifdef __WIN32
typedef void(*PFNGLACTIVETEXTURE)( GLenum );
#endif

static PFNGLGENBUFFERS glGenBuffers;
static PFNGLBINDBUFFER glBindBuffer;
static PFNGLBUFFERDATA glBufferData;
static PFNGLDELETEBUFFERS glDeleteBuffers;
static PFNGLGENVERTEXARRAYS glGenVertexArrays;
static PFNGLDELETEVERTEXARRAYS glDeleteVertexArrays;
static PFNGLBINDVERTEXARRAY glBindVertexArray;
static PFNGLVERTEXATTRIBPOINTER glVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray;
static PFNGLCREATEPROGRAM glCreateProgram;
static PFNGLDELETEPROGRAM glDeleteProgram;
static PFNGLCREATESHADER glCreateShader;
static PFNGLDELETESHADER glDeleteShader;
static PFNGLSHADERSOURCE glShaderSource;
static PFNGLATTACHSHADER glAttachShader;
static PFNGLDETACHSHADER glDetachShader;
static PFNGLUSEPROGRAM glUseProgram;
static PFNGLCOMPILESHADER glCompileShader;
static PFNGLLINKPROGRAM glLinkProgram;
static PFNGLGETUNIFORMLOCATION glGetUniformLocation;
static PFNGLUNIFORM1F glUniform1f;
static PFNGLUNIFORM2F glUniform2f;
static PFNGLUNIFORM3F glUniform3f;
static PFNGLUNIFORM1I glUniform1i;
static PFNGLUNIFORM2I glUniform2i;
static PFNGLUNIFORM3I glUniform3i;
static PFNGLUNIFORMMATRIX glUniformMatrix4fv;
static PFNGLGETPROGRAMIV glGetProgramiv;
static PFNGLGETPROGRAMINFOLOG glGetProgramInfoLog;
static PFNGLGETSHADERIV glGetShaderiv;
static PFNGLGETSHADERINFOLOG glGetShaderInfoLog;
static PFNGLGENERATEMIPMAP glGenerateMipmap;
#ifdef __WIN32
static PFNGLACTIVETEXTURE glActiveTexture;
#endif

void* get_gl_function( const char* proc_name );

void init_gl_functions( void );

#endif
