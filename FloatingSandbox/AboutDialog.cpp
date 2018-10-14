/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-03-12
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AboutDialog.h"

#include "Version.h"

#include <wx/generic/statbmpg.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

wxBEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_CLOSE(AboutDialog::OnClose)
wxEND_EVENT_TABLE()

AboutDialog::AboutDialog(
    wxWindow * parent,
    ResourceLoader const & resourceLoader)
    : mParent(parent)
{
    Create(
        mParent,
        wxID_ANY,
        _("About " + GetVersionInfo(VersionFormat::Long)),
        wxDefaultPosition, 
        wxSize(760, 450),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP,
        _T("About Window"));

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->AddSpacer(5);


    //
    // Title
    //

    wxStaticText * titleLabel = new wxStaticText(this, wxID_ANY, _(""));
    titleLabel->SetLabelText(GetVersionInfo(VersionFormat::LongWithDate));
    titleLabel->SetFont(wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN)));
    mainSizer->Add(titleLabel, 0, wxALIGN_CENTRE);

    mainSizer->AddSpacer(1);

    wxStaticText * title2Label = new wxStaticText(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    title2Label->SetLabelText("Original concept (c) Luke Wren 2013\n(c) Gabriele Giuseppini 2018\nThis version licensed to Mattia");
    mainSizer->Add(title2Label, 0, wxALIGN_CENTRE);

    mainSizer->AddSpacer(5);


    //
    // Image
    //

    wxBitmap* bmp = new wxBitmap(resourceLoader.GetArtFilepath("splash_screen").string(), wxBITMAP_TYPE_PNG);

    wxStaticBitmap * stBmp = new wxStaticBitmap(this, wxID_ANY, *bmp, wxDefaultPosition, wxSize(400, 150), wxBORDER_SIMPLE);
    stBmp->SetScaleMode(wxStaticBitmap::Scale_AspectFill);

    mainSizer->Add(stBmp, 0, wxALIGN_CENTER);

    mainSizer->AddSpacer(5);


    //
    // Credits title
    //

    wxStaticText * creditsTitleLabel = new wxStaticText(this, wxID_ANY, _("Credits:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    creditsTitleLabel->SetFont(wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN)));
    mainSizer->Add(creditsTitleLabel, 0, wxALIGN_CENTRE);

    mainSizer->AddSpacer(2);


    //
    // Credits content
    //

    mCreditsPanel = new wxScrolled<wxPanel>(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(-1, -1),
        wxVSCROLL);


    //
    // Populate credits
    //

    std::vector<std::pair<std::string, std::string>> credits
    {
        {"Cover art:\t", "Dimitar Katsarov - https://www.artstation.com/stukata/profile" },
        {"Ship art:\t\t", "OceanLinerOrca - https://www.deviantart.com/oceanlinerorca" },
        {"Textures:\t", "Tune 'Prototstar' Katerungroch"},
        {"wxWidgets:\t", "Copyright (c) 1998-2005 Julian Smart, Robert Roebling et al - https://www.wxwidgets.org/"},
        {"SFML:\t\t", "Copyright (c) Laurent Gomila - https://www.sfml-dev.org/"},
        {"DevIL:\t\t", "Denton Woods et al - http://openil.sourceforge.net/" },
        {"picojson:\t", "Copyright (c) 2009-2010 Cybozu Labs, Inc.; Copyright (c) 2011-2014 Kazuho Oku - https://github.com/kazuho/picojson"},
        {"Bitmap Font Generator:\t\t", "Copyright (c) 2005-2011 Karl Walsh (Codehead) - http://www.codehead.co.uk/cbfg/" },
        {"OpenGL tutorial:\t", "Joey de Vries - https://learnopengl.com/" }
    };

    wxFont creditsTitleFont(wxFontInfo(8).Bold());
    wxFont creditsContentFont(wxFontInfo(8));

    wxFlexGridSizer * creditsSizer = new wxFlexGridSizer(4, 0, 2);
    for (auto const & credit : credits)
    {
        creditsSizer->AddSpacer(5);

        wxStaticText * credits1Label = new wxStaticText(mCreditsPanel, wxID_ANY, credit.first, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
        credits1Label->SetFont(creditsTitleFont);
        creditsSizer->Add(credits1Label, 0, wxALIGN_LEFT);

        creditsSizer->AddSpacer(5);

        wxStaticText * credits2Label = new wxStaticText(mCreditsPanel, wxID_ANY, credit.second, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
        credits2Label->SetFont(creditsContentFont);
        creditsSizer->Add(credits2Label, 1, wxALIGN_LEFT);
    }    

    mCreditsPanel->SetSizer(creditsSizer);
    mCreditsPanel->FitInside();
    mCreditsPanel->SetScrollRate(5, 5);

    mainSizer->Add(
        mCreditsPanel,
        1,                  // Proportion
        wxEXPAND | wxALL,
        4);                 // Border


    //
    // Finalize
    //

    SetSizer(mainSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::Open()
{
    this->ShowModal();
}

void AboutDialog::OnClose(wxCloseEvent & event)
{
    event.Skip();
}

