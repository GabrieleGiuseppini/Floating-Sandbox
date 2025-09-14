/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-08-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "CreditsPanel.h"

#include <Game/GameVersion.h>

#include <Core/BuildInfo.h>

#include <utility>

CreditsPanel::CreditsPanel(wxWindow* parent)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_NONE)
    , mLastMousePosition(0, 0)
{
    SetMinSize(parent->GetSize()); // Occupy all space

#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    Bind(wxEVT_PAINT, (wxObjectEventFunction)&CreditsPanel::OnPaint, this);
    Bind(wxEVT_ERASE_BACKGROUND, (wxObjectEventFunction)&CreditsPanel::OnEraseBackground, this);
    Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&CreditsPanel::OnLeftDown, this);
    Bind(wxEVT_MOTION, (wxObjectEventFunction)&CreditsPanel::OnMouseMove, this);

    //
    // Initialize look'n'feel
    //

    SetBackgroundColour(wxColour("BLACK"));

    mFonts = {
        wxFont(wxFontInfo(20).Family(wxFONTFAMILY_ROMAN)),
        wxFont(wxFontInfo(14).Family(wxFONTFAMILY_ROMAN)),
        wxFont(wxFontInfo(10).Family(wxFONTFAMILY_ROMAN)),
        wxFont(wxFontInfo(10).Family(wxFONTFAMILY_ROMAN).Italic()) };

    //
    // Start timer
    //

    mStartTimestamp = std::chrono::steady_clock::now();
    mCurrentScrollOffsetY = 0;

    mScrollTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Bind(wxEVT_TIMER, &CreditsPanel::OnScrollTimer, this, mScrollTimer->GetId());
    mScrollTimer->Start(20, false);
}

