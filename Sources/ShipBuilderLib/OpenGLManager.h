/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-01-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <OpenGLCore/GameOpenGL.h>

#include <wx/glcanvas.h> // Need to include this *after* our glad.h has been included, so that wxGLCanvas ends
                         // up *not* including the system's OpenGL header but glad's instead

#include <memory>

namespace ShipBuilder {

/*
 * Placeholder to make ownership of OpenGL contexts explicit.
 */
struct OpenGLContext final
{
public:

    OpenGLContext(std::unique_ptr<wxGLContext> glContext)
        : mGLContext(std::move(glContext))
    {}

private:

    std::unique_ptr<wxGLContext> mGLContext;
};

/*
 * Manages OpenGL - its initialization and the lifetime of contexts.
 *
 * Constraints:
 *  - OpenGL must be initialized *after* a context has been created
 */
class OpenGLManager final
{
public:

    OpenGLManager(
        wxGLCanvas & glCanvas,
        bool doOpenGLInitialization)
        : mGLCanvas(glCanvas)
        , mNeedToInitializeOpenGL(doOpenGLInitialization)
    {}

    std::unique_ptr<OpenGLContext> MakeContextAndMakeCurrent()
    {
        auto glContext = std::make_unique<wxGLContext>(&mGLCanvas);
        glContext->SetCurrent(mGLCanvas);

        if (mNeedToInitializeOpenGL)
        {
            // First context has been created, initialize OpenGL now
            GameOpenGL::InitOpenGL();

            mNeedToInitializeOpenGL = false;
        }

        return std::make_unique<OpenGLContext>(std::move(glContext));
    }

private:

    wxGLCanvas & mGLCanvas;
    bool mNeedToInitializeOpenGL;
};

}
