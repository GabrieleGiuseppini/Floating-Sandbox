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
class GameGLCanvas final : public wxGLCanvas
{
public:

    // Throws if cannot find a suitable pixel format
    static GameGLCanvas * Create(
        wxWindow * parent,
        wxWindowID id);

    ~GameGLCanvas();

    bool GetIsMultisamplingSupported() const
    {
        return mIsMultisamplingSupported;
    }

private:

    GameGLCanvas(
        wxWindow * parent,
        wxWindowID id,
        wxGLAttributes const & glAttributes,
        bool isMultisamplingSupported);

    bool const mIsMultisamplingSupported;
};