void CreditsPanel::RenderCredits(wxSize panelSize)
{
    int constexpr VMargin = 30;
    int constexpr VMargin3 = VMargin * 3;
    int constexpr VMargin5 = VMargin * 5;

    //
    // Titles
    //

    std::vector<Title> titles = {

        {0, std::string(APPLICATION_NAME_WITH_LONG_VERSION), 0},
        {1, BuildInfo::GetBuildInfo().ToString(), panelSize.GetHeight() / 2},

        {1, _("(c) Gabriele Giuseppini (G2-Labs) 2018-2025"), 0},
        {2, _("Original concept (c) Luke Wren, Francis Racicot (Pac0master) 2013"), VMargin},

        {1, _("This software is licensed to Mattia, Elia, and all the others kids in the world!"), panelSize.GetHeight() / 2},

        {0, _("PROGRAMMING"), 0},
        {1, wxS("Gabriele Giuseppini"), VMargin3},

        {0, _("NPC ASSETS"), 0},
        {1, wxS("Officer TimCan"), VMargin3},

        {0, _("TRANSLATION"), 0},
        {1, wxS("Denis (Ukrainian)"), 0},
        {1, wxS("Dmitrii Kuznetzov (Dkuz) (Russian)"), 0},
        {1, wxS("Gabriele Giuseppini (Italian)"), 0},
        {1, wxS("Ilya Voloshin (https://vk.com/1lvol) (Russian)"), 0},
        {1, wxS("Joaquin Olivera (Joadix100) (Spanish)"), 0},
        {1, wxS("Roman Shavernew (DioxCode) (Russian, Ukrainian)"), VMargin3},

        {0, _("TESTING"), 0},
        {1, wxS("Pac0master"), 0},
        {1, wxS("McShooter2018"), 0},
        {1, wxS("Wreno"), 0},
        {1, wxS("Dkuz"), 0},
        {1, wxS("_ASTYuu_"), 0},
        {1, wxS("sinking_feeling"), 0},
        {1, wxS("Kiko"), 0},
        {1, wxS("Michael Bozarth (https://www.youtube.com/channel/UCaJkgYP6yNw64U3WUZ3t1sw)"), 0},
        {1, wxS("Officer TimCan (https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg)"), 0},
        {1, wxS("DioxCode (https://www.youtube.com/channel/UC7Fk3s8hw_CQydnOG4epYFQ)"), 0},
        {1, wxS("m2"), 0},
        {1, wxS("Oxurus"), 0},
        {1, wxS("bishobe644"), 0},
        {1, wxS("TheCrafters001"), 0},
        {1, wxS("Mia"), 0},
        {1, wxS("minch"), 0},
        {1, wxS("Rpr5704"), 0},
        {1, wxS("Pocketwatch"), 0},
        {1, wxS("Denis"), 0},
        {1, wxS("Damien"), 0},
        {1, wxS("Longhorn"), 0},
        {1, wxS("Dario Bazzichetto"), VMargin3},

        {0, _("BUILD ENGINEERING"), 0},
        {1, wxS("The_SamminAter (macOS)"), 0},
        {1, wxS("Daniel Tammeling (linux)"), VMargin3},

        {0, _("SHIP ENGINEERING"), 0},
        {2, wxS("Albert Windsor    Takara    Rockabilly Rebel    McShooter2018    sinking_feeling    braun    P1X    Higuys153    QHM    Mrs. Magic"), 0},
        {2, wxS("Pandadude12345    John Smith    Dkuz    Loree    Daewoom    Arkstar_    BeamierBoomer    Kazindel    KV Mauvmellow    Longhorn"), 0},
        {2, wxS("JackTheBrickfilmMaker    Michael Bozarth    Officer TimCan    Darek225    HummeL    Fox Assor    Mattytitanic    LostLinerLegend"), 0},
        {2, wxS("Pac0master    CorbinPasta93    Yorkie    Bluefox    Kiko    Raynair    Menta1ity    Transportation Fan    BumBumBaby    LJKMagic"), 0},
        {2, wxS("Matthew Anderson    DennisDanielGrimaldo    blue_funnel    Charles Calvin    Denis    Aqua    Hellooping    Ventrix    M2L"), 0},
        {2, wxS("Dumbphones    NotTelling    Hugo_2503    _ASTYuu_    Serhiiiihres    CPM    Pocketwatch    MTF    Gustav Shedletsky"), 0},
        {2, wxS("Mia    Truce#3326    RetroGraczzPL    Nomadavid    Wreno    R.M.S. Atlantic    Golden    doctor1922    TheCochu444yt"), 0},
        {2, wxS("MasterGarfield    Aur\xe9lien WOLFF    Alex di Roma    2017 Leonardo    FER ZCL    AvSimplified    Techo    Ha-Ha Hans"), VMargin3},

        {0, _("FACTORY OF IDEAS"), 0},
        {1, wxS("Mattia Giuseppini"), VMargin3},

        {0, _("SHIP LITERATURE"), 0},
        {1, wxS("Maximord"), VMargin3},

        {0, _("MUSIC"), VMargin},

        {1, wxS("\"Intervention\""), 0},
        {3, wxS("Scott Buckley (https://www.scottbuckley.com.au)"), 0},
        {2, _("Licensed under Creative Commons: By Attribution 4.0 License"), VMargin},

        {1, wxS("\"Nightmare\""), 0},
        {3, wxS("Kukan Effect (https://kukaneffect.bandcamp.com/)"), VMargin},

        {1, wxS("\"The Short Journey to the Seabed\""), 0},
        {3, wxS("Soul Heater (https://soundcloud.com/soul-heater)"), 0},
        {2, _("Licensed under Creative Commons: By Attribution 4.0 License"), VMargin},

        {1, wxS("\"Long Note Four\""), 0},
        {3, wxS("Kevin MacLeod (https://incompetech.com)"), 0},
        {2, _("Licensed under Creative Commons: By Attribution 4.0 License"), VMargin},

        {1, wxS("\"Symmetry\""), 0},
        {3, wxS("Kevin MacLeod (https://incompetech.com)"), 0},
        {2, _("Licensed under Creative Commons: By Attribution 4.0 License"), VMargin},

        {1, wxS("\"Shadowlands 4 - Breath\""), 0},
        {3, wxS("Kevin MacLeod (https://incompetech.com)"), 0},
        {2, _("Licensed under Creative Commons: By Attribution 4.0 License"), VMargin},

        {1, wxS("\"Untitled #1\""), 0},
        {3, wxS("Michael Bozarth; Stuart's Piano World (https://stuartspianoworld.com/)"), VMargin},

        {1, wxS("\"Untitled #2\""), 0},
        {3, wxS("Officer TimCan (https://www.youtube.com/channel/UCXXWokC-BXQ_jeq1rIQN0dg)"), VMargin3},

        {0, _("3RD-PARTY SOFTWARE"), VMargin},

        {1, wxS("wxWidgets (https://www.wxwidgets.org/)"), 0},
        {2, _("Copyright (c) 1998-2005 Julian Smart, Robert Roebling et al"), VMargin},

        {1, wxS("SFML (https://www.sfml-dev.org/)"), 0},
        {2, _("Copyright (c) Laurent Gomila"), VMargin},

        {1, wxS("picojson (https://github.com/kazuho/picojson)"), 0},
        {2, _("Copyright (c) 2009-2010 Cybozu Labs, Inc.; Copyright (c) 2011-2014 Kazuho Oku"), VMargin},

        {1, wxS("Bitmap Font Generator (http://www.codehead.co.uk/cbfg/)"), 0},
        {2, _("Copyright (c) 2005-2011 Karl Walsh (Codehead)"), VMargin},

        {1, wxS("Fast approx routines (http://www.machinedlearnings.com/)"), 0},
        {2, _("Copyright (c) 2011 Paul Mineiro"), VMargin3},

        { 0, _("SPECIAL THANKS"), 0 },
        { 1, wxS("Monica, Mattia, and Elia Giuseppini"), 0 },
        { 1, wxS("The Shipbucket Project (shipbucket.com)"), 0 },
        { 1, wxS("Bas van den Berg"), 0 },
        { 1, wxS("Daniel Gasperment"), 0 },
        { 1, wxS("Dario Bazzichetto"), 0 },
        { 1, wxS("Joey de Vries (OpenGL tutorial, http://openil.sourceforge.net/)"), 0 },
        { 1, wxS("Mart Slot"), 0 },
        { 1, wxS("Mathias Garbe"), 0 },
        { 1, wxS("Walther Zwart"), 0 },
        { 1, wxS("Wyatt Rosenberry"), VMargin5 },

        { 1, _("A G2-Labs Production"), VMargin5 },

        { 1, _("Programmed in Amsterdam, the Netherlands"), 0 }
    };

    //
    // Calculate size needed to render all titles
    //

    int const centerX = panelSize.GetWidth() / 2;
    int const startY = panelSize.GetHeight() / 2;

    int currentY = startY;
    {
        auto tmpBitmap = std::make_unique<wxBitmap>(wxSize(panelSize.GetWidth(), 100));
        auto tmpBitmapBufferedPaintDC = std::make_unique<wxBufferedPaintDC>(this, *tmpBitmap);

        for (auto const & title : titles)
        {
            RenderTitle(
                title,
                centerX,
                currentY,
                *tmpBitmapBufferedPaintDC,
                false);
        }
    }

    // Final full-page blank
    currentY += panelSize.GetHeight();

    //
    // Render onto bitmap
    //

    mCreditsBitmap = std::make_unique<wxBitmap>(wxSize(panelSize.GetWidth(), currentY));
    mCreditsBitmapBufferedPaintDC = std::make_unique<wxBufferedPaintDC>(this, *mCreditsBitmap);
    mMaxScrollOffsetY = currentY - panelSize.GetHeight();

    mCreditsBitmapBufferedPaintDC->Clear();
    mCreditsBitmapBufferedPaintDC->SetTextForeground(wxColour("WHITE"));

    currentY = startY;
    for (auto const & title : titles)
    {
        RenderTitle(
            title,
            centerX,
            currentY,
            *mCreditsBitmapBufferedPaintDC,
            true);
    }
}

