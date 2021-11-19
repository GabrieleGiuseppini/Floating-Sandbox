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
    // TODO: recalc if needed
    // image.Rescale(size.width, size.height, wxIMAGE_QUALITY_HIGH);
    mShipBitmap = wxBitmap(mShipVisualization, wxBITMAP_SCREEN_DEPTH);
    mShipOrigin = wxPoint(0, 0);

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
        mShipBitmap,
        mShipOrigin,
        true);

    //
    // Draw guides
    //

    dc.SetPen(mGuidesPen);
    dc.DrawLine(0, size.GetHeight() / 2, size.GetWidth(), size.GetHeight() / 2);
    dc.DrawLine(size.GetWidth() / 2, 0, size.GetWidth() / 2, size.GetHeight());
}

}