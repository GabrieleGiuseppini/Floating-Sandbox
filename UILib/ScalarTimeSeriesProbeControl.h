/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/CircularList.h>

#include <wx/wx.h>

#include <memory>

class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width);

    virtual ~ScalarTimeSeriesProbeControl();

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
