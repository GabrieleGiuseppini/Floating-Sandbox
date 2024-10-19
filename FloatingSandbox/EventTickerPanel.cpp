/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "EventTickerPanel.h"

#include <wx/dcbuffer.h>

#include <cassert>
#include <sstream>

static size_t constexpr TickerTextSize = 1024u;
static int constexpr TickerFontSize = 12; // Not a pixel size
static unsigned int constexpr TickerCharStep = 1;
static unsigned int constexpr TickerPanelHeight = 1 + TickerFontSize + 1;

EventTickerPanel::EventTickerPanel(wxWindow* parent)
    : UnFocusablePanel(
        parent,
        wxBORDER_SIMPLE)
    , mCurrentTickerText(TickerTextSize, ' ')
    , mFutureTickerText()
{
    SetMinSize(wxSize(-1, TickerPanelHeight));
    SetMaxSize(wxSize(-1, TickerPanelHeight));

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&EventTickerPanel::OnPaint);
    Connect(this->GetId(), wxEVT_ERASE_BACKGROUND, (wxObjectEventFunction)&EventTickerPanel::OnEraseBackground);

    //
    // Create font
    //

    wxFont font(wxFontInfo(wxSize(TickerFontSize, TickerFontSize)).Family(wxFONTFAMILY_TELETYPE));
    SetFont(font);

    mCharSize = GetTextExtent("Z");
    mCurrentCharWidthStep = mCharSize.GetWidth(); // Initialize
}

void EventTickerPanel::UpdateSimulation()
{
    mCurrentCharWidthStep += TickerCharStep;
    if (mCurrentCharWidthStep >= mCharSize.GetWidth())
    {
        mCurrentCharWidthStep = 0;

        // Pop first char
        assert(TickerTextSize == mCurrentTickerText.size());
        mCurrentTickerText.erase(mCurrentTickerText.begin());

        // Add last char
        if (!mFutureTickerText.empty())
        {
            mCurrentTickerText.push_back(mFutureTickerText.front());
            mFutureTickerText.erase(mFutureTickerText.begin());
        }
        else
        {
            mCurrentTickerText.push_back(' ');
        }
    }

    // Rendering costs ~2%, hence let's do it only when needed!
    if (this->IsShown())
    {
        Refresh();
    }
}

///////////////////////////////////////////////////////////////////////////////////////

void EventTickerPanel::OnGameReset()
{
    mCurrentTickerText = std::string(TickerTextSize, ' ');
    mFutureTickerText.clear();
}

