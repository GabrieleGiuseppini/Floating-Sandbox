/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalettePanel_NEW.h"

#include <UILib/WxHelpers.h>

#include <Core/Log.h>

#include <wx/dcbuffer.h>

#include <cassert>
#include <iomanip>
#include <sstream>

namespace ShipBuilder {

template<LayerType TLayer>
MaterialPalettePanel<TLayer>::MaterialPalettePanel(
    wxWindow * parent,
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager)
    : IMaterialPalettePanel(parent)
    , mShipTexturizer(shipTexturizer)
    , mGameAssetManager(gameAssetManager)
    , mRenderBuffer() // Start empty
{
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    SetBackgroundColour(wxColour("WHITE"));

    {
        auto font = GetFont();
        font.SetPointSize(font.GetPointSize() - 2);

        SetFont(font);
    }

    using PanelClass = MaterialPalettePanel<TLayer>;

    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&PanelClass::OnPaint);

    // TODOHERE
    SetSize(100, 100);
    SetMinSize(wxSize(100, 100));
    RenderPanel();
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::StartBuild()
{
    // TODOHERE
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::Add(
    TMaterial const * material,
    bool startNewRow)
{
    // TODOHERE
    (void)material;
    (void)startNewRow;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::AddSeparator()
{
    // TODOHERE
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::EndBuild()
{
    // TODOHERE
}

///////////////////////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnPaint(wxPaintEvent & /*event*/)
{
    assert(mRenderBuffer);

    // TODO: see client size/scroll and blitting only needed portion
    wxPaintDC dc(this);
    dc.DrawBitmap(*mRenderBuffer, 0, 0);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::RenderPanel()
{
    assert(!mRenderBuffer);

    wxSize const size = GetSize();
    mRenderBuffer = std::make_unique<wxBitmap>(size);

    // Create DC for rendering into buffer
    wxBufferedDC dc(nullptr, *mRenderBuffer, wxBUFFER_VIRTUAL_AREA);

    // Render
    // TODOTEST
    dc.Clear();
    auto pen = wxPen(wxColor(0x20, 0x20, 0x20), 1, wxPENSTYLE_SOLID);
    dc.SetPen(pen);
    dc.DrawLine(0, 0, 100, 100);
}

//
// Explicit specializations for all material layers
//

template class MaterialPalettePanel<LayerType::Structural>;
template class MaterialPalettePanel<LayerType::Electrical>;
template class MaterialPalettePanel<LayerType::Ropes>;

}