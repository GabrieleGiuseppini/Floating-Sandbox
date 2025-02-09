/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/CircularList.h>

#include <wx/wx.h>

#include <memory>

class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width);

    virtual ~ScalarTimeSeriesProbeControl() = default;

    void RegisterSample(float value);

    void UpdateSimulation();

    void Reset();

private:

    void OnMouseClick(wxMouseEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

    inline int MapValueToY(float value) const;

private:

    int const mWidth;

    std::unique_ptr<wxBitmap> mBufferedDCBitmap;
    wxPen const mTimeSeriesPen;
    wxPen mGridPen;

    float mMaxValue;
    float mMinValue;
    float mGridValueSize;

    CircularList<float, 200> mSamples;
};

class IntegratingScalarTimeSeriesProbeControl : public ScalarTimeSeriesProbeControl
{
public:

    IntegratingScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width)
        : ScalarTimeSeriesProbeControl(parent, width)
        , mCurrentSum(0.0f)
    {}

    void RegisterSample(float value)
    {
        mCurrentSum += value;
        ScalarTimeSeriesProbeControl::RegisterSample(mCurrentSum);
    }

    void Reset()
    {
        ScalarTimeSeriesProbeControl::Reset();
        mCurrentSum = 0.0f;
    }

private:

    float mCurrentSum;
};