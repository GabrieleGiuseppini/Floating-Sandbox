/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameOpenGL_Ext.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

template <typename TFunc>
bool LoadEx(char * const functionName, TFunc * & pFunc, GLADloadproc load)
{
    pFunc = (TFunc *)load(functionName);

    LogMessage("Has ", functionName, ": ", nullptr != pFunc ? "YES" : "NO");

    return nullptr != pFunc;
}

bool HasExtEx(char * const extensionName)
{
    bool result = has_ext(extensionName);

    LogMessage("Has ", extensionName, ": ", result ? "YES" : "NO");

    return result;
}

//////////////////////////////////////////////////////////////////////////
// Framebuffer
//////////////////////////////////////////////////////////////////////////

std::unique_ptr<OpenGLExt_Framebuffer_Core const> gglFramebuffer_Core;
std::unique_ptr<OpenGLExt_Framebuffer_EXT const> gglFramebuffer_EXT;

void InitOpenGLExt_Framebuffer(GLADloadproc load)
{
    {
        // Core

        OpenGLExt_Framebuffer_Core * ps = new OpenGLExt_Framebuffer_Core;

        bool isComplete = true;

        isComplete &= LoadEx("glIsRenderbuffer", ps->IsRenderbuffer, load);
        isComplete &= LoadEx("glBindRenderbuffer", ps->BindRenderbuffer, load);
        isComplete &= LoadEx("glDeleteRenderbuffers", ps->DeleteRenderbuffers, load);
        isComplete &= LoadEx("glGenRenderbuffers", ps->GenRenderbuffers, load);
        isComplete &= LoadEx("glRenderbufferStorage", ps->RenderbufferStorage, load);
        isComplete &= LoadEx("glGetRenderbufferParameteriv", ps->GetRenderbufferParameteriv, load);
        isComplete &= LoadEx("glIsFramebuffer", ps->IsFramebuffer, load);
        isComplete &= LoadEx("glBindFramebuffer", ps->BindFramebuffer, load);
        isComplete &= LoadEx("glDeleteFramebuffers", ps->DeleteFramebuffers, load);
        isComplete &= LoadEx("glGenFramebuffers", ps->GenFramebuffers, load);
        isComplete &= LoadEx("glCheckFramebufferStatus", ps->CheckFramebufferStatus, load);
        isComplete &= LoadEx("glFramebufferTexture1D", ps->FramebufferTexture1D, load);
        isComplete &= LoadEx("glFramebufferTexture2D", ps->FramebufferTexture2D, load);
        isComplete &= LoadEx("glFramebufferTexture3D", ps->FramebufferTexture3D, load);
        isComplete &= LoadEx("glFramebufferRenderbuffer", ps->FramebufferRenderbuffer, load);
        isComplete &= LoadEx("glGetFramebufferAttachmentParameteriv", ps->GetFramebufferAttachmentParameteriv, load);

        if (isComplete)
            gglFramebuffer_Core.reset(ps);
        else
            delete ps;
    }

    if (HasExtEx("GL_EXT_framebuffer_object"))
    {
        // Ext

        OpenGLExt_Framebuffer_EXT * ps = new OpenGLExt_Framebuffer_EXT;

        bool isComplete = true;

        isComplete &= LoadEx("glIsRenderbufferEXT", ps->IsRenderbufferEXT, load);
        isComplete &= LoadEx("glBindRenderbufferEXT", ps->BindRenderbufferEXT, load);
        isComplete &= LoadEx("glDeleteRenderbuffersEXT", ps->DeleteRenderbuffersEXT, load);
        isComplete &= LoadEx("glGenRenderbuffersEXT", ps->GenRenderbuffersEXT, load);
        isComplete &= LoadEx("glRenderbufferStorageEXT", ps->RenderbufferStorageEXT, load);
        isComplete &= LoadEx("glGetRenderbufferParameterivEXT", ps->GetRenderbufferParameterivEXT, load);
        isComplete &= LoadEx("glIsFramebufferEXT", ps->IsFramebufferEXT, load);
        isComplete &= LoadEx("glBindFramebufferEXT", ps->BindFramebufferEXT, load);
        isComplete &= LoadEx("glDeleteFramebuffersEXT", ps->DeleteFramebuffersEXT, load);
        isComplete &= LoadEx("glGenFramebuffersEXT", ps->GenFramebuffersEXT, load);
        isComplete &= LoadEx("glCheckFramebufferStatusEXT", ps->CheckFramebufferStatusEXT, load);
        isComplete &= LoadEx("glFramebufferTexture1DEXT", ps->FramebufferTexture1DEXT, load);
        isComplete &= LoadEx("glFramebufferTexture2DEXT", ps->FramebufferTexture2DEXT, load);
        isComplete &= LoadEx("glFramebufferTexture3DEXT", ps->FramebufferTexture3DEXT, load);
        isComplete &= LoadEx("glFramebufferRenderbufferEXT", ps->FramebufferRenderbufferEXT, load);
        isComplete &= LoadEx("glGetFramebufferAttachmentParameterivEXT", ps->GetFramebufferAttachmentParameterivEXT, load);

        if (isComplete)
            gglFramebuffer_EXT.reset(ps);
        else
            delete ps;
    }
}

//////////////////////////////////////////////////////////////////////////
// Draw Instanced
//////////////////////////////////////////////////////////////////////////

