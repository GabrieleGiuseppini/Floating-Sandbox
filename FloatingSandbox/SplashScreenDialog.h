/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GLCanvas.h"

#include <Game/ResourceLocator.h>

#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/stattext.h>

#include <cassert>
#include <memory>
#include <string>

class SplashScreenDialog : public wxFrame
{
public:

	SplashScreenDialog(ResourceLocator const & resourceLocator);

	virtual ~SplashScreenDialog();

	/*
	 * We also create and export a (temporary) OpenGL canvas,
	 * which may be used for binding an OpenGL context to while
	 * the main frame's canvas is still hiddden
	 */
	GLCanvas * GetOpenGLCanvas() const
	{
		assert(nullptr != mGLCanvas);
		return mGLCanvas;
	}

	void UpdateProgress(
		float progress,
		std::string const & message);

private:

	void OnPaint(wxPaintEvent & event);

private:

	GLCanvas * mGLCanvas;
	wxGauge * mGauge;
	wxStaticText * mProgressText;
};
