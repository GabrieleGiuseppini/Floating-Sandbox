/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-08-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Materials.h>
#include <Simulation/ShipTexturizer.h>

#include <wx/wx.h>

#include <vector>

namespace ShipBuilder {

template<LayerType TLayer>
class MaterialPalettePanel :
    public wxPanel
{
public:

    using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

    MaterialPalettePanel(
        wxWindow * parent,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & gameAssetManager);

    void StartBuild();
    void Add(TMaterial const * material, bool startNewRow);
    void AddSeparator();
    void EndBuild();

    void SetSelected(TMaterial const * material);

private:

    void OnPaint(wxPaintEvent & event);

    void RenderPanel(wxDC & dc, wxSize const & size);

    wxSize RenderMaterialCell(
        TMaterial const * material,
        wxDC & dc,
        wxPoint const & origin,
        bool isSelected);

private:

    ShipTexturizer const & mShipTexturizer;
    GameAssetManager const & mGameAssetManager;

    std::unique_ptr<wxBitmap> mRenderBuffer;

    //
    // Grid structure
    //

    struct Cell
    {
        enum class KindType
        {
            Material
        };

        KindType Kind;
        // TODO: geometry

        // Iff Kind==Material
        TMaterial const * Material;

        Cell(TMaterial const * material)
            : Kind(KindType::Material)
            , Material(material)
        {
        }
    };

    struct Row
    {
        enum class KindType
        {
            Cells,
            Separator
        };

        KindType Kind;
        int YTop;
        int Height;

        // Iff Kind==Cells
        std::vector<Cell> Cells;

        Row(KindType kind, int yTop)
            : Kind(kind)
            , YTop(yTop)
            , Height(0)
            , Cells()
        { }
    };

    std::vector<Row> mRows;

    //
    // Render style
    //

    wxFont mNameFont;
    wxFont mDataFont;
};

}