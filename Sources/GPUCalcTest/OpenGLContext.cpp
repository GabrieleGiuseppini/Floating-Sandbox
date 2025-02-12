/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-04
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "OpenGLContext.h"

#include <wx/window.h>

#include <cassert>

OpenGLContext::OpenGLContext()
{
    //
    // Create dummy window
    //

    mFrame = std::make_unique<wxFrame>(
        nullptr, // No parent
        wxID_ANY,
        "OpenGLContext Dummy Frame",
        wxDefaultPosition,
        wxSize(100, 100),
        wxSTAY_ON_TOP);


    //
    // Build GL canvas
    //

    // Note: Using the wxWidgets 3.1 style does not work on OpenGL 4 drivers; it forces a 1.1.0 context

    int glCanvasAttributes[] =
    {
        WX_GL_RGBA,
        WX_GL_DEPTH_SIZE,      16,
        WX_GL_STENCIL_SIZE,    1,
        0, 0
    };

    mGLCanvas = std::make_unique<wxGLCanvas>(
        mFrame.get(),
        wxID_ANY,
        glCanvasAttributes,
        wxDefaultPosition,
        wxSize(100, 100),
        0L);

    // Take context for this canvas
    mGLContext = std::make_unique<wxGLContext>(mGLCanvas.get());
}

OpenGLContext::~OpenGLContext()
{
    assert(!!mFrame);
    mFrame->Destroy();
}

void OpenGLContext::Activate()
{
    mGLContext->SetCurrent(*mGLCanvas);
}