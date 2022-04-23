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

    // Check overlay
    auto const mouseCoordinates = GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        auto const mouseShipSpaceCoords = ScreenToShipSpace(*mouseCoordinates);

        DrawOverlay(mouseShipSpaceCoords);

        mController.GetUserInterface().RefreshView();

        mController.BroadcastSampledInformationUpdatedAt(mouseShipSpaceCoords, LayerType::Ropes);
    }
}

RopePencilTool::~RopePencilTool()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    // Remove overlay, if any
    if (mHasOverlay)
    {
        HideOverlay();
    }

    // Reset sampled information
    mController.BroadcastSampledInformationUpdatedNone();

    mController.LayerChangeEpilog();
}

void RopePencilTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    auto const mouseShipSpaceCoords = ScreenToShipSpace(mouseCoordinates);

    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    // Show sampled information
    mController.BroadcastSampledInformationUpdatedAt(mouseShipSpaceCoords, LayerType::Ropes);

    // Do overlay
    DrawOverlay(mouseShipSpaceCoords);

    // Do ephemeral visualization
    if (mEngagementData.has_value())
    {
        DoTempVisualization(mouseShipSpaceCoords);
    }

    mController.LayerChangeEpilog();
}

void RopePencilTool::OnLeftMouseDown()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Check if should start engagement
    if (!mEngagementData.has_value())
    {
        CheckEngagement(mouseCoordinates, MaterialPlaneType::Foreground);
    }

    // Do ephemeral visualization
    if (mEngagementData.has_value())
    {
        DoTempVisualization(mouseCoordinates);
    }

    // Leave overlay

    mController.LayerChangeEpilog();
}

void RopePencilTool::OnLeftMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Check if should stop engagement
    bool hasEdited = false;
    if (mEngagementData.has_value())
    {
        hasEdited = CommmitAndStopEngagement(mouseCoordinates);
    }

    // No ephemeral visualization
    assert(!mEngagementData.has_value());

    // Leave overlay

    mController.LayerChangeEpilog(hasEdited ? LayerType::Ropes : std::optional<LayerType>());
}

void RopePencilTool::OnRightMouseDown()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Check if should start engagement
    if (!mEngagementData)
    {
        CheckEngagement(mouseCoordinates, MaterialPlaneType::Background);
    }

    // Do ephemeral visualization
    if (mEngagementData.has_value())
    {
        DoTempVisualization(mouseCoordinates);
    }

    // Leave overlay

    mController.LayerChangeEpilog();
}

void RopePencilTool::OnRightMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Check if should stop engagement
    bool hasEdited = false;
    if (mEngagementData.has_value())
    {
        hasEdited = CommmitAndStopEngagement(mouseCoordinates);
    }

    // No ephemeral visualization
    assert(!mEngagementData.has_value());

    // Leave overlay

    mController.LayerChangeEpilog(hasEdited ? LayerType::Ropes : std::optional<LayerType>());
}

//////////////////////////////////////////////////////////////////////////////

void RopePencilTool::CheckEngagement(
    ShipSpaceCoordinates const & coords,
    MaterialPlaneType materialPlane)
{
    assert(!mEngagementData.has_value());

    //  - If outside ship rect : No engagement
    //  - Else: always OK to engage - either for new rope or for moving

    if (coords.IsInSize(mController.GetModelController().GetShipSize()))
    {
        //
        // OK to engage
        //

        mEngagementData.emplace(
            mController.GetModelController().CloneExistingLayer<LayerType::Ropes>(),
            mController.GetModelController().GetDirtyState(),
            coords,
            mController.GetModelController().GetRopeElementIndexAt(coords),
            materialPlane);
    }
}

void RopePencilTool::DoTempVisualization(ShipSpaceCoordinates const & coords)
{
    assert(!mHasTempVisualization);

    assert(mEngagementData.has_value());

    //
    // Check conditions for doing action:
    //  - If outside ship rect : NO
    //  - Else: may release only if there's no other rope endpoint at that position
    //

    if (coords.IsInSize(mController.GetModelController().GetShipSize())
        && !mController.GetModelController().GetRopeElementIndexAt(coords).has_value())
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

bool RopePencilTool::CommmitAndStopEngagement(ShipSpaceCoordinates const & coords)
{
    assert(mEngagementData.has_value());

    assert(!mHasTempVisualization);

    //
    // Check conditions for doing action:
    //  - If same coords as startL NO
    //  - If outside ship rect : NO
    //  - Else: may release only if there's no other rope endpoint at that position
    //

    bool hasEdited = false;

    if (coords.IsInSize(mController.GetModelController().GetShipSize())
        && !mController.GetModelController().GetRopeElementIndexAt(coords).has_value()
        && coords != mEngagementData->StartCoords)
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
                    coords,
                    GetMaterial(mEngagementData->Plane));
            }
            else
            {
                mController.GetModelController().MoveRopeEndpoint(
                    *mEngagementData->ExistingRopeElementIndex,
                    mEngagementData->StartCoords,
                    coords);
            }
        }

        // Create undo action
        {
            mController.StoreUndoAction(
                _("Pencil Ropes"),
                mEngagementData->OriginalLayerClone.Buffer.GetSize() * sizeof(RopeElement),
                mEngagementData->OriginalDirtyState,
                [originalLayerClone = std::move(mEngagementData->OriginalLayerClone)](Controller & controller) mutable
                {
                    controller.RestoreRopesLayerForUndo(std::make_unique<RopesLayerData>(std::move(originalLayerClone)));
                });
        }

        // Show sampled information
        mController.BroadcastSampledInformationUpdatedAt(coords, LayerType::Ropes);

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
    //  - If outside ship rect : No overlay
    //  - Else if engaged : check if OK to release
    //      - May release only if there's no other rope endpoint at that position
    //  - Else(!engaged) : check if OK to engage
    //      - Always

    if (!coords.IsInSize(mController.GetModelController().GetShipSize()))
    {
        if (mHasOverlay)
        {
            HideOverlay();
        }
    }
    else
    {
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