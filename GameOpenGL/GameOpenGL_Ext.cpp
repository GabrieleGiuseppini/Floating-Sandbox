/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameOpenGL_Ext.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

template <typename TFunc>
void LoadAndVerify(char const * functionName, TFunc * & pFunc, GLADloadproc load)
{
    pFunc = reinterpret_cast<TFunc *>(load(functionName));
    if (nullptr == pFunc)
    {
        throw GameException(std::string("OpenGL function '") + functionName + "' is not supported");
    }
}

bool HasExt(char const * extensionName)
{
    bool result = has_ext(extensionName);

    LogMessage("Has ", extensionName, ": ", result ? "YES" : "NO");

    return result;
}

//////////////////////////////////////////////////////////////////////////
// Framebuffer
//////////////////////////////////////////////////////////////////////////

PFNGLISRENDERBUFFERPROC glIsRenderbuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = NULL;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = NULL;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = NULL;
PFNGLISFRAMEBUFFERPROC glIsFramebuffer = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = NULL;

void InitOpenGLExt_Framebuffer(GLADloadproc load)
{
    if (GLVersion.major >= 3) // Core in 3.0
    {
        // Core

        LoadAndVerify("glIsRenderbuffer", glIsRenderbuffer, load);
        LoadAndVerify("glIsRenderbuffer", glIsRenderbuffer, load);
        LoadAndVerify("glBindRenderbuffer", glBindRenderbuffer, load);
        LoadAndVerify("glDeleteRenderbuffers", glDeleteRenderbuffers, load);
        LoadAndVerify("glGenRenderbuffers", glGenRenderbuffers, load);
        LoadAndVerify("glRenderbufferStorage", glRenderbufferStorage, load);
        LoadAndVerify("glGetRenderbufferParameteriv", glGetRenderbufferParameteriv, load);
        LoadAndVerify("glIsFramebuffer", glIsFramebuffer, load);
        LoadAndVerify("glBindFramebuffer", glBindFramebuffer, load);
        LoadAndVerify("glDeleteFramebuffers", glDeleteFramebuffers, load);
        LoadAndVerify("glGenFramebuffers", glGenFramebuffers, load);
        LoadAndVerify("glCheckFramebufferStatus", glCheckFramebufferStatus, load);
        LoadAndVerify("glFramebufferTexture1D", glFramebufferTexture1D, load);
        LoadAndVerify("glFramebufferTexture2D", glFramebufferTexture2D, load);
        LoadAndVerify("glFramebufferTexture3D", glFramebufferTexture3D, load);
        LoadAndVerify("glFramebufferRenderbuffer", glFramebufferRenderbuffer, load);
        LoadAndVerify("glGetFramebufferAttachmentParameteriv", glGetFramebufferAttachmentParameteriv, load);
    }
    else if (HasExt("GL_EXT_framebuffer_object"))
    {
        // Ext

        LoadAndVerify("glIsRenderbufferEXT", glIsRenderbuffer, load);
        LoadAndVerify("glIsRenderbufferEXT", glIsRenderbuffer, load);
        LoadAndVerify("glBindRenderbufferEXT", glBindRenderbuffer, load);
        LoadAndVerify("glDeleteRenderbuffersEXT", glDeleteRenderbuffers, load);
        LoadAndVerify("glGenRenderbuffersEXT", glGenRenderbuffers, load);
        LoadAndVerify("glRenderbufferStorageEXT", glRenderbufferStorage, load);
        LoadAndVerify("glGetRenderbufferParameterivEXT", glGetRenderbufferParameteriv, load);
        LoadAndVerify("glIsFramebufferEXT", glIsFramebuffer, load);
        LoadAndVerify("glBindFramebufferEXT", glBindFramebuffer, load);
        LoadAndVerify("glDeleteFramebuffersEXT", glDeleteFramebuffers, load);
        LoadAndVerify("glGenFramebuffersEXT", glGenFramebuffers, load);
        LoadAndVerify("glCheckFramebufferStatusEXT", glCheckFramebufferStatus, load);
        LoadAndVerify("glFramebufferTexture1DEXT", glFramebufferTexture1D, load);
        LoadAndVerify("glFramebufferTexture2DEXT", glFramebufferTexture2D, load);
        LoadAndVerify("glFramebufferTexture3DEXT", glFramebufferTexture3D, load);
        LoadAndVerify("glFramebufferRenderbufferEXT", glFramebufferRenderbuffer, load);
        LoadAndVerify("glGetFramebufferAttachmentParameterivEXT", glGetFramebufferAttachmentParameteriv, load);
    }
    else
    {
        throw GameException("Framebuffer functionality is not supported");
    }
}

//////////////////////////////////////////////////////////////////////////
// Draw Instanced
//////////////////////////////////////////////////////////////////////////

PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = NULL;