void CreditsPanel::RenderTitle(
    Title const & title,
    int centerX,
    int & currentY, // @ vertical middle of line
    wxDC & dc,
    bool doRender)
{
    dc.SetFont(mFonts[title.FontIndex]);
    auto const extent = dc.GetTextExtent(title.Text);

    int const y = currentY - extent.GetHeight() / 2;

    if(doRender)
        dc.DrawText(title.Text, centerX - extent.GetWidth() / 2, y);

    currentY =
        y
        + extent.GetHeight()
        + 10 // Fixed under-row margin
        + title.BottomMargin;
}

void CreditsPanel::OnPaint(wxPaintEvent & /*event*/)
{
    auto const paintSize = this->GetSize();

    if (!mCreditsBitmapBufferedPaintDC
        || mCreditsBitmapBufferedPaintDC->GetSize().GetWidth() != paintSize.GetWidth())
    {
        RenderCredits(paintSize);
    }

    //
    // Blit the bitmap to its location
    //

    wxPaintDC dc(this);
    dc.Blit(
        0, 0, // Dest coords
        paintSize.GetWidth(), paintSize.GetHeight(), // Dest size
        mCreditsBitmapBufferedPaintDC.get(),
        0, mCurrentScrollOffsetY, // Src coords
        wxCOPY);
}

void CreditsPanel::OnEraseBackground(wxPaintEvent & /*event*/)
{
    // Do nothing, eat event
}

void CreditsPanel::OnLeftDown(wxMouseEvent & event)
{
    mLastMousePosition = event.GetPosition();
}

void CreditsPanel::OnMouseMove(wxMouseEvent & event)
{
    if (event.LeftIsDown())
    {
        int deltaY = event.GetPosition().y - mLastMousePosition.y;

        mCurrentScrollOffsetY = std::min(
            std::max(
                mCurrentScrollOffsetY - deltaY,
                0),
            mMaxScrollOffsetY - 40);

        mLastMousePosition = event.GetPosition();
    }
}

void CreditsPanel::OnScrollTimer(wxTimerEvent & /*event*/)
{
    if (!mCreditsBitmapBufferedPaintDC)
        return;

    auto now = std::chrono::steady_clock::now();
    if (now - mStartTimestamp > std::chrono::seconds(2))
    {
        mCurrentScrollOffsetY += 2;
        if (mCurrentScrollOffsetY > mMaxScrollOffsetY)
        {
            // Reset state
            mStartTimestamp = now;
            mCurrentScrollOffsetY = 0;
        }

        this->Refresh();
    }
}