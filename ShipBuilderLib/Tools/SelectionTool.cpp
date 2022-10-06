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

template<LayerType TLayer>
SelectionTool<TLayer>::SelectionTool(
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
{
    SetCursor(WxHelpers::LoadCursorImage("selection_cursor", 10, 10, resourceLocator));
}

template<LayerType TLayer>
SelectionTool<TLayer>::~SelectionTool()
{
    if (mCurrentSelection || mEngagementData)
    {
        // Remove overlay
        mController.GetView().RemoveSelectionOverlay();
        mController.GetUserInterface().RefreshView();
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinate(
            ScreenToShipSpaceNearest(mouseCoordinates),
            mIsShiftDown ? mEngagementData->SelectionStartCorner : std::optional<ShipSpaceCoordinates>());

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseDown()
{
    assert(!mEngagementData);

    auto const mouseCoordinates = GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        // TODOHERE: check if we have a rect and if it matches existing corner

        // Engage at current coordinates
        auto const cornerCoordinates = ScreenToShipSpaceNearest(*mouseCoordinates);
        mEngagementData.emplace(cornerCoordinates);

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseUp()
{
    assert(mEngagementData);
    if (mEngagementData)
    {
        // Calculate corner
        ShipSpaceCoordinates const cornerCoordinates = 
            ScreenToShipSpaceNearest(GetCurrentMouseCoordinates())
            .Clamp(mController.GetModelController().GetShipSize());

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
            mController.GetView().UploadSelectionOverlay(
                mEngagementData->SelectionStartCorner,
                cornerCoordinates);

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(selection->size);
        }
        else
        {
            // Empty selection

            // Update overlay
            mController.GetView().RemoveSelectionOverlay();

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

template<LayerType TLayer>
void SelectionTool<TLayer>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinate(
            ScreenToShipSpaceNearest(GetCurrentMouseCoordinates()),
            mIsShiftDown ? mEngagementData->SelectionStartCorner : std::optional<ShipSpaceCoordinates>());

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinate(
            ScreenToShipSpaceNearest(GetCurrentMouseCoordinates()),
            mIsShiftDown ? mEngagementData->SelectionStartCorner : std::optional<ShipSpaceCoordinates>());

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
ShipSpaceCoordinates SelectionTool<TLayer>::GetCornerCoordinate(
    ShipSpaceCoordinates const & input,
    std::optional<ShipSpaceCoordinates> constrainToSquareCorner) const
{
    // Clamp
    ShipSpaceCoordinates const currentMouseCoordinates = input.Clamp(mController.GetModelController().GetShipSize());

    // Eventually constrain to square
    if (constrainToSquareCorner)
    {
        if (std::abs(currentMouseCoordinates.x - constrainToSquareCorner->x) < std::abs(currentMouseCoordinates.y - constrainToSquareCorner->y))
        {
            // Use width
            return ShipSpaceCoordinates(
                currentMouseCoordinates.x,
                constrainToSquareCorner->y + (constrainToSquareCorner->x - currentMouseCoordinates.x));
        }
        else
        {
            // Use height
            return ShipSpaceCoordinates(
                constrainToSquareCorner->x + (constrainToSquareCorner->y - currentMouseCoordinates.y),
                currentMouseCoordinates.y);
        }
    }
    else
    {
        return currentMouseCoordinates;
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::UpdateEphemeralSelection(ShipSpaceCoordinates const & cornerCoordinates)
{
    // Update overlay
    mController.GetView().UploadSelectionOverlay(
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