/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-03-12
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AboutDialog.h"

#include <GameCore/Version.h>

#include <wx/generic/statbmpg.h>
#include <wx/hyperlink.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

wxBEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_CLOSE(AboutDialog::OnClose)
wxEND_EVENT_TABLE()

AboutDialog::AboutDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mParent(parent)
{
    Create(
        mParent,
        wxID_ANY,
        _("About " + std::string(APPLICATION_NAME_WITH_SHORT_VERSION)),
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
    titleLabel->SetLabelText(std::string(APPLICATION_NAME_WITH_LONG_VERSION " (" __DATE__ ")"));
    titleLabel->SetFont(wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN)));
    mainSizer->Add(titleLabel, 0, wxALIGN_CENTRE);

    mainSizer->AddSpacer(1);

    wxStaticText * title2Label = new wxStaticText(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    title2Label->SetLabelText(
        "Original concept (c) Luke Wren, Francis Racicot (Pac0master) 2013\n"
        "(c) Gabriele Giuseppini 2018-2020\n"
        "This version licensed to Mattia, Elia, and all the others kids in the world");
    mainSizer->Add(title2Label, 0, wxALIGN_CENTRE);

    mainSizer->AddSpacer(5);


    //
    // Image
    //

    wxBitmap bmp(resourceLocator.GetArtFilepath("splash_screen").string(), wxBITMAP_TYPE_PNG);

    wxStaticBitmap * stBmp = new wxStaticBitmap(this, wxID_ANY, bmp, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);

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

    std::vector<std::tuple<std::string, std::string, std::string>> credits
    {
        {"Ship engineers:", "TopHatLemons", "" },
        {"\t\t\t\t", "Truce#3326", "" },
        {"\t\t\t\t", "RetroGraczzPL", "" },
        {"\t\t\t\t", "Nomadavid", "" },
        {"\t\t\t\t", "Wreno", "" },
        {"\t\t\t\t", "Pac0master", "" },
        {"\t\t\t\t", "CorbinPasta93", "" },
        {"\t\t\t\t", "Yorkie", "" },
        {"\t\t\t\t", "Bluefox", "" },
        {"\t\t\t\t", "KikoTheBoatBuilder", "" },
        {"\t\t\t\t", "Albert Windsor", "" },
        {"\t\t\t\t", "ShipBuilder1912", "" },
        {"\t\t\t\t", "Michael Bozarth", "https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw" },
        {"\t\t\t\t", "Rockabilly Rebel", "" },
        {"\t\t\t\t", "McShooter2018", "" },
        {"\t\t\t\t", "Dumbphones", "" },
        {"\t\t\t\t", "NotTelling", "" },
        {"\t\t\t\t", "Hugo_2503", "" },
        {"\t\t\t\t", "_ASTYuu_", "" },
        {"\t\t\t\t", "Serhiiiihres", "" },
        {"\t\t\t\t", "JackTheBrickfilmMaker", "https://www.youtube.com/channel/UCshPbiTqFuwpNNh7BlpffhQ" },
        {"\t\t\t\t", "Pandadude12345", "" },
        {"\t\t\t\t", "John Smith", "" },
        {"\t\t\t\t", "Dkuz", "" },
        {"\t\t\t\t", "Loree", "" },
        {"\t\t\t\t", "Daewoom", "" },
        {"\t\t\t\t", "Aqua", "" },
        {"\t\t\t\t", "MasterGarfield", "" },
        {"\t\t\t\t", "Darek225", "https://www.youtube.com/channel/UC5l6t4P8NLA8n81XdX6yl6w" },
        {"\t\t\t\t", "Aur\xe9lien WOLFF", "" },
        {"\t\t\t\t", "HummeL", "https://www.youtube.com/c/HummeL_Prog" },
        {"\t\t\t\t", "Alex di Roma", "" },
        {"\t\t\t\t", "Fox Assor", "https://vk.com/id448121270" },
        {"\t\t\t\t", "Officer TimCan", "https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg" },
        {"\t\t\t\t", "2017 Leonardo", "" },
        {"\t\t\t\t", "sinking_feeling", "" },

        {"Music:\t\t", "\"The Short Journey to the Seabed\", Soul Heater", "https://soundcloud.com/soul-heater" },
        {"\t\t\t\t", "> Licensed under Creative Commons: By Attribution 4.0 License", "https://creativecommons.org/licenses/by/4.0/" },
        {"\t\t\t\t", "\"Long Note Four\", Kevin MacLeod", "https://incompetech.com" },
        {"\t\t\t\t", "> Licensed under Creative Commons: By Attribution 4.0 License", "https://creativecommons.org/licenses/by/4.0/" },
        {"\t\t\t\t", "\"Symmetry\", Kevin MacLeod", "https://incompetech.com" },
        {"\t\t\t\t", "> Licensed under Creative Commons: By Attribution 4.0 License", "https://creativecommons.org/licenses/by/4.0/" },
        {"\t\t\t\t", "Michael Bozarth; Stuart's Piano World", "https://stuartspianoworld.com/" },
        {"\t\t\t\t", "Officer TimCan", "https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg" },

        {"Testing:\t\t", "Pac0master", "" },
        {"\t\t\t\t", "McShooter2018", "" },
        {"\t\t\t\t", "Wreno", "" },
        {"\t\t\t\t", "Dkuz", "" },
        {"\t\t\t\t", "Michael Bozarth", "https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw" },
		{"\t\t\t\t", "_ASTYuu_", "" },
		{"\t\t\t\t", "sinking_feeling", "" },
		{"\t\t\t\t", "Officer TimCan", "https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg" },
        {"\t\t\t\t", "KikoTheBoatBuilder", "" },
        {"\t\t\t\t", "DioxCode ", "https://www.youtube.com/channel/UC7Fk3s8hw_CQydnOG4epYFQ" },

        {"Factory of ideas:\t", "Mattia Giuseppini", "" },

        {"macOS build engineer:\t", "The_SamminAter", "" },

        {"Chief ship literature officer:", "Maximord", "" },

        {"", "", ""},

        {"Textures:\t", "Tune 'Prototstar' Katerungroch", ""},
        {"wxWidgets:\t", "Copyright (c) 1998-2005 Julian Smart, Robert Roebling et al", "https://www.wxwidgets.org/"},
        {"SFML:\t\t", "Copyright (c) Laurent Gomila", "https://www.sfml-dev.org/"},
        {"DevIL:\t\t", "Denton Woods et al", "http://openil.sourceforge.net/" },
        {"picojson:\t", "Copyright (c) 2009-2010 Cybozu Labs, Inc.; Copyright (c) 2011-2014 Kazuho Oku", "https://github.com/kazuho/picojson"},
        {"Bitmap Font Generator:\t\t", "Copyright (c) 2005-2011 Karl Walsh (Codehead)", "http://www.codehead.co.uk/cbfg/" },
        {"OpenGL tutorial:\t", "Joey de Vries", "https://learnopengl.com/" },
        {"Fast approx:\t", "Copyright (c) 2011 Paul Mineiro", "http://www.machinedlearnings.com/"}
    };

    wxFont creditsTitleFont(wxFontInfo(8).Bold());
    wxFont creditsContentFont(wxFontInfo(8));

    wxFlexGridSizer * creditsSizer = new wxFlexGridSizer(4, 0, 2);
    for (auto const & credit : credits)
    {
        if (std::get<0>(credit).empty() && std::get<1>(credit).empty() && std::get<2>(credit).empty())
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

            wxStaticText * credits1Label = new wxStaticText(mCreditsPanel, wxID_ANY, std::get<0>(credit), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
            credits1Label->SetFont(creditsTitleFont);
            creditsSizer->Add(credits1Label, 0, wxALIGN_LEFT);

            creditsSizer->AddSpacer(5);

            wxPanel * credits23Panel = new wxPanel(mCreditsPanel);

            {
                wxBoxSizer * credits23Sizer = new wxBoxSizer(wxHORIZONTAL);

                wxStaticText * credits2Label = new wxStaticText(
                    credits23Panel,
                    wxID_ANY,
                    std::get<1>(credit) + (!std::get<2>(credit).empty() ? " - " : ""),
                    wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                credits2Label->SetFont(creditsContentFont);
                credits23Sizer->Add(credits2Label, 0, wxALIGN_LEFT | wxEXPAND);

                if (!std::get<2>(credit).empty())
                {
                    wxHyperlinkCtrl * credits3Label = new wxHyperlinkCtrl(
                        credits23Panel,
                        wxID_ANY,
                        std::get<2>(credit),
                        std::get<2>(credit),
                        wxDefaultPosition,
                        wxDefaultSize,
                        wxHL_ALIGN_LEFT);
                    credits3Label->SetFont(creditsContentFont);
                    credits23Sizer->Add(credits3Label, 0, wxALIGN_LEFT);
                }

                credits23Panel->SetSizerAndFit(credits23Sizer);
            }

            creditsSizer->Add(credits23Panel, 1, wxALIGN_LEFT | wxEXPAND);
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