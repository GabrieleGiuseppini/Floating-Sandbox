/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PasteTool.h"

#include <Controller.h>

#include "GameCore/GameMath.h"

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
    , mPendingSessionData()
    , mMouseAnchor()
{
    SetCursor(WxHelpers::LoadCursorImage("pan_cursor", 16, 16, resourceLocator));

    // Begin pending session
    mPendingSessionData.emplace(
        std::move(pasteRegion),
        isTransparent,
        CalculateInitialMouseOrigin());

    DrawEphemeralVisualization();

    mController.LayerChangeEpilog();
}

PasteTool::~PasteTool()
{
    if (mPendingSessionData)
    {
        Commit();
    }
}

void PasteTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    assert(mPendingSessionData);

    if (mMouseAnchor)
    {
        if (mPendingSessionData->EphemeralVisualization)
        {
            UndoEphemeralVisualization();
            assert(!mPendingSessionData->EphemeralVisualization);

            mController.LayerChangeEpilog(); // Since we're moving, it's likely that 2xregion is less expensive than union of two regions
        }

        // Move mouse coords
        auto const shipMouseCoordinates = ScreenToShipSpace(mouseCoordinates);
        mPendingSessionData->MousePasteCoords = ClampMousePasteCoords(
            mPendingSessionData->MousePasteCoords + (shipMouseCoordinates - *mMouseAnchor),
            mPendingSessionData->PasteRegion.Size);

        DrawEphemeralVisualization();

        mController.LayerChangeEpilog();

        mMouseAnchor = shipMouseCoordinates;
    }
}

void PasteTool::OnLeftMouseDown()
{
    auto const coords = GetCurrentMouseShipCoordinatesIfInWorkCanvas();
    if (coords)
    {
        // Start engagement
        mMouseAnchor = *coords;
    }
}

void PasteTool::OnLeftMouseUp()
{
    mMouseAnchor.reset();
}

void PasteTool::OnShiftKeyDown()
{
    // TODO
}

void PasteTool::OnShiftKeyUp()
{
    // TODO
}

void PasteTool::Commit()
{
    assert (mPendingSessionData);

    if (mPendingSessionData->EphemeralVisualization)
    {
        UndoEphemeralVisualization();
        assert(!mPendingSessionData->EphemeralVisualization);
    }

    // Calculate affected layers

    auto const affectedLayers = mController.GetModelController().CalculateAffectedLayers(mPendingSessionData->PasteRegion);
    if (!affectedLayers.empty())
    {
        // Commit

        ShipSpaceCoordinates const pasteOrigin = MousePasteCoordsToActualPasteOrigin(
            mPendingSessionData->MousePasteCoords,
            mPendingSessionData->PasteRegion.Size);

        GenericUndoPayload undoPayload = mController.GetModelController().Paste(
            mPendingSessionData->PasteRegion,
            pasteOrigin,
            mPendingSessionData->IsTransparent);

        // Store undo

        size_t const undoPayloadCost = undoPayload.GetTotalCost();

        mController.StoreUndoAction(
            _("Paste"),
            undoPayloadCost,
            mController.GetModelController().GetDirtyState(),
            [undoPayload = std::move(undoPayload)](Controller & controller) mutable
            {
                controller.Restore(std::move(undoPayload));
            });
    }

    // Finalize

    mController.LayerChangeEpilog(affectedLayers);

    mPendingSessionData.reset();
}

void PasteTool::Abort()
{
    assert(mPendingSessionData);

    if (mPendingSessionData->EphemeralVisualization)
    {
        UndoEphemeralVisualization();
        assert(!mPendingSessionData->EphemeralVisualization);
    }

    mController.LayerChangeEpilog();

    mPendingSessionData.reset();
}

void PasteTool::SetIsTransparent(bool isTransparent)
{
    assert(mPendingSessionData);

    if (mPendingSessionData->EphemeralVisualization)
    {
        UndoEphemeralVisualization();
        assert(!mPendingSessionData->EphemeralVisualization);
    }

    mPendingSessionData->IsTransparent = isTransparent;

    DrawEphemeralVisualization();

    mController.LayerChangeEpilog();
}

void PasteTool::Rotate90CW()
{
    assert(mPendingSessionData);

    if (mPendingSessionData->EphemeralVisualization)
    {
        UndoEphemeralVisualization();
        assert(!mPendingSessionData->EphemeralVisualization);
    }

    mPendingSessionData->PasteRegion.Rotate90(RotationDirectionType::Clockwise);

    DrawEphemeralVisualization();

    mController.LayerChangeEpilog();
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

ShipSpaceCoordinates PasteTool::MousePasteCoordsToActualPasteOrigin(
    ShipSpaceCoordinates const & mousePasteCoords,
    ShipSpaceSize const & pasteRegionSize) const
{
    // We want paste's top-left corner to be at the top-left corner of the ship "square"
    // whose bottom-left corner is the specified mouse coords
    return ShipSpaceCoordinates(
        mousePasteCoords.x,
        mousePasteCoords.y)
        - ShipSpaceSize(0, pasteRegionSize.height - 1);
}

ShipSpaceCoordinates PasteTool::ClampMousePasteCoords(
    ShipSpaceCoordinates const & mousePasteCoords,
    ShipSpaceSize const & pasteRegionSize) const
{
    return ShipSpaceCoordinates(
        Clamp(mousePasteCoords.x, -pasteRegionSize.width, mController.GetModelController().GetShipSize().width),
        Clamp(mousePasteCoords.y, -1, mController.GetModelController().GetShipSize().height + pasteRegionSize.height - 1));
}

void PasteTool::DrawEphemeralVisualization()
{
    assert(mPendingSessionData);
    assert(!mPendingSessionData->EphemeralVisualization);

    ShipSpaceCoordinates const pasteOrigin = MousePasteCoordsToActualPasteOrigin(
        mPendingSessionData->MousePasteCoords,
        mPendingSessionData->PasteRegion.Size);

    GenericEphemeralVisualizationRestorePayload pasteEphVizRestore = mController.GetModelController().PasteForEphemeralVisualization(
        mPendingSessionData->PasteRegion,
        pasteOrigin,
        mPendingSessionData->IsTransparent);

    mController.GetView().UploadSelectionOverlay(
        pasteOrigin,
        pasteOrigin + mPendingSessionData->PasteRegion.Size);

    mPendingSessionData->EphemeralVisualization.emplace(std::move(pasteEphVizRestore));
}

void PasteTool::UndoEphemeralVisualization()
{
    assert(mPendingSessionData);
    assert(mPendingSessionData->EphemeralVisualization);

    mController.GetView().RemoveSelectionOverlay();

    mController.GetModelController().RestoreEphemeralVisualization(std::move(*(mPendingSessionData->EphemeralVisualization)));

    mPendingSessionData->EphemeralVisualization.reset();
}

}