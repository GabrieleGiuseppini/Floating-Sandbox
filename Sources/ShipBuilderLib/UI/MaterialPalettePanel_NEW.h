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
    void AddCreateNewButton(TMaterial const * parentMaterial);
    void EndBuild();

    void SetSelected(TMaterial const * material);

private:

    void OnPaint(wxPaintEvent & event);

    void RenderPanel(wxDC & dc, wxRect const & region);

    struct Cell;
    void RenderMaterialCell(
        Cell const & cell,
        wxDC & dc);

    wxBitmap MakeMaterialSample(TMaterial const * material);

private:

    ShipTexturizer const & mShipTexturizer;
    GameAssetManager const & mGameAssetManager;

    std::unique_ptr<wxBitmap> mRenderBuffer;

    //
    // Grid structure
    //
    // Layout absolute sizes are exclusive of margins

    struct Cell
    {
        enum class KindType
        {
            CreateNewButton,
            Material
        };

        KindType Kind;

        // Iff Kind==CreateNewButton|Material
        TMaterial const * Material;
        wxBitmap CellBitmap; // Sample or whole
        // Iff Kind==Material
        bool IsSelected;

        // Layout
        wxPoint Origin; // Set at Layout
        wxSize Size; // Set at cctor

        Cell(
            KindType kind,
            TMaterial const * material,
            wxSize size)
            : Kind(kind)
            , Material(material)
            , CellBitmap()
            , IsSelected(false)
            , Origin(0, 0)
            , Size(size)
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

        // Iff Kind==Cells
        std::vector<Cell> Cells;

        // Layout
        wxPoint Origin; // Set at Layout
        wxSize Size; // Set at Layout

        Row(KindType kind)
            : Kind(kind)
            , Cells()
            , Origin(0, 0)
            , Size(0, 0)
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