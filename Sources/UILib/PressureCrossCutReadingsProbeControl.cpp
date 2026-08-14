/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-07-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PressureCrossCutReadingsProbeControl.h"

#include <Simulation/Physics/Formulae.h>
#include <Simulation/SimulationParameters.h>

#include <wx/dcbuffer.h>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

static constexpr int Height = 260;

PressureCrossCutReadingsProbeControl::PressureCrossCutReadingsProbeControl(
    wxWindow * parent,
    int width)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE)
    , mWidth(width)
    , mReferencePressure(Physics::Formulae::PressureToEquivalentWaterHeight(SimulationParameters::AirPressureAtSeaLevel, SimulationParameters::WaterMass))
    , mBufferedDCBitmap()
    , mAirPressurePen(wxColor("RED"), 2, wxPENSTYLE_SOLID)
    , mSqueezedAirPressurePen(wxColour(200, 100, 100), 2, wxPENSTYLE_SOLID)
    , mWaterPressurePen(wxColor("BLUE"), 2, wxPENSTYLE_SOLID)
    , mTotalPressurePen(wxColour(50, 50, 50), 2, wxPENSTYLE_SOLID)
    , mReferencePressurePen(wxColour(200, 200, 200), 1, wxPENSTYLE_SHORT_DASH)
    , mViewZoom(1)
    , mViewLeftSampleI(0)
{
    SetMinSize(wxSize(width, Height));
    SetMaxSize(wxSize(width, Height));

#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    SetBackgroundColour(wxColour("WHITE"));

    wxFont font(wxFontInfo(wxSize(8, 8)).Family(wxFONTFAMILY_TELETYPE));
    SetFont(font);

    Connect(this->GetId(), wxEVT_LEFT_DOWN, (wxObjectEventFunction)&PressureCrossCutReadingsProbeControl::OnLeftMouseClick);
    Connect(this->GetId(), wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&PressureCrossCutReadingsProbeControl::OnRightMouseClick);
    Connect(this->GetId(), wxEVT_MOVE, (wxObjectEventFunction)&PressureCrossCutReadingsProbeControl::OnMouseMove);
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&PressureCrossCutReadingsProbeControl::OnPaint);
    Connect(this->GetId(), wxEVT_ERASE_BACKGROUND, (wxObjectEventFunction)&PressureCrossCutReadingsProbeControl::OnEraseBackground);

    Reset();
}

void PressureCrossCutReadingsProbeControl::RegisterReadings(std::vector<PressureReading> const & readings)
{
    // TODOTEST
    //if (mReadings.empty())
    {
        RecalculateReadingsStatistics(readings);
    }

    mReadings = readings;
}

void PressureCrossCutReadingsProbeControl::UpdateSimulation()
{
    Refresh();
}

void PressureCrossCutReadingsProbeControl::Reset()
{
    mReadings.clear();
}

///////////////////////////////////////////////////////////////////////////////////////

void PressureCrossCutReadingsProbeControl::OnLeftMouseClick(wxMouseEvent & /*event*/)
{
    // Reset

    mViewZoom = 1;
    mViewLeftSampleI = 0;

    mMaxValue = std::numeric_limits<float>::lowest();
    mMinValue = std::numeric_limits<float>::max();
    RecalculateReadingsStatistics(mReadings);

    Refresh();
}

void PressureCrossCutReadingsProbeControl::OnRightMouseClick(wxMouseEvent & event)
{
    int const clickedSampleI = mViewLeftSampleI + event.GetX() * mReadings.size() / (mWidth * mViewZoom);
    mViewZoom *= 2;
    int const newSampleWindowWidth = mReadings.size() / mViewZoom;
    mViewLeftSampleI = std::max(clickedSampleI - newSampleWindowWidth / 2, 0);

    Refresh();
}

void PressureCrossCutReadingsProbeControl::OnMouseMove(wxMouseEvent & /*event*/)
{
    // Nop for now
    Refresh();
}

void PressureCrossCutReadingsProbeControl::OnPaint(wxPaintEvent & /*event*/)
{
    if (!mBufferedDCBitmap || mBufferedDCBitmap->GetSize() != this->GetSize())
    {
        mBufferedDCBitmap = std::make_unique<wxBitmap>(this->GetSize());
    }

    wxBufferedPaintDC bufDc(this, *mBufferedDCBitmap);

    Render(bufDc);
}

