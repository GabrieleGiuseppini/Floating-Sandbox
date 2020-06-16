/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/glcanvas.h>
#include <wx/stattext.h>

#include <memory>
#include <string>

class SplashScreenDialog : public wxFrame
{
public:

    SplashScreenDialog(ResourceLocator const & resourceLocator);

	virtual ~SplashScreenDialog();

    std::shared_ptr<wxGLContext> GetOpenGLContext() const
    {
        return mGLContext;
    }

	void UpdateProgress(
        float progress,
        std::string const & message);

private:

    std::shared_ptr<wxGLContext> mGLContext;
    wxGauge * mGauge;
    wxStaticText * mProgressText;
};
