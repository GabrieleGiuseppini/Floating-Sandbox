/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-04
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GPUCalc/IOpenGLContext.h>

#include <wx/frame.h>
#include <wx/glcanvas.h>

#include <memory>

/*
 * Implementation of the IOpenGLContext interface for an OpenGL context
 * created with wxWidgets.
 */
class OpenGLContext : public IOpenGLContext
{
public:

    OpenGLContext();
    ~OpenGLContext();

public:

    void Activate() override;

private:

    std::unique_ptr<wxFrame> mFrame;
    std::unique_ptr<wxGLCanvas> mGLCanvas;
    std::unique_ptr<wxGLContext> mGLContext;
};