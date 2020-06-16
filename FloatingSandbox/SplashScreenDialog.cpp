/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SplashScreenDialog.h"

#include "GLCanvas.h"

#include <wx/generic/statbmpg.h>
#include <wx/settings.h>
#include <wx/sizer.h>

#include <cassert>

SplashScreenDialog::SplashScreenDialog(ResourceLocator const & resourceLocator)
{
	Create(
        nullptr, // Orphan
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxSize(800, 400),
        wxSTAY_ON_TOP | wxFRAME_NO_TASKBAR,
		_T("Splash Screen"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

    //
    // Create OpenGL canvas and context; we'll use this context
    // as the main OpenGL context of the game
    //

    wxGLCanvas * glCanvas = new GLCanvas(this, wxID_ANY);
    mainSizer->Add(glCanvas);

    // Make context
    mGLContext = std::make_shared<wxGLContext>(glCanvas);


    //
    // Create Image
    //

    wxBitmap* bmp = new wxBitmap(resourceLocator.GetArtFilepath("splash_screen").string(), wxBITMAP_TYPE_PNG);

    wxStaticBitmap * stBmp = new wxStaticBitmap(
        this,
        wxID_ANY,
        *bmp);

    mainSizer->Add(stBmp, 0, wxALIGN_CENTER);

    mainSizer->AddSpacer(5);


    //
    // Create Progress Bar
    //

    mGauge = new wxGauge(
        this,
        wxID_ANY,
        101,
        wxDefaultPosition,
        wxSize(bmp->GetWidth() - 20, 30),
        wxGA_HORIZONTAL);

    mainSizer->Add(mGauge, 1, wxALIGN_CENTER_HORIZONTAL);


	//
	// Create Text control
	//

    mProgressText = new wxStaticText(
        this,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(400, 20),
        wxALIGN_CENTER | wxBORDER_NONE);

	wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    mProgressText->SetFont(font);

    mainSizer->AddSpacer(2);
    mainSizer->Add(mProgressText, 0, wxALIGN_CENTER);


    //
    // Finalize dialog
    //

    SetSizerAndFit(mainSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);

    Show();
}

SplashScreenDialog::~SplashScreenDialog()
{
}

void SplashScreenDialog::UpdateProgress(
    float progress,
    std::string const & message)
{
    mGauge->SetValue(1 + static_cast<int>(100.0f * progress));

    mProgressText->SetLabelText(message);
}