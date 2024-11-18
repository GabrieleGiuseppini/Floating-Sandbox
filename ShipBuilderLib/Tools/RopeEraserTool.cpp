/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopeEraserTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

RopeEraserTool::RopeEraserTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::RopeEraser,
        controller)
    , mOriginalLayerClone(mController.GetModelController().CloneExistingLayer<LayerType::Ropes>())
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("eraser_cursor", 8, 27, resourceLocator));

    // Check if we draw the overlay right away
    auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseShipCoordinates)
    {
        DrawOverlay(*mouseShipCoordinates);

        mController.BroadcastSampledInformationUpdatedAt(*mouseShipCoordinates, LayerType::Ropes);
    }
}

RopeEraserTool::~RopeEraserTool()
{
    Leave(false);
}

void RopeEraserTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    auto const mouseShipCoords = GetCurrentMouseShipCoordinatesIfInShip(mouseCoordinates);
    if (mouseShipCoords.has_value())
    {
        if (mEngagementData.has_value())
        {
            // Do action
            DoAction(*mouseShipCoords);

            // No need to do eph viz when engaged
        }
        else
        {
            // Just draw overlay
            DrawOverlay(*mouseShipCoords);

            // Show sampled information
            mController.BroadcastSampledInformationUpdatedAt(*mouseShipCoords, LayerType::Ropes);
        }
    }
    else
    {
        // Hide overlay, if any
        if (mHasOverlay)
        {
            HideOverlay();
        }
    }
}

void RopeEraserTool::OnLeftMouseDown()
{
    OnMouseDown();
}

void RopeEraserTool::OnLeftMouseUp()
{
    OnMouseUp();
}

void RopeEraserTool::OnRightMouseDown()
{
    OnMouseDown();
}

void RopeEraserTool::OnRightMouseUp()
{
    OnMouseUp();
}

void RopeEraserTool::OnMouseLeft()
{
    Leave(true);
}

//////////////////////////////////////////////////////////////////////////////

void RopeEraserTool::OnMouseDown()
{
    // Stop overlay, if any
    if (mHasOverlay)
    {
        HideOverlay();

        assert(!mHasOverlay);
    }

    // Engage
    StartEngagement();

    auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseShipCoordinates)
    {
        // Do action
        DoAction(*mouseShipCoordinates);
    }

    // No need to do eph viz when engaged
}

void RopeEraserTool::OnMouseUp()
{
    // Check if should stop engagement
    if (mEngagementData.has_value())
    {
        assert(!mHasOverlay);

        // Disengage
        StopEngagement();

        auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
        if (mouseShipCoordinates)
        {
            // Restart overlay
            DrawOverlay(*mouseShipCoordinates);

            assert(mHasOverlay);
        }
    }
}

void RopeEraserTool::Leave(bool doCommitIfEngaged)
{
    // Remove our overlay, if any
    if (mHasOverlay)
    {
        HideOverlay();
    }

    // Disengage, eventually
    if (mEngagementData)
    {
        if (doCommitIfEngaged)
        {
            // Disengage
            StopEngagement();
        }
        else
        {
            // Plainly disengage
            mEngagementData.reset();
        }

        assert(!mEngagementData);
    }

    // Reset sampled information
    mController.BroadcastSampledInformationUpdatedNone();
}

void RopeEraserTool::StartEngagement()
{
    assert(!mHasOverlay);

    assert(!mEngagementData.has_value());

    mEngagementData.emplace(mController.GetModelController().GetDirtyState());
}

void RopeEraserTool::DoAction(ShipSpaceCoordinates const & coords)
{
    assert(!mHasOverlay);

    assert(mEngagementData.has_value());

    bool const hasErased = mController.GetModelController().EraseRopeAt(coords);
    if (hasErased)
    {
        mEngagementData->HasEdited = true;

        // Show sampled information
        mController.BroadcastSampledInformationUpdatedAt(coords, LayerType::Ropes);

        mController.LayerChangeEpilog({LayerType::Ropes});
    }
    else
    {
        mController.LayerChangeEpilog();
    }
}

void RopeEraserTool::StopEngagement()
{
    assert(!mHasOverlay);

    assert(mEngagementData.has_value());

    if (mEngagementData->HasEdited)
    {
        //
        // Create undo action
        //

        mController.StoreUndoAction(
            _("Eraser Ropes"),
            mOriginalLayerClone.Buffer.GetByteSize(),
            mEngagementData->OriginalDirtyState,
            [originalLayerClone = std::move(mOriginalLayerClone)](Controller & controller) mutable
            {
                controller.RestoreRopesLayerForUndo(std::make_unique<RopesLayerData>(std::move(originalLayerClone)));
            });

        // Take new orig clone
        mOriginalLayerClone = mController.GetModelController().CloneExistingLayer<LayerType::Ropes>();
    }

    // Stop engagement
    mEngagementData.reset();
}

void RopeEraserTool::DrawOverlay(ShipSpaceCoordinates const & coords)
{
    mController.GetView().UploadCircleOverlay(
        coords,
        mController.GetModelController().GetRopeElementIndexAt(coords).has_value()
        ? View::OverlayMode::Default
        : View::OverlayMode::Error);

    mController.GetUserInterface().RefreshView();

    mHasOverlay = true;
}

void RopeEraserTool::HideOverlay()
{
    assert(mHasOverlay);

    mController.GetView().RemoveCircleOverlay();

    mController.GetUserInterface().RefreshView();

    mHasOverlay = false;
}

}