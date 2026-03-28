/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameGLCanvas.h"

#include <Core/GameExceptions.h>
#include <Core/Log.h>

GameGLCanvas * GameGLCanvas::Create(
    wxWindow * parent,
    wxWindowID id,
    bool doForceNoMultiSampling)
{
    wxGLAttributes ssAttributes;
    ssAttributes
        .RGBA()
        .DoubleBuffer()
        .Depth(16);

    wxGLAttributes msAttributes = ssAttributes;
    msAttributes
        .SampleBuffers(1)
        .Samplers(4) // Sic
        .EndList();

    ssAttributes.EndList();

    if (!doForceNoMultiSampling && wxGLCanvas::IsDisplaySupported(msAttributes))
    {
        return new GameGLCanvas(parent, id, msAttributes, true);
    }
    else if (wxGLCanvas::IsDisplaySupported(ssAttributes))
    {
        return new GameGLCanvas(parent, id, ssAttributes, false);
    }
    else
    {
        throw GameException("We are sorry, but this game requires OpenGL functionality which your graphics driver appears to not support; cannot find a suitable pixel format");
    }
}

GameGLCanvas::GameGLCanvas(
    wxWindow * parent,
    wxWindowID id,
    wxGLAttributes const & glAttributes,
    bool isMultisamplingSupported)
    : wxGLCanvas(
        parent,
        glAttributes,
        id,
        wxDefaultPosition,
        wxDefaultSize,
        0L)
    , mIsMultisamplingSupported(isMultisamplingSupported)
{
    LogMessage("GameGLCanvas(", id, "): IsMultisamplingSupported=", mIsMultisamplingSupported, " ContentScaleFactor = ", this->GetContentScaleFactor(), " DPIScaleFactor = ", this->GetDPIScaleFactor(),
        " 100x100dip=", this->FromDIP(wxSize(100, 100)).GetWidth(), "x", this->FromDIP(wxSize(100, 100)).GetHeight(), "px");
}

GameGLCanvas::~GameGLCanvas()
{
    LogMessage("GameGLCanvas::~GameGLCanvas()");
}
