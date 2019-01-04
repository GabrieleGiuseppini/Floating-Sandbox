/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ResourceLoader.h>

#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/stattext.h>

#include <memory>
#include <string>

class SplashScreenDialog : public wxDialog
{
public:

    SplashScreenDialog(ResourceLoader const & resourceLoader);

	virtual ~SplashScreenDialog();

	void UpdateProgress(
        float progress,
        std::string const & message);

private:

    wxGauge * mGauge;
    wxStaticText * mProgressText;
};
