/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-12-09
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipCanvasResizeVisualizationControl.h"

#include <UILib/WxHelpers.h>

#include <Core/Vectors.h>

#include <wx/dcclient.h>

#include <cassert>

namespace ShipBuilder {

int constexpr TargetMargin = 20;

ShipCanvasResizeVisualizationControl::ShipCanvasResizeVisualizationControl(
    wxWindow * parent,
    int width,
    int height,
    std::function<void()> onCustomOffset)
    : mOnCustomOffset(std::move(onCustomOffset))
    , mTargetSize(0, 0)
    , mOffset(0, 0)
    , mIsMouseCaptured(false)
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

    Bind(wxEVT_PAINT, &ShipCanvasResizeVisualizationControl::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &ShipCanvasResizeVisualizationControl::OnMouseLeftDown, this);
    Bind(wxEVT_LEFT_UP, &ShipCanvasResizeVisualizationControl::OnMouseLeftUp, this);
    Bind(wxEVT_MOTION, &ShipCanvasResizeVisualizationControl::OnMouseMove, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif
    SetBackgroundColour(wxColour(150, 150, 150));
    mTargetPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
    mTargetBrush = wxBrush(wxColor(255, 255, 255), wxBRUSHSTYLE_SOLID);
}

void ShipCanvasResizeVisualizationControl::Initialize(
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

void ShipCanvasResizeVisualizationControl::Deinitialize()
{
    mImage.Destroy();
    mResizedBitmapClip = wxBitmap();
}

void ShipCanvasResizeVisualizationControl::SetTargetSize(IntegralRectSize const & targetSize)
{
    mTargetSize = targetSize;

    OnChange();
}

void ShipCanvasResizeVisualizationControl::SetAnchor(std::optional<IntegralCoordinates> const & anchorCoordinates)
{
    mAnchorCoordinates = anchorCoordinates;

    OnChange();
}

void ShipCanvasResizeVisualizationControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipCanvasResizeVisualizationControl::OnMouseLeftDown(wxMouseEvent & event)
{
    mCurrentMouseTrajectoryStartDC.emplace(event.GetX(), event.GetY());

    if (!mIsMouseCaptured)
    {
        CaptureMouse();
        mIsMouseCaptured = true;
    }
}

void ShipCanvasResizeVisualizationControl::OnMouseLeftUp(wxMouseEvent & /*event*/)
{
    if (mIsMouseCaptured)
    {
        ReleaseMouse();
        mIsMouseCaptured = false;
    }

    mCurrentMouseTrajectoryStartDC.reset();
}

void ShipCanvasResizeVisualizationControl::OnMouseMove(wxMouseEvent & event)
{
    if (mCurrentMouseTrajectoryStartDC.has_value())
    {
        // Stop using anchors
        mAnchorCoordinates.reset();
        mOnCustomOffset();

        // Calculate new offset

        wxPoint newMouseCoords(event.GetX(), event.GetY());

        IntegralRectSize offsetOffset = IntegralRectSize(
            static_cast<IntegralRectSize::integral_type>(std::round(static_cast<float>(newMouseCoords.x - mCurrentMouseTrajectoryStartDC->x) / mIntegralToDC)),
            static_cast<IntegralRectSize::integral_type>(std::round(static_cast<float>(newMouseCoords.y - mCurrentMouseTrajectoryStartDC->y) / mIntegralToDC)));

        if (offsetOffset.width != 0 || offsetOffset.height != 0)
        {
            mOffset += offsetOffset;

            OnChange();

            // Remember coords for next move
            mCurrentMouseTrajectoryStartDC.emplace(newMouseCoords);
        }
    }
}

void ShipCanvasResizeVisualizationControl::OnChange()
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

    // Calculate new (screen) size of image required to fit the scale
    FloatSize const newImageSizeDC = FloatSize(
        static_cast<float>(mImage.GetWidth()) * mIntegralToDC,
        static_cast<float>(mImage.GetHeight()) * mIntegralToDC);

    // Calculate new image (screen) origin, relative to (0, 0) of this control
    vec2f const newImageOriginDC = vec2f(
        static_cast<float>(mTargetOriginDC.x) + static_cast<float>(mOffset.x) * mIntegralToDC,
        static_cast<float>(mTargetOriginDC.y) + static_cast<float>(mOffset.y) * mIntegralToDC);

    // Calculate visible portion of resized image
    std::optional<FloatRect> const newImageRectDC =
        FloatRect(newImageOriginDC, newImageSizeDC)
        .MakeIntersectionWith(
            FloatRect(
                vec2f(0.0f, 0.0f),
                FloatSize(static_cast<float>(sizeDC.GetWidth()), static_cast<float>(sizeDC.GetHeight()))));

    if (newImageRectDC.has_value())
    {
        // Convert DC coordinates back into bitmap coordinates

        vec2i newImageSizeImage = (newImageRectDC->size / mIntegralToDC).to_vec2i_round();

        vec2i const newImageOriginImage = (vec2f(
            std::max(-newImageOriginDC.x, 0.0f),
            std::max(-newImageOriginDC.y, 0.0f)) / mIntegralToDC).to_vec2i_round();

        // Make sure we don't need an image larger than the original one
        newImageSizeImage = vec2i(
            std::min(newImageSizeImage.x, mImage.GetWidth() - newImageOriginImage.x),
            std::min(newImageSizeImage.y, mImage.GetHeight() - newImageOriginImage.y));

        if (newImageSizeImage.x > 0 && newImageSizeImage.y > 0)
        {
            //
            // Create new clip
            //

            assert(newImageOriginImage.x + newImageSizeImage.x <= mImage.GetWidth());
            assert(newImageOriginImage.y + newImageSizeImage.y <= mImage.GetHeight());

            auto clippedImage = wxImage(newImageSizeImage.x, newImageSizeImage.y, false);
            clippedImage.Paste(mImage, -newImageOriginImage.x, -newImageOriginImage.y);

            vec2i const newImageSizeDCi = newImageRectDC->size.to_vec2i_round();
            vec2i const newImageOriginDCi = newImageRectDC->origin.to_vec2i_round();

            mResizedBitmapClip = wxBitmap(
                clippedImage
                .Scale(
                    newImageSizeDCi.x,
                    newImageSizeDCi.y,
                    wxIMAGE_QUALITY_NEAREST),
                wxBITMAP_SCREEN_DEPTH);

            mResizedBitmapOriginDC = wxPoint(newImageOriginDCi.x, newImageOriginDCi.y);
        }
        else
        {
            // No clip
            mResizedBitmapClip = wxBitmap();
        }
    }
    else
    {
        // No clip
        mResizedBitmapClip = wxBitmap();
    }

    // Render
    Refresh(false);
}

void ShipCanvasResizeVisualizationControl::Render(wxDC & dc)
{
    dc.Clear();

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