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
    ShipLayers && pasteRegion,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        ToolType::StructuralPaste,
        controller,
        resourceLocator)
{}

ElectricalPasteTool::ElectricalPasteTool(
    ShipLayers && pasteRegion,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        ToolType::ElectricalPaste,
        controller,
        resourceLocator)
{}

RopePasteTool::RopePasteTool(
    ShipLayers && pasteRegion,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        ToolType::RopePaste,
        controller,
        resourceLocator)
{}

TexturePasteTool::TexturePasteTool(
    ShipLayers && pasteRegion,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        ToolType::TexturePaste,
        controller,
        resourceLocator)
{}

template<LayerType TLayerType>
PasteTool<TLayerType>::PasteTool(
    ShipLayers && pasteRegion,
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mPasteRegion(std::move(pasteRegion))
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