void PressureCrossCutReadingsProbeControl::OnEraseBackground(wxPaintEvent & /*event*/)
{
    // Do nothing, eat event
}

void PressureCrossCutReadingsProbeControl::Render(wxDC & dc)
{
    dc.Clear();

    // Draw reference pressure
    int const referencePressureY = MapValueToY(mReferencePressure);
    dc.SetPen(mReferencePressurePen);
    dc.DrawLine(0, referencePressureY, mWidth - 1, referencePressureY);


    if (!mReadings.empty())
    {
        //
        // Draw pressures
        //

        float const xToSampleI = static_cast<float>(mReadings.size() / mViewZoom) / static_cast<float>(mWidth);

        int prevAirY = 0;
        int prevSqueezedAirY = 0;
        int prevWaterY = 0;
        int prevTotalY = 0;
        float lastTotalValue = 0.0f;
        for (int x = 0; x < mWidth; ++x)
        {
            size_t leftSampleI = mViewLeftSampleI + static_cast<size_t>(std::roundf(static_cast<float>(x) * xToSampleI));
            leftSampleI = Clamp(leftSampleI, size_t(0), mReadings.size() - 1);
            size_t rightSampleI = mViewLeftSampleI + static_cast<size_t>(std::roundf(static_cast<float>(x + 1) * xToSampleI));
            rightSampleI = Clamp(rightSampleI, size_t(0), mReadings.size() - 1);

            assert(leftSampleI <= rightSampleI);

            float airSum = 0.0f;
            float squeezedAirSum = 0.0f;
            float waterSum = 0.0f;
            for (size_t si = leftSampleI; si <= rightSampleI; ++si)
            {
                airSum += mReadings[si].AirPressure;
                squeezedAirSum += mReadings[si].SqueezedAirPressure;
                waterSum += mReadings[si].WaterPressure;
            }

            int const airY = MapValueToY(airSum / static_cast<float>(rightSampleI - leftSampleI + 1));
            int const squeezedAirY = MapValueToY(squeezedAirSum / static_cast<float>(rightSampleI - leftSampleI + 1));
            int const waterY = MapValueToY(waterSum / static_cast<float>(rightSampleI - leftSampleI + 1));
            lastTotalValue = (airSum + waterSum) / static_cast<float>(rightSampleI - leftSampleI + 1);
            //lastTotalValue = (squeezedAirSum + waterSum) / static_cast<float>(rightSampleI - leftSampleI + 1);
            int const totalY = MapValueToY(lastTotalValue);

            if (x > 0)
            {
                dc.SetPen(mTotalPressurePen);
                dc.DrawLine(x - 1, prevTotalY, x, totalY);

                //dc.SetPen(mSqueezedAirPressurePen);
                //dc.DrawLine(x - 1, prevSqueezedAirY, x, squeezedAirY);

                dc.SetPen(mAirPressurePen);
                dc.DrawLine(x-1, prevAirY, x, airY);

                dc.SetPen(mWaterPressurePen);
                dc.DrawLine(x-1, prevWaterY, x, waterY);
            }

            prevAirY = airY;
            prevSqueezedAirY = squeezedAirY;
            prevWaterY = waterY;
            prevTotalY = totalY;
        }

        //
        // Draw label
        //

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << lastTotalValue;

        wxString labelText(ss.str());
        dc.DrawText(labelText, 0, 1);
    }
}

void PressureCrossCutReadingsProbeControl::RecalculateReadingsStatistics(std::vector<PressureReading> const & readings)
{
    for (auto const & r : readings)
    {
        mMaxValue = std::max(mMaxValue, r.AirPressure + r.WaterPressure);
        mMinValue = std::min(mMinValue, r.AirPressure + r.WaterPressure);
    }
}

int PressureCrossCutReadingsProbeControl::MapValueToY(float value) const
{
    if (mMaxValue == mMinValue)
        return Height - 3;

    float y = static_cast<float>(Height - 4) * value / mMaxValue;
    return Height - 3 - static_cast<int>(round(y));
}

