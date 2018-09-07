/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ScalarTimeSeriesProbeControl.h"

#include <wx/dcbuffer.h>

#include <cassert>
#include <limits>
#include <numeric>

static constexpr int Height = 80;

ScalarTimeSeriesProbeControl::ScalarTimeSeriesProbeControl(
    wxWindow * parent,
    int width)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition, 
        wxDefaultSize,
        wxBORDER_SIMPLE)
    , mWidth(width)
    , mBufferedDCBitmap()
    , mTimeSeriesPen(wxColor("BLACK"), 2, wxPENSTYLE_SOLID)
    , mGridPen(wxColor(0xa0, 0xa0, 0xa0), 1, wxPENSTYLE_SOLID)
{
    SetMinSize(wxSize(width, Height));
    SetMaxSize(wxSize(width, Height));

    SetDoubleBuffered(true);

    SetBackgroundColour(wxColour("WHITE"));

    wxFont font(wxFontInfo(wxSize(8, 8)).Family(wxFONTFAMILY_TELETYPE));
    SetFont(font);

    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnPaint);
    Connect(this->GetId(), wxEVT_ERASE_BACKGROUND, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnEraseBackground);

    Reset();
}

ScalarTimeSeriesProbeControl::~ScalarTimeSeriesProbeControl()
{
}

void ScalarTimeSeriesProbeControl::RegisterSample(float value)
{
    if (value > mMaxValue)
        mMaxValue = value;

    if (value < mMinValue)
        mMinValue = value;

    mSamples.emplace(
        [](float) {},
        value);
}

void ScalarTimeSeriesProbeControl::Update()
{
    Refresh();
}

void ScalarTimeSeriesProbeControl::Reset()
{
    mSamples.clear();

    mMaxValue = std::numeric_limits<float>::lowest();
    mMinValue = std::numeric_limits<float>::max();
}

///////////////////////////////////////////////////////////////////////////////////////

void ScalarTimeSeriesProbeControl::OnPaint(wxPaintEvent & /*event*/)
{
    if (!mBufferedDCBitmap || mBufferedDCBitmap->GetSize() != this->GetSize())
    {
        mBufferedDCBitmap = std::make_unique<wxBitmap>(this->GetSize());
    }

    wxBufferedPaintDC bufDc(this, *mBufferedDCBitmap);

    Render(bufDc);
}

void ScalarTimeSeriesProbeControl::OnEraseBackground(wxPaintEvent & /*event*/)
{
    // Do nothing
}

int ScalarTimeSeriesProbeControl::MapValueToY(float value) const
{
    if (mMaxValue == mMinValue)
        return Height / 2;

    float y = static_cast<float>(Height - 4) * (value - mMinValue) / (mMaxValue - mMinValue);
    return Height - 3 - static_cast<int>(round(y));
}

void ScalarTimeSeriesProbeControl::Render(wxDC & dc)
{
    dc.Clear();

    if (!mSamples.empty())
    {
        //
        // Check if need to resize grid
        //

        // TODO: member
        static float mGridValueSize = 0.0f;

        // Calculate new grid step
        float numberOfGridLines = 6.0f;
        float const currentValueExtent = mMaxValue - mMinValue;
        if (currentValueExtent > 0.0f)
        {
            if (mGridValueSize == 0.0f)
                mGridValueSize = currentValueExtent / 6.0f;

            // Number of grid lines we would have with the current extent
            numberOfGridLines = currentValueExtent / mGridValueSize;
            if (numberOfGridLines > 20.0f)
            {
                // Recalc
                mGridValueSize = currentValueExtent / 6.0f;
                numberOfGridLines = 6.0f;
            }
        }

        static const int xGridStepSize = mWidth / 6;
        int yGridStepSize = std::min(mWidth, Height) / static_cast<int>(ceil(numberOfGridLines));


        //
        // Draw grid
        //

        dc.SetPen(mGridPen);

        for (int y = yGridStepSize; y < Height - 1; y += yGridStepSize)
        {
            dc.DrawLine(0, y, mWidth - 1, y);
        }

        for (int x = xGridStepSize; x < mWidth - 1; x += xGridStepSize)
        {
            dc.DrawLine(x, 0, x, Height - 1);
        }


        //
        // Draw chart
        //

        dc.SetPen(mTimeSeriesPen);

        auto it = mSamples.begin();
        int lastX = mWidth - 2;
        int lastY = MapValueToY(*it);
        ++it;

        if (it == mSamples.end())
        {
            // Draw just a point
            dc.DrawPoint(lastX, lastY);
        }
        else
        {
            // Draw lines
            do
            {
                int newX = lastX - 1;
                if (newX == 0)
                    break;

                int newY = MapValueToY(*it);

                dc.DrawLine(newX, newY, lastX, lastY);

                lastX = newX;
                lastY = newY;

                ++it;
            }
            while (it != mSamples.end());
        }
    

        //
        // Draw label
        //

        // TODO: write text at sensible place
        wxString testText(std::to_string(*mSamples.begin()));        
        dc.DrawText(testText, 0, Height - 9);
    }
}
