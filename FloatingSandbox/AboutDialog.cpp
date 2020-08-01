/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-03-12
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AboutDialog.h"

#include <GameCore/SysSpecifics.h>
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
		"About " + std::string(APPLICATION_NAME_WITH_SHORT_VERSION),
		wxDefaultPosition,
		wxSize(780, 620),
		wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP,
		wxS("About Window"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

	mainSizer->AddSpacer(5);


	//
	// Title
	//

	auto title = std::string(APPLICATION_NAME_WITH_LONG_VERSION);

#if defined(FS_ARCHITECTURE_ARM_32)
	title += " ARM 32-bit";
#elif defined(FS_ARCHITECTURE_ARM_64)
	title += " ARM 64-bit";
#elif defined(FS_ARCHITECTURE_X86_32)
	title += " x86 32-bit";
#elif defined(FS_ARCHITECTURE_X86_64)
	title += " x86 64-bit";
#else
	title += " <ARCH?>";
#endif

#if defined(FS_OS_LINUX)
	title += " Linux";
#elif defined(FS_OS_MACOS)
	title += " MacOS";
#elif defined(FS_OS_WINDOWS)
	title += " Windows";
#else
	title += " <OS?>";
#endif

	title += " (" __DATE__ ")";

	wxStaticText * titleLabel = new wxStaticText(this, wxID_ANY, title);
	titleLabel->SetFont(wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN)));
	mainSizer->Add(titleLabel, 0, wxALIGN_CENTRE);

	mainSizer->AddSpacer(1);

	wxStaticText * title2Label = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	title2Label->SetLabelText(wxS(
		"Original concept (c) Luke Wren, Francis Racicot (Pac0master) 2013\n"
		"(c) Gabriele Giuseppini 2018-2020\n"
		"This version licensed to Mattia, Elia, and all the others kids in the world"));
	mainSizer->Add(title2Label, 0, wxALIGN_CENTRE);

	mainSizer->AddSpacer(5);


	//
	// Image
	//

	wxBitmap bmp(resourceLocator.GetArtFilePath("splash_screen").string(), wxBITMAP_TYPE_PNG);

	wxStaticBitmap * stBmp = new wxStaticBitmap(this, wxID_ANY, bmp, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);

	mainSizer->Add(stBmp, 1, wxALIGN_CENTER_HORIZONTAL);

	mainSizer->AddSpacer(5);


	//
	// Credits title
	//

	wxStaticText * creditsTitleLabel = new wxStaticText(this, wxID_ANY, wxS("Credits:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
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

	std::vector<std::tuple<wxString, wxString, wxString>> credits
	{
		{wxS("Ship engineers:"), wxS("TopHatLemons"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Truce#3326"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("RetroGraczzPL"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Nomadavid"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Wreno"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Pac0master"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("CorbinPasta93"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Yorkie"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Bluefox"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("KikoTheBoatBuilder"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Albert Windsor"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Takara"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Michael Bozarth"), wxS("https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw") },
		{wxS("\t\t\t\t"), wxS("Rockabilly Rebel"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("McShooter2018"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Dumbphones"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("NotTelling"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Hugo_2503"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("_ASTYuu_"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Serhiiiihres"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("JackTheBrickfilmMaker"), wxS("https://www.youtube.com/channel/UCshPbiTqFuwpNNh7BlpffhQ") },
		{wxS("\t\t\t\t"), wxS("Pandadude12345"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("John Smith"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Dkuz"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Loree"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Daewoom"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Aqua"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("MasterGarfield"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Darek225"), wxS("https://www.youtube.com/channel/UC5l6t4P8NLA8n81XdX6yl6w") },
		{wxS("\t\t\t\t"), wxS("Aur\xe9lien WOLFF"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("HummeL"), wxS("https://www.youtube.com/c/HummeL_Prog") },
		{wxS("\t\t\t\t"), wxS("Alex di Roma"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("Fox Assor"), wxS("https://vk.com/id448121270") },
		{wxS("\t\t\t\t"), wxS("Officer TimCan"), wxS("https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg") },
		{wxS("\t\t\t\t"), wxS("2017 Leonardo"), wxEmptyString },
		{wxS("\t\t\t\t"), wxS("sinking_feeling"), wxEmptyString },

		{wxS("Music:\t\t"), wxS("\"The Short Journey to the Seabed\", Soul Heater"), wxS("https://soundcloud.com/soul-heater")},
		{wxS("\t\t\t\t"), wxS("> Licensed under Creative Commons: By Attribution 4.0 License"), wxS("https://creativecommons.org/licenses/by/4.0/")},
		{wxS("\t\t\t\t"), wxS("\"Long Note Four\", Kevin MacLeod"), wxS("https://incompetech.com")},
		{wxS("\t\t\t\t"), wxS("> Licensed under Creative Commons: By Attribution 4.0 License"), wxS("https://creativecommons.org/licenses/by/4.0/")},
		{wxS("\t\t\t\t"), wxS("\"Symmetry\", Kevin MacLeod"), wxS("https://incompetech.com")},
		{wxS("\t\t\t\t"), wxS("> Licensed under Creative Commons: By Attribution 4.0 License"), wxS("https://creativecommons.org/licenses/by/4.0/")},
		{wxS("\t\t\t\t"), wxS("Michael Bozarth; Stuart's Piano World"), wxS("https://stuartspianoworld.com/")},
		{wxS("\t\t\t\t"), wxS("Officer TimCan"), wxS("https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg")},

		{wxS("Testing:\t\t"), wxS("Pac0master"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("McShooter2018"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("Wreno"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("Dkuz"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("Michael Bozarth"), wxS("https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw")},
		{wxS("\t\t\t\t"), wxS("_ASTYuu_"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("sinking_feeling"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("Officer TimCan"), wxS("https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg")},
		{wxS("\t\t\t\t"), wxS("KikoTheBoatBuilder"), wxEmptyString},
		{wxS("\t\t\t\t"), wxS("DioxCode "), wxS("https://www.youtube.com/channel/UC7Fk3s8hw_CQydnOG4epYFQ")},

		{wxS("Factory of ideas:\t"), wxS("Mattia Giuseppini"), wxEmptyString},

		{wxS("macOS build engineer:\t"), wxS("The_SamminAter"), wxEmptyString},

		{wxS("Chief ship literature officer:"), wxS("Maximord"), wxEmptyString},

		{wxEmptyString, wxEmptyString, wxEmptyString},

		{wxS("Textures:\t"), wxS("Tune 'Prototstar' Katerungroch"), wxEmptyString},
		{wxS("wxWidgets:\t"), wxS("Copyright (c) 1998-2005 Julian Smart, Robert Roebling et al"), wxS("https://www.wxwidgets.org/")},
		{wxS("SFML:\t\t"), wxS("Copyright (c) Laurent Gomila"), wxS("https://www.sfml-dev.org/")},
		{wxS("DevIL:\t\t"), wxS("Denton Woods et al"), wxS("http://openil.sourceforge.net/")},
		{wxS("picojson:\t"), wxS("Copyright (c) 2009-2010 Cybozu Labs, Inc.; Copyright (c) 2011-2014 Kazuho Oku"), wxS("https://github.com/kazuho/picojson")},
		{wxS("Bitmap Font Generator:\t\t"), wxS("Copyright (c) 2005-2011 Karl Walsh (Codehead)"), wxS("http://www.codehead.co.uk/cbfg/")},
		{wxS("OpenGL tutorial:\t"), wxS("Joey de Vries"), wxS("https://learnopengl.com/")},
		{wxS("Fast approx:\t"), wxS("Copyright (c) 2011 Paul Mineiro"), wxS("http://www.machinedlearnings.com/")}
	};

	wxFont creditsTitleFont(wxFontInfo(8).Bold());
	wxFont creditsContentFont(wxFontInfo(8));

	wxFlexGridSizer * creditsSizer = new wxFlexGridSizer(4, 0, 2);
	for (auto const & credit : credits)
	{
		if (std::get<0>(credit).IsEmpty() && std::get<1>(credit).IsEmpty() && std::get<2>(credit).IsEmpty())
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
					std::get<1>(credit) + (!std::get<2>(credit).IsEmpty() ? wxS(" - ") : wxS("")),
					wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
				credits2Label->SetFont(creditsContentFont);
				credits23Sizer->Add(credits2Label, 0, wxALIGN_LEFT | wxEXPAND);

				if (!std::get<2>(credit).IsEmpty())
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