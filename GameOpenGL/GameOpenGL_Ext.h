/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <glad/glad.h>
#include <glad/g_glad.h>

#include <memory>

//////////////////////////////////////////////////////////////////////////
// Framebuffer
//////////////////////////////////////////////////////////////////////////

struct OpenGLExt_Framebuffer_Core
{
    //
    // Functions
    //

    typedef GLboolean(APIENTRYP PFNGLISRENDERBUFFERPROC)(GLuint renderbuffer);
    PFNGLISRENDERBUFFERPROC IsRenderbuffer = NULL;

    typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
    PFNGLBINDRENDERBUFFERPROC BindRenderbuffer = NULL;

    typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint *renderbuffers);
    PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers = NULL;

    typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint *renderbuffers);
    PFNGLGENRENDERBUFFERSPROC GenRenderbuffers = NULL;

    typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage = NULL;

    typedef void (APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIVPROC)(GLenum target, GLenum pname, GLint *params);
    PFNGLGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv = NULL;

    typedef GLboolean(APIENTRYP PFNGLISFRAMEBUFFERPROC)(GLuint framebuffer);
    PFNGLISFRAMEBUFFERPROC IsFramebuffer = NULL;

    typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer = NULL;

    typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers = NULL;

    typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers = NULL;

    typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE1DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    PFNGLFRAMEBUFFERTEXTURE1DPROC FramebufferTexture1D = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE3DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
    PFNGLFRAMEBUFFERTEXTURE3DPROC FramebufferTexture3D = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer = NULL;

    typedef void (APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC GetFramebufferAttachmentParameteriv = NULL;

    //
    // Enumerants
    //

    static constexpr GLenum INVALID_FRAMEBUFFER_OPERATION = 0x0506;
    static constexpr GLenum MAX_RENDERBUFFER_SIZE = 0x84E8;
    static constexpr GLenum FRAMEBUFFER_BINDING = 0x8CA6;
    static constexpr GLenum RENDERBUFFER_BINDING = 0x8CA7;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE = 0x8CD0;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_OBJECT_NAME = 0x8CD1;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL = 0x8CD2;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE = 0x8CD3;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER = 0x8CD4;
    static constexpr GLenum FRAMEBUFFER_COMPLETE = 0x8CD5;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_ATTACHMENT = 0x8CD6;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER = 0x8CDB;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_READ_BUFFER = 0x8CDC;
    static constexpr GLenum FRAMEBUFFER_UNSUPPORTED = 0x8CDD;
    static constexpr GLenum MAX_COLOR_ATTACHMENTS = 0x8CDF;
    static constexpr GLenum COLOR_ATTACHMENT0 = 0x8CE0;
    static constexpr GLenum COLOR_ATTACHMENT1 = 0x8CE1;
    static constexpr GLenum COLOR_ATTACHMENT2 = 0x8CE2;
    static constexpr GLenum COLOR_ATTACHMENT3 = 0x8CE3;
    static constexpr GLenum COLOR_ATTACHMENT4 = 0x8CE4;
    static constexpr GLenum COLOR_ATTACHMENT5 = 0x8CE5;
    static constexpr GLenum COLOR_ATTACHMENT6 = 0x8CE6;
    static constexpr GLenum COLOR_ATTACHMENT7 = 0x8CE7;
    static constexpr GLenum COLOR_ATTACHMENT8 = 0x8CE8;
    static constexpr GLenum COLOR_ATTACHMENT9 = 0x8CE9;
    static constexpr GLenum COLOR_ATTACHMENT10 = 0x8CEA;
    static constexpr GLenum COLOR_ATTACHMENT11 = 0x8CEB;
    static constexpr GLenum COLOR_ATTACHMENT12 = 0x8CEC;
    static constexpr GLenum COLOR_ATTACHMENT13 = 0x8CED;
    static constexpr GLenum COLOR_ATTACHMENT14 = 0x8CEE;
    static constexpr GLenum COLOR_ATTACHMENT15 = 0x8CEF;
    static constexpr GLenum DEPTH_ATTACHMENT = 0x8D00;
    static constexpr GLenum STENCIL_ATTACHMENT = 0x8D20;
    static constexpr GLenum FRAMEBUFFER = 0x8D40;
    static constexpr GLenum RENDERBUFFER = 0x8D41;
    static constexpr GLenum RENDERBUFFER_WIDTH = 0x8D42;
    static constexpr GLenum RENDERBUFFER_HEIGHT = 0x8D43;
    static constexpr GLenum RENDERBUFFER_INTERNAL_FORMAT = 0x8D44;
    static constexpr GLenum STENCIL_INDEX1 = 0x8D46;
    static constexpr GLenum STENCIL_INDEX4 = 0x8D47;
    static constexpr GLenum STENCIL_INDEX8 = 0x8D48;
    static constexpr GLenum STENCIL_INDEX16 = 0x8D49;
    static constexpr GLenum RENDERBUFFER_RED_SIZE = 0x8D50;
    static constexpr GLenum RENDERBUFFER_GREEN_SIZE = 0x8D51;
    static constexpr GLenum RENDERBUFFER_BLUE_SIZE = 0x8D52;
    static constexpr GLenum RENDERBUFFER_ALPHA_SIZE = 0x8D53;
    static constexpr GLenum RENDERBUFFER_DEPTH_SIZE = 0x8D54;
    static constexpr GLenum RENDERBUFFER_STENCIL_SIZE = 0x8D55;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_MULTISAMPLE = 0x8D56;
};

