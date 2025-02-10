/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-19
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/ImageData.h>

#include <wx/dc.h>
#include <wx/image.h>
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

    void Initialize(
        RgbaImageData const & shipVisualization,
        ShipSpaceToWorldSpaceCoordsRatio const & shipSpaceToWorldSpaceCoordsRatio,
        float offsetX,
        float offsetY);

    void Deinitialize();
    
    void SetOffsetX(float offsetX);
    void SetOffsetY(float offsetY);

private:

    void OnPaint(wxPaintEvent & event);
    void OnChange();

    void Render(wxDC & dc);

private:

    wxImage mShipVisualization;
    ShipSpaceToWorldSpaceCoordsRatio mShipSpaceToWorldSpaceCoordsRatio;
    float mOffsetX;
    float mOffsetY;
    
    wxBrush mSeaBrush;
    wxPen mSeaPen;
    wxPen mGuidesPen;

    // Calculated members    
    wxBitmap mResizedShipBitmap;
    wxPoint mResizedShipOrigin;
};

}