/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

// Bring-in the total glad environment
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include "glad/glad.h"
#include "glad/g_glad.h"

/*
 * Our OpenGL extensions loaded into global functions and enumerants.
 */

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// Framebuffer
//////////////////////////////////////////////////////////////////////////

typedef GLboolean(APIENTRYP PFNGLISRENDERBUFFERPROC)(GLuint renderbuffer);
GLAPI PFNGLISRENDERBUFFERPROC glIsRenderbuffer;

typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
GLAPI PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;

typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint *renderbuffers);
GLAPI PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;

typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint *renderbuffers);
GLAPI PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;

typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GLAPI PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;

typedef void (APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIVPROC)(GLenum target, GLenum pname, GLint *params);
GLAPI PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;

typedef GLboolean(APIENTRYP PFNGLISFRAMEBUFFERPROC)(GLuint framebuffer);
GLAPI PFNGLISFRAMEBUFFERPROC glIsFramebuffer;

typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
GLAPI PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;

typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
GLAPI PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;

typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
GLAPI PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;

typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
GLAPI PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;

typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE1DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;

typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;

typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE3DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
GLAPI PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;

typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLAPI PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;

typedef void (APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
GLAPI PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;

//
// Enumerants
//

#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#define GL_COLOR_ATTACHMENT8 0x8CE8
#define GL_COLOR_ATTACHMENT9 0x8CE9
#define GL_COLOR_ATTACHMENT10 0x8CEA
#define GL_COLOR_ATTACHMENT11 0x8CEB
#define GL_COLOR_ATTACHMENT12 0x8CEC
#define GL_COLOR_ATTACHMENT13 0x8CED
#define GL_COLOR_ATTACHMENT14 0x8CEE
#define GL_COLOR_ATTACHMENT15 0x8CEF
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_STENCIL_INDEX1 0x8D46
#define GL_STENCIL_INDEX4 0x8D47
#define GL_STENCIL_INDEX8 0x8D48
#define GL_STENCIL_INDEX16 0x8D49
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56

//////////////////////////////////////////////////////////////////////////
// Draw Instanced
//////////////////////////////////////////////////////////////////////////

//
// Functions
//

typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
GLAPI PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;

typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDPROC)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
GLAPI PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;

//////////////////////////////////////////////////////////////////////////
// VAO
//////////////////////////////////////////////////////////////////////////

//
// Functions
//

typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
GLAPI PFNGLBINDVERTEXARRAYPROC glBindVertexArray;

typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
GLAPI PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;

typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
GLAPI PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;

typedef GLboolean(APIENTRYP PFNGLISVERTEXARRAYPROC)(GLuint array);
GLAPI PFNGLISVERTEXARRAYPROC glIsVertexArray;

//
// Enumerants
//

#define GL_VERTEX_ARRAY_BINDING 0x85B5

//////////////////////////////////////////////////////////////////////////
// Texture Float
//////////////////////////////////////////////////////////////////////////

//
// Enumerants
//

#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815
#define GL_ALPHA32F 0x8816
#define GL_INTENSITY32F 0x8817
#define GL_LUMINANCE32F 0x8818
#define GL_LUMINANCE_ALPHA32F 0x8819
#define GL_RGBA16F 0x881a
#define GL_RGB16F 0x881b
#define GL_ALPHA16F 0x881C
#define GL_INTENSITY16F 0x881D
#define GL_LUMINANCE16F 0x881E
#define GL_LUMINANCE_ALPHA16F 0x881F

//////////////////////////////////////////////////////////////////////////
// Texture RG
//////////////////////////////////////////////////////////////////////////

//
// Enumerants
//

#define GL_COMPRESSED_RED          0x8225
#define GL_COMPRESSED_RG           0x8226
#define GL_RG                      0x8227
#define GL_RG_INTEGER              0x8228
#define GL_R8                      0x8229
#define GL_R16                     0x822A
#define GL_RG8                     0x822B
#define GL_RG16                    0x822C
#define GL_R16F                    0x822D
#define GL_R32F                    0x822E
#define GL_RG16F                   0x822F
#define GL_RG32F                   0x8230
#define GL_R8I                     0x8231
#define GL_R8UI                    0x8232
#define GL_R16I                    0x8233
#define GL_R16UI                   0x8234
#define GL_R32I                    0x8235
#define GL_R32UI                   0x8236
#define GL_RG8I                    0x8237
#define GL_RG8UI                   0x8238
#define GL_RG16I                   0x8239
#define GL_RG16UI                  0x823A
#define GL_RG32I                   0x823B
#define GL_RG32UI                  0x823C

//////////////////////////////////////////////////////////////////////////
// Misc
//////////////////////////////////////////////////////////////////////////

//
// Functions
//

typedef void (APIENTRYP PFNGLGETPROGRAMBINARYPROC)(GLuint program, GLsizei bufSize, GLsizei * length, GLenum * binaryFormat, void * binary);
GLAPI PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;

typedef void (APIENTRY * DEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * userParam);
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKARB)(DEBUGPROCARB callback, const void * userParam);
GLAPI PFNGLDEBUGMESSAGECALLBACKARB glDebugMessageCallback;

//
// Enumerants
//

#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_SEVERITY_HIGH_ARB 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_ARB 0x9147
#define GL_DEBUG_SEVERITY_LOW_ARB 0x9148

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////
// Init
//////////////////////////////////////////////////////////////////////////

void InitOpenGLExt();
