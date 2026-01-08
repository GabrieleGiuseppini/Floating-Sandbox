/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-01-07
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipTextureResizeVisualizationControl.h"

#include <UILib/WxHelpers.h>

#include <Core/ImageTools.h>
#include <Core/Vectors.h>

#include <wx/dcclient.h>

#include <cassert>

namespace ShipBuilder {

int constexpr TargetMargin = 20;

ShipTextureResizeVisualizationControl::ShipTextureResizeVisualizationControl(
    wxWindow * parent,
    int width,
    int height)
    : mShipSize(0, 0) // Will be set
    , mDoMaintainAspectRatio(false) // Will be set
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

    Bind(wxEVT_PAINT, &ShipTextureResizeVisualizationControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif
    SetBackgroundColour(wxColour(150, 150, 150));
    mTargetPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
    mTargetBrush = wxBrush(wxColor(255, 255, 255), wxBRUSHSTYLE_SOLID);
}

void ShipTextureResizeVisualizationControl::Initialize(
    RgbaImageData const & image,
    ShipSpaceSize const & shipSize,
    bool doMaintainAspectRatio)
{
    mImage = std::make_unique<RgbaImageData>(image.Clone());
    mShipSize = shipSize;
    mDoMaintainAspectRatio = doMaintainAspectRatio;

    OnChange();
}

void ShipTextureResizeVisualizationControl::Deinitialize()
{
    mImage.reset();
    mThumbnailBitmap = wxBitmap();
}

void ShipTextureResizeVisualizationControl::SetDoMaintainAspectRatio(bool doMaintainAspectRatio)
{
    mDoMaintainAspectRatio = doMaintainAspectRatio;

    OnChange();
}

void ShipTextureResizeVisualizationControl::OnChange()
{
    wxSize const totalSizeDC = GetSize();
    if (totalSizeDC.GetWidth() <= 2 * TargetMargin || totalSizeDC.GetHeight() <= 2 * TargetMargin
        || mShipSize.width == 0 || mShipSize.height == 0
        || !mImage)
    {
        return;
    }

    //
    // Calculate target DC size of thumbnail image, as ShipSize scaled to fit in total DC size (maintaining A/R)
    //

    // Whole DC minus margins
    auto const stageRectDC = wxRect(
        wxPoint(TargetMargin, TargetMargin),
        wxSize(totalSizeDC.GetWidth() - 2 * TargetMargin, totalSizeDC.GetHeight() - 2 * TargetMargin));

    auto const shipRect = mShipSize.Fit(ImageSize(stageRectDC.GetWidth(), stageRectDC.GetHeight()));

    //
    // Calculate thumbnail image
    //

    if (mDoMaintainAspectRatio)
    {
        //
        // Reframe first, then resize
        //

        ImageSize const reframedSize = mImage->Size.ResizeToAspectRatioOf(shipRect);
        RgbaImageData reframedImage = mImage->MakeReframed(
            reframedSize,
            ImageCoordinates(
                std::max((reframedSize.width - mImage->Size.width) / 2, 0),
                std::max((reframedSize.height - mImage->Size.height) / 2, 0)),
            rgbaColor::zero());

        mThumbnailBitmap = WxHelpers::MakeBitmap(
            ImageTools::Resize(
                reframedImage,
                ImageSize(shipRect.width, shipRect.height)));
    }
    else
    {
        // In reality we won't change the texture size, and rely on OpenGL to do the streching/resizing
        // for us. We need to visualize this here though

        mThumbnailBitmap = WxHelpers::MakeBitmap(
            ImageTools::Resize(
                *mImage,
                ImageSize(shipRect.width, shipRect.height)));
    }

    //
    // Calculate rect and center
    //

    mShipRectDC = wxRect(
        wxPoint(
            stageRectDC.x + (stageRectDC.GetWidth() - shipRect.width) / 2,
            stageRectDC.y + (stageRectDC.GetHeight() - shipRect.height) / 2),
        wxSize(shipRect.width, shipRect.height));

    mThumbnailOriginDC = wxPoint(
        totalSizeDC.GetWidth() / 2 - mShipRectDC.GetWidth() / 2,
        totalSizeDC.GetHeight() / 2 - mShipRectDC.GetHeight() / 2);

    //
    // Draw
    //

    Refresh(false);
}

void ShipTextureResizeVisualizationControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipTextureResizeVisualizationControl::Render(wxDC & dc)
{
    dc.Clear();

    //
    // Draw target rectangle 1
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(mTargetBrush);
    dc.DrawRectangle(mShipRectDC);

    //
    // Draw ship
    //

    if (mThumbnailBitmap.IsOk())
    {
        dc.DrawBitmap(
            mThumbnailBitmap,
            mThumbnailOriginDC,
            true);
    }

    //
    // Draw target rectangle 2
    //

    dc.SetPen(mTargetPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(mShipRectDC);
}

}