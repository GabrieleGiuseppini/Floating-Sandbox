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
    , mLinearRegressionPen(wxColour(200, 200, 200), 1, wxPENSTYLE_SOLID)
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
    RecalculateReadingsStatistics(readings);

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
        // Draw vertical reference line
        //

        for (size_t s = mViewLeftSampleI; s < mReadings.size() - 1; ++s)
        {
            // Draw vertical reference line
            if (mReadings[s].WorldY >= 0.0f && mReadings[s+1].WorldY <= 0.0f)
            {
                dc.SetPen(mReferencePressurePen);

                float const sampleIToX = static_cast<float>(mWidth) * mViewZoom / static_cast<float>(mReadings.size());

                int const x =
                    MapSampleIndexToX(s - mViewLeftSampleI)
                    + static_cast<int>((mReadings[s].WorldY) / (mReadings[s].WorldY - mReadings[s + 1].WorldY) * sampleIToX);
                dc.DrawLine(x, 1, x, GetSize().GetHeight() - 1);
            }
        }



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
        // Calculate and draw linear regression
        //

        std::string linearRegressionLabelPrefix = "";

        {
            // X: sample index space
            // Y: pressure space

            float sumX = 0.0f;
            float sumXSquared = 0.0f;
            float sumY = 0.0f;
            float sumXY = 0.0f;
            size_t n = 0;
            size_t startSampleIndex = 0;

            for (size_t s = 0; s < mReadings.size(); ++s)
            {
                if (mReadings[s].WaterPressure >= mReferencePressure - 0.5f)
                {
                    // Begin
                    startSampleIndex = s;
                    for (; s < mReadings.size(); ++s)
                    {
                        float const x = static_cast<float>(s);
                        sumX += x;
                        sumXSquared += x * x;
                        sumY += mReadings[s].WaterPressure;
                        sumXY += x * mReadings[s].WaterPressure;
                        n += 1;
                    }
                }
            }

            if (n >= 5) // At least these many points
            {
                float const nF = static_cast<float>(n);
                float const m = (nF * sumXY - sumX * sumY) / (nF * sumXSquared - sumX * sumX);
                float const b = (sumY - m * sumX) / nF;

                dc.SetPen(mLinearRegressionPen);
                dc.DrawLine(
                    MapSampleIndexToX(0),
                    MapValueToY(b),
                    MapSampleIndexToX(mReadings.size()),
                    MapValueToY(m * static_cast<float>(mReadings.size()) + b));

                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << "xL=" << (-b / m) << " m=" << m << " yR=" << (m * static_cast<float>(mReadings.size()) + b);
                linearRegressionLabelPrefix = ss.str();
            }
        }

        //
        // Draw label
        //

        std::stringstream ss;
        if (linearRegressionLabelPrefix.empty())
        {
            ss << std::fixed << std::setprecision(1) << lastTotalValue;
        }
        else
        {
            ss << linearRegressionLabelPrefix;
        }

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

int PressureCrossCutReadingsProbeControl::MapSampleIndexToX(size_t sampleIndex) const
{
    float const sampleIToX = static_cast<float>(mWidth) * mViewZoom / static_cast<float>(mReadings.size());
    return static_cast<int>(std::roundf(sampleIToX * static_cast<float>(sampleIndex)));
}