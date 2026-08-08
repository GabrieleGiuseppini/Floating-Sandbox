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
        wxRect Rect; // Origin set at Layout, Size set at cctor

        Cell(
            KindType kind,
            TMaterial const * material,
            wxSize size)
            : Kind(kind)
            , Material(material)
            , CellBitmap()
            , IsSelected(false)
            , Rect(wxPoint(0, 0), size)
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
        wxRect Rect; // Origin set at Layout, Size set at Layout

        Row(KindType kind)
            : Kind(kind)
            , Cells()
            , Rect(wxPoint(0, 0), wxSize(0, 0))
        { }
    };

    std::vector<Row> mRows;

    //
    // Render style
    //

    wxBrush mBackgroundBrush;
    wxFont mNameFont;
    wxFont mDataFont;
};

}