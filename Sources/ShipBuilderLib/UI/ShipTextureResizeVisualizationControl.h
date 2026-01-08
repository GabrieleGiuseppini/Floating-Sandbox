/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-01-07
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/ImageData.h>

#include <wx/dc.h>
#include <wx/image.h>
#include <wx/panel.h>

#include <functional>
#include <memory>
#include <optional>

namespace ShipBuilder {

class ShipTextureResizeVisualizationControl : public wxPanel
{
public:

    ShipTextureResizeVisualizationControl(
        wxWindow * parent,
        int width,
        int height);

    void Initialize(
        RgbaImageData const & image,
        ShipSpaceSize const & shipSize,
        bool doMaintainAspectRatio);

    void Deinitialize();

    void SetDoMaintainAspectRatio(bool doMaintainAspectRatio);

private:

    void OnChange();
    void OnPaint(wxPaintEvent & event);
    void Render(wxDC & dc);

private:

    wxPen mTargetPen;
    wxBrush mTargetBrush;

    // State
    std::unique_ptr<RgbaImageData> mImage;
    ShipSpaceSize mShipSize;
    bool mDoMaintainAspectRatio;

    //
    // Calculated members
    //

    wxRect mShipRectDC;

    wxBitmap mThumbnailBitmap;
    wxPoint mThumbnailOriginDC;
};

}