/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-12-09
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipResizeVisualizationControl.h"

#include <UILib/WxHelpers.h>

#include <wx/dcclient.h>

#include <cassert>

namespace ShipBuilder {

int constexpr TargetMargin = 20;

ShipResizeVisualizationControl::ShipResizeVisualizationControl(
    wxWindow * parent,
    int width,
    int height)
    : mTargetSize(0, 0)
    , mOffset(0, 0)
{
    Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(width, height),
        wxBORDER_SIMPLE);

    Bind(
        wxEVT_SIZE, 
        [this](wxSizeEvent &)
        {
            OnChange();
        });

    Bind(wxEVT_PAINT, &ShipResizeVisualizationControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif
    SetBackgroundColour(wxColour("WHITE"));
    mTargetPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
}

void ShipResizeVisualizationControl::Initialize(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize,
    IntegralCoordinates initialOffset)
{
    mImage = WxHelpers::MakeImage(image);
    mTargetSize = targetSize;
    mOffset = initialOffset;
    
    OnChange();
}

void ShipResizeVisualizationControl::Deinitialize()
{
    mImage.Destroy();
    mResizedBitmap = wxBitmap();
}

void ShipResizeVisualizationControl::SetTargetSize(IntegralRectSize const & targetSize)
{
    mTargetSize = targetSize;

    OnChange();
}

void ShipResizeVisualizationControl::SetOffset(IntegralCoordinates const & offset)
{
    mOffset = offset;

    OnChange();
}

void ShipResizeVisualizationControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipResizeVisualizationControl::OnChange()
{
    wxSize const size = GetSize();

    if (size.GetWidth() <= 2 * TargetMargin || size.GetHeight() <= 2 * TargetMargin
        || mTargetSize.width == 0 || mTargetSize.height == 0)
    {
        return;
    }

    // Calculate conversion factor for image->DC conversions
    float integralToDC;
    if (mTargetSize.width * (size.GetHeight() - 2 * TargetMargin) >= mTargetSize.height * (size.GetWidth() - 2 * TargetMargin))
    {
        // Use the target width as the stick
        integralToDC = static_cast<float>(size.GetWidth() - 2 * TargetMargin) / static_cast<float>(mTargetSize.width);
    }
    else
    {
        // Use the target height as the stick
        integralToDC = static_cast<float>(size.GetHeight() - 2 * TargetMargin) / static_cast<float>(mTargetSize.height);
    }

    // Calculate target coords in DC
    mTargetSizeDC = wxSize(
        static_cast<int>(std::round(static_cast<float>(mTargetSize.width) * integralToDC)),
        static_cast<int>(std::round(static_cast<float>(mTargetSize.height) * integralToDC)));
    mTargetOriginDC = wxPoint(
        size.GetWidth() / 2 - mTargetSizeDC.GetWidth() / 2,
        size.GetHeight() / 2 - mTargetSizeDC.GetHeight() / 2);

    // Calculate size of image
    wxSize const newImageSize = wxSize(
        std::max(static_cast<int>(std::round(static_cast<float>(mImage.GetWidth()) * integralToDC)), 1),
        std::max(static_cast<int>(std::round(static_cast<float>(mImage.GetHeight()) * integralToDC)), 1));

    // Create new preview if needed
    if (!mResizedBitmap.IsOk()
        || mResizedBitmap.GetSize() != newImageSize)
    {
        mResizedBitmap = wxBitmap(
            mImage.Scale(newImageSize.GetWidth(), newImageSize.GetHeight(), wxIMAGE_QUALITY_HIGH),
            wxBITMAP_SCREEN_DEPTH);
    }

    // Calculate resized bitmap origin
    mResizedBitmapOrigin = wxPoint(
        size.GetWidth() / 2 - static_cast<int>(std::round(static_cast<float>(mImage.GetWidth() / 2 + mOffset.x) * integralToDC)),
        size.GetHeight() / 2 - static_cast<int>(std::round(static_cast<float>(mImage.GetHeight() / 2 + mOffset.y) * integralToDC)));

    // Render
    Refresh(false);
}

void ShipResizeVisualizationControl::Render(wxDC & dc)
{
    dc.Clear();

    wxSize const size = GetSize();

    //
    // Draw ship
    //

    dc.DrawBitmap(
        mResizedBitmap,
        mResizedBitmapOrigin,
        true);

    //
    // Draw target rectangle
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(wxRect(mTargetOriginDC, mTargetSizeDC));
}

}