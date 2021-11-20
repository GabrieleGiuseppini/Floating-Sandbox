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
    : mOffsetX(initialOffsetX)
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
    int offsetX,
    int offsetY)
{
    mShipVisualization = WxHelpers::MakeImage(shipVisualization);
    mOffsetX = offsetX;
    mOffsetY = offsetY;
    OnChange();
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

    std::optional<float> bestShipSpaceMultiplier;

    float const niceWorldX = static_cast<float>(GetSize().GetWidth() - Margin - Margin) / 2.0f;
    float const niceWorldY = static_cast<float>(GetSize().GetHeight() - Margin - Margin) / 2.0f;

    // Check multiplier to bring ship's left world X at "nice place"
    float const leftShipWorldX = -static_cast<float>(mShipVisualization.GetWidth()) / 2.0f + mOffsetX;
    if (leftShipWorldX < 0.0f)
    {
        float const multiplier = -niceWorldX / leftShipWorldX;
        bestShipSpaceMultiplier = multiplier;
    }

    // Check multiplier to bring ship's right world X at "nice place"
    float const rightShipWorldX = static_cast<float>(mShipVisualization.GetWidth()) / 2.0f + mOffsetX;
    if (rightShipWorldX > 0.0f)
    {
        float const multiplier = niceWorldX / rightShipWorldX;
        if (!bestShipSpaceMultiplier.has_value())
            bestShipSpaceMultiplier = multiplier;
        else
            bestShipSpaceMultiplier = std::min(*bestShipSpaceMultiplier, multiplier);
    }

    // Check multiplier to bring ship's top world Y at "nice place"
    float const topShipWorldY = static_cast<float>(mShipVisualization.GetHeight()) + mOffsetY;
    if (topShipWorldY > 0.0f)
    {
        float const multiplier = niceWorldY / topShipWorldY;
        if (!bestShipSpaceMultiplier.has_value())
            bestShipSpaceMultiplier = multiplier;
        else
            bestShipSpaceMultiplier = std::min(*bestShipSpaceMultiplier, multiplier);
    }

    // Check multiplier to bring ship's bottom world Y at "nice place"
    float const bottomShipWorldY = 0.0f + mOffsetY;
    if (bottomShipWorldY < 0.0f)
    {
        float const multiplier = -niceWorldY / bottomShipWorldY;
        if (!bestShipSpaceMultiplier.has_value())
            bestShipSpaceMultiplier = multiplier;
        else
            bestShipSpaceMultiplier = std::min(*bestShipSpaceMultiplier, multiplier);
    }

    if (bestShipSpaceMultiplier.has_value())
    {
        //
        // Create rescaled ship
        //

        float const rescaledWidth = static_cast<float>(mShipVisualization.GetWidth()) * (*bestShipSpaceMultiplier);
        float const rescaledHeight = static_cast<float>(mShipVisualization.GetHeight()) * (*bestShipSpaceMultiplier);
        wxImage rescaledShip = mShipVisualization.Scale(
            static_cast<int>(rescaledWidth),
            static_cast<int>(rescaledHeight),
            wxIMAGE_QUALITY_HIGH);

        mResizedShipBitmap = wxBitmap(rescaledShip, wxBITMAP_SCREEN_DEPTH);
        mResizedShipOrigin = wxPoint(
            static_cast<int>(static_cast<float>(GetSize().GetWidth()) / 2.0f - (rescaledWidth / 2.0f + mOffsetX)),
            static_cast<int>(static_cast<float>(GetSize().GetHeight()) / 2.0f - (rescaledHeight + mOffsetY))); // Note: this is the top of the bitmap, which then is drawn extending *DOWN*
    }
    else
    {
        mResizedShipBitmap = wxBitmap();
        mResizedShipOrigin = wxPoint(0, 0);
    }

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