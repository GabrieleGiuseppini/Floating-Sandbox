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
    bool isTransparent,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        isTransparent,
        ToolType::StructuralPaste,
        controller,
        resourceLocator)
{}

ElectricalPasteTool::ElectricalPasteTool(
    ShipLayers && pasteRegion,
    bool isTransparent,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        isTransparent,
        ToolType::ElectricalPaste,
        controller,
        resourceLocator)
{}

RopePasteTool::RopePasteTool(
    ShipLayers && pasteRegion,
    bool isTransparent,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        isTransparent,
        ToolType::RopePaste,
        controller,
        resourceLocator)
{}

TexturePasteTool::TexturePasteTool(
    ShipLayers && pasteRegion,
    bool isTransparent,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : PasteTool(
        std::move(pasteRegion),
        isTransparent,
        ToolType::TexturePaste,
        controller,
        resourceLocator)
{}

PasteTool::PasteTool(
    ShipLayers && pasteRegion,
    bool isTransparent,
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mPasteRegion(std::move(pasteRegion))
    , mIsTransparent(isTransparent)
{
    SetCursor(WxHelpers::LoadCursorImage("pan_cursor", 16, 16, resourceLocator));

    // TODO
}

PasteTool::~PasteTool()
{
    // TODO
}

void PasteTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // TODO
    (void)mouseCoordinates;
}

void PasteTool::OnLeftMouseDown()
{
    // TODO
}

void PasteTool::OnLeftMouseUp()
{
    // TODO
}

void PasteTool::OnShiftKeyDown()
{
    // TODO
}

void PasteTool::OnShiftKeyUp()
{
    // TODO
}

void PasteTool::OnMouseLeft()
{
    mController.BroadcastSampledInformationUpdatedNone();
}

void PasteTool::SetIsTransparent(bool isTransparent)
{
    mIsTransparent = isTransparent;

    // TODO
}

void PasteTool::Rotate90CW()
{
    // TODO
}

void PasteTool::Rotate90CCW()
{
    // TODO
}

void PasteTool::FlipH()
{
    // TODO
}

void PasteTool::FlipV()
{
    // TODO
}

//////////////////////////////////////////////////////////////////////////////

}