extern std::unique_ptr<OpenGLExt_Framebuffer_Core const> gglFramebuffer_Core;

struct OpenGLExt_Framebuffer_EXT
{
    //
    // Functions
    //

    typedef GLboolean(APIENTRYP PFNGLISRENDERBUFFEREXTPROC)(GLuint renderbuffer);
    PFNGLISRENDERBUFFEREXTPROC IsRenderbufferEXT = NULL;

    typedef void (APIENTRYP PFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
    PFNGLBINDRENDERBUFFEREXTPROC BindRenderbufferEXT = NULL;

    typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint *renderbuffers);
    PFNGLDELETERENDERBUFFERSEXTPROC DeleteRenderbuffersEXT = NULL;

    typedef void (APIENTRYP PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint *renderbuffers);
    PFNGLGENRENDERBUFFERSEXTPROC GenRenderbuffersEXT = NULL;

    typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    PFNGLRENDERBUFFERSTORAGEEXTPROC RenderbufferStorageEXT = NULL;

    typedef void (APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)(GLenum target, GLenum pname, GLint *params);
    PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC GetRenderbufferParameterivEXT = NULL;

    typedef GLboolean(APIENTRYP PFNGLISFRAMEBUFFEREXTPROC)(GLuint framebuffer);
    PFNGLISFRAMEBUFFEREXTPROC IsFramebufferEXT = NULL;

    typedef void (APIENTRYP PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
    PFNGLBINDFRAMEBUFFEREXTPROC BindFramebufferEXT = NULL;

    typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint *framebuffers);
    PFNGLDELETEFRAMEBUFFERSEXTPROC DeleteFramebuffersEXT = NULL;

    typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint *framebuffers);
    PFNGLGENFRAMEBUFFERSEXTPROC GenFramebuffersEXT = NULL;

    typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
    PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC CheckFramebufferStatusEXT = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    PFNGLFRAMEBUFFERTEXTURE1DEXTPROC FramebufferTexture1DEXT = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC FramebufferTexture2DEXT = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
    PFNGLFRAMEBUFFERTEXTURE3DEXTPROC FramebufferTexture3DEXT = NULL;

    typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC FramebufferRenderbufferEXT = NULL;

    typedef void (APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC GetFramebufferAttachmentParameterivEXT = NULL;

    //
    // Enumerants
    //

    static constexpr GLenum INVALID_FRAMEBUFFER_OPERATION_EXT = 0x0506;
    static constexpr GLenum MAX_RENDERBUFFER_SIZE_EXT = 0x84E8;
    static constexpr GLenum FRAMEBUFFER_BINDING_EXT = 0x8CA6;
    static constexpr GLenum RENDERBUFFER_BINDING_EXT = 0x8CA7;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT = 0x8CD0;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT = 0x8CD1;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT = 0x8CD2;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT = 0x8CD3;
    static constexpr GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT = 0x8CD4;
    static constexpr GLenum FRAMEBUFFER_COMPLETE_EXT = 0x8CD5;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT = 0x8CD6;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT = 0x8CD7;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT = 0x8CD9;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_FORMATS_EXT = 0x8CDA;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT = 0x8CDB;
    static constexpr GLenum FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT = 0x8CDC;
    static constexpr GLenum FRAMEBUFFER_UNSUPPORTED_EXT = 0x8CDD;
    static constexpr GLenum MAX_COLOR_ATTACHMENTS_EXT = 0x8CDF;
    static constexpr GLenum COLOR_ATTACHMENT0_EXT = 0x8CE0;
    static constexpr GLenum COLOR_ATTACHMENT1_EXT = 0x8CE1;
    static constexpr GLenum COLOR_ATTACHMENT2_EXT = 0x8CE2;
    static constexpr GLenum COLOR_ATTACHMENT3_EXT = 0x8CE3;
    static constexpr GLenum COLOR_ATTACHMENT4_EXT = 0x8CE4;
    static constexpr GLenum COLOR_ATTACHMENT5_EXT = 0x8CE5;
    static constexpr GLenum COLOR_ATTACHMENT6_EXT = 0x8CE6;
    static constexpr GLenum COLOR_ATTACHMENT7_EXT = 0x8CE7;
    static constexpr GLenum COLOR_ATTACHMENT8_EXT = 0x8CE8;
    static constexpr GLenum COLOR_ATTACHMENT9_EXT = 0x8CE9;
    static constexpr GLenum COLOR_ATTACHMENT10_EXT = 0x8CEA;
    static constexpr GLenum COLOR_ATTACHMENT11_EXT = 0x8CEB;
    static constexpr GLenum COLOR_ATTACHMENT12_EXT = 0x8CEC;
    static constexpr GLenum COLOR_ATTACHMENT13_EXT = 0x8CED;
    static constexpr GLenum COLOR_ATTACHMENT14_EXT = 0x8CEE;
    static constexpr GLenum COLOR_ATTACHMENT15_EXT = 0x8CEF;
    static constexpr GLenum DEPTH_ATTACHMENT_EXT = 0x8D00;
    static constexpr GLenum STENCIL_ATTACHMENT_EXT = 0x8D20;
    static constexpr GLenum FRAMEBUFFER_EXT = 0x8D40;
    static constexpr GLenum RENDERBUFFER_EXT = 0x8D41;
    static constexpr GLenum RENDERBUFFER_WIDTH_EXT = 0x8D42;
    static constexpr GLenum RENDERBUFFER_HEIGHT_EXT = 0x8D43;
    static constexpr GLenum RENDERBUFFER_INTERNAL_FORMAT_EXT = 0x8D44;
    static constexpr GLenum STENCIL_INDEX1_EXT = 0x8D46;
    static constexpr GLenum STENCIL_INDEX4_EXT = 0x8D47;
    static constexpr GLenum STENCIL_INDEX8_EXT = 0x8D48;
    static constexpr GLenum STENCIL_INDEX16_EXT = 0x8D49;
    static constexpr GLenum RENDERBUFFER_RED_SIZE_EXT = 0x8D50;
    static constexpr GLenum RENDERBUFFER_GREEN_SIZE_EXT = 0x8D51;
    static constexpr GLenum RENDERBUFFER_BLUE_SIZE_EXT = 0x8D52;
    static constexpr GLenum RENDERBUFFER_ALPHA_SIZE_EXT = 0x8D53;
    static constexpr GLenum RENDERBUFFER_DEPTH_SIZE_EXT = 0x8D54;
    static constexpr GLenum RENDERBUFFER_STENCIL_SIZE_EXT = 0x8D55;
};