void EventTickerPanel::OnShipLoaded(
    unsigned int /*id*/,
    ShipMetadata const & shipMetadata)
{
    std::stringstream ss;
    ss << "Loaded " << shipMetadata.ShipName;

    if (shipMetadata.Author.has_value())
        ss << " by " << *shipMetadata.Author;

    if (shipMetadata.ArtCredits.has_value())
        ss << "; art by " << *shipMetadata.ArtCredits;

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnSinkingBegin(ShipId shipId)
{
    std::stringstream ss;
    ss << "SHIP " << shipId << " IS SINKING!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnSinkingEnd(ShipId shipId)
{
    std::stringstream ss;
    ss << "SHIP " << shipId << " HAS STOPPED SINKING!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnStress(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    std::stringstream ss;
    ss << "Stressed " << size << "x" << structuralMaterial.Name << (isUnderwater ? " underwater" : "") << "!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnBreak(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    std::stringstream ss;
    ss << "Broken " << size << "x" << structuralMaterial.Name << (isUnderwater ? " underwater" : "") << "!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnTsunami(float x)
{
    std::stringstream ss;
    ss << "WARNING: Tsunami at " << x;

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnDestroy(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    std::stringstream ss;
    ss << "Destroyed " << size << "x" << structuralMaterial.Name << (isUnderwater ? " underwater" : "") << "!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnSpringRepaired(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    std::stringstream ss;
    ss << "Repaired spring " << size << "x" << structuralMaterial.Name << (isUnderwater ? " underwater" : "") << "!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnTriangleRepaired(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    std::stringstream ss;
    ss << "Repaired triangle " << size << "x" << structuralMaterial.Name << (isUnderwater ? " underwater" : "") << "!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnSwitchEnabled(
    GlobalElectricalElementId electricalElementId,
    bool isEnabled)
{
    std::stringstream ss;
    ss << "Switch '" << electricalElementId << "' "
        << (isEnabled ? "enabled" : "disabled")
        << "!";
    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnSwitchToggled(
    GlobalElectricalElementId electricalElementId,
    ElectricalState newState)
{
    std::stringstream ss;
    ss << "Switch '" << electricalElementId << "' toggled to " << newState << "!";
    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnPowerProbeToggled(
    GlobalElectricalElementId electricalElementId,
    ElectricalState newState)
{
    std::stringstream ss;
    ss << "Monitor '" << electricalElementId << "' toggled to " << newState << "!";
    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnGadgetPlaced(
    GlobalGadgetId /*gadgetId*/,
    GadgetType gadgetType,
    bool /*isUnderwater*/)
{
    std::stringstream ss;
    switch (gadgetType)
    {
        case GadgetType::AntiMatterBomb:
        {
            ss << "Anti-matter bomb";
            break;
        }

        case GadgetType::ImpactBomb:
        {
            ss << "Impact bomb";
            break;
        }

        case GadgetType::PhysicsProbe:
        {
            ss << "Physics probe";
            break;
        }

        case GadgetType::RCBomb:
        {
            ss << "Remote-controlled bomb";
            break;
        }

        case GadgetType::TimerBomb:
        {
            ss << "Timer bomb";
            break;
        }
    }

    ss << " placed!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnGadgetRemoved(
    GlobalGadgetId /*gadgetId*/,
    GadgetType gadgetType,
    std::optional<bool> /*isUnderwater*/)
{
    std::stringstream ss;
    switch (gadgetType)
    {
        case GadgetType::AntiMatterBomb:
        {
            ss << "Anti-matter bomb";
            break;
        }

        case GadgetType::ImpactBomb:
        {
            ss << "Impact bomb";
            break;
        }

        case GadgetType::PhysicsProbe:
        {
            ss << "Physics probe";
            break;
        }

        case GadgetType::RCBomb:
        {
            ss << "Remote-controlled bomb";
            break;
        }

        case GadgetType::TimerBomb:
        {
            ss << "Timer bomb";
            break;
        }
    }

    ss << " removed!";

    AppendFutureTickerText(ss.str());
}

void EventTickerPanel::OnBombExplosion(
    GadgetType /*gadgetType*/,
    bool /*isUnderwater*/,
    unsigned int size)
{
    std::stringstream ss;
    ss << "Bomb" << (size > 1 ? "s" : "") << " exploded!";

    AppendFutureTickerText(ss.str());
}

///////////////////////////////////////////////////////////////////////////////////////

void EventTickerPanel::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);

    Render(dc);
}

void EventTickerPanel::OnEraseBackground(wxPaintEvent & /*event*/)
{
    // Do nothing
}

void EventTickerPanel::AppendFutureTickerText(std::string const & text)
{
    mFutureTickerText.clear();

    assert(!mCurrentTickerText.empty());
    if (mCurrentTickerText.back() != ' '
        && mCurrentTickerText.back() != '>')
    {
        mFutureTickerText.append(">");
    }

    mFutureTickerText.append(text);
}

void EventTickerPanel::Render(wxDC & dc)
{
    int const tickerPanelWidth = dc.GetSize().GetWidth();
    auto const charWidth = mCharSize.GetWidth();
    int const leftX = tickerPanelWidth + charWidth - mCurrentCharWidthStep - (TickerTextSize * charWidth);

    wxString const tickerText(mCurrentTickerText, TickerTextSize);

    dc.Clear();
    dc.DrawText(tickerText, leftX, TickerFontSize - mCharSize.GetHeight() + 1);
}