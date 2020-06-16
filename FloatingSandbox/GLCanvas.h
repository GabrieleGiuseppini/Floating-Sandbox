/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/glcanvas.h>

/*
 * Our own GLCanvas that comes with the attributes we require for canvasses.
 * It allows us to have multiple canvasses which may share the same OpenGL context.
 */
class GLCanvas : public wxGLCanvas
{
public:

    GLCanvas(
        wxWindow * parent,
        wxWindowID id)
        : wxGLCanvas(
            parent,
            id,
            mMainGLCanvasAttributes,
            wxDefaultPosition,
            wxDefaultSize,
            0L)
    {
    }

private:

    // Note: Using the wxWidgets 3.1 style does not work on OpenGL 4 drivers; it forces a 1.1.0 context

    int mMainGLCanvasAttributes[6] {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        WX_GL_DEPTH_SIZE,      16,

        // Cannot specify CORE_PROFILE or else wx tries OpenGL 3.0 and fails if it's not supported
        //WX_GL_CORE_PROFILE,

        // Useless to specify version as Glad will always take the max
        //WX_GL_MAJOR_VERSION,    GameOpenGL::MinOpenGLVersionMaj,
        //WX_GL_MINOR_VERSION,    GameOpenGL::MinOpenGLVersionMin,

        0, 0
    };
};