extern std::unique_ptr<OpenGLExt_Framebuffer_EXT const> gglFramebuffer_EXT;

//////////////////////////////////////////////////////////////////////////
// Draw Instanced
//////////////////////////////////////////////////////////////////////////

struct OpenGLExt_DrawInstanced_Core
{
    //
    // Functions
    //

    typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    PFNGLDRAWARRAYSINSTANCEDPROC DrawArraysInstanced = NULL;

    typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDPROC)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
    PFNGLDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced = NULL;
};

extern std::unique_ptr<OpenGLExt_DrawInstanced_Core const> gglDrawInstanced_Core;

struct OpenGLExt_DrawInstanced_ARB
{
    //
    // Functions
    //

    typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDARBPROC)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
    PFNGLDRAWARRAYSINSTANCEDARBPROC DrawArraysInstancedARB = NULL;

    typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDARBPROC)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
    PFNGLDRAWELEMENTSINSTANCEDARBPROC DrawElementsInstancedARB = NULL;
};

extern std::unique_ptr<OpenGLExt_DrawInstanced_ARB const> gglDrawInstanced_ARB;

struct OpenGLExt_DrawInstanced_EXT
{
    //
    // Functions
    //

    typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDEXTPROC)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
    PFNGLDRAWARRAYSINSTANCEDEXTPROC DrawArraysInstancedEXT = NULL;

    typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDEXTPROC)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
    PFNGLDRAWELEMENTSINSTANCEDEXTPROC DrawElementsInstancedEXT = NULL;
};

extern std::unique_ptr<OpenGLExt_DrawInstanced_EXT const> gglDrawInstanced_EXT;

//////////////////////////////////////////////////////////////////////////
// VAO
//////////////////////////////////////////////////////////////////////////

// The ARB made it into Core as an alias, hence we only need one struct
struct OpenGLExt_VertexArray_Core
{
    //
    // Functions
    //

    typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
    PFNGLBINDVERTEXARRAYPROC BindVertexArray = NULL;

    typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
    PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = NULL;

    typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
    PFNGLGENVERTEXARRAYSPROC GenVertexArrays = NULL;

    typedef GLboolean(APIENTRYP PFNGLISVERTEXARRAYPROC)(GLuint array);
    PFNGLISVERTEXARRAYPROC IsVertexArray = NULL;

    //
    // Enumerants
    //

    static constexpr GLenum VERTEX_ARRAY_BINDING = 0x85B5;
};

extern std::unique_ptr<OpenGLExt_VertexArray_Core const> gglVertexArray_Core;


//////////////////////////////////////////////////////////////////////////
// Init
//////////////////////////////////////////////////////////////////////////

void InitOpenGLExt();
