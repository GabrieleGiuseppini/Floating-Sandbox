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
int constexpr CellInnerMargin = 4;
ImageSize constexpr MaterialSampleSize(64, 46);

int constexpr SelectionFrameThickness = 1;
static_assert(SelectionFrameThickness < CellInnerMargin); // To fit selection frame inside cell

int constexpr MaterialSampleToNameGapHeight = 2;
int constexpr NameTextHeight = 9;
int constexpr NameToNameGapHeight = 1;
int constexpr NameToDataGapHeight = 2;
int constexpr DataTextHeight = 8;

ImageSize constexpr MaterialCellSize(
    CellInnerMargin + MaterialSampleSize.width + CellInnerMargin,
    CellInnerMargin + MaterialSampleSize.height + MaterialSampleToNameGapHeight + NameTextHeight + NameToNameGapHeight + NameTextHeight + NameToDataGapHeight + DataTextHeight + CellInnerMargin);

int constexpr SeparatorThickness = 1;
int constexpr SeparatorRowHeight = 1 + SeparatorThickness + 1;

int const MaterialSampleOverlayMargin = 1;

// To guarantee aligned cols
int constexpr StrideHSpacing = CellHSpacing + MaterialCellSize.width + CellHSpacing;

//
// Style
//

static wxColor const BaseColor1 = wxColor(0x00, 0x78, 0xd4);

////////////////////////////////////////////////////////////////

wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_OUT, fsStructuralMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_IN, fsStructuralMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_PALETTE_CLICKED, fsStructuralMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_OUT, fsElectricalMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_IN, fsElectricalMaterialPaletteEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_PALETTE_CLICKED, fsElectricalMaterialPaletteEvent);

wxDEFINE_EVENT(fsEVT_MATERIAL_PALETTE_CHECKED_STATE_CHANGED, fsCheckedStateChanged);

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
    , mFreeMaterialSampleBitmapIndices()
    // Resources
    , mShipOverlayImage(mGameAssetManager.LoadPngImageRgba(mGameAssetManager.GetPngImageFilePath("ship_overlay_small")))
    , mCheckboxOffImage(mGameAssetManager.LoadPngImageRgba(mGameAssetManager.GetPngImageFilePath("checkbox_off_small")))
    , mEditButtonImage(mGameAssetManager.LoadPngImageRgba(mGameAssetManager.GetPngImageFilePath("edit_icon_small")))
    // State
    , mCurrentSelectedCellId(NoneCellId)
    , mNextCellId(0)
    , mCheckedCellsCount(0)
{
    SetBackgroundColour(*wxWHITE);

    //
    // Build style
    //

    mSelectionPen = wxPen(BaseColor1, SelectionFrameThickness, wxPENSTYLE_SOLID);
    mCreateNewFrameBorderPen = wxPen(BaseColor1, 1, wxPENSTYLE_SHORT_DASH);

    // Make name font
    mNameFont = GetFont();
    mNameFont.SetPointSize(NameTextHeight - 3);

    // Make data font
    mDataFont = GetFont();
    mDataFont.SetPointSize(DataTextHeight - 3);

    mTextForegroundColor = *wxBLACK;

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
    mFreeMaterialSampleBitmapIndices.clear();
    mCurrentSelectedCellId = NoneCellId;
    mNextCellId = 0;
    mCheckedCellsCount = 0;
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

    AppendMaterialCell(
        material,
        std::nullopt,
        mRows.size() - 1,
        mRows.back().Strides.size() - 1);
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
    stride.iUserCustomStartIndex++;
    stride.iShipCustomStartIndex++;
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
    // Make a whole-dirty session
    LayoutSession layoutSession;
    for (size_t iRow = 0; iRow < mRows.size(); ++iRow)
    {
        if (mRows[iRow].Kind == Row::KindType::SubCategory)
        {
            layoutSession.RecordDirtyBoundary(iRow, 0, 0);
        }
    }

    // Layout
    LayoutAndRender(layoutSession, true);
}

template<LayerType TLayer>
std::any MaterialPalettePanel<TLayer>::StartCustomMaterialsLayout()
{
    return LayoutSession();
}

