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

    if (size.GetWidth() == 0 || size.GetHeight() == 0
        || mTargetSize.width == 0 || mTargetSize.height == 0)
    {
        return;
    }

    // Calculate conversion factor for image->DC conversions
    float integralToDC;
    if (mTargetSize.width * size.GetHeight() <= mTargetSize.height * size.GetWidth())
    {
        // Use the target width as the stick
        integralToDC = static_cast<float>(size.GetWidth() - 2 * TargetMargin) / static_cast<float>(mTargetSize.width);
    }
    else
    {
        // Use the target height as the stick
        integralToDC = static_cast<float>(size.GetHeight() - 2 * TargetMargin) / static_cast<float>(mTargetSize.height);
    }

    // TODOHERE

    Refresh(false);
}

void ShipResizeVisualizationControl::Render(wxDC & dc)
{
    dc.Clear();

    wxSize const size = GetSize();

    // TODOHERE

    //
    // Draw target rectangle
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(wxNullBrush);
    dc.DrawRectangle(
        TargetMargin,
        TargetMargin,
        size.GetWidth() - 2 * TargetMargin,
        size.GetHeight() - 2 * TargetMargin);
}

}