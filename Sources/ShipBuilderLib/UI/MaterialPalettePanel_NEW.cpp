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

//
// Geometry
//

// Margin around the interior of the panel
int constexpr InternalWindowMargin = 4;

int constexpr CellHMargin = 0;
int constexpr CellVMargin = 0;

ImageSize constexpr MaterialSampleSize(80, 60);
int constexpr MaterialCellInnerMargin = 8;
int constexpr MateralSelectionFrameThickness = 2;

int constexpr SeparatorThickness = 1;

////////////////////////////////////////////////////////////////

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
    mBackgroundBrush = wxBrush(wxColour("WHITE"), wxBRUSHSTYLE_SOLID);

    //
    // Build style
    //

    mSeparatorBrush = wxBrush(wxColor(0xa0, 0xa0, 0xa0), wxBRUSHSTYLE_SOLID);

    // Make name font
    mNameFont = GetFont();

    // Make data font
    mDataFont = GetFont();
    mDataFont.SetPointSize(mDataFont.GetPointSize() - 1);

    //
    // Connect events
    //

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
    //
    // Select row
    //

    if (startNewRow || mRows.empty() || mRows.back().Kind == Row::KindType::Separator)
    {
        mRows.emplace_back(Row::KindType::Cells);
    }

    Row & row = mRows.back();

    //
    // Create cell
    //

    // Sample bitmap

    wxBitmap const sampleBitmap = MakeMaterialSample(material);

    // Text

    // TODOHERE

    // Store cell

    wxSize const cellSize = wxSize(
        MaterialCellInnerMargin + sampleBitmap.GetSize().GetWidth() + MaterialCellInnerMargin,
        MaterialCellInnerMargin + sampleBitmap.GetSize().GetHeight() + MaterialCellInnerMargin); // TODO: include text

    Cell & cell = row.Cells.emplace_back(
        Cell::KindType::Material,
        material,
        cellSize);

    cell.Bitmap = sampleBitmap;
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
                // We'll calculate size later
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

    // Create DC for rendering into buffer
    // TODO: try with wxMemoryDC
    wxBufferedDC dc(nullptr, *mRenderBuffer, wxBUFFER_VIRTUAL_AREA);

    //
    // Render panel
    //

    RenderPanel(dc, size);
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
void MaterialPalettePanel<TLayer>::RenderPanel(wxDC & dc, wxRect const & region)
{
    // Clear
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(mBackgroundBrush);
    dc.DrawRectangle(region);

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
                        switch (cell.Kind)
                        {
                            case Cell::KindType::CreateNewButton:
                            {
                                // TODO

                                break;
                            }

                            case Cell::KindType::Material:
                            {
                                RenderMaterialCell(cell, dc);

                                break;
                            }
                        }
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
void MaterialPalettePanel<TLayer>::RenderMaterialCell(
    Cell const & cell,
    wxDC & dc)
{
    assert(cell.Kind == Cell::KindType::Material);

    int const leftX = cell.Rect.GetX() + MaterialCellInnerMargin;
    int topY = cell.Rect.GetY() + MaterialCellInnerMargin;

    // Material sample

    dc.DrawBitmap(cell.Bitmap, leftX, topY);

    topY += cell.Bitmap.GetSize().GetHeight();

    // Text

    // TODOHERE

    // Selection

    if (cell.IsSelected)
    {
        // TODOTEST
        auto pen = wxPen(wxColor(0x20, 0x20, 0x20), 1, wxPENSTYLE_SOLID);
        dc.SetPen(pen);
        dc.DrawLine(cell.Rect.x, cell.Rect.y, cell.Rect.x + cell.Rect.width, cell.Rect.y + cell.Rect.height);
    }
}

template<LayerType TLayer>
wxBitmap MaterialPalettePanel<TLayer>::MakeMaterialSample(TMaterial const * material)
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

//
// Explicit specializations for all material layers
//

template class MaterialPalettePanel<LayerType::Structural>;
template class MaterialPalettePanel<LayerType::Electrical>;
template class MaterialPalettePanel<LayerType::Ropes>;

}