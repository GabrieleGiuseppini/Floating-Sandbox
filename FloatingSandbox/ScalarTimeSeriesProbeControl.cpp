/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ScalarTimeSeriesProbeControl.h"

#include <wx/dcbuffer.h>

#include <cassert>
#include <limits>

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
    , mMaxValue(std::numeric_limits<float>::lowest())
    , mMinValue(std::numeric_limits<float>::max())
{
    SetMinSize(wxSize(width, Height));
    SetMaxSize(wxSize(width, Height));

    SetDoubleBuffered(true);

    SetBackgroundColour(wxColour("WHITE"));

    wxFont font(wxFontInfo(wxSize(8, 8)).Family(wxFONTFAMILY_TELETYPE));
    SetFont(font);

    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnPaint);
    Connect(this->GetId(), wxEVT_ERASE_BACKGROUND, (wxObjectEventFunction)&ScalarTimeSeriesProbeControl::OnEraseBackground);
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

void ScalarTimeSeriesProbeControl::Render(wxDC & dc)
{
    dc.Clear();

    if (!mSamples.empty())
    {
        // TODO: draw
    
        // TODO: write text at sensible place
        wxString testText(std::to_string(*mSamples.begin()));        
        dc.DrawText(testText, 0, Height - 9);
    }
}
