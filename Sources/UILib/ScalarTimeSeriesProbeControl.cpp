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

template<typename...TElement>
ScalarTimeSeriesProbeControl<TElement...>::ScalarTimeSeriesProbeControl(
    wxWindow * parent,
    int width,
    PenTuple pens,
    ScalarTimeSeriesProbeControlOptions flags)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE)
    , mWidth(width)
    , mBufferedDCBitmap()
    , mTimeSeriesPens(pens)
    , mLabelColors(MakeDarkerColors(pens))
    , mZeroLinePens(MakeZeroLinePens(pens))
    , mGridPen(wxColor(0xa0, 0xa0, 0xa0), 1, wxPENSTYLE_SOLID)
    , mFlags(flags)
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

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::RegisterSample(ValueTuple values)
{
    mMaxValues = Max(mMaxValues, values);
    mMinValues = Min(mMinValues, values);

    mSamples.emplace(
        [](ValueTuple) {},
        values);
}

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::UpdateSimulation()
{
    Refresh();
}

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::Reset()
{
    mSamples.clear();

    mMaxValues = InitTuple(std::numeric_limits<float>::lowest());
    mMinValues = InitTuple(std::numeric_limits<float>::max());

    mGridValueSize = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::OnMouseClick(wxMouseEvent & /*event*/)
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

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::OnPaint(wxPaintEvent & /*event*/)
{
    if (!mBufferedDCBitmap || mBufferedDCBitmap->GetSize() != this->GetSize())
    {
        mBufferedDCBitmap = std::make_unique<wxBitmap>(this->GetSize());
    }

    wxBufferedPaintDC bufDc(this, *mBufferedDCBitmap);

    Render(bufDc);
}

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::OnEraseBackground(wxPaintEvent & /*event*/)
{
    // Do nothing, eat event
}

template<typename...TElement>
void ScalarTimeSeriesProbeControl<TElement...>::Render(wxDC & dc)
{
    dc.Clear();

    if (!mSamples.empty())
    {
        if constexpr (sizeof...(TElement) == 1)
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
        // Draw charts
        //

        DrawCharts(dc, std::make_index_sequence<sizeof...(TElement)>{});
    }
}

template<typename...TElement>
template<size_t IElement>
void ScalarTimeSeriesProbeControl<TElement...>::DrawChart(wxDC& dc)
{
    // Zero line

    if ((mFlags & ScalarTimeSeriesProbeControlOptions::ZeroLine) != ScalarTimeSeriesProbeControlOptions::None)
    {
        dc.SetPen(std::get<IElement>(mZeroLinePens));

        int zeroY = MapValueToY<IElement>(0.0f);
        dc.DrawLine(1, zeroY, mWidth - 1, zeroY);
    }

    // Chart

    dc.SetPen(std::get<IElement>(mTimeSeriesPens));

    auto it = mSamples.cbegin();
    int lastX = mWidth - 2;
    int lastY = MapValueToY<IElement>(*it);
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

            int newY = MapValueToY<IElement>(*it);

            dc.DrawLine(newX, newY, lastX, lastY);

            lastX = newX;
            lastY = newY;

            ++it;
        } while (it != mSamples.cend());
    }


    //
    // Draw label
    //

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);

    float const currentValue = std::get<IElement>(*mSamples.cbegin());
    if (currentValue >= 0.0f)
        ss << ' ';
    ss << currentValue << " (" << std::get<IElement>(mMaxValues) << ")";

    wxString labelText(ss.str());
    dc.SetTextForeground(std::get<IElement>(mLabelColors));
    dc.DrawText(labelText, 0, 1 + 9 * static_cast<int>(IElement));
}

template<typename...TElement>
template<size_t IElement>
int ScalarTimeSeriesProbeControl<TElement...>::MapValueToY(ValueTuple const & t) const
{
    return MapValueToY<IElement>(std::get<IElement>(t));
}

template<typename...TElement>
template<size_t IElement>
int ScalarTimeSeriesProbeControl<TElement...>::MapValueToY(float value) const
{
    if (std::get<IElement>(mMaxValues) == std::get<IElement>(mMinValues))
        return Height / 2;

    float y = static_cast<float>(Height - 4) * (value - std::get<IElement>(mMinValues)) / (std::get<IElement>(mMaxValues) - std::get<IElement>(mMinValues));
    return Height - 3 - static_cast<int>(round(y));
}

// Force specializations
template class ScalarTimeSeriesProbeControl<float>;
template class ScalarTimeSeriesProbeControl<float, float>;