template<LayerType TLayer>
std::any MaterialPalettePanel<TLayer>::RemoveAllShipCustomMaterials(std::any layoutSession)
{
    LayoutSession _layoutSession = std::any_cast<LayoutSession>(layoutSession);

    // Visit all rows
    for (size_t r = 0; r < mRows.size(); ++r)
    {
        Row & row = mRows[r];
        if (row.Kind == Row::KindType::SubCategory)
        {
            // Visit all strides
            for (size_t s = 0; s < row.Strides.size(); ++s)
            {
                MaterialStride & stride = row.Strides[s];
                if (stride.Cells.size() > stride.iShipCustomStartIndex)
                {
                    while (stride.Cells.size() > stride.iShipCustomStartIndex)
                    {
                        assert(stride.Cells.back().Kind == Cell::KindType::Material); // Wouldn't be in the "ship" section otherwise
                        mFreeMaterialSampleBitmapIndices.push_back(stride.Cells.back().MaterialSampleBitmapIndex);
                        stride.Cells.pop_back();
                    }

                    // Right after this stride we're dirty
                    _layoutSession.RecordDirtyBoundary(r, s, stride.Cells.size() - 1);
                }
            }
        }
    }

    return _layoutSession;
}

template<LayerType TLayer>
std::any MaterialPalettePanel<TLayer>::AddCustomMaterial(TMaterial const * material, CustomMaterialRegisterKindType registerKind, std::any layoutSession)
{
    return AppendCustomMaterial(material, registerKind, std::any_cast<LayoutSession>(layoutSession));
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::EndCustomMaterialsLayout(std::any layoutSession)
{
    LayoutAndRender(std::any_cast<LayoutSession>(layoutSession), false);
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
    wxPoint clickAbsoluteCoords = event.GetPosition();
    auto * cell = FindCellAt(clickAbsoluteCoords);
    if (cell != nullptr)
    {
        switch (cell->Kind)
        {
            case Cell::KindType::CreateNewButton:
            {
                OnEditButton(*cell);

                break;
            }

            case Cell::KindType::Material:
            {
                // Check if edit button
                clickAbsoluteCoords -= wxSize(cell->Rect.x, cell->Rect.y);
                if (cell->EditButtonRect.has_value()
                    && cell->EditButtonRect->Contains(clickAbsoluteCoords))
                {
                    OnEditButton(*cell);
                }
                else if (cell->CheckboxRect.has_value()
                    && cell->CheckboxRect->Contains(clickAbsoluteCoords))
                {
                    OnCheckboxToggled(*cell);
                }
                else
                {
                    assert(cell->Material != nullptr);

                    // Fire clicked event
                    if constexpr (TLayer == LayerType::Structural || TLayer == LayerType::Ropes)
                    {
                        auto eventToFire = fsStructuralMaterialPaletteEvent(
                            fsEVT_STRUCTURAL_MATERIAL_PALETTE_CLICKED,
                            this->GetId(),
                            cell->Material);

                        ProcessWindowEvent(eventToFire);
                    }
                    else
                    {
                        assert(TLayer == LayerType::Electrical);

                        auto eventToFire = fsElectricalMaterialPaletteEvent(
                            fsEVT_ELECTRICAL_MATERIAL_PALETTE_CLICKED,
                            this->GetId(),
                            cell->Material);

                        ProcessWindowEvent(eventToFire);
                    }
                }

                break;
            }
        }
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnMouseLeftUp(wxMouseEvent & event)
{
    // TODO: button feedback if any, otherwise nuke
    (void)event;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnCreateNewButton(Cell & cell)
{
    assert(cell.Material != nullptr);

    // Call up
    if constexpr (TLayer == LayerType::Structural)
    {
        mMaterialPalettesController.OnNewUserCustomStructuralMaterial(cell.Material);
    }
    else if constexpr (TLayer == LayerType::Electrical)
    {
        mMaterialPalettesController.OnNewUserCustomElectricalMaterial(cell.Material);
    }
    else
    {
        static_assert(TLayer == LayerType::Ropes);
        mMaterialPalettesController.OnNewUserCustomRopesMaterial(cell.Material);
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnEditButton(Cell & cell)
{
    // Call up
    if constexpr (TLayer == LayerType::Structural)
    {
        mMaterialPalettesController.OnEditCustomStructuralMaterial(cell.Material, cell.CustomRegisterKind);
    }
    else if constexpr (TLayer == LayerType::Electrical)
    {
        mMaterialPalettesController.OnEditCustomElectricalMaterial(cell.Material, cell.CustomRegisterKind);
    }
    else
    {
        static_assert(TLayer == LayerType::Ropes);
        mMaterialPalettesController.OnEditCustomRopesMaterial(cell.Material, cell.CustomRegisterKind);
    }
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::OnCheckboxToggled(Cell & cell)
{
    // Change cell state
    assert(cell.CustomRegisterKind.has_value());
    if (cell.IsChecked)
    {
        cell.IsChecked = false;

        assert(mCheckedCellsCount > 0);
        --mCheckedCellsCount;
    }
    else
    {
        cell.IsChecked = true;
        ++mCheckedCellsCount;
    }

    // Redraw cell
    RenderCell(cell);
    Refresh(false);

    // Send event for browser, it'll reconciliate
    auto eventToFire = fsCheckedStateChanged(
        fsEVT_MATERIAL_PALETTE_CHECKED_STATE_CHANGED,
        this->GetId());

    ProcessWindowEvent(eventToFire);
}

template<LayerType TLayer>
typename MaterialPalettePanel<TLayer>::LayoutSession MaterialPalettePanel<TLayer>::AppendCustomMaterial(
    TMaterial const * material,
    CustomMaterialRegisterKindType registerKind,
    LayoutSession layoutSession)
{
    size_t const iRow = FindRowFor(material);
    size_t const iStride = FindStrideFor(material, mRows[iRow]);

    size_t const iCell = AppendMaterialCell(
        material,
        registerKind,
        iRow,
        iStride);

    // Update layoutSession
    layoutSession.RecordDirtyBoundary(iRow, iStride, iCell);

    return layoutSession;
}

template<LayerType TLayer>
size_t MaterialPalettePanel<TLayer>::AppendMaterialCell(
    TMaterial const * material,
    std::optional<CustomMaterialRegisterKindType> customRegisterKind,
    size_t iRow,
    size_t iStride)
{
    Row & row = mRows[iRow];
    MaterialStride & stride = row.Strides[iStride];
    assert(material->PaletteSubCategoryBaseMaterialOrdinal == stride.SubCategoryBaseMaterialOrdinal);

    //
    // Make material sample
    //

    std::optional<wxRect> shipOverlayRect;
    if (customRegisterKind == CustomMaterialRegisterKindType::Ship)
    {
        shipOverlayRect = wxRect(
            MaterialSampleOverlayMargin,
            MaterialSampleOverlayMargin,
            mShipOverlayImage.Size.width,
            mShipOverlayImage.Size.height);
    }

    std::optional<wxRect> editButtonRect;
    std::optional<wxRect> checkboxRect;
    if (customRegisterKind.has_value())
    {
        editButtonRect = wxRect(
            MaterialSampleSize.width - MaterialSampleOverlayMargin - mEditButtonImage.Size.width,
            MaterialSampleOverlayMargin,
            mEditButtonImage.Size.width,
            mEditButtonImage.Size.height);

        checkboxRect = wxRect(
            MaterialSampleSize.width - MaterialSampleOverlayMargin - mCheckboxOffImage.Size.width,
            editButtonRect->y + editButtonRect->height + MaterialSampleOverlayMargin,
            mCheckboxOffImage.Size.width,
            mCheckboxOffImage.Size.height);
    }

    wxBitmap materialSampleBitmap = MakeMaterialSample(
        material,
        shipOverlayRect,
        editButtonRect,
        checkboxRect);

    //
    // Insert cell
    //

    size_t cellInsertIndex = std::numeric_limits<size_t>::max();
    if (!customRegisterKind.has_value())
    {
        cellInsertIndex = static_cast<size_t>(stride.iUserCustomStartIndex);
        stride.iUserCustomStartIndex++;
        stride.iShipCustomStartIndex++;
    }
    else
    {
        switch (*customRegisterKind)
        {
            case CustomMaterialRegisterKindType::User:
            {
                cellInsertIndex = static_cast<size_t>(stride.iShipCustomStartIndex);
                stride.iShipCustomStartIndex++;
                break;
            }

            case CustomMaterialRegisterKindType::Ship:
            {
                cellInsertIndex = stride.Cells.size();
            }
        }
    }

    int materialSampleBitmapIndex;
    if (!mFreeMaterialSampleBitmapIndices.empty())
    {
        materialSampleBitmapIndex = mFreeMaterialSampleBitmapIndices.back();
        mFreeMaterialSampleBitmapIndices.pop_back();
        mMaterialSampleBitmaps.Replace(materialSampleBitmapIndex, materialSampleBitmap);
    }
    else
    {
        materialSampleBitmapIndex = mMaterialSampleBitmaps.Add(materialSampleBitmap);
    }

    Cell & cell = *stride.Cells.insert(
        std::next(stride.Cells.begin(), cellInsertIndex),
        Cell::MakeMaterialCell(
            MakeNextCellId(),
            material,
            customRegisterKind,
            materialSampleBitmapIndex,
            row.Rect.GetTop(),
            wxSize(
                MaterialCellSize.width,
                MaterialCellSize.height)));

    cell.EditButtonRect = editButtonRect;
    cell.CheckboxRect = checkboxRect;

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
                    cell.Name2 = wxEmptyString;
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
        cell.Name2 = wxEmptyString;
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

    return cellInsertIndex;
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::LayoutAndRender(
    LayoutSession const & layoutSession,
    bool doRenderSeparators)
{
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

        if (layoutSession.DirtyBoundaries.count(iRow) == 1)
        {
            //
            // Need to layout (part of) this row
            //

            assert(row.Kind == Row::KindType::SubCategory);

            auto [startStrideIndex, startCellIndex] = layoutSession.DirtyBoundaries.at(iRow);

            // Figure out where to start: points to end of previous cell
            int currentX;
            if (startCellIndex > 0)
            {
                auto const & prevCell = row.Strides[startStrideIndex].Cells[startCellIndex - 1];
                currentX = prevCell.Rect.x + prevCell.Rect.width;
            }
            else
            {
                // Cell is first of a stride
                if (startStrideIndex == 0)
                {
                    currentX = InternalWindowMargin;
                }
                else
                {
                    auto const & prevCell = row.Strides[startStrideIndex - 1].Cells.back();
                    currentX = prevCell.Rect.x + prevCell.Rect.width;
                }
            }

            for (size_t iStride = startStrideIndex; iStride < row.Strides.size(); ++iStride)
            {
                MaterialStride & stride = row.Strides[iStride];

                if (iStride > 0 && startCellIndex == 0)
                {
                    currentX += StrideHSpacing;
                }

                for (size_t iCell = startCellIndex; iCell < stride.Cells.size(); ++iCell)
                {
                    Cell & cell = stride.Cells[iCell];

                    if (iCell > 0)
                    {
                        currentX += CellHSpacing;
                    }

                    cell.Rect.x = currentX;

                    currentX += cell.Rect.width;
                }

                startCellIndex = 0;
            }

            currentX += InternalWindowMargin;

            // Set row width
            row.Rect.width = currentX;

            // Maintain max row width
            maxRowWidth = std::max(maxRowWidth, currentX);
        }
        else
        {
            //
            // Row stays, takes its width
            //

            if (row.Kind == Row::KindType::SubCategory)
            {
                // Maintain max row width
                maxRowWidth = std::max(maxRowWidth, row.Rect.width);
            }
        }
    }

    // Finalize separators' layouts
    for (auto & row : mRows)
    {
        if (row.Kind == Row::KindType::Separator)
        {
            // For separators, the rect is the actual separator rectangle
            row.Rect.width = maxRowWidth - 2 * InternalWindowMargin;
        }
    }

    //
    // Create buffer
    //

    // Calculate total panel size
    wxSize const panelSize(maxRowWidth, mRows.back().Rect.y + mRows.back().Rect.height);

    assert(!mRenderBuffer || panelSize.GetHeight() == mRenderBuffer->GetHeight()); // Height won't change

    if (!mRenderBuffer || panelSize.GetWidth() != mRenderBuffer->GetWidth())
    {
        //
        // Panel changes size
        //

        // Save old render buffer
        auto oldRenderBuffer = std::move(mRenderBuffer);

        // Alloc new buffer
        mRenderBuffer = std::make_unique<wxBitmap>(panelSize);

        if (oldRenderBuffer)
        {
            // Blit old into new
            auto oldDc = wxMemoryDC(*oldRenderBuffer);
            auto newDc = wxMemoryDC(*mRenderBuffer);
            newDc.Blit(
                0, // xDest
                0, // yDest
                std::min(oldRenderBuffer->GetWidth(), mRenderBuffer->GetWidth()), // width
                mRenderBuffer->GetHeight(), // height
                &oldDc,
                0, // xSrc
                0); // ySrc

            // Clear part of new that's extra
            if (mRenderBuffer->GetWidth() > oldRenderBuffer->GetWidth())
            {
                newDc.SetPen(*wxTRANSPARENT_PEN);
                newDc.SetBrush(SharedUIResources::GetInstance().GetWhiteSolidBackgroundBrush());
                newDc.DrawRectangle(wxRect(oldRenderBuffer->GetWidth(), 0, panelSize.GetWidth(), panelSize.GetHeight()));
            }
        }
        else
        {
            // Clear whole new
            auto dc = MakeDc();
            dc->SetPen(*wxTRANSPARENT_PEN);
            dc->SetBrush(SharedUIResources::GetInstance().GetWhiteSolidBackgroundBrush());
            dc->DrawRectangle(wxRect(0, 0, panelSize.GetWidth(), panelSize.GetHeight()));
        }

        // Make sure we render separators
        doRenderSeparators = true;
    }

    // Set panel size
    SetSize(panelSize);
    SetMinSize(panelSize);

    //
    // Render panel
    //

    RenderPanel(layoutSession, doRenderSeparators);

    Refresh(false);
}

template<LayerType TLayer>
std::unique_ptr<wxMemoryDC> MaterialPalettePanel<TLayer>::MakeDc()
{
    return std::make_unique<wxMemoryDC>(*mRenderBuffer);
}

template<LayerType TLayer>
void MaterialPalettePanel<TLayer>::RenderPanel(
    LayoutSession const & layoutSession,
    bool doRenderSeparators)
{
    // Create DC for rendering into buffer
    auto dc_ptr = MakeDc();
    auto & dc = *dc_ptr;

    // Setup
    dc.SetTextForeground(mTextForegroundColor);

    // Visit all rows
    for (size_t iRow = 0; iRow < mRows.size(); ++iRow)
    {
        Row const & row = mRows[iRow];

        if (layoutSession.DirtyBoundaries.count(iRow) == 1)
        {
            assert(row.Kind == Row::KindType::SubCategory);

            //
            // Need to render (part of) this row
            //

            auto [startStrideIndex, startCellIndex] = layoutSession.DirtyBoundaries.at(iRow);

            //
            // Clear
            //

            Cell const & startCell = row.Strides[startStrideIndex].Cells[startCellIndex];

            wxRect const clearRect(
                startCell.Rect.x,
                row.Rect.y,
                row.Rect.width - startCell.Rect.x,
                row.Rect.height);

            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(SharedUIResources::GetInstance().GetWhiteSolidBackgroundBrush());
            dc.DrawRectangle(clearRect);

            //
            // Render cells
            //

            for (size_t iStride = startStrideIndex; iStride < row.Strides.size(); ++iStride)
            {
                MaterialStride const & stride = row.Strides[iStride];

                for (size_t iCell = startCellIndex; iCell < stride.Cells.size(); ++iCell)
                {
                    RenderCell(stride.Cells[iCell], dc);
                }

                startCellIndex = 0;
            }
        }
        else if (row.Kind == Row::KindType::Separator && doRenderSeparators)
        {
            //
            // Clear
            //

            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(SharedUIResources::GetInstance().GetWhiteSolidBackgroundBrush());
            dc.DrawRectangle(
                wxRect( // Clear to the end
                    row.Rect.x,
                    row.Rect.y,
                    dc.GetSize().GetWidth() - row.Rect.x,
                    row.Rect.height));

            //
            // Render separator
            //

            int const centerY = row.Rect.y + row.Rect.height / 2;

            wxRect const separatorRect = wxRect(
                row.Rect.x,
                centerY - SeparatorRowHeight / 2,
                row.Rect.width,
                SeparatorThickness);

            dc.SetBrush(SharedUIResources::GetInstance().GetSeparatorBrush());
            dc.DrawRectangle(separatorRect);
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

            // Checkbox

            if (cell.IsChecked)
            {
                assert(cell.CheckboxRect.has_value());
                dc.DrawBitmap(
                    SharedUIResources::GetInstance().GetCheckboxOnIcon(),
                    cell.Rect.GetX() + CellInnerMargin + cell.CheckboxRect->x,
                    cell.Rect.GetY() + CellInnerMargin + cell.CheckboxRect->y);
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
wxBitmap MaterialPalettePanel<TLayer>::MakeMaterialSample(
    TMaterial const * material,
    std::optional<wxRect> shipOverlayRect,
    std::optional<wxRect> editButtonRect,
    std::optional<wxRect> checkboxRect) const
{
    std::unique_ptr<RgbaImageData> sampleImageData;
    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        ShipAutoTexturizationSettings texturizationSettings;
        texturizationSettings.MaterialTextureMagnification = 0.5f;

        sampleImageData = std::make_unique<RgbaImageData>(
            mShipTexturizer.MakeMaterialTextureSample(
                texturizationSettings,
                MaterialSampleSize,
                *material,
                mGameAssetManager));
    }
    else
    {
        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

        sampleImageData = std::make_unique<RgbaImageData>(
            ImageTools::MakeMatteImage<RgbaImageData>(
                rgbaColor(material->RenderColor, 255),
                MaterialSampleSize));
    }

    if (shipOverlayRect.has_value())
    {
        // Apply ship overlay
        ImageTools::Overlay(
            *sampleImageData,
            mShipOverlayImage,
            shipOverlayRect->x,
            MaterialSampleSize.height - mShipOverlayImage.Size.height - shipOverlayRect->y);
    }

    if (editButtonRect.has_value())
    {
        // Apply edit button overlay
        ImageTools::Overlay(
            *sampleImageData,
            mEditButtonImage,
            editButtonRect->x,
            MaterialSampleSize.height - mEditButtonImage.Size.height - editButtonRect->y);
    }

    if (checkboxRect.has_value())
    {
        // Apply checbox overlay - we start OFF
        ImageTools::Overlay(
            *sampleImageData,
            mCheckboxOffImage,
            checkboxRect->x,
            MaterialSampleSize.height - mCheckboxOffImage.Size.height - checkboxRect->y);
    }

    // Apply frame
    ImageTools::OverlayFrame(*sampleImageData, rgbaColor(0, 0, 0, rgbaColor::data_type_max));

    return WxHelpers::MakeBitmap(*sampleImageData);
}

template<LayerType TLayer>
size_t MaterialPalettePanel<TLayer>::FindRowFor(TMaterial const * material) const
{
    for (size_t iRow = 0; iRow < mRows.size(); ++iRow)
    {
        Row const & row = mRows[iRow];
        if (row.Kind == Row::KindType::SubCategory
            && row.SubCategory == material->PaletteCoordinates->SubCategory)
        {
            return iRow;
        }
    }

    assert(false);
    return std::numeric_limits<size_t>::max();
}

template<LayerType TLayer>
size_t MaterialPalettePanel<TLayer>::FindStrideFor(TMaterial const * material, Row const & row) const
{
    assert(row.Kind == Row::KindType::SubCategory);

    for (size_t iStride = 0; iStride < row.Strides.size(); ++iStride)
    {
        if (row.Strides[iStride].SubCategoryBaseMaterialOrdinal == material->PaletteSubCategoryBaseMaterialOrdinal)
        {
            return iStride;
        }
    }

    assert(false);
    return std::numeric_limits<size_t>::max();
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