std::unique_ptr<OpenGLExt_DrawInstanced_Core const> gglDrawInstanced_Core;
std::unique_ptr<OpenGLExt_DrawInstanced_ARB const> gglDrawInstanced_ARB;
std::unique_ptr<OpenGLExt_DrawInstanced_EXT const> gglDrawInstanced_EXT;

void InitOpenGLExt_DrawInstanced(GLADloadproc load)
{
    {
        // Core

        OpenGLExt_DrawInstanced_Core * ps = new OpenGLExt_DrawInstanced_Core;

        bool isComplete = true;

        isComplete &= LoadEx("glDrawArraysInstanced", ps->DrawArraysInstanced, load);
        isComplete &= LoadEx("glDrawElementsInstanced", ps->DrawElementsInstanced, load);

        if (isComplete)
            gglDrawInstanced_Core.reset(ps);
        else
            delete ps;
    }

    if (HasExtEx("GL_ARB_draw_instanced"))
    {
        // ARB

        OpenGLExt_DrawInstanced_ARB * ps = new OpenGLExt_DrawInstanced_ARB;

        bool isComplete = true;

        isComplete &= LoadEx("glDrawArraysInstancedARB", ps->DrawArraysInstancedARB, load);
        isComplete &= LoadEx("glDrawElementsInstancedARB", ps->DrawElementsInstancedARB, load);

        if (isComplete)
            gglDrawInstanced_ARB.reset(ps);
        else
            delete ps;
    }

    if (HasExtEx("GL_EXT_draw_instanced"))
    {
        // EXT

        OpenGLExt_DrawInstanced_EXT * ps = new OpenGLExt_DrawInstanced_EXT;

        bool isComplete = true;

        isComplete &= LoadEx("glDrawArraysInstancedEXT", ps->DrawArraysInstancedEXT, load);
        isComplete &= LoadEx("glDrawElementsInstancedEXT", ps->DrawElementsInstancedEXT, load);

        if (isComplete)
            gglDrawInstanced_EXT.reset(ps);
        else
            delete ps;
    }
}

//////////////////////////////////////////////////////////////////////////
// VAO
//////////////////////////////////////////////////////////////////////////

std::unique_ptr<OpenGLExt_VertexArray_Core const> gglVertexArray_Core;

void InitOpenGLExt_VertexArray(GLADloadproc load)
{
    {
        // Core

        OpenGLExt_VertexArray_Core * ps = new OpenGLExt_VertexArray_Core;

        bool isComplete = true;

        isComplete &= LoadEx("glBindVertexArray", ps->BindVertexArray, load);
        isComplete &= LoadEx("glDeleteVertexArrays", ps->DeleteVertexArrays, load);
        isComplete &= LoadEx("glGenVertexArrays", ps->GenVertexArrays, load);
        isComplete &= LoadEx("glIsVertexArray", ps->IsVertexArray, load);

        if (isComplete)
            gglVertexArray_Core.reset(ps);
        else
            delete ps;
    }
}

//////////////////////////////////////////////////////////////////////////
// Texture Float
//////////////////////////////////////////////////////////////////////////

std::unique_ptr<OpenGLExt_TextureFloat_Core const> gglTextureFloat_Core;
std::unique_ptr<OpenGLExt_TextureFloat_ARB const> gglTextureFloat_ARB;

void InitOpenGLExt_TextureFloat(GLADloadproc /*load*/)
{
    if (GLVersion.major >= 3) // Core in 3.0
    {
        // Core

        OpenGLExt_TextureFloat_Core * ps = new OpenGLExt_TextureFloat_Core;

        gglTextureFloat_Core.reset(ps);
    }

    if (HasExtEx("GL_ARB_texture_float"))
    {
        // ARB

        OpenGLExt_TextureFloat_ARB * ps = new OpenGLExt_TextureFloat_ARB;

        gglTextureFloat_ARB.reset(ps);
    }
}

//////////////////////////////////////////////////////////////////////////
// Init
//////////////////////////////////////////////////////////////////////////

void InitOpenGLExt()
{
    if (open_gl())
    {
        if (get_exts())
        {
            InitOpenGLExt_Framebuffer(&get_proc);

            InitOpenGLExt_DrawInstanced(&get_proc);

            InitOpenGLExt_VertexArray(&get_proc);

            InitOpenGLExt_TextureFloat(&get_proc);

            free_exts();
        }

        close_gl();
    }

    //
    // Check completeness
    //

    if (!gglFramebuffer_Core && !gglFramebuffer_EXT)
    {
        throw GameException("Sorry, but this game requires Framebuffer OpenGL support; it appears your computer doesn't have any");
    }

    if (!gglDrawInstanced_Core && !gglDrawInstanced_ARB && !gglDrawInstanced_EXT)
    {
        throw GameException("Sorry, but this game requires Instanced Drawing OpenGL support; it appears your computer doesn't have any");
    }

    if (!gglVertexArray_Core)
    {
        throw GameException("Sorry, but this game requires Vertex Array OpenGL support; it appears your computer doesn't have any");
    }

    if (!gglTextureFloat_Core && !gglTextureFloat_ARB)
    {
        throw GameException("Sorry, but this game requires Texture Float OpenGL support; it appears your computer doesn't have any");
    }
}