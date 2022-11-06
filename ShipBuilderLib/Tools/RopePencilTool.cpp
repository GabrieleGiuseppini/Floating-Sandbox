/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopePencilTool.h"

#include <Controller.h>

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

/*
 * Overlay rules:
 * - Only in ship
 * - Regardless of engagement
 * - Mode depends on whether we can engage/disengage there
 *
 * Overlay always drawn before eph viz
 */

RopePencilTool::RopePencilTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::RopePencil,
        controller)
    , mHasTempVisualization(false)
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("pencil_cursor", 2, 22, resourceLocator));

    // Check if should act right away
    auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseShipCoordinates)
    {
        // Update sampled information
        mController.BroadcastSampledInformationUpdatedAt(*mouseShipCoordinates, LayerType::Ropes);

        // Draw overlay
        DrawOverlay(*mouseShipCoordinates);
        mController.GetUserInterface().RefreshView();
    }
}

RopePencilTool::~RopePencilTool()
{
    Leave(false);
}

void RopePencilTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    if (mEngagementData.has_value())
    {
        //
        // Engaged: we clip
        //

        auto const mouseShipSpaceCoords = GetCurrentMouseShipCoordinatesClampedToShip(mouseCoordinates);

        // Show sampled information
        mController.BroadcastSampledInformationUpdatedAt(mouseShipSpaceCoords, LayerType::Ropes);

        // Do overlay
        DrawOverlay(mouseShipSpaceCoords);

        // Do ephemeral visualization
        DoTempVisualization(mouseShipSpaceCoords);
    }
    else
    {
        //
        // Not engaged: we don't clip
        //

        auto const mouseShipSpaceCoords = GetCurrentMouseShipCoordinatesIfInShip(mouseCoordinates);

        // Show sampled information (or clear it)
        mController.BroadcastSampledInformationUpdatedAt(mouseShipSpaceCoords, LayerType::Ropes);

        if (mouseShipSpaceCoords)
        {
            // Do overlay
            DrawOverlay(*mouseShipSpaceCoords);
        }
        else
        {
            // Hide overlay
            if (mHasOverlay)
            {
                HideOverlay();
            }
        }
    }

    mController.LayerChangeEpilog();
}

void RopePencilTool::OnLeftMouseDown()
{
    OnMouseDown(MaterialPlaneType::Foreground);
}

void RopePencilTool::OnLeftMouseUp()
{
    OnMouseUp();
}

void RopePencilTool::OnRightMouseDown()
{
    OnMouseDown(MaterialPlaneType::Background);
}

void RopePencilTool::OnRightMouseUp()
{
    OnMouseUp();
}

void RopePencilTool::OnMouseLeft()
{
    Leave(true);
}

//////////////////////////////////////////////////////////////////////////////

void RopePencilTool::OnMouseDown(MaterialPlaneType plane)
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    auto const mouseShipSpaceCoords = GetCurrentMouseShipCoordinatesIfInShip();

    if (mouseShipSpaceCoords)
    {
        // Check if should start engagement
        if (!mEngagementData.has_value())
        {
            StartEngagement(*mouseShipSpaceCoords, plane);
        }

        // Leave overlay as it is now

        // Do ephemeral visualization
        if (mEngagementData.has_value())
        {
            DoTempVisualization(*mouseShipSpaceCoords);
        }
    }

    mController.LayerChangeEpilog();
}

void RopePencilTool::OnMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    // Check if should commit (and stop engagement)
    bool hasEdited = false;
    if (mEngagementData.has_value())
    {
        hasEdited = CommmitAndStopEngagement();
    }

    // No ephemeral visualization
    assert(!mEngagementData.has_value());

    // Leave overlay as-is

    if (hasEdited)
        mController.LayerChangeEpilog({LayerType::Ropes});
    else
        mController.LayerChangeEpilog();
}

void RopePencilTool::Leave(bool doCommitIfEngaged)
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    // Check if should commit (and stop engagement)
    bool hasEdited = false;
    if (mEngagementData.has_value())
    {
        if (doCommitIfEngaged)
        {
            hasEdited = CommmitAndStopEngagement();
        }
        else
        {
            mEngagementData.reset();
        }
    }

    // Remove overlay, if any
    if (mHasOverlay)
    {
        HideOverlay();
    }

    // Reset sampled information
    mController.BroadcastSampledInformationUpdatedNone();

    if (hasEdited)
        mController.LayerChangeEpilog({ LayerType::Ropes });
    else
        mController.LayerChangeEpilog();
}

void RopePencilTool::StartEngagement(
    ShipSpaceCoordinates const & coords,
    MaterialPlaneType materialPlane)
{
    assert(!mEngagementData.has_value());

    //
    // OK to engage - either for new rope or for moving
    //

    mEngagementData.emplace(
        mController.GetModelController().CloneExistingLayer<LayerType::Ropes>(),
        mController.GetModelController().GetDirtyState(),
        coords,
        mController.GetModelController().GetRopeElementIndexAt(coords),
        materialPlane);
}

