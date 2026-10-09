/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-08-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalettePanel.h"

#include <UILib/SharedUIResources.h>
#include <UILib/WxHelpers.h>

#include <Core/Log.h>

#include <cassert>
#include <iomanip>
#include <sstream>

namespace ShipBuilder {

//
// Geometry
//

// Margin around the interior of the panel
int constexpr InternalWindowMargin = 4;

// Spacing around cells
int constexpr CellHSpacing = 0;
int constexpr CellVSpacing = 0;

// Cell
int constexpr CellInnerMargin = 8;
ImageSize constexpr MaterialSampleSize(80, 60);

int constexpr SelectionFrameThickness = 1;
static_assert(SelectionFrameThickness < CellInnerMargin); // To fit selection frame inside cell

int constexpr MaterialSampleToNameGapHeight = 2;
int constexpr NameTextHeight = 9;
int constexpr NameToNameGapHeight = 0;
int constexpr NameToDataGapHeight = 2;
int constexpr DataTextHeight = 8;

ImageSize constexpr MaterialCellSize(
    CellInnerMargin + MaterialSampleSize.width + CellInnerMargin,
    CellInnerMargin + MaterialSampleSize.height + MaterialSampleToNameGapHeight + NameTextHeight + NameToNameGapHeight + NameTextHeight + NameToDataGapHeight + DataTextHeight + CellInnerMargin);

int constexpr SeparatorThickness = 1;
int constexpr SeparatorRowHeight = 1 + SeparatorThickness + 1;

int constexpr StrideHSpacing = 8;

////////////////////////////////////////////////////////////////

wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_OUT, fsStructuralMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_IN, fsStructuralMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_CLICKED, fsStructuralMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_OUT, fsElectricalMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_IN, fsElectricalMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_CLICKED, fsElectricalMaterialPaletteEvent);

