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
    Bind(wxEVT_LEFT_DOWN, &ShipResizeVisualizationControl::OnMouseLeftDown, this);
    Bind(wxEVT_LEFT_UP, &ShipResizeVisualizationControl::OnMouseLeftUp, this);
    Bind(wxEVT_MOTION, &ShipResizeVisualizationControl::OnMouseMove, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif
    SetBackgroundColour(wxColour(150, 150, 150));
    mTargetPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
    mTargetBrush = wxBrush(wxColor(255, 255, 255), wxBRUSHSTYLE_SOLID);
}

void ShipResizeVisualizationControl::Initialize(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize)
{
    mImage = WxHelpers::MakeImage(image);
    mTargetSize = targetSize;
    mOffset = { 0, 0 };

    mCurrentMouseTrajectoryStartDC.reset();
    
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

void ShipResizeVisualizationControl::SetAnchor(int anchorMatrixX, int anchorMatrixY)
{
    // Offset is relative to top-left

    int xOffset;
    switch (anchorMatrixX)
    {
        case 0:
        {
            // Left-aligned
            xOffset = 0;
            break;
        }

        case 1:
        {
            // Center-aligned
            xOffset = mTargetSize.width / 2 - mImage.GetSize().GetWidth() / 2;
            break;
        }

        default:
        {
            assert(anchorMatrixX == 2);

            // Right-aligned
            xOffset = mTargetSize.width - mImage.GetSize().GetWidth();
            break;
        }
    }

    int yOffset;
    switch (anchorMatrixY)
    {
        case 0:
        {
            // Top-aligned
            yOffset = 0;
            break;
        }

        case 1:
        {
            // Center-aligned
            yOffset = mTargetSize.height / 2 - mImage.GetSize().GetHeight() / 2;
            break;
        }

        default:
        {
            assert(anchorMatrixY == 2);

            // Bottom-aligned
            yOffset = mTargetSize.height - mImage.GetSize().GetHeight();
            break;
        }
    }

    mOffset = { xOffset, yOffset };

    OnChange();
}

void ShipResizeVisualizationControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipResizeVisualizationControl::OnMouseLeftDown(wxMouseEvent & event)
{
    mCurrentMouseTrajectoryStartDC.emplace(event.GetX(), event.GetY());
}

void ShipResizeVisualizationControl::OnMouseLeftUp(wxMouseEvent & /*event*/)
{
    mCurrentMouseTrajectoryStartDC.reset();
}

void ShipResizeVisualizationControl::OnMouseMove(wxMouseEvent & event)
{
    if (mCurrentMouseTrajectoryStartDC.has_value())
    {
        wxPoint newMouseCoords(event.GetX(), event.GetY());

        mOffset += IntegralRectSize(
            static_cast<IntegralRectSize::integral_type>(std::round(static_cast<float>(newMouseCoords.x - mCurrentMouseTrajectoryStartDC->x) / mIntegralToDC)),
            static_cast<IntegralRectSize::integral_type>(std::round(static_cast<float>(newMouseCoords.y - mCurrentMouseTrajectoryStartDC->y) / mIntegralToDC)));

        OnChange();

        mCurrentMouseTrajectoryStartDC.emplace(newMouseCoords);
    }
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
    if (mTargetSize.width * (size.GetHeight() - 2 * TargetMargin) >= mTargetSize.height * (size.GetWidth() - 2 * TargetMargin))
    {
        // Use the target width as the stick
        mIntegralToDC = static_cast<float>(size.GetWidth() - 2 * TargetMargin) / static_cast<float>(mTargetSize.width);
    }
    else
    {
        // Use the target height as the stick
        mIntegralToDC = static_cast<float>(size.GetHeight() - 2 * TargetMargin) / static_cast<float>(mTargetSize.height);
    }

    // Calculate target coords in DC
    mTargetSizeDC = wxSize(
        static_cast<int>(std::round(static_cast<float>(mTargetSize.width) * mIntegralToDC)),
        static_cast<int>(std::round(static_cast<float>(mTargetSize.height) * mIntegralToDC)));
    mTargetOriginDC = wxPoint(
        size.GetWidth() / 2 - mTargetSizeDC.GetWidth() / 2,
        size.GetHeight() / 2 - mTargetSizeDC.GetHeight() / 2);

    // Calculate size of image
    wxSize const newImageSize = wxSize(
        std::max(static_cast<int>(std::round(static_cast<float>(mImage.GetWidth()) * mIntegralToDC)), 1),
        std::max(static_cast<int>(std::round(static_cast<float>(mImage.GetHeight()) * mIntegralToDC)), 1));

    // Create new preview if needed
    if (!mResizedBitmap.IsOk()
        || mResizedBitmap.GetSize() != newImageSize)
    {
        mResizedBitmap = wxBitmap(
            mImage.Scale(newImageSize.GetWidth(), newImageSize.GetHeight(), wxIMAGE_QUALITY_HIGH),
            wxBITMAP_SCREEN_DEPTH);
    }

    // Calculate resized bitmap origin
    mResizedBitmapOriginDC = wxPoint(
        mTargetOriginDC.x + static_cast<int>(std::round(static_cast<float>(mOffset.x) * mIntegralToDC)),
        mTargetOriginDC.y + static_cast<int>(std::round(static_cast<float>(mOffset.y) * mIntegralToDC)));

    // Render
    Refresh(false);
}

void ShipResizeVisualizationControl::Render(wxDC & dc)
{
    dc.Clear();

    wxSize const size = GetSize();

    //
    // Draw target rectangle 1
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(mTargetBrush);
    dc.DrawRectangle(wxRect(mTargetOriginDC, mTargetSizeDC));

    //
    // Draw ship
    //

    dc.DrawBitmap(
        mResizedBitmap,
        mResizedBitmapOriginDC,
        true);

    //
    // Draw target rectangle 2
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(wxRect(mTargetOriginDC, mTargetSizeDC));
}

}