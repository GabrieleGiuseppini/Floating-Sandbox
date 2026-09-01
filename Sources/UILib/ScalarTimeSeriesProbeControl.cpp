/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ScalarTimeSeriesProbeControl.h"

#include <wx/dcbuffer.h>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

static constexpr int Height = 80;

template<typename TTuple>
ScalarTimeSeriesProbeControl<TTuple>::ScalarTimeSeriesProbeControl(
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
    , mGridPen(wxColor(0xa0, 0xa0, 0xa0), 1, wxPENSTYLE_SOLID)
{
    SetMinSize(wxSize(width, Height));
    SetMaxSize(wxSize(width, Height));

#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    SetBackgroundColour(wxColour("WHITE"));

    wxFont font(wxFontInfo(wxSize(8, 8)).Family(wxFONTFAMILY_TELETYPE));
    SetFont(font);

    Connect(this->GetId(), wxEVT_LEFT_DOWN, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnMouseClick);
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnPaint);
    Connect(this->GetId(), wxEVT_ERASE_BACKGROUND, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnEraseBackground);

    Reset();
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::SetPens(PenTuple pens)
{
    mTimeSeriesPens = pens;
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::RegisterSample(TTuple values)
{
    mMaxValues = Max(mMaxValues, values);
    mMinValues = Min(mMinValues, values);

    mSamples.emplace(
        [](TTuple) {},
        values);
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::UpdateSimulation()
{
    Refresh();
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::Reset()
{
    mSamples.clear();

    mMaxValues = InitTuple(std::numeric_limits<float>::lowest());
    mMinValues = InitTuple(std::numeric_limits<float>::max());

    mGridValueSize = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::OnMouseClick(wxMouseEvent & /*event*/)
{
    // Reset extent
    mMaxValues = InitTuple(std::numeric_limits<float>::lowest());
    mMinValues = InitTuple(std::numeric_limits<float>::max());
    for (auto const & sample : mSamples)
    {
        mMaxValues = Max(mMaxValues, sample);
        mMinValues = Min(mMinValues, sample);
    }

    Refresh();
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::OnPaint(wxPaintEvent & /*event*/)
{
    if (!mBufferedDCBitmap || mBufferedDCBitmap->GetSize() != this->GetSize())
    {
        mBufferedDCBitmap = std::make_unique<wxBitmap>(this->GetSize());
    }

    wxBufferedPaintDC bufDc(this, *mBufferedDCBitmap);

    Render(bufDc);
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::OnEraseBackground(wxPaintEvent & /*event*/)
{
    // Do nothing, eat event
}

template<typename TTuple>
void ScalarTimeSeriesProbeControl<TTuple>::Render(wxDC & dc)
{
    dc.Clear();

    if (!mSamples.empty())
    {
        if constexpr (std::tuple_size<TTuple>{} == 1)
        {
            //
            // Check if need to resize grid
            //

            // Calculate new grid step
            float numberOfGridLines = 6.0f;
            float const currentValueExtent = std::get<0>(mMaxValues) - std::get<0>(mMinValues);
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
        }


        //
        // Draw chart
        //

        int labelY = 1;

        dc.SetPen(std::get<0>(mTimeSeriesPens));

        auto it = mSamples.cbegin();
        int lastX = mWidth - 2;
        int lastY = MapValueToY(std::get<0>(*it), std::get<0>(mMinValues), std::get<0>(mMaxValues));
        ++it;

        if (it == mSamples.cend())
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

                int newY = MapValueToY(std::get<0>(*it), std::get<0>(mMinValues), std::get<0>(mMaxValues));

                dc.DrawLine(newX, newY, lastX, lastY);

                lastX = newX;
                lastY = newY;

                ++it;
            }
            while (it != mSamples.cend());
        }


        //
        // Draw label
        //

        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << std::get<0>(*mSamples.cbegin()) << " (" << std::get<0>(mMaxValues) << ")";

        wxString labelText(ss.str());
        dc.DrawText(labelText, 0, labelY);
    }
}

template<typename TTuple>
int ScalarTimeSeriesProbeControl<TTuple>::MapValueToY(float value, float minValue, float maxValue)
{
    if (maxValue == minValue)
        return Height / 2;

    float y = static_cast<float>(Height - 4) * (value - minValue) / (maxValue - minValue);
    return Height - 3 - static_cast<int>(round(y));
}

// Force specializations
template class ScalarTimeSeriesProbeControl<std::tuple<float>>;
template class ScalarTimeSeriesProbeControl<std::tuple<float, float>>;