template<LayerType TLayer>
MaterialPalettePanel<TLayer>::MaterialPalettePanel(
    wxWindow * parent,
    IMaterialPalettesController & materialPalettesController,
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager)
    : wxPanel(parent)
    , mMaterialPalettesController(materialPalettesController)
    , mShipTexturizer(shipTexturizer)
    , mGameAssetManager(gameAssetManager)
    , mRenderBuffer() // Start empty
    , mRows() // Start empty
    , mMaterialSampleBitmaps(MaterialSampleSize.width, MaterialSampleSize.height, false)
    // State
    , mCurrentSelectedCellId(NoneCellId)
    , mNextCellId(0)
{
    SetBackgroundColour(wxColour("WHITE"));

    //
    // Build style
    //

    wxColor const baseColor1 = wxColor(0x00, 0x78, 0xd4);

    mSelectionPen = wxPen(baseColor1, SelectionFrameThickness, wxPENSTYLE_SOLID);
    mCreateNewFrameBorderPen = wxPen(baseColor1, 1, wxPENSTYLE_SHORT_DASH);

    // Make name font
    mNameFont = GetFont();
    mNameFont.SetPointSize(mNameFont.GetPointSize());

    // Make data font
    mDataFont = GetFont();
    mDataFont.SetPointSize(mDataFont.GetPointSize() - 1);

    mTextForegroundColor = wxColour("BLACK");

    //
    // Connect events
    //

    using PanelClass = MaterialPalettePanel<TLayer>;
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&PanelClass::OnPaint);
    Connect(this->GetId(), wxEVT_LEAVE_WINDOW, (wxObjectEventFunction)&PanelClass::OnMouseLeave);
    Connect(this->GetId(), wxEVT_MOTION, (wxObjectEventFunction)&PanelClass::OnMouseMoved);
    Connect(this->GetId(), wxEVT_LEFT_DOWN, (wxObjectEventFunction)&PanelClass::OnMouseLeftDown);
    Connect(this->GetId(), wxEVT_LEFT_UP, (wxObjectEventFunction)&PanelClass::OnMouseLeftUp);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::StartDefaultMaterialsLayout()
{
    mRenderBuffer.reset();
    mRows.clear();
    mMaterialSampleBitmaps.RemoveAll();
    mCurrentSelectedCellId = NoneCellId;
    mNextCellId = 0;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::StartNewSubcategoryRow(std::string const & subCategory)
{
    wxPoint const rowOrigin(
        InternalWindowMargin,
        mRows.empty() ? InternalWindowMargin : mRows.back().Rect.y + mRows.back().Rect.height + CellVSpacing);

    mRows.emplace_back(Row::MakeSubCategoryRow(subCategory, rowOrigin, MaterialCellSize.height));
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::StartNewMaterialStride(unsigned int subCategoryBaseMaterialOrdinal)
{
    assert(!mRows.empty());
    assert(mRows.back().Kind == Row::KindType::SubCategory);

    mRows.back().Strides.emplace_back(subCategoryBaseMaterialOrdinal);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::AddDefaultMaterial(TMaterial const * material)
{
    assert(!mRows.empty());
    assert(mRows.back().Kind == Row::KindType::SubCategory);
    assert(!mRows.back().Strides.empty());

    Row & row = mRows.back();
    MaterialStride & stride = row.Strides.back();
    assert(material->PaletteSubCategoryBaseMaterialOrdinal == stride.SubCategoryBaseMaterialOrdinal);

    //
    // Store cell
    //

    int const materialSampleBitmapIndex = mMaterialSampleBitmaps.Add(MakeMaterialSample(material));

    Cell & cell = stride.Cells.emplace_back(
        Cell::MakeMaterialCell(
            MakeNextCellId(),
            material,
            Cell::CustomKindType::None,
            materialSampleBitmapIndex,
            row.Rect.GetTop(),
            wxSize(
                MaterialCellSize.width,
                MaterialCellSize.height)));
    stride.iShipCustomStartIndex++;
    stride.iUserCustomStartIndex++;

    //
    // Layout text
    //
    // Assumption: text is normalized (wrt whitespaces, etc.)
    //

    int currentTopYOffset =
        CellInnerMargin
        + MaterialSampleSize.height
        + MaterialSampleToNameGapHeight;

    auto const previousFont = GetFont();

    // Name

    SetFont(mNameFont);

    auto nameSizeWidth = GetTextExtent(material->Name).GetWidth();
    if (nameSizeWidth > MaterialSampleSize.width)
    {
        int lastSpaceIndex = -1;
        while (true)
        {
            auto const nextSpace = material->Name.find(' ', lastSpaceIndex + 1);
            if (nextSpace == std::string::npos
                || GetTextExtent(material->Name.substr(0, nextSpace)).GetWidth() > MaterialSampleSize.width)
            {
                // Use up to last space
                if (lastSpaceIndex > 0)
                {
                    cell.Name1 = material->Name.substr(0, lastSpaceIndex);
                    cell.Name2 = TruncateAsNeeded(material->Name.substr(lastSpaceIndex + 1), MaterialSampleSize.width);
                }
                else
                {
                    // Single string, too long though
                    cell.Name1 = TruncateAsNeeded(material->Name, MaterialSampleSize.width);
                    cell.Name2 = "";
                }

                break;
            }
            else
            {
                // Up to this space would be good, continue searching
                lastSpaceIndex = nextSpace;
            }
        }
    }
    else
    {
        // Fits all
        cell.Name1 = material->Name;
        cell.Name2 = "";
    }

    auto const name1Size = GetTextExtent(cell.Name1);
    cell.Name1Width = name1Size.GetWidth();
    cell.Name1YTopOffset = currentTopYOffset;

    currentTopYOffset += NameTextHeight;

    if (!cell.Name2.IsEmpty())
    {
        currentTopYOffset += NameToNameGapHeight;

        auto const name2Size = GetTextExtent(cell.Name2);
        cell.Name2Width = name2Size.GetWidth();
        cell.Name2YTopOffset = currentTopYOffset;

        currentTopYOffset += NameTextHeight;
    }

    // Data

    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        SetFont(mDataFont);

        std::stringstream ss;

        ss << std::fixed << std::setprecision(2)
            << "M:" << material->GetMass()
            << "    "
            << "S:" << material->Strength;

        cell.Data = ss.str();

        currentTopYOffset += NameToDataGapHeight;

        auto const dataSize = GetTextExtent(cell.Data);
        cell.DataWidth = dataSize.GetWidth();
        cell.DataYTopOffset = currentTopYOffset;

        currentTopYOffset += DataTextHeight;
    }

    currentTopYOffset += CellInnerMargin;

    assert(currentTopYOffset <= MaterialCellSize.height);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::AddCreateNewCustomMaterialButton(TMaterial const * parentMaterial)
{
    assert(!mRows.empty());
    assert(mRows.back().Kind == Row::KindType::SubCategory);
    assert(!mRows.back().Strides.empty());

    Row & row = mRows.back();
    MaterialStride & stride = row.Strides.back();
    assert(parentMaterial->PaletteSubCategoryBaseMaterialOrdinal == stride.SubCategoryBaseMaterialOrdinal);

    //
    // Store cell
    //

    stride.Cells.emplace_back(
        Cell::MakeCreateNewButtonCell(
            MakeNextCellId(),
            parentMaterial,
            row.Rect.GetTop(),
            wxSize(
                MaterialCellSize.width,
                MaterialCellSize.height)));
    stride.iShipCustomStartIndex++;
    stride.iUserCustomStartIndex++;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::AddSeparatorRow()
{
    assert(!mRows.empty()); // Ugly otherwise

    wxPoint const rowOrigin(
        InternalWindowMargin,
        mRows.empty() ? InternalWindowMargin : mRows.back().Rect.y + mRows.back().Rect.height + CellVSpacing);

    mRows.emplace_back(Row::MakeSeparatorRow(rowOrigin, SeparatorRowHeight));
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::EndDefaultMaterialsLayout()
{
    assert(!mRows.empty());

    //
    // Layout:
    //  - Calculate cells' x's
    //  - Calculate max width
    //  - Set row widths
    //

    int maxRowWidth = 0;

    for (size_t iRow = 0; iRow < mRows.size(); ++iRow)
    {
        Row & row = mRows[iRow];

        if (row.Kind == Row::KindType::SubCategory)
        {
            // Layout cells

            int currentX = InternalWindowMargin;

            for (size_t iStride = 0; iStride < row.Strides.size(); ++iStride)
            {
                MaterialStride & stride = row.Strides[iStride];

                if (iStride > 0)
                {
                    currentX += StrideHSpacing;
                }

                for (size_t iCell = 0; iCell < stride.Cells.size(); ++iCell)
                {
                    Cell & cell = stride.Cells[iCell];

                    if (iCell > 0)
                    {
                        currentX += CellHSpacing;
                    }

                    cell.Rect.x = currentX;

                    currentX += cell.Rect.width;
                }
            }

            currentX += InternalWindowMargin;

            // Set row width
            row.Rect.width = currentX;

            // Maintain max row width
            maxRowWidth = std::max(maxRowWidth, currentX);
        }
    }

    // Calculate total panel size
    wxSize const panelSize(maxRowWidth, mRows.back().Rect.y + mRows.back().Rect.height);

    // Set panel size
    SetSize(panelSize);
    SetMinSize(panelSize);

    // Finalize separators' layouts
    for (auto & row : mRows)
    {
        if (row.Kind == Row::KindType::Separator)
        {
            // For separators, the rect is the actual separator rectangle
            row.Rect.width = panelSize.GetWidth() - 2 * InternalWindowMargin;
        }
    }

    //
    // Create buffer
    //

    assert(!mRenderBuffer);
    mRenderBuffer = std::make_unique<wxBitmap>(panelSize);

    //
    // Render panel
    //

    RenderPanel(panelSize);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::SetSelected(TMaterial const * material)
{
    auto const * cell = FindCellFor(material);
    if (cell != nullptr)
    {
        if (cell->Id != mCurrentSelectedCellId)
        {
            ToggleSelectionTo(*cell);

            Refresh(false);
        }
    }
    else if (mCurrentSelectedCellId != NoneCellId)
    {
        ToggleSelectionToNone();

        Refresh(false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnPaint(wxPaintEvent & /*event*/)
{
    assert(mRenderBuffer);

    wxPaintDC dc(this);
    dc.DrawBitmap(*mRenderBuffer, 0, 0);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnMouseLeave()
{
    if (mCurrentSelectedCellId != NoneCellId)
    {
        ToggleSelectionToNone();

        Refresh(false);
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnMouseMoved(wxMouseEvent & event)
{
    auto const * cell = FindCellAt(event.GetPosition());
    if (cell != nullptr)
    {
        if (cell->Id != mCurrentSelectedCellId)
        {
            ToggleSelectionTo(*cell);

            Refresh(false);
        }
    }
    else if (mCurrentSelectedCellId != NoneCellId)
    {
        ToggleSelectionToNone();

        Refresh(false);
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnMouseLeftDown(wxMouseEvent & event)
{
    auto const * cell = FindCellAt(event.GetPosition());
    if (cell != nullptr && cell->Kind == Cell::KindType::Material)
    {
        switch (cell->Kind)
        {
            case Cell::KindType::CreateNewButton:
            {
                // TODO
                break;
            }

            case Cell::KindType::Material:
            {
                assert(cell->Material != nullptr);

                // Fire clicked event
                if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
                {
                    auto eventToFire = fsStructuralMaterialPaletteEvent(
                        fsEVT_STRUCTURAL_MATERIAL_PALETTE_CLICKED,
                        this->GetId(),
                        cell->Material);

                    ProcessWindowEvent(eventToFire);
                }
                else
                {
                    assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

                    auto eventToFire = fsElectricalMaterialPaletteEvent(
                        fsEVT_ELECTRICAL_MATERIAL_PALETTE_CLICKED,
                        this->GetId(),
                        cell->Material);

                    ProcessWindowEvent(eventToFire);
                }

                break;
            }
        }
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnMouseLeftUp(wxMouseEvent & event)
{
    // TODOHERE: button feedback if any, otherwise nuke
    (void)event;
}

template<LayerType TLayer>
std::unique_ptr<wxMemoryDC> MaterialPalettePanel<TLayer>::MakeDc()
{
    return std::make_unique<wxMemoryDC>(*mRenderBuffer);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::RenderPanel(wxRect const & region)
{
    // Create DC for rendering into buffer
    auto dc_ptr = MakeDc();
    auto & dc = *dc_ptr;

    // Clear
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(SharedUIResources::GetInstance().GetWhiteSolidBackgroundBrush());
    dc.DrawRectangle(region);

    // Setup
    dc.SetTextForeground(mTextForegroundColor);

    // Visit all rows intersecting region
    for (Row const & row : mRows)
    {
        if (region.Intersects(row.Rect))
        {
            //
            // Draw row
            //

            switch (row.Kind)
            {
                case Row::KindType::SubCategory:
                {
                    for (MaterialStride const & stride : row.Strides)
                    {
                        for (Cell const & cell : stride.Cells)
                        {
                            RenderCell(cell, dc);
                        }
                    }

                    break;
                }

                case Row::KindType::Separator:
                {
                    int const centerY = row.Rect.y + row.Rect.height / 2;

                    wxRect const separatorRect = wxRect(
                        row.Rect.x,
                        centerY - SeparatorRowHeight / 2,
                        row.Rect.width,
                        SeparatorThickness);

                    dc.SetPen(*wxTRANSPARENT_PEN);
                    dc.SetBrush(SharedUIResources::GetInstance().GetSeparatorBrush());
                    dc.DrawRectangle(separatorRect);

                    break;
                }
            }
        }
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::RenderCell(Cell const & cell)
{
    // Create DC for rendering into buffer
    auto dc_ptr = MakeDc();
    auto & dc = *dc_ptr;

    // Clear
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(SharedUIResources::GetInstance().GetWhiteSolidBackgroundBrush());
    dc.DrawRectangle(cell.Rect);

    // Render cell
    RenderCell(cell, dc);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::RenderCell(
    Cell const & cell,
    wxDC & dc)
{
    int const leftX = cell.Rect.GetX() + CellInnerMargin;
    int const centerX = cell.Rect.GetX() + cell.Rect.GetWidth() / 2;

    switch (cell.Kind)
    {
        case Cell::KindType::CreateNewButton:
        {
            // Add new button

            // Frame
            dc.SetPen(mCreateNewFrameBorderPen);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRoundedRectangle(
                leftX,
                cell.Rect.GetY() + CellInnerMargin,
                MaterialSampleSize.width,
                MaterialSampleSize.height,
                5.0);

            // Plus
            auto const & bitmap = SharedUIResources::GetInstance().GetAddNewMaterialPlusIcon();
            dc.DrawBitmap(
                bitmap,
                centerX - bitmap.GetWidth() / 2,
                cell.Rect.GetY() + CellInnerMargin + MaterialSampleSize.height / 2 - bitmap.GetHeight() / 2);

            break;
        }

        case Cell::KindType::Material:
        {
            // Material sample

            assert(cell.MaterialSampleBitmapIndex < mMaterialSampleBitmaps.GetImageCount());
            mMaterialSampleBitmaps.Draw(
                cell.MaterialSampleBitmapIndex,
                dc,
                leftX,
                cell.Rect.GetY() + CellInnerMargin,
                wxIMAGELIST_DRAW_NORMAL,
                true);

            // Name

            int nameX = centerX - cell.Name1Width / 2;
            dc.SetFont(mNameFont);
            dc.DrawText(cell.Name1, nameX, cell.Rect.GetY() + cell.Name1YTopOffset);

            if (!cell.Name2.IsEmpty())
            {
                nameX = centerX - cell.Name2Width / 2;
                dc.DrawText(cell.Name2, nameX, cell.Rect.GetY() + cell.Name2YTopOffset);
            }

            // Data

            if (!cell.Data.IsEmpty())
            {
                int const dataX = centerX - cell.DataWidth / 2;
                dc.SetFont(mDataFont);
                dc.DrawText(cell.Data, dataX, cell.Rect.GetY() + cell.DataYTopOffset);
            }

            // Edit button

            // TODO

            // Checkbox

            // TODO

            // Ship icon overlay

            // TODO

            break;
        }
    }

    // Selection

    if (cell.Id == mCurrentSelectedCellId)
    {
        dc.SetPen(mSelectionPen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(
            cell.Rect.GetX() + CellInnerMargin / 2 - SelectionFrameThickness / 2,
            cell.Rect.GetY() + CellInnerMargin / 2 - SelectionFrameThickness / 2,
            cell.Rect.GetWidth() - CellInnerMargin + SelectionFrameThickness - 1,
            cell.Rect.GetHeight() - CellInnerMargin + SelectionFrameThickness - 1);
    }
}

template<LayerType TLayer>
wxBitmap MaterialPalettePanel<TLayer>::MakeMaterialSample(TMaterial const * material) const
{
    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        ShipAutoTexturizationSettings texturizationSettings;
        texturizationSettings.MaterialTextureMagnification = 0.5f;

        return WxHelpers::MakeBitmap(
            mShipTexturizer.MakeMaterialTextureSample(
                texturizationSettings,
                MaterialSampleSize,
                *material,
                mGameAssetManager));
    }
    else
    {
        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

        return WxHelpers::MakeMatteBitmap(
            rgbaColor(material->RenderColor, 255),
            MaterialSampleSize);
    }
}

template<LayerType TLayer>
typename MaterialPalettePanel<TLayer>::Cell * MaterialPalettePanel<TLayer>::FindCell(CellIdType const & id)
{
    for (auto & row : mRows)
    {
        if (row.Kind == Row::KindType::SubCategory)
        {
            for (auto & stride : row.Strides)
            {
                for (auto & cell : stride.Cells)
                {
                    if (cell.Id == id)
                    {
                        return &cell;
                    }
                }
            }
        }
    }

    return nullptr;
}

template<LayerType TLayer>
typename MaterialPalettePanel<TLayer>::Cell * MaterialPalettePanel<TLayer>::FindCellAt(wxPoint const & position)
{
    for (auto & row : mRows)
    {
        if (row.Rect.Contains(position))
        {
            if (row.Kind == Row::KindType::SubCategory)
            {
                for (auto & stride : row.Strides)
                {
                    for (auto & cell : stride.Cells)
                    {
                        if (cell.Rect.Contains(position))
                        {
                            return &cell;
                        }
                    }
                }
            }

            // In this row, but no cell found
            break;
        }
    }

    return nullptr;
}

template<LayerType TLayer>
typename MaterialPalettePanel<TLayer>::Cell * MaterialPalettePanel<TLayer>::FindCellFor(TMaterial const * material)
{
    assert(material != nullptr);

    for (auto & row : mRows)
    {
        if (row.Kind == Row::KindType::SubCategory)
        {
            for (auto & stride : row.Strides)
            {
                for (auto & cell : stride.Cells)
                {
                    if (cell.Kind == Cell::KindType::Material && cell.Material == material)
                    {
                        return &cell;
                    }
                }
            }
        }
    }

    return nullptr;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::ToggleSelectionTo(Cell const & cell)
{
    assert(cell.Id != mCurrentSelectedCellId);

    if (mCurrentSelectedCellId != NoneCellId)
    {
        ToggleSelectionToNone();
    }

    mCurrentSelectedCellId = cell.Id;

    RenderCell(cell);

    if (cell.Kind == Cell::KindType::Material)
    {
        // Fire hovered-in event
        if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
        {
            auto eventToFire = fsStructuralMaterialPaletteEvent(
                fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_IN,
                this->GetId(),
                cell.Material);

            ProcessWindowEvent(eventToFire);
        }
        else
        {
            assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

            auto eventToFire = fsElectricalMaterialPaletteEvent(
                fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_IN,
                this->GetId(),
                cell.Material);

            ProcessWindowEvent(eventToFire);
        }
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::ToggleSelectionToNone()
{
    assert(mCurrentSelectedCellId != NoneCellId);

    Cell * oldSelectedCell = FindCell(mCurrentSelectedCellId);
    assert(oldSelectedCell != nullptr);

    mCurrentSelectedCellId = NoneCellId;

    if (oldSelectedCell) // For safety
    {
        RenderCell(*oldSelectedCell);

        if (oldSelectedCell->Kind == Cell::KindType::Material)
        {
            // Fire hovered-out event
            if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
            {
                auto eventToFire = fsStructuralMaterialPaletteEvent(
                    fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_OUT,
                    this->GetId(),
                    nullptr);

                ProcessWindowEvent(eventToFire);
            }
            else
            {
                assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

                auto eventToFire = fsElectricalMaterialPaletteEvent(
                    fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_OUT,
                    this->GetId(),
                    nullptr);

                ProcessWindowEvent(eventToFire);
            }
        }
    }
}

template<LayerType TLayer>
wxString MaterialPalettePanel<TLayer>::TruncateAsNeeded(std::string const & input, int maxWidth) const
{
    wxString wxText = wxString(input);
    wxSize textSize = GetTextExtent(wxText);
    while (textSize.GetWidth() > maxWidth
        && wxText.Len() > 3)
    {
        // Make ellipsis
        wxText.Truncate(wxText.Len() - 4).Append("...");

        // Recalc width now
        textSize = GetTextExtent(wxText);
    }

    return wxText;
}

template<LayerType TLayer>
typename MaterialPalettePanel<TLayer>::CellIdType MaterialPalettePanel<TLayer>::MakeNextCellId()
{
    return mNextCellId++;
}

//
// Explicit specializations for all material layers
//

template class MaterialPalettePanel<LayerType::Structural>;
template class MaterialPalettePanel<LayerType::Electrical>;
template class MaterialPalettePanel<LayerType::Ropes>;

}