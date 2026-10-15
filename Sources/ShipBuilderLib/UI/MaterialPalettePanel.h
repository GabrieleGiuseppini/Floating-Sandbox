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

#include <Core/ImageData.h>

#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/imaglist.h>

#include <any>
#include <cstdint>
#include <memory>
#include <optional>
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

/*
 * Event fired when the checked-cell state of this panel has changed.
 */
class fsCheckedStateChanged : public wxEvent
{
public:

    fsCheckedStateChanged(
        wxEventType eventType,
        int winid)
        : wxEvent(winid, eventType)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    fsCheckedStateChanged(fsCheckedStateChanged  const & other)
        : wxEvent(other)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent * Clone() const override
    {
        return new fsCheckedStateChanged(*this);
    }
};

wxDECLARE_EVENT(fsEVT_MATERIAL_PALETTE_CHECKED_STATE_CHANGED, fsCheckedStateChanged);

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

    bool IsAnyChecked() const
    {
        return mCheckedCellsCount > 0;
    }

    void StartDefaultMaterialsLayout();
    void StartNewSubcategoryRow(std::string const & subCategory);
    void StartNewMaterialStride(unsigned int subCategoryBaseMaterialOrdinal);
    void AddDefaultMaterial(TMaterial const * material);
    void AddCreateNewCustomMaterialButton(TMaterial const * parentMaterial);
    void AddSeparatorRow();
    void EndDefaultMaterialsLayout();

    std::any StartCustomMaterialsLayout();
    std::any RemoveAllShipCustomMaterials(std::any layoutSession);
    std::any AddCustomMaterial(TMaterial const * material, CustomMaterialRegisterKindType registerKind, std::any layoutSession);
    void EndCustomMaterialsLayout(std::any layoutSession);

    void SetSelected(TMaterial const * material);

private:

    using CellIdType = std::uint64_t;
    static CellIdType constexpr NoneCellId = std::numeric_limits<CellIdType>::max();

    struct Cell;
    struct Row;

    void OnPaint(wxPaintEvent & event);
    void OnMouseLeave();
    void OnMouseMoved(wxMouseEvent & event);
    void OnMouseLeftDown(wxMouseEvent & event);
    void OnMouseLeftUp(wxMouseEvent & event);

    void OnEditButton(Cell & cell);
    void OnCheckboxToggled(Cell & cell);

    struct LayoutSession
    {
        // Row index (Subcategory only) => <Leftmost Stride Index, Leftmost Cell Index in that stride>
        std::map<size_t, std::tuple<size_t, size_t>> DirtyBoundaries;

        void RecordDirtyBoundary(
            size_t rowIndex,
            size_t strideIndex,
            size_t cellIndex)
        {
            auto [it, isInserted] = DirtyBoundaries.try_emplace(rowIndex, std::make_tuple(strideIndex, cellIndex));
            if (!isInserted)
            {
                if (std::get<0>(it->second) > strideIndex)
                {
                    it->second = std::make_tuple(strideIndex, cellIndex);
                }
                else if (std::get<0>(it->second) == strideIndex)
                {
                    std::get<1>(it->second) = std::min(std::get<1>(it->second), cellIndex);
                }
            }
        }
    };

    LayoutSession AppendCustomMaterial(
        TMaterial const * material,
        CustomMaterialRegisterKindType registerKind,
        LayoutSession layoutSession);

    size_t AppendMaterialCell(
        TMaterial const * material,
        std::optional<CustomMaterialRegisterKindType> customRegisterKind,
        size_t iRow,
        size_t iStride);

    void LayoutAndRender(
        LayoutSession const & layoutSession,
        bool doRenderSeparators);

    std::unique_ptr<wxMemoryDC> MakeDc();

    void RenderPanel(
        LayoutSession const & layoutSession,
        bool doRenderSeparators);

    void RenderCell(Cell const & cell);
    void RenderCell(Cell const & cell, wxDC & dc);

    wxBitmap MakeMaterialSample(
        TMaterial const * material,
        std::optional<wxRect> shipOverlayRect,
        std::optional<wxRect> editButtonRect,
        std::optional<wxRect> checkboxRect) const;

    size_t FindRowFor(TMaterial const * material) const;
    size_t FindStrideFor(TMaterial const * material, Row const & row) const;
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
        CellIdType Id; // const, but need assignment

        enum class KindType
        {
            CreateNewButton,
            Material
        };

        KindType Kind; // const, but need assignment

        // Iff Kind==CreateNewButton|Material
        TMaterial const * Material; // CreateNewButton:parent material|Material:material itself
        // Iff Kind==Material
        std::optional<CustomMaterialRegisterKindType> CustomRegisterKind; // const, but need assignment
        int MaterialSampleBitmapIndex; // const, but need assignment
        wxString Name1;
        int Name1Width;
        int Name1YTopOffset; // Relative to cell
        wxString Name2;
        int Name2Width;
        int Name2YTopOffset; // Relative to cell
        wxString Data;
        int DataWidth;
        int DataYTopOffset; // Relative to cell
        std::optional<wxRect> EditButtonRect; // Iff customRegisterKind!=None; Relative to cell + margin (relative to sample thumbnail)
        std::optional<wxRect> CheckboxRect; // Iff customRegisterKind!=None; Relative to cell + margin (relative to sample thumbnail)
        bool IsChecked; // Iff customRegisterKind!=None

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
                std::nullopt,
                -1,
                originY,
                size);
        }

        static Cell MakeMaterialCell(
            CellIdType id,
            TMaterial const * material,
            std::optional<CustomMaterialRegisterKindType> customRegisterKind,
            int materialSampleBitmapIndex,
            int originY,
            wxSize size)
        {
            return Cell(
                id,
                KindType::Material,
                material,
                customRegisterKind,
                materialSampleBitmapIndex,
                originY,
                size);
        }

    private:

        Cell(
            CellIdType id,
            KindType kind,
            TMaterial const * material,
            std::optional<CustomMaterialRegisterKindType> customRegisterKind,
            int materialSampleBitmapIndex,
            int originY,
            wxSize size)
            : Id(id)
            , Kind(kind)
            , Material(material)
            , CustomRegisterKind(customRegisterKind)
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
    // ...and here we track which are free to use, to avoid
    // having to renumber entries
    std::vector<size_t> mFreeMaterialSampleBitmapIndices;

    //
    // Reused resources
    //

    RgbaImageData const mShipOverlayImage;
    RgbaImageData const mCheckboxOffImage;
    RgbaImageData const mEditButtonImage;

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
    size_t mCheckedCellsCount;
};

}