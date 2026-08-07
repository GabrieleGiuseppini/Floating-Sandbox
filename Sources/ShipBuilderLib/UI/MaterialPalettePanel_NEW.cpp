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

ImageSize constexpr MaterialSampleSize(80, 60);

template<LayerType TLayer>
MaterialPalettePanel<TLayer>::MaterialPalettePanel(
    wxWindow * parent,
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager)
    : wxPanel(parent)
    , mShipTexturizer(shipTexturizer)
    , mGameAssetManager(gameAssetManager)
    , mRenderBuffer() // Start empty
    , mRows() // Start empty
{
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    SetBackgroundColour(wxColour("WHITE"));

    // Make name font
    mNameFont = GetFont();

    // Make data font
    mDataFont = GetFont();
    mDataFont.SetPointSize(mDataFont.GetPointSize() - 1);

    // Connect events
    using PanelClass = MaterialPalettePanel<TLayer>;
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&PanelClass::OnPaint);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::StartBuild()
{
    mRenderBuffer.reset();
    mRows.clear();
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
    assert(!mRows.empty());

    //
    // Calculate size
    //

    // TODOHERE
    wxSize const size(100, 100);

    //
    // Create buffer
    //

    assert(!mRenderBuffer);
    mRenderBuffer = std::make_unique<wxBitmap>(size);

    // Create DC for rendering into buffer
    wxBufferedDC dc(nullptr, *mRenderBuffer, wxBUFFER_VIRTUAL_AREA);

    //
    // Render panel
    //

    RenderPanel(dc, size);

    //
    // Set our size
    //

    SetSize(size);
    SetMinSize(size);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::SetSelected(TMaterial const * material)
{
    // TODO: visit all cells, deselect non-matching and select matching;
    // force redraw of two quads into buffer(call RenderMaterialCell)
    (void)material;
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
void MaterialPalettePanel<TLayer>::RenderPanel(wxDC & dc, wxSize const & size)
{
    // TODOTEST
    dc.Clear();
    auto pen = wxPen(wxColor(0x20, 0x20, 0x20), 1, wxPENSTYLE_SOLID);
    dc.SetPen(pen);
    dc.DrawLine(0, 0, size.GetWidth(), size.GetHeight());
}

template<LayerType TLayer>
wxSize MaterialPalettePanel<TLayer>::RenderMaterialCell(
    TMaterial const * material,
    wxDC & dc,
    wxPoint const & origin,
    bool isSelected)
{
    //
    // Make material sample
    //

    wxBitmap materialSampleBitmap;

    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        ShipAutoTexturizationSettings texturizationSettings;
        texturizationSettings.MaterialTextureMagnification = 0.5f;

        materialSampleBitmap = WxHelpers::MakeBitmap(
            mShipTexturizer.MakeMaterialTextureSample(
                texturizationSettings,
                MaterialSampleSize,
                *material,
                mGameAssetManager));
    }
    else
    {
        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

        materialSampleBitmap = WxHelpers::MakeMatteBitmap(
            rgbaColor(material->RenderColor, 255),
            MaterialSampleSize);
    }

    // TODOHERE
    (void)dc;
    (void)origin;
    (void)isSelected;

    return wxSize(80, 200);
}

//
// Explicit specializations for all material layers
//

template class MaterialPalettePanel<LayerType::Structural>;
template class MaterialPalettePanel<LayerType::Electrical>;
template class MaterialPalettePanel<LayerType::Ropes>;

}