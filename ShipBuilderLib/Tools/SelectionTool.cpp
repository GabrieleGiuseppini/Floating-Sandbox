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
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::StructuralSelection,
        controller,
        resourceLocator)
{}

ElectricalSelectionTool::ElectricalSelectionTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::ElectricalSelection,
        controller,
        resourceLocator)
{}

RopeSelectionTool::RopeSelectionTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::RopeSelection,
        controller,
        resourceLocator)
{}

TextureSelectionTool::TextureSelectionTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::TextureSelection,
        controller,
        resourceLocator)
{}

template<LayerType TLayer>
SelectionTool<TLayer>::SelectionTool(
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mCurrentRect()
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("selection_cursor", 10, 10, resourceLocator));
}

template<LayerType TLayer>
SelectionTool<TLayer>::~SelectionTool()
{
    if (mCurrentRect && !mCurrentRect->IsEmpty())
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
        // TODO: clip to ship canvas, etc.
        // TODO: snap to 0.5
        auto const mouseShipSpaceCoords = ScreenToShipSpace(mouseCoordinates);
        mController.GetView().UploadSelectionOverlay(
            mEngagementData->SelectionStartCorner,
            mouseShipSpaceCoords);
        mController.GetUserInterface().RefreshView();
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

        // Engage
        auto const mouseShipSpaceCoords = ScreenToShipSpace(*mouseCoordinates);
        mEngagementData.emplace(mouseShipSpaceCoords);

        // No point in updating, selection rect is empty now
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseUp()
{
    // TODO

    assert(mEngagementData);
    mEngagementData.reset();
    mController.GetView().RemoveSelectionOverlay();
    mController.GetUserInterface().RefreshView();
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnShiftKeyDown()
{
    // TODO
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnShiftKeyUp()
{
    // TODO
}

//////////////////////////////////////////////////////////////////////////////

}