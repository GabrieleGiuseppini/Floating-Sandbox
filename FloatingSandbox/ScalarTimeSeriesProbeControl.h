/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameLib/CircularList.h>

#include <wx/wx.h>

#include <vector>

class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width);
    
    virtual ~ScalarTimeSeriesProbeControl();

    void RegisterSample(float value);

    void Update();

    void Reset();

private:

    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

private:

    int const mWidth;

    std::unique_ptr<wxBitmap> mBufferedDCBitmap;

    float mMaxValue;
    float mMinValue;

    CircularList<float, 200> mSamples;
};
