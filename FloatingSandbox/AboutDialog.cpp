/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-03-12
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
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
    // Credits Text control
    //

    mTextCtrl = new wxTextCtrl(
        this,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(-1, -1),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxVSCROLL | wxHSCROLL | wxBORDER_NONE);

    mTextCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
   
    mainSizer->Add(
        mTextCtrl, 
        1,                  // Proportion
        wxEXPAND | wxALL, 
        10);                // Border


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
        { "OpenGL tutorial:\t", "Joey de Vries - https://learnopengl.com/" }
    };

    wxTextAttr titleAttr;
    titleAttr.SetFontPixelSize(10);
    titleAttr.SetFontWeight(wxFONTWEIGHT_BOLD);

    wxTextAttr creditsAttr;
    creditsAttr.SetFontPixelSize(10);
    creditsAttr.SetFontWeight(wxFONTWEIGHT_NORMAL);

    for (size_t c = 0; c < credits.size(); ++c)
    {        
        mTextCtrl->SetDefaultStyle(titleAttr);
        mTextCtrl->WriteText(credits[c].first);

        mTextCtrl->SetDefaultStyle(creditsAttr);
        mTextCtrl->WriteText(credits[c].second);

        if (c < credits.size() - 1)
            mTextCtrl->WriteText("\n");
    }
    
    mTextCtrl->SetFocus();
    mTextCtrl->HideNativeCaret();

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
    mTextCtrl->ShowPosition(0);
    mTextCtrl->SetScrollPos(wxVERTICAL, 0, true);
    mTextCtrl->SetInsertionPoint(0);

    this->ShowModal();
}

void AboutDialog::OnClose(wxCloseEvent & event)
{
	event.Skip();
}

