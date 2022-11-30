/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SelectionTool.h"

#include "Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

StructuralSelectionTool::StructuralSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::StructuralSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

ElectricalSelectionTool::ElectricalSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::ElectricalSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

RopeSelectionTool::RopeSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::RopeSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

TextureSelectionTool::TextureSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::TextureSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

SelectionTool::SelectionTool(
    ToolType toolType,
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mSelectionManager(selectionManager)
    , mCurrentSelection()
    , mEngagementData()
    , mIsShiftDown(false)
    , mPointerCursor(WxHelpers::LoadCursorImage("selection_cursor", 11, 11, resourceLocator))
    , mBaseCornerCursor(WxHelpers::LoadCursorImage("corner_cursor", 15, 15, resourceLocator))
{
    SetCursor(mPointerCursor);
}

SelectionTool::~SelectionTool()
{
    if (mCurrentSelection || mEngagementData)
    {
        // Remove selection
        mSelectionManager.SetSelection(std::nullopt);

        // Remove overlay
        mController.GetView().RemoveDashedRectangleOverlay();
        mController.GetUserInterface().RefreshView();

        // Remove measurement
        mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
    }
}

void SelectionTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged(mouseCoordinates);

        UpdateEphemeralSelection(cornerCoordinates);

        SetCursor(mPointerCursor);
    }
    else
    {
        // Check if hitting a corner
        if (mCurrentSelection)
        {
            auto const cornerCoordinates = GetCornerCoordinatesFree();
            if (cornerCoordinates == mCurrentSelection->MinMin())
            {
                SetCursor(mBaseCornerCursor.Rotate90(false));
            }
            else if (cornerCoordinates == mCurrentSelection->MaxMin())
            {
                SetCursor(mBaseCornerCursor.Rotate180());
            }
            else if (cornerCoordinates == mCurrentSelection->MaxMax())
            {
                SetCursor(mBaseCornerCursor.Rotate90(true));
            }
            else if (cornerCoordinates == mCurrentSelection->MinMax())
            {
                SetCursor(mBaseCornerCursor);
            }
            else
            {
                SetCursor(mPointerCursor);
            }
        }
    }
}

void SelectionTool::OnLeftMouseDown()
{
    assert(!mEngagementData);

    auto const cornerCoordinates = GetCornerCoordinatesFree();
    if (cornerCoordinates)
    {
        // Create new corner - init with current coords, eventually
        // we end up with an empty rect
        ShipSpaceCoordinates selectionStartCorner = *cornerCoordinates;

        // Check if hitting a corner
        if (mCurrentSelection)
        {
            if (*cornerCoordinates == mCurrentSelection->MinMin())
            {
                selectionStartCorner = mCurrentSelection->MaxMax();
            }
            else if (*cornerCoordinates == mCurrentSelection->MaxMin())
            {
                selectionStartCorner = mCurrentSelection->MinMax();
            }
            else if (*cornerCoordinates == mCurrentSelection->MaxMax())
            {
                selectionStartCorner = mCurrentSelection->MinMin();
            }
            else if (*cornerCoordinates == mCurrentSelection->MinMax())
            {
                selectionStartCorner = mCurrentSelection->MaxMin();
            }
        }

        // Engage at selection start corner
        mEngagementData.emplace(selectionStartCorner);

        UpdateEphemeralSelection(*cornerCoordinates);
    }
}

void SelectionTool::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        // Calculate corner
        ShipSpaceCoordinates const cornerCoordinates = GetCornerCoordinatesEngaged();

        // Calculate selection
        std::optional<ShipSpaceRect> selection;
        if (cornerCoordinates.x != mEngagementData->SelectionStartCorner.x
            && cornerCoordinates.y != mEngagementData->SelectionStartCorner.y)
        {
            // Non-empty selection
            selection = ShipSpaceRect(
                mEngagementData->SelectionStartCorner,
                cornerCoordinates);

            // Update overlay
            mController.GetView().UploadDashedRectangleOverlay(
                mEngagementData->SelectionStartCorner,
                cornerCoordinates);

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(selection->size);
        }
        else
        {
            // Empty selection

            // Update overlay
            mController.GetView().RemoveDashedRectangleOverlay();

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
        }

        // Commit selection
        mCurrentSelection = selection;
        mSelectionManager.SetSelection(mCurrentSelection);

        // Disengage
        mEngagementData.reset();

        mController.GetUserInterface().RefreshView();
    }
}

void SelectionTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged();

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

void SelectionTool::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged();

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

void SelectionTool::SelectAll()
{
    // Create selection
    auto const selection = ShipSpaceRect(mController.GetModelController().GetShipSize());

    // Update overlay
    mController.GetView().UploadDashedRectangleOverlay(
        selection.MinMin(),
        selection.MaxMax());

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(selection.size);

    // Commit selection
    mCurrentSelection = selection;
    mSelectionManager.SetSelection(mCurrentSelection);

    // Disengage
    mEngagementData.reset();

    mController.GetUserInterface().RefreshView();
}

void SelectionTool::Deselect()
{
    if (mCurrentSelection || mEngagementData)
    {
        // Update overlay
        mController.GetView().RemoveDashedRectangleOverlay();
    }

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);

    // Commit selection
    mCurrentSelection.reset();
    mSelectionManager.SetSelection(std::nullopt);

    // Disengage
    mEngagementData.reset();

    mController.GetUserInterface().RefreshView();
}

//////////////////////////////////////////////////////////////////////////////

ShipSpaceCoordinates SelectionTool::GetCornerCoordinatesEngaged() const
{
    return GetCornerCoordinatesEngaged(GetCurrentMouseCoordinates());
}

ShipSpaceCoordinates SelectionTool::GetCornerCoordinatesEngaged(DisplayLogicalCoordinates const & input) const
{
    // Convert to ship coords closest to grid point
    ShipSpaceCoordinates const nearestGridPointCoordinates = ScreenToShipSpaceNearest(input);

    // Clamp - allowing for point at (w,h)
    ShipSpaceCoordinates const cornerCoordinates = nearestGridPointCoordinates.Clamp(mController.GetModelController().GetShipSize());

    // Eventually constrain to square
    if (mIsShiftDown)
    {
        auto const width = cornerCoordinates.x - mEngagementData->SelectionStartCorner.x;
        auto const height = cornerCoordinates.y - mEngagementData->SelectionStartCorner.y;
        if (std::abs(width) < std::abs(height))
        {
            // Use width
            return ShipSpaceCoordinates(
                cornerCoordinates.x,
                mEngagementData->SelectionStartCorner.y + std::abs(width) * Sign(height));
        }
        else
        {
            // Use height
            return ShipSpaceCoordinates(
                mEngagementData->SelectionStartCorner.x + std::abs(height) * Sign(width),
                cornerCoordinates.y);
        }
    }
    else
    {
        return cornerCoordinates;
    }
}

std::optional<ShipSpaceCoordinates> SelectionTool::GetCornerCoordinatesFree() const
{
    ShipSpaceCoordinates const mouseShipCoordinates = ScreenToShipSpaceNearest(GetCurrentMouseCoordinates());

    auto const shipSize = mController.GetModelController().GetShipSize();
    if (mouseShipCoordinates.IsInRect(
        ShipSpaceRect(
            ShipSpaceCoordinates(0, 0),
            ShipSpaceSize(shipSize.width + 1, shipSize.height + 1))))
    {
        return mouseShipCoordinates;
    }
    else
    {
        return std::nullopt;
    }
}

void SelectionTool::UpdateEphemeralSelection(ShipSpaceCoordinates const & cornerCoordinates)
{
    // Update overlay
    mController.GetView().UploadDashedRectangleOverlay(
        mEngagementData->SelectionStartCorner,
        cornerCoordinates);
    mController.GetUserInterface().RefreshView();

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(
        ShipSpaceSize(
            std::abs(cornerCoordinates.x - mEngagementData->SelectionStartCorner.x),
            std::abs(cornerCoordinates.y - mEngagementData->SelectionStartCorner.y)));
}

}