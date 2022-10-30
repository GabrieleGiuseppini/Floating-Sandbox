/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PasteTool.h"

#include <Controller.h>

#include <type_traits>

namespace ShipBuilder {

StructuralPasteTool::StructuralPasteTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        ToolType::StructuralPaste,
        controller,
        resourceLocator)
{}

ElectricalPasteTool::ElectricalPasteTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        ToolType::ElectricalPaste,
        controller,
        resourceLocator)
{}

RopePasteTool::RopePasteTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        ToolType::RopePaste,
        controller,
        resourceLocator)
{}

TexturePasteTool::TexturePasteTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        ToolType::TexturePaste,
        controller,
        resourceLocator)
{}

template<LayerType TLayerType>
PasteTool<TLayerType>::PasteTool(
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mCursorImage(WxHelpers::LoadCursorImage("pan_cursor", 16, 16, resourceLocator))
{
    SetCursor(mCursorImage);

    // TODO
}

template<LayerType TLayerType>
PasteTool<TLayerType>::~PasteTool()
{
    // TODO
}

template<LayerType TLayer>
void PasteTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // TODO
    (void)mouseCoordinates;
}

template<LayerType TLayer>
void PasteTool<TLayer>::OnLeftMouseDown()
{
    // TODO
}

template<LayerType TLayer>
void PasteTool<TLayer>::OnLeftMouseUp()
{
    // TODO
}

template<LayerType TLayer>
void PasteTool<TLayer>::OnShiftKeyDown()
{
    // TODO
}

template<LayerType TLayer>
void PasteTool<TLayer>::OnShiftKeyUp()
{
    // TODO
}

template<LayerType TLayer>
void PasteTool<TLayer>::OnMouseLeft()
{
    mController.BroadcastSampledInformationUpdatedNone();
}

//////////////////////////////////////////////////////////////////////////////

}