/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SplashScreenDialog.h"

#include <wx/generic/statbmpg.h>
#include <wx/settings.h>
#include <wx/sizer.h>

#include <cassert>

SplashScreenDialog::SplashScreenDialog(ResourceLoader const & resourceLoader)
{
	Create(
        nullptr,
		wxID_ANY,
		wxEmptyString, 
		wxDefaultPosition, 
		wxSize(800, 400),
        wxSTAY_ON_TOP,
		_T("Splash Screen"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Create Image
    //

    wxBitmap* bmp = new wxBitmap(resourceLoader.GetArtFilepath("splash_screen").string(), wxBITMAP_TYPE_PNG);

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
        wxSize(400, 30),
        wxGA_HORIZONTAL);
    
    mainSizer->Add(mGauge, 0, wxALIGN_CENTER);


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
