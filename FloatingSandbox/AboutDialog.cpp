/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-03-12
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AboutDialog.h"

#include <GameCore/Version.h>

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
        wxSize(780, 620),
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
    title2Label->SetLabelText("Original concept (c) Luke Wren, Francis Racicot (Pac0master) 2013\n(c) Gabriele Giuseppini 2018-2019\nThis version licensed to Mattia");
    mainSizer->Add(title2Label, 0, wxALIGN_CENTRE);

    mainSizer->AddSpacer(5);


    //
    // Image
    //

    wxBitmap* bmp = new wxBitmap(resourceLoader.GetArtFilepath("splash_screen").string(), wxBITMAP_TYPE_PNG);

    wxStaticBitmap * stBmp = new wxStaticBitmap(this, wxID_ANY, *bmp, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);

    mainSizer->Add(stBmp, 1, wxALIGN_CENTER_HORIZONTAL);

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
        {"Ship engineers:", "TopHatLemons - https://discordapp.com/" },
        {"\t\t\t\t", "Truce#3326 - https://discordapp.com/" },
        {"\t\t\t\t", "RetroGraczzPL - https://discordapp.com/" },
        {"\t\t\t\t", "SS Nomadavid - https://discordapp.com/" },
        {"\t\t\t\t", "Wreno - https://discordapp.com/" },
        {"\t\t\t\t", "Pac0master - https://discordapp.com/" },
        {"\t\t\t\t", "CorbinPasta93 - https://discordapp.com/" },
        {"\t\t\t\t", "Yorkie - https://discordapp.com/" },
        {"\t\t\t\t", "Artica - https://discordapp.com/" },
        {"\t\t\t\t", "KikoTheBoatBuilder - https://discordapp.com/" },
        {"\t\t\t\t", "Albert Windsor - https://discordapp.com/" },
        {"\t\t\t\t", "ShipBuilder1912 - https://discordapp.com/" },
        {"\t\t\t\t", "Michael Bozarth - https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw" },
        {"\t\t\t\t", "Rockabilly Rebel - https://discordapp.com/" },
        {"\t\t\t\t", "McShooter2018 - https://discordapp.com/" },
        {"\t\t\t\t", "Dumbphones - https://discordapp.com/" },
        {"\t\t\t\t", "NotTelling - https://discordapp.com/" },
        {"\t\t\t\t", "Hugo_2503 - https://discordapp.com/" },
        {"\t\t\t\t", "_ASTYuu_ - https://discordapp.com/" },
        {"\t\t\t\t", "Serhiiiihres - https://discordapp.com/" },
        {"\t\t\t\t", "JackTheBrickfilmMaker - https://www.youtube.com/channel/UCshPbiTqFuwpNNh7BlpffhQ" },
        {"\t\t\t\t", "Pandadude12345 - https://discordapp.com/" },
        {"\t\t\t\t", "John Smith - https://discordapp.com/" },
        {"\t\t\t\t", "Dkuz - https://discordapp.com/" },
        {"\t\t\t\t", "Loree - https://discordapp.com/" },
        {"\t\t\t\t", "Daewoom - https://discordapp.com/" },
        {"\t\t\t\t", "Aqua - https://discordapp.com/" },
        {"\t\t\t\t", "MasterGarfield - https://discordapp.com/" },
        {"\t\t\t\t", "Darek225 - https://www.youtube.com/channel/UC5l6t4P8NLA8n81XdX6yl6w" },

        {"Ship art:\t\t", "OceanLinerOrca - https://www.deviantart.com/oceanlinerorca" },

        {"Chief ship literature officer:", "Maximord - https://discordapp.com/" },

        {"Music:\t\t", "Dario Bazzichetto (Soul Heater) - https://soundcloud.com/soul-heater" },
        {"\t\t\t\t", "Michael Bozarth - https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw" },

        {"Testing:\t\t", "Pac0master - https://discordapp.com/" },
        {"\t\t\t\t", "Maximord - https://discordapp.com/" },
        {"\t\t\t\t", "The_SamminAter - https://discordapp.com/" },
        {"\t\t\t\t", "McShooter2018 - https://discordapp.com/" },

        {"Webmaster:\t\t", "Maximord - https://discordapp.com/" },

        {"", ""},

        {"Textures:\t", "Tune 'Prototstar' Katerungroch"},
        {"wxWidgets:\t", "Copyright (c) 1998-2005 Julian Smart, Robert Roebling et al - https://www.wxwidgets.org/"},
        {"SFML:\t\t", "Copyright (c) Laurent Gomila - https://www.sfml-dev.org/"},
        {"DevIL:\t\t", "Denton Woods et al - http://openil.sourceforge.net/" },
        {"picojson:\t", "Copyright (c) 2009-2010 Cybozu Labs, Inc.; Copyright (c) 2011-2014 Kazuho Oku - https://github.com/kazuho/picojson"},
        {"Bitmap Font Generator:\t\t", "Copyright (c) 2005-2011 Karl Walsh (Codehead) - http://www.codehead.co.uk/cbfg/" },
        {"OpenGL tutorial:\t", "Joey de Vries - https://learnopengl.com/" },
        {"Fast approx:\t", "Copyright (c) 2011 Paul Mineiro - http://www.machinedlearnings.com/"}
    };

    wxFont creditsTitleFont(wxFontInfo(8).Bold());
    wxFont creditsContentFont(wxFontInfo(8));

    wxFlexGridSizer * creditsSizer = new wxFlexGridSizer(4, 0, 2);
    for (auto const & credit : credits)
    {
        if (credit.first.empty() && credit.second.empty())
        {
            // Spacer
            creditsSizer->AddSpacer(5);
            creditsSizer->AddSpacer(5);
            creditsSizer->AddSpacer(5);
            creditsSizer->AddSpacer(5);
        }
        else
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