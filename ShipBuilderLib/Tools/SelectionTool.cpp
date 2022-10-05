/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SelectionTool.h"

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

    // TODO
}

template<LayerType TLayer>
SelectionTool<TLayer>::~SelectionTool()
{
    if (mCurrentRect && !mCurrentRect->IsEmpty())
    {
        // Remove overlay
        // TODOHERE
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // TODO
    (void)mouseCoordinates;
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseDown()
{
    // TODO
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseUp()
{
    // TODO
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnRightMouseDown()
{
    // TODO
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnRightMouseUp()
{
    // TODO
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