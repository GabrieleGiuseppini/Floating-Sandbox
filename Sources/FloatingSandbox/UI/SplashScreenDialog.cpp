/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SplashScreenDialog.h"

#include <Game/GameVersion.h>

#include <Core/GameException.h>
#include <Core/Log.h>

#include <wx/generic/statbmpg.h>
#include <wx/settings.h>
#include <wx/sizer.h>

#include <cassert>

SplashScreenDialog::SplashScreenDialog(GameAssetManager const & gameAssetManager)
{
	Create(
        nullptr, // Orphan
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxSize(800, 400),
        wxSTAY_ON_TOP | wxFRAME_NO_TASKBAR,
		"Splash Screen");

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    Bind(wxEVT_PAINT, &SplashScreenDialog::OnPaint, this);

    wxBoxSizer * mainVSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Create Image
    //

    wxBitmap * bmp;

    {
        bmp = new wxBitmap(gameAssetManager.GetArtFilePath("splash_screen").string(), wxBITMAP_TYPE_PNG);
        if (!bmp->IsOk())
        {
            // This is likely to be the first resource load of the game; if it's broken,
            // then the game will likely be utterly broken, so bail out now
            throw GameException("Cannot load splash screen. The installation is likely corrupted, please repair the game by running the installer again.");
        }

        wxStaticBitmap * stBmp = new wxStaticBitmap(
            this,
            wxID_ANY,
            *bmp);

        mainVSizer->Add(stBmp, 0, wxALIGN_CENTER);
    }

    mainVSizer->AddSpacer(4);

    //
    // Create OpenGL canvas
    //

    {
        mGLCanvas = new GLCanvas(this, wxID_ANY);

        mainVSizer->Add(mGLCanvas);
    }

    //
    // Create Progress Bar
    //

    {
        mGauge = new wxGauge(
            this,
            wxID_ANY,
            100,
            wxDefaultPosition,
            wxSize(bmp->GetWidth() - 20, 30),
            wxGA_HORIZONTAL | wxGA_PROGRESS);

        mainVSizer->Add(mGauge, 1, wxALIGN_CENTER_HORIZONTAL);
    }

    mainVSizer->AddSpacer(2);

    //
    // Bottom row
    //

    {
        int constexpr WingWidth = 80;

        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        {
            hSizer->AddSpacer(WingWidth);
        }

        {
            mProgressText = new wxStaticText(
                this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(400, 20),
                wxALIGN_CENTER | wxBORDER_NONE);

            wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            mProgressText->SetFont(font);

            hSizer->Add(mProgressText, 1, wxALIGN_CENTER);
        }

        {
            auto * versionText = new wxStaticText(
                this,
                wxID_ANY,
                APPLICATION_VERSION_LONG_STR,
                wxDefaultPosition,
                wxSize(WingWidth, -1),
                wxALIGN_RIGHT | wxBORDER_NONE);

            wxFont font(7, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            versionText->SetFont(font);

            hSizer->Add(versionText, 0, wxALIGN_BOTTOM);
        }

        mainVSizer->Add(hSizer, 0, wxALIGN_CENTER);
    }

    //
    // Populate progress strings
    //

    {
        mProgressStrings.Add(wxEmptyString);
        mProgressStrings.Add(_("Loading fonts..."));
        mProgressStrings.Add(_("Initializing OpenGL..."));
        mProgressStrings.Add(_("Loading shaders..."));
        mProgressStrings.Add(_("Initializing noise..."));
        mProgressStrings.Add(_("Loading generic textures..."));
        mProgressStrings.Add(_("Loading explosion texture atlas..."));
        mProgressStrings.Add(_("Loading cloud texture atlas..."));
        mProgressStrings.Add(_("Loading fish texture atlas..."));
        mProgressStrings.Add(_("Loading world textures..."));
        mProgressStrings.Add(_("Initializing graphics..."));
        mProgressStrings.Add(_("Initializing UI..."));
        mProgressStrings.Add(_("Loading sounds..."));
        mProgressStrings.Add(_("Loading music..."));
        mProgressStrings.Add(_("Loading electrical panel..."));
        mProgressStrings.Add(_("Loading ShipBuilder..."));
        mProgressStrings.Add(_("Loading materials palette..."));
        mProgressStrings.Add(_("Calibrating game on the computer..."));
        mProgressStrings.Add(_("Ready!"));

        assert(mProgressStrings.GetCount() == static_cast<size_t>(ProgressMessageType::_Last) + 1);
    }

    //
    // Finalize dialog
    //

    SetSizerAndFit(mainVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);

    Show();

    LogMessage("SplashScreenDialog::Show(): Completed");
}

SplashScreenDialog::~SplashScreenDialog()
{
}

void SplashScreenDialog::UpdateProgress(
    float progress,
    ProgressMessageType message)
{
    mGauge->SetValue(static_cast<int>(100.0f * progress));

    assert(static_cast<size_t>(message) < mProgressStrings.GetCount());

    mProgressText->SetLabelText(mProgressStrings[static_cast<size_t>(message)]);
}

void SplashScreenDialog::OnPaint(wxPaintEvent & event)
{
    event.Skip();
}
