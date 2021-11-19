/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-19
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <wx/dc.h>
#include <wx/panel.h>

namespace ShipBuilder {

class ShipOffsetVisualizationControl : public wxPanel
{
public:

    ShipOffsetVisualizationControl(
        wxWindow * parent,
        int width,
        int height,
        float initialOffsetX,
        float initialOffsetY);

    void SetOffsetX(float offsetX);
    void SetOffsetY(float offsetY);

private:

    void OnPaint(wxPaintEvent & event);
    void OnChange();

    void Render(wxDC & dc);

private:

    int mOffsetX;
    int mOffsetY;

    wxBrush mSeaBrush;
    wxPen mSeaPen;
    wxPen mGuidesPen;
};

}