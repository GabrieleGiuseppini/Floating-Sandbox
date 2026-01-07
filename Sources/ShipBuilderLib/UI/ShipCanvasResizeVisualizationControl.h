/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-12-09
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/ImageData.h>

#include <wx/dc.h>
#include <wx/image.h>
#include <wx/panel.h>

#include <functional>
#include <optional>

namespace ShipBuilder {

class ShipCanvasResizeVisualizationControl : public wxPanel
{
public:

    ShipCanvasResizeVisualizationControl(
        wxWindow * parent,
        int width,
        int height,
        std::function<void()> onCustomOffset);

    IntegralCoordinates const & GetOffset() const
    {
        return mOffset;
    }

    void Initialize(
        RgbaImageData const & image,
        IntegralRectSize const & targetSize,
        std::optional<IntegralCoordinates> anchorCoordinates);

    void Deinitialize();

    void SetTargetSize(IntegralRectSize const & targetSize);
    void SetAnchor(std::optional<IntegralCoordinates> const & anchorCoordinates); // wrt. top-left corner

private:

    void OnPaint(wxPaintEvent & event);
    void OnMouseLeftDown(wxMouseEvent & event);
    void OnMouseLeftUp(wxMouseEvent & event);
    void OnMouseMove(wxMouseEvent & event);

    void OnChange();

    void Render(wxDC & dc);

private:

    wxPen mTargetPen;
    wxBrush mTargetBrush;

    std::function<void()> const mOnCustomOffset;

    // State
    wxImage mImage;
    IntegralRectSize mTargetSize;
    std::optional<IntegralCoordinates> mAnchorCoordinates;
    IntegralCoordinates mOffset;
    std::optional<wxPoint> mCurrentMouseTrajectoryStartDC;

    bool mIsMouseCaptured;

    //
    // Calculated members
    //

    float mIntegralToDC;
    wxPoint mTargetOriginDC;
    wxSize mTargetSizeDC;

    // Resized and clipped image
    wxBitmap mResizedBitmapClip;
    wxPoint mResizedBitmapOriginDC;
};

}