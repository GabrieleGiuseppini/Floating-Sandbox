/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-07-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/CircularList.h>
#include <Core/GameTypes.h>

#include <wx/wx.h>

#include <memory>
#include <vector>

class PressureCrossCutReadingsProbeControl : public wxPanel
{
public:

    PressureCrossCutReadingsProbeControl(
        wxWindow * parent,
        int width);

    virtual ~PressureCrossCutReadingsProbeControl() = default;

    void RegisterReadings(std::vector<PressureReading> const & readings);

    void UpdateSimulation();

    void Reset();

private:

    void OnLeftMouseClick(wxMouseEvent & event);
    void OnRightMouseClick(wxMouseEvent & event);
    void OnMouseMove(wxMouseEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

    void RecalculateReadingsStatistics(std::vector<PressureReading> const & readings);
    inline int MapValueToY(float value) const;

private:

    int const mWidth;

    std::unique_ptr<wxBitmap> mBufferedDCBitmap;
    wxPen const mAirPressurePen;
    wxPen const mWaterPressurePen;
    wxPen const mTotalPressurePen;
    int mViewZoom;
    int mViewLeftSampleI;

    float mMaxValue;
    float mMinValue;

    std::vector<PressureReading> mReadings;
};

