/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-08-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include "IMaterialPalettesController.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>
#include <Simulation/ShipTexturizer.h>

#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/imaglist.h>

#include <cstdint>
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
        IMaterialPalettesController & materialPalettesController,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & gameAssetManager);

    void StartDefaultMaterialsLayout();
    void StartNewSubcategoryRow(std::string const & subCategory);
    void StartNewMaterialStride(unsigned int subCategoryBaseMaterialOrdinal);
    void AddDefaultMaterial(TMaterial const * material);
    void AddCreateNewCustomMaterialButton(TMaterial const * parentMaterial);
    void AddSeparatorRow();
    void EndDefaultMaterialsLayout();

    void SetSelected(TMaterial const * material);

private:

    using CellIdType = std::uint64_t;
    static CellIdType constexpr NoneCellId = std::numeric_limits<CellIdType>::max();

    void OnPaint(wxPaintEvent & event);
    void OnMouseLeave();
    void OnMouseMoved(wxMouseEvent & event);
    void OnMouseLeftDown(wxMouseEvent & event);
    void OnMouseLeftUp(wxMouseEvent & event);

    std::unique_ptr<wxMemoryDC> MakeDc();

    void RenderPanel(wxRect const & region);

    struct Cell;
    void RenderCell(Cell const & cell);
    void RenderCell(Cell const & cell, wxDC & dc);

    wxBitmap MakeMaterialSample(TMaterial const * material) const;

    Cell * FindCell(CellIdType const & id);
    Cell * FindCellAt(wxPoint const & position);
    Cell * FindCellFor(TMaterial const * material);

    // These two do _not_ invoke Refresh(), but they take care of events
    void ToggleSelectionTo(Cell const & cell);
    void ToggleSelectionToNone();

    // Requires font to be set
    wxString TruncateAsNeeded(std::string const & input, int maxWidth) const;

    CellIdType MakeNextCellId();

private:

    IMaterialPalettesController & mMaterialPalettesController;
    ShipTexturizer const & mShipTexturizer;
    GameAssetManager const & mGameAssetManager;

    std::unique_ptr<wxBitmap> mRenderBuffer;

    //
    // Grid structure
    //

    struct Cell
    {
        CellIdType const Id;

        enum class KindType
        {
            CreateNewButton,
            Material
        };

        KindType const Kind;

        enum class CustomKindType
        {
            None,
            User,
            Ship
        };

        // Iff Kind==CreateNewButton|Material
        TMaterial const * Material; // CreateNewButton:parent material|Material:material itself
        // Iff Kind==Material
        CustomKindType const CustomKind;
        int const MaterialSampleBitmapIndex;
        wxString Name1;
        int Name1Width;
        int Name1YTopOffset; // Relative to cell
        wxString Name2;
        int Name2Width;
        int Name2YTopOffset; // Relative to cell
        wxString Data;
        int DataWidth;
        int DataYTopOffset; // Relative to cell
        bool IsChecked; // Iff CustomKind!=None

        // Layout
        wxRect Rect; // Origin.x set at Layout, Origin.y set at cctor, Size set at cctor

        static Cell MakeCreateNewButtonCell(
            CellIdType id,
            TMaterial const * parentMaterial,
            int originY,
            wxSize size)
        {
            return Cell(
                id,
                KindType::CreateNewButton,
                parentMaterial,
                CustomKindType::None,
                -1,
                originY,
                size);
        }

        static Cell MakeMaterialCell(
            CellIdType id,
            TMaterial const * material,
            CustomKindType customKind,
            int materialSampleBitmapIndex,
            int originY,
            wxSize size)
        {
            return Cell(
                id,
                KindType::Material,
                material,
                customKind,
                materialSampleBitmapIndex,
                originY,
                size);
        }

    private:

        Cell(
            CellIdType id,
            KindType kind,
            TMaterial const * material,
            CustomKindType customKind,
            int materialSampleBitmapIndex,
            int originY,
            wxSize size)
            : Id(id)
            , Kind(kind)
            , Material(material)
            , CustomKind(customKind)
            , IsChecked(false)
            , MaterialSampleBitmapIndex(materialSampleBitmapIndex)
            , Name1Width(0)
            , Name1YTopOffset(0)
            , Name2Width(0)
            , Name2YTopOffset(0)
            , DataWidth(0)
            , DataYTopOffset(0)
            , Rect(wxPoint(0, originY), size)
        { }
    };

    struct MaterialStride
    {
        // Made of N1 default materials, 1 AddNewButton, N2 user custom materials, N3 ship custom materials
        std::vector<Cell> Cells;
        size_t iUserCustomStartIndex; // Index in list of cells where user custom materials start
        size_t iShipCustomStartIndex; // Index in list of cells where ship custom materials start

        unsigned int const SubCategoryBaseMaterialOrdinal; // Ordinal in default materials's subcategory elements of the base from which this stride starts

        MaterialStride(unsigned int subCategoryBaseMaterialOrdinal)
            : Cells()
            , iUserCustomStartIndex(0)
            , iShipCustomStartIndex(0)
            , SubCategoryBaseMaterialOrdinal(subCategoryBaseMaterialOrdinal)
        { }
    };

    struct Row
    {
        enum class KindType
        {
            SubCategory,
            Separator
        };

        KindType const Kind;

        // Iff Kind==Subcategory
        std::string const SubCategory;
        std::vector<MaterialStride> Strides;

        // Layout
        wxRect Rect; // Origin set at cctor, Height set at cctor, Width set at Layout

        static Row MakeSubCategoryRow(
            std::string const & subCategory,
            wxPoint origin,
            int height)
        {
            return Row(
                KindType::SubCategory,
                subCategory,
                origin,
                height);
        }

        static Row MakeSeparatorRow(
            wxPoint origin,
            int height)
        {
            return Row(
                KindType::Separator,
                std::string(),
                origin,
                height);
        }

    private:

        Row(
            KindType kind,
            std::string const & subCategory,
            wxPoint origin,
            int height)
            : Kind(kind)
            , SubCategory(subCategory)
            , Strides()
            , Rect(origin, wxSize(0, height))
        { }
    };

    std::vector<Row> mRows;

    // Images are stored here, in order to limit number of GDI objects
    wxImageList mMaterialSampleBitmaps;

    //
    // Render style
    //

    wxPen mSelectionPen;
    wxPen mCreateNewFrameBorderPen;
    wxFont mNameFont;
    wxFont mDataFont;
    wxColor mTextForegroundColor;

    //
    // State
    //

    CellIdType mCurrentSelectedCellId;
    CellIdType mNextCellId;
};

}