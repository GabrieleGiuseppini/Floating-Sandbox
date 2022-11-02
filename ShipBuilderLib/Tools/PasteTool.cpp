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
    , mEphemeralVisualization()
    , mMouseCoords(CalculateInitialMouseOrigin())
{
    SetCursor(WxHelpers::LoadCursorImage("pan_cursor", 16, 16, resourceLocator));

    DrawEphemeralVisualization();

    mController.LayerChangeEpilog();
}

PasteTool::~PasteTool()
{
    // TODO
    if (mEphemeralVisualization)
    {
        UndoEphemeralVisualization();
        assert(!mEphemeralVisualization);

        mController.LayerChangeEpilog();
    }
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
    // TODO
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

ShipSpaceCoordinates PasteTool::CalculateInitialMouseOrigin() const
{
    ShipSpaceRect visibleShipRect = mController.GetView().GetDisplayShipSpaceRect();

    // Use mouse coordinates if they are visible
    ShipSpaceCoordinates const coordinates = GetCurrentMouseShipCoordinates();
    if (coordinates.IsInRect(visibleShipRect))
    {
        return coordinates;
    }

    // Choose mid of visible rect
    return visibleShipRect.Center();
}

void PasteTool::DrawEphemeralVisualization()
{
    assert(!mEphemeralVisualization);

    ShipSpaceCoordinates const pasteOrigin = MouseCoordsToPasteOrigin(mMouseCoords);

    GenericUndoPayload pasteUndo = mController.GetModelController().PasteForEphemeralVisualization(
        mPasteRegion,
        pasteOrigin,
        mIsTransparent);

    mController.GetView().UploadSelectionOverlay(
        pasteOrigin,
        pasteOrigin + mPasteRegion.Size);

    mEphemeralVisualization.emplace(std::move(pasteUndo));
}

void PasteTool::UndoEphemeralVisualization()
{
    assert(mEphemeralVisualization);

    mController.GetView().RemoveSelectionOverlay();

    mController.GetModelController().RestoreForEphemeralVisualization(std::move(*mEphemeralVisualization));

    mEphemeralVisualization.reset();
}

}