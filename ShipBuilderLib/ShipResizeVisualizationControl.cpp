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
    int height,
    std::function<void()> onCustomOffset)
    : mOnCustomOffset(std::move(onCustomOffset))
    , mTargetSize(0, 0)
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
    IntegralRectSize const & targetSize,
    std::optional<IntegralCoordinates> anchorCoordinates)
{
    mImage = WxHelpers::MakeImage(image);
    mTargetSize = targetSize;
    mAnchorCoordinates = anchorCoordinates;
    mOffset = { 0, 0 };
    mCurrentMouseTrajectoryStartDC.reset();
    
    OnChange();
}

void ShipResizeVisualizationControl::Deinitialize()
{
    mImage.Destroy();
    mResizedBitmapClip = wxBitmap();
}

void ShipResizeVisualizationControl::SetTargetSize(IntegralRectSize const & targetSize)
{
    mTargetSize = targetSize;

    OnChange();
}

void ShipResizeVisualizationControl::SetAnchor(std::optional<IntegralCoordinates> const & anchorCoordinates)
{
    mAnchorCoordinates = anchorCoordinates;

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
        // Stop using anchors
        mAnchorCoordinates.reset();
        mOnCustomOffset();

        // Calculate new offset

        wxPoint newMouseCoords(event.GetX(), event.GetY());

        mOffset += IntegralRectSize(
            static_cast<IntegralRectSize::integral_type>(std::round(static_cast<float>(newMouseCoords.x - mCurrentMouseTrajectoryStartDC->x) / mIntegralToDC)),
            static_cast<IntegralRectSize::integral_type>(std::round(static_cast<float>(newMouseCoords.y - mCurrentMouseTrajectoryStartDC->y) / mIntegralToDC)));

        OnChange();

        // Remember coords for next move
        mCurrentMouseTrajectoryStartDC.emplace(newMouseCoords);
    }
}

void ShipResizeVisualizationControl::OnChange()
{
    wxSize const sizeDC = GetSize();

    if (sizeDC.GetWidth() <= 2 * TargetMargin || sizeDC.GetHeight() <= 2 * TargetMargin
        || mTargetSize.width == 0 || mTargetSize.height == 0)
    {
        return;
    }

    // Calculate conversion factor for image->DC conversions
    if (mTargetSize.width * (sizeDC.GetHeight() - 2 * TargetMargin) >= mTargetSize.height * (sizeDC.GetWidth() - 2 * TargetMargin))
    {
        // Use the target width as the stick
        mIntegralToDC = static_cast<float>(sizeDC.GetWidth() - 2 * TargetMargin) / static_cast<float>(mTargetSize.width);
    }
    else
    {
        // Use the target height as the stick
        mIntegralToDC = static_cast<float>(sizeDC.GetHeight() - 2 * TargetMargin) / static_cast<float>(mTargetSize.height);
    }

    // Calculate target coords in DC
    mTargetSizeDC = wxSize(
        static_cast<int>(std::round(static_cast<float>(mTargetSize.width) * mIntegralToDC)),
        static_cast<int>(std::round(static_cast<float>(mTargetSize.height) * mIntegralToDC)));
    mTargetOriginDC = wxPoint(
        sizeDC.GetWidth() / 2 - mTargetSizeDC.GetWidth() / 2,
        sizeDC.GetHeight() / 2 - mTargetSizeDC.GetHeight() / 2);

    // Calculate anchor offset - offset is relative to top-left
    if (mAnchorCoordinates.has_value())
    {
        int xOffset;
        switch (mAnchorCoordinates->x)
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
                assert(mAnchorCoordinates->x == 2);

                // Right-aligned
                xOffset = mTargetSize.width - mImage.GetSize().GetWidth();
                break;
            }
        }

        int yOffset;
        switch (mAnchorCoordinates->y)
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
                assert(mAnchorCoordinates->y == 2);

                // Bottom-aligned
                yOffset = mTargetSize.height - mImage.GetSize().GetHeight();
                break;
            }
        }

        mOffset = { xOffset, yOffset };
    }

    //
    // Resize image
    //

    // TODO: do all calc's in float

    // Calculate new (screen) size of image required to fit the scale
    wxSize const newImageSizeDC = wxSize(
        std::max(static_cast<int>(std::round(static_cast<float>(mImage.GetWidth()) * mIntegralToDC)), 1),
        std::max(static_cast<int>(std::round(static_cast<float>(mImage.GetHeight()) * mIntegralToDC)), 1));

    // Calculate new image (screen) origin, relative to (0, 0) of this control
    wxPoint const newImageOriginDC = wxPoint(
        mTargetOriginDC.x + static_cast<int>(std::round(static_cast<float>(mOffset.x) * mIntegralToDC)),
        mTargetOriginDC.y + static_cast<int>(std::round(static_cast<float>(mOffset.y) * mIntegralToDC)));

    // Calculate visible portion of resized image
    wxRect const newImageRectDC = 
        wxRect(newImageOriginDC, newImageSizeDC)
        .Intersect(wxRect(wxPoint(0, 0), sizeDC));

    if (!newImageRectDC.IsEmpty())
    {
        // Convert DC coordinates back into bitmap coordinates

        wxSize const newImageSizeImage = wxSize(
            static_cast<int>(std::round(static_cast<float>(newImageRectDC.GetWidth()) / mIntegralToDC)),
            static_cast<int>(std::round(static_cast<float>(newImageRectDC.GetHeight()) / mIntegralToDC)));

        wxPoint const newImageOriginImage = wxPoint(
            static_cast<int>(std::round(static_cast<float>(std::max(-newImageOriginDC.x, 0)) / mIntegralToDC)),
            static_cast<int>(std::round(static_cast<float>(std::max(-newImageOriginDC.y, 0)) / mIntegralToDC)));

        wxRect const resizedBitmapClipRectImage = wxRect(newImageOriginImage, newImageSizeImage);

        //
        // Create new clip
        //
        
        auto clippedImage = wxImage(newImageSizeImage, false);
        clippedImage.Paste(mImage, -newImageOriginImage.x, -newImageOriginImage.y);
        mResizedBitmapClip = wxBitmap(
            clippedImage
            .Scale(newImageRectDC.GetWidth(), newImageRectDC.GetHeight(), wxIMAGE_QUALITY_HIGH),
            wxBITMAP_SCREEN_DEPTH);

        mResizedBitmapOriginDC = newImageRectDC.GetPosition();
    }
    else
    {
        // No clip
        mResizedBitmapClip = wxBitmap();
    }

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

    if (mResizedBitmapClip.IsOk())
    {
        dc.DrawBitmap(
            mResizedBitmapClip,
            mResizedBitmapOriginDC,
            true);
    }

    //
    // Draw target rectangle 2
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(wxRect(mTargetOriginDC, mTargetSizeDC));
}

}