/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-08-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalettePanel.h"

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

int constexpr CellHMargin = 0;
int constexpr CellVMargin = 0;
int constexpr CellInnerMargin = 8;

ImageSize constexpr MaterialSampleSize(80, 60);

int constexpr SelectionFrameThickness = 1;
static_assert(SelectionFrameThickness < CellInnerMargin); // To fit selection frame inside cell

int constexpr SeparatorThickness = 1;

int constexpr MaterialSampleToNameGapHeight = 2;
int constexpr NameToNameGapHeight = 0;
int constexpr NameToDataGapHeight = 2;

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
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager)
    : wxPanel(parent)
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
    mBackgroundBrush = wxBrush(wxColour("WHITE"), wxBRUSHSTYLE_SOLID);

    //
    // Build style
    //

    mSeparatorBrush = wxBrush(wxColor(0xa0, 0xa0, 0xa0), wxBRUSHSTYLE_SOLID);
    mSelectionPen = wxPen(wxColor(0x00, 0x78, 0xd4), SelectionFrameThickness, wxPENSTYLE_SOLID);

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
void MaterialPalettePanel<TLayer>::StartBuild()
{
    mRenderBuffer.reset();
    mRows.clear();
    mMaterialSampleBitmaps.RemoveAll();
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::Add(
    TMaterial const * material,
    bool startNewRow)
{
    //
    // Prepare row
    //

    if (startNewRow || mRows.empty() || mRows.back().Kind == Row::KindType::Separator)
    {
        mRows.emplace_back(Row::KindType::Cells);
    }

    Row & row = mRows.back();

    //
    // Create cell
    //

    int currentTopYOffset = CellInnerMargin;

    // Sample bitmap

    int const materialSampleBitmapIndex = mMaterialSampleBitmaps.Add(MakeMaterialSample(material));

    // Store cell

    int const innerCellWidth = MaterialSampleSize.width;

    wxSize const cellSize = wxSize(
        CellInnerMargin + innerCellWidth + CellInnerMargin,
        0); // Recalculated later

    Cell & cell = row.Cells.emplace_back(
        MakeNextCellId(),
        Cell::KindType::Material,
        material,
        cellSize);

    cell.MaterialSampleBitmapIndex = materialSampleBitmapIndex;
    cell.MaterialSampleBitmapYTopOffset = currentTopYOffset;

    currentTopYOffset +=
        MaterialSampleSize.height
        + MaterialSampleToNameGapHeight;

    //
    // Text
    //
    // Assumption: text is normalized (wrt whitespaces, etc.)
    //

    auto const previousFont = GetFont();

    // Name

    SetFont(mNameFont);

    auto nameSizeWidth = GetTextExtent(material->Name).GetWidth();
    if (nameSizeWidth > innerCellWidth)
    {
        int lastSpaceIndex = -1;
        while (true)
        {
            auto const nextSpace = material->Name.find(' ', lastSpaceIndex + 1);
            if (nextSpace == std::string::npos
                || GetTextExtent(material->Name.substr(0, nextSpace)).GetWidth() > innerCellWidth)
            {
                // Use up to last space
                if (lastSpaceIndex > 0)
                {
                    cell.Name1 = material->Name.substr(0, lastSpaceIndex);
                    cell.Name2 = TruncateAsNeeded(material->Name.substr(lastSpaceIndex + 1), innerCellWidth);
                }
                else
                {
                    // Single string, too long though
                    cell.Name1 = TruncateAsNeeded(material->Name, innerCellWidth);
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

    currentTopYOffset += name1Size.GetHeight();

    if (!cell.Name2.IsEmpty())
    {
        currentTopYOffset += NameToNameGapHeight;

        auto const name2Size = GetTextExtent(cell.Name2);
        cell.Name2Width = name2Size.GetWidth();
        cell.Name2YTopOffset = currentTopYOffset;

        currentTopYOffset += name2Size.GetHeight();
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

        currentTopYOffset += dataSize.GetHeight();
    }

    currentTopYOffset += CellInnerMargin;

    //
    // Store final height
    //

    cell.Rect.SetHeight(currentTopYOffset);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::AddSeparator()
{
    assert(!mRows.empty()); // Ugly otherwise

    mRows.emplace_back(Row::KindType::Separator);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::AddCreateNewButton(TMaterial const * parentMaterial)
{
    // TODO
    (void)parentMaterial;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::EndBuild()
{
    assert(!mRows.empty());

    //
    // Layout and calculate size
    //

    int currentY = InternalWindowMargin;

    int maxRowWidth = 0;

    for (size_t iRow = 0; iRow < mRows.size(); ++iRow)
    {
        Row & row = mRows[iRow];

        int currentX = InternalWindowMargin;

        if (iRow > 0)
        {
            currentY += CellVMargin;
        }

        row.Rect.SetPosition(wxPoint(currentX, currentY));

        int rowHeight = 0;
        switch (row.Kind)
        {
            case Row::KindType::Cells:
            {
                for (size_t iCell = 0; iCell < row.Cells.size(); ++iCell)
                {
                    Cell & cell = row.Cells[iCell];

                    if (iCell > 0)
                    {
                        currentX += CellHMargin;
                    }

                    cell.Rect.SetPosition(wxPoint(currentX, currentY));

                    currentX += cell.Rect.GetWidth();

                    rowHeight = std::max(rowHeight, cell.Rect.GetHeight());
                }

                break;
            }

            case Row::KindType::Separator:
            {
                // We'll calculate width later

                rowHeight = SeparatorThickness;

                break;
            }
        }

        currentX += InternalWindowMargin;

        // Set row size
        row.Rect.SetSize(wxSize(currentX, rowHeight));

        // Maintain max row width
        maxRowWidth = std::max(maxRowWidth, row.Rect.GetWidth());

        currentY += rowHeight;
    }

    currentY += InternalWindowMargin;

    // Calculate total panel size
    wxSize const size(maxRowWidth, currentY);

    // Set panel size
    SetSize(size);
    SetMinSize(size);

    // Finalize separators' layouts
    for (auto & row : mRows)
    {
        if (row.Kind == Row::KindType::Separator)
        {
            row.Rect.SetSize(wxSize(size.GetWidth() - 2 * InternalWindowMargin, SeparatorThickness));
        }
    }

    //
    // Create buffer
    //

    assert(!mRenderBuffer);
    mRenderBuffer = std::make_unique<wxBitmap>(size);

    //
    // Render panel
    //

    RenderPanel(size);
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
    dc.SetBrush(mBackgroundBrush);
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
                case Row::KindType::Cells:
                {
                    for (Cell const & cell : row.Cells)
                    {
                        RenderCell(cell, dc);
                    }

                    break;
                }

                case Row::KindType::Separator:
                {
                    dc.SetPen(*wxTRANSPARENT_PEN);
                    dc.SetBrush(mSeparatorBrush);
                    dc.DrawRectangle(row.Rect);

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
    dc.SetBrush(mBackgroundBrush);
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
            // TODO

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
                cell.Rect.GetY() + cell.MaterialSampleBitmapYTopOffset,
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
        if (row.Kind == Row::KindType::Cells)
        {
            for (auto & cell : row.Cells)
            {
                if (cell.Id == id)
                {
                    return &cell;
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
            if (row.Kind == Row::KindType::Cells)
            {
                for (auto & cell : row.Cells)
                {
                    if (cell.Rect.Contains(position))
                    {
                        return &cell;
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
        if (row.Kind == Row::KindType::Cells)
        {
            for (auto & cell : row.Cells)
            {
                if (cell.Kind == Cell::KindType::Material && cell.Material == material)
                {
                    return &cell;
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