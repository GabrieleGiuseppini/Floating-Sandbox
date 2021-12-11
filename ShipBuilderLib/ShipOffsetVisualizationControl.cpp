/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-19
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipOffsetVisualizationControl.h"

#include <UILib/WxHelpers.h>

#include <wx/dcclient.h>

#include <cassert>

namespace ShipBuilder {

ShipOffsetVisualizationControl::ShipOffsetVisualizationControl(
    wxWindow * parent,
    int width,
    int height,
    float initialOffsetX,
    float initialOffsetY)
    : mShipSpaceToWorldSpaceCoordsRatio(1.0f, 1.0f)
    , mOffsetX(initialOffsetX)
    , mOffsetY(initialOffsetY)
{
    Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(width, height),
        wxBORDER_SIMPLE);

    Bind(wxEVT_PAINT, &ShipOffsetVisualizationControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif
    SetBackgroundColour(wxColour("WHITE"));
    mSeaBrush = wxBrush(wxColor(77, 172, 255), wxBRUSHSTYLE_SOLID);
    mSeaPen = wxPen(mSeaBrush.GetColour(), 1, wxPENSTYLE_SOLID);
    mGuidesPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
}

void ShipOffsetVisualizationControl::Initialize(
    RgbaImageData const & shipVisualization,
    ShipSpaceToWorldSpaceCoordsRatio const & shipSpaceToWorldSpaceCoordsRatio,
    float offsetX,
    float offsetY)
{
    mShipVisualization = WxHelpers::MakeImage(shipVisualization);
    mShipSpaceToWorldSpaceCoordsRatio = shipSpaceToWorldSpaceCoordsRatio;
    mOffsetX = offsetX;
    mOffsetY = offsetY;
    OnChange();
}

void ShipOffsetVisualizationControl::Deinitialize()
{
    mShipVisualization.Destroy();
    mResizedShipBitmap = wxBitmap();
}

void ShipOffsetVisualizationControl::SetOffsetX(float offsetX)
{
    mOffsetX = offsetX;
    OnChange();
}

void ShipOffsetVisualizationControl::SetOffsetY(float offsetY)
{
    mOffsetY = offsetY;
    OnChange();
}

void ShipOffsetVisualizationControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipOffsetVisualizationControl::OnChange()
{
    //
    // Calculate best multiplier of ShipSpaceSize to map ship space coordinates into physical
    // pixel coordinates in this control
    //

    int constexpr Margin = 5;
    float const niceWorldX = static_cast<float>(GetSize().GetWidth() - Margin - Margin) / 2.0f;
    float const niceWorldY = static_cast<float>(GetSize().GetHeight() - Margin - Margin) / 2.0f;

    vec2f const shipWorldSize = 
        ShipSpaceSize(mShipVisualization.GetWidth(), mShipVisualization.GetHeight())
        .ToFractionalCoords(mShipSpaceToWorldSpaceCoordsRatio);

    float const furthestShipX = std::max(
        std::abs(mOffsetX - shipWorldSize.x / 2.0f),     // L
        std::abs(mOffsetX + shipWorldSize.x / 2.0f));    // R

    float const furthestShipY = std::max(
        std::abs(mOffsetY + shipWorldSize.y),   // Top
        std::abs(mOffsetY));                    // Bottom

    // Calculate multiplier to bring ship's furthest point (L, R, T, or B) at "nice place"
    float const bestShipSpaceMultiplier = std::min(
        niceWorldX / furthestShipX,
        niceWorldY / furthestShipY);

    //
    // Create rescaled ship
    //
    
    float const rescaledWidth = shipWorldSize.x * bestShipSpaceMultiplier;
    float const rescaledHeight = shipWorldSize.y * bestShipSpaceMultiplier;

    mResizedShipBitmap = wxBitmap(
        mShipVisualization.Scale(
            std::max(static_cast<int>(rescaledWidth), 1),
            std::max(static_cast<int>(rescaledHeight), 1),
            wxIMAGE_QUALITY_HIGH),
        wxBITMAP_SCREEN_DEPTH);

    mResizedShipOrigin = wxPoint(
        static_cast<int>(static_cast<float>(GetSize().GetWidth()) / 2.0f - rescaledWidth / 2.0f + mOffsetX * bestShipSpaceMultiplier),
        static_cast<int>(static_cast<float>(GetSize().GetHeight()) / 2.0f - rescaledHeight - mOffsetY * bestShipSpaceMultiplier)); // Note: this is the top of the bitmap, which then is drawn extending *DOWN*

    Refresh(false);
}

void ShipOffsetVisualizationControl::Render(wxDC & dc)
{
    dc.Clear();

    wxSize const size = GetSize();

    //
    // Draw sea
    //

    dc.SetPen(mSeaPen);
    dc.SetBrush(mSeaBrush);
    dc.DrawRectangle(
        0,
        size.GetHeight() / 2,
        size.GetWidth(),
        size.GetHeight() / 2);


    //
    // Draw ship
    //

    dc.DrawBitmap(
        mResizedShipBitmap,
        mResizedShipOrigin,
        true);

    //
    // Draw guides
    //

    dc.SetPen(mGuidesPen);
    dc.DrawLine(0, size.GetHeight() / 2, size.GetWidth(), size.GetHeight() / 2);
    dc.DrawLine(size.GetWidth() / 2, 0, size.GetWidth() / 2, size.GetHeight());
}

}