void InitOpenGLExt_DrawInstanced(GLADloadproc load)
{
    if (GLVersion.major > 3 // Core in 3.1
        || (GLVersion.major == 3 && GLVersion.minor >= 1))
    {
        // Core

        LoadAndVerify("glDrawArraysInstanced", glDrawArraysInstanced, load);
        LoadAndVerify("glDrawElementsInstanced", glDrawElementsInstanced, load);
    }
    else if (HasExt("GL_ARB_draw_instanced"))
    {
        LoadAndVerify("glDrawArraysInstancedARB", glDrawArraysInstanced, load);
        LoadAndVerify("glDrawElementsInstancedARB", glDrawElementsInstanced, load);
    }
    else if (HasExt("GL_EXT_draw_instanced"))
    {
        LoadAndVerify("glDrawArraysInstancedEXT", glDrawArraysInstanced, load);
        LoadAndVerify("glDrawElementsInstancedEXT", glDrawElementsInstanced, load);
    }
    else
    {
        throw GameException("Instanced Drawing functionality is not supported");
    }
}

//////////////////////////////////////////////////////////////////////////
// VAO
//////////////////////////////////////////////////////////////////////////

PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLISVERTEXARRAYPROC glIsVertexArray = NULL;

void InitOpenGLExt_VertexArray(GLADloadproc load)
{
    if (GLVersion.major >= 3 // Core in 3.0
        || HasExt("GL_ARB_vertex_array_object"))
    {
        // Core or ARB - maintains name

        LoadAndVerify("glBindVertexArray", glBindVertexArray, load);
        LoadAndVerify("glDeleteVertexArrays", glDeleteVertexArrays, load);
        LoadAndVerify("glGenVertexArrays", glGenVertexArrays, load);
        LoadAndVerify("glIsVertexArray", glIsVertexArray, load);
    }
    else if (HasExt("GL_APPLE_vertex_array_object"))
    {
        LoadAndVerify("glBindVertexArrayAPPLE", glBindVertexArray, load);
        LoadAndVerify("glDeleteVertexArraysAPPLE", glDeleteVertexArrays, load);
        LoadAndVerify("glGenVertexArraysAPPLE", glGenVertexArrays, load);
        LoadAndVerify("glIsVertexArrayAPPLE", glIsVertexArray, load);
    }
    else
    {
        throw GameException("VAO functionality is not supported");
    }
}

//////////////////////////////////////////////////////////////////////////
// Texture Float (https://registry.khronos.org/OpenGL/extensions/ARB/ARB_texture_float.txt)
//////////////////////////////////////////////////////////////////////////

void InitOpenGLExt_TextureFloat(GLADloadproc /*load*/)
{
    if (GLVersion.major >= 3 // Core in 3.0
        || HasExt("GL_ARB_texture_float"))
    {
        // Core or ARB - maintains enumerants
    }
    else
    {
        throw GameException("Texture Float functionality is not supported");
    }
}

//////////////////////////////////////////////////////////////////////////
// Texture RG (https://registry.khronos.org/OpenGL/extensions/ARB/ARB_texture_rg.txt)
//////////////////////////////////////////////////////////////////////////

void InitOpenGLExt_TextureRG(GLADloadproc /*load*/)
{
    if (GLVersion.major >= 3 // Core in 3.0
        || HasExt("GL_ARB_texture_rg"))
    {
        // Core or ARB - maintains enumerants
    }
    else
    {
        throw GameException("Texture RG functionality is not supported");
    }
}

//////////////////////////////////////////////////////////////////////////
// Misc
//////////////////////////////////////////////////////////////////////////

PFNGLGETPROGRAMBINARYPROC glGetProgramBinary = NULL;
PFNGLDEBUGMESSAGECALLBACKARB glDebugMessageCallback = NULL;

void InitOpenGLExt_Misc(GLADloadproc load)
{
    if (GLVersion.major > 4 // Core in 4.1
        || (GLVersion.major == 4 && GLVersion.minor >= 1))
    {
        // Core

        LoadAndVerify("glGetProgramBinary", glGetProgramBinary, load);
    }
    else if (HasExt("GL_ARB_get_program_binary"))
    {
        LoadAndVerify("glGetProgramBinary", glGetProgramBinary, load);
    }
    else
    {
        // Ignore
    }

    if (HasExt("GL_ARB_debug_output"))
    {
        LoadAndVerify("glDebugMessageCallbackARB", glDebugMessageCallback, load);
    }
}

//////////////////////////////////////////////////////////////////////////
// Init
//////////////////////////////////////////////////////////////////////////

void InitOpenGLExt()
{
    try
    {
        if (open_gl())
        {
            if (get_exts())
            {
                InitOpenGLExt_Framebuffer(&get_proc);

                InitOpenGLExt_DrawInstanced(&get_proc);

                InitOpenGLExt_VertexArray(&get_proc);

                InitOpenGLExt_TextureFloat(&get_proc);

                InitOpenGLExt_TextureRG(&get_proc);

                InitOpenGLExt_Misc(&get_proc);

                free_exts();
            }

            close_gl();
        }
    }
    catch (std::exception const & ex)
    {
        throw  GameException(
            std::string("We are sorry, but this game requires OpenGL functionality which your graphics driver appears to not support;")
            + " the error is: " + ex.what());
    }
}