void RopePencilTool::DoTempVisualization(ShipSpaceCoordinates const & coords)
{
    assert(!mHasTempVisualization);

    assert(mEngagementData.has_value());

    assert(coords.IsInSize(mController.GetModelController().GetShipSize()));

    //
    // Check conditions for doing action:
    //  - May release only if there's no other rope endpoint at that position
    //

    if (!mController.GetModelController().GetRopeElementIndexAt(coords).has_value())
    {
        if (!mEngagementData->ExistingRopeElementIndex.has_value())
        {
            mController.GetModelController().AddRopeForEphemeralVisualization(
                mEngagementData->StartCoords,
                coords,
                GetMaterial(mEngagementData->Plane));
        }
        else
        {
            mController.GetModelController().MoveRopeEndpointForEphemeralVisualization(
                *mEngagementData->ExistingRopeElementIndex,
                mEngagementData->StartCoords,
                coords);
        }

        mHasTempVisualization = true;
    }
}

void RopePencilTool::MendTempVisualization()
{
    assert(mHasTempVisualization);

    assert(mEngagementData.has_value());

    mController.GetModelController().RestoreRopesLayerForEphemeralVisualization(mEngagementData->OriginalLayerClone);

    mHasTempVisualization = false;
}

bool RopePencilTool::CommmitAndStopEngagement()
{
    assert(mEngagementData.has_value());

    assert(!mHasTempVisualization);

    //
    // Check conditions for doing action:
    //  - If same coords as start: NO
    //  - Else: may release only if there's no other rope endpoint at that position
    //

    bool hasEdited = false;

    auto const releaseShipCoords = GetCurrentMouseShipCoordinatesClampedToShip();

    if (!mController.GetModelController().GetRopeElementIndexAt(releaseShipCoords).has_value()
        && releaseShipCoords != mEngagementData->StartCoords)
    {
        //
        // May release here
        //

        // Commit action
        {
            if (!mEngagementData->ExistingRopeElementIndex.has_value())
            {
                mController.GetModelController().AddRope(
                    mEngagementData->StartCoords,
                    releaseShipCoords,
                    GetMaterial(mEngagementData->Plane));
            }
            else
            {
                mController.GetModelController().MoveRopeEndpoint(
                    *mEngagementData->ExistingRopeElementIndex,
                    mEngagementData->StartCoords,
                    releaseShipCoords);
            }
        }

        // Create undo action
        {
            mController.StoreUndoAction(
                _("Pencil Ropes"),
                mEngagementData->OriginalLayerClone.Buffer.GetByteSize(),
                mEngagementData->OriginalDirtyState,
                [originalLayerClone = std::move(mEngagementData->OriginalLayerClone)](Controller & controller) mutable
                {
                    controller.RestoreRopesLayerForUndo(std::make_unique<RopesLayerData>(std::move(originalLayerClone)));
                });
        }

        // Show sampled information
        mController.BroadcastSampledInformationUpdatedAt(releaseShipCoords, LayerType::Ropes);

        hasEdited = true;
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();

    return hasEdited;
}

void RopePencilTool::DrawOverlay(ShipSpaceCoordinates const & coords)
{
    assert(coords.IsInSize(mController.GetModelController().GetShipSize()));

    //  - If engaged: check if OK to release
    //      - May release only if there's no other rope endpoint at that position
    //  - Else(!engaged): check if OK to engage
    //      - Always

    View::OverlayMode overlayMode;
    if (mEngagementData.has_value())
    {
        // May release only if there's no other rope endpoint at that position
        auto const existingRopeElementIndex = mController.GetModelController().GetRopeElementIndexAt(coords);
        if (!existingRopeElementIndex.has_value()
            || existingRopeElementIndex == mEngagementData->ExistingRopeElementIndex)
        {
            overlayMode = View::OverlayMode::Default;
        }
        else
        {
            // Can't release here
            overlayMode = View::OverlayMode::Error;
        }
    }
    else
    {
        overlayMode = View::OverlayMode::Default;
    }

    mController.GetView().UploadCircleOverlay(
        coords,
        overlayMode);

    mHasOverlay = true;
}

void RopePencilTool::HideOverlay()
{
    mController.GetView().RemoveCircleOverlay();

    mHasOverlay = false;
}

inline StructuralMaterial const * RopePencilTool::GetMaterial(MaterialPlaneType plane) const
{
    if (plane == MaterialPlaneType::Foreground)
    {
        return mController.GetWorkbenchState().GetRopesForegroundMaterial();
    }
    else
    {
        assert(plane == MaterialPlaneType::Background);

        return mController.GetWorkbenchState().GetRopesBackgroundMaterial();
    }
}

}