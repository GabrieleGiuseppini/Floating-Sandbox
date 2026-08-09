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
#include <wx/dcbuffer.h>
#include <wx/imaglist.h>

#include <memory>
#include <vector>

namespace ShipBuilder {

/*
 * Event fired for a structural|electrical|ropes material.
 */
template<typename TMaterial>
class _fsMaterialPaletteEvent : public wxEvent
{
public:

    _fsMaterialPaletteEvent(
        wxEventType eventType,
        int winid,
        TMaterial const * material)
        : wxEvent(winid, eventType)
        , mMaterial(material)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    _fsMaterialPaletteEvent(_fsMaterialPaletteEvent  const & other)
        : wxEvent(other)
        , mMaterial(other.mMaterial)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent * Clone() const override
    {
        return new _fsMaterialPaletteEvent(*this);
    }

    TMaterial const * GetMaterial() const
    {
        return mMaterial;
    }

private:

    TMaterial const * const mMaterial;
};

using fsStructuralMaterialPaletteEvent = _fsMaterialPaletteEvent<StructuralMaterial>;
using fsElectricalMaterialPaletteEvent = _fsMaterialPaletteEvent<ElectricalMaterial>;

wxDECLARE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_OUT, fsStructuralMaterialPaletteEvent);
wxDECLARE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_IN, fsStructuralMaterialPaletteEvent);
wxDECLARE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_CLICKED, fsStructuralMaterialPaletteEvent);
wxDECLARE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_OUT, fsElectricalMaterialPaletteEvent);
wxDECLARE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_IN, fsElectricalMaterialPaletteEvent);
wxDECLARE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_CLICKED, fsElectricalMaterialPaletteEvent);

///////////////////////////////////////////////////////////////////

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
    void OnMouseLeave();
    void OnMouseMoved(wxMouseEvent & event);
    void OnMouseLeftDown(wxMouseEvent & event);
    void OnMouseLeftUp(wxMouseEvent & event);

    std::unique_ptr<wxBufferedDC> MakeDc();

    void RenderPanel(wxRect const & region);

    struct Cell;
    void RenderMaterialCell(Cell const & cell);
    void RenderMaterialCell(Cell const & cell, wxDC & dc);

    wxBitmap MakeMaterialSample(TMaterial const * material) const;

    Cell * FindCellAt(wxPoint const & position);
    Cell * FindCellFor(TMaterial const * material);

    // Requires font to be set
    wxString TruncateAsNeeded(std::string const & input, int maxWidth) const;

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
            CreateNewButton,
            Material
        };

        KindType Kind;

        // Iff Kind==CreateNewButton|Material
        TMaterial const * Material;
        // Iff Kind==Material
        int MaterialSampleBitmapIndex;
        int MaterialSampleBitmapYTopOffset; // Relative to cell
        wxString Name1;
        int Name1Width;
        int Name1YTopOffset; // Relative to cell
        wxString Name2;
        int Name2Width;
        int Name2YTopOffset; // Relative to cell
        wxString Data;
        int DataWidth;
        int DataYTopOffset; // Relative to cell

        // Layout
        wxRect Rect; // Origin set at Layout, Size set at cctor

        Cell(
            KindType kind,
            TMaterial const * material,
            wxSize size)
            : Kind(kind)
            , Material(material)
            , MaterialSampleBitmapIndex(-1)
            , MaterialSampleBitmapYTopOffset(0)
            , Name1Width(0)
            , Name1YTopOffset(0)
            , Name2Width(0)
            , Name2YTopOffset(0)
            , DataWidth(0)
            , DataYTopOffset(0)
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

    // Images are stored here, in order to limit number of GDI objects
    wxImageList mMaterialSampleBitmaps;

    //
    // Render style
    //

    wxBrush mBackgroundBrush;
    wxBrush mSeparatorBrush;
    wxFont mNameFont;
    wxFont mDataFont;
    wxColor mTextForegroundColor;

    //
    // State
    //

    TMaterial const * mCurrentSelectedMaterial;
};

}