/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopePencilTool.h"

#include "Controller.h"

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

RopePencilTool::RopePencilTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::RopePencil,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mHasTempVisualization(false)
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("pencil_cursor", 2, 22, resourceLocator));

    // Check overlay
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DrawOverlay(*mouseCoordinates);

        mUserInterface.RefreshView();
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

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

void RopePencilTool::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    // Do overlay
    DrawOverlay(mouseCoordinates);

    // Do ephemeral visualization
    if (mEngagementData.has_value())
    {
        DoTempVisualization(mouseCoordinates);
    }

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

void RopePencilTool::OnLeftMouseDown()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

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

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

void RopePencilTool::OnLeftMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Check if should stop engagement
    if (mEngagementData.has_value())
    {
        CommmitAndStopEngagement(mouseCoordinates);
    }

    // No ephemeral visualization
    assert(!mEngagementData.has_value());

    // Leave overlay

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

void RopePencilTool::OnRightMouseDown()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

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

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

void RopePencilTool::OnRightMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasTempVisualization)
    {
        MendTempVisualization();

        assert(!mHasTempVisualization);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Check if should stop engagement
    if (mEngagementData.has_value())
    {
        CommmitAndStopEngagement(mouseCoordinates);
    }

    // No ephemeral visualization
    assert(!mEngagementData.has_value());

    // Leave overlay

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

//////////////////////////////////////////////////////////////////////////////

void RopePencilTool::CheckEngagement(
    ShipSpaceCoordinates const & coords,
    MaterialPlaneType materialPlane)
{
    assert(!mEngagementData.has_value());

    //  - If outside ship rect : No engagement
    //  - Else: always OK to engage - either for new rope or for moving

    if (coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        //
        // OK to engage
        //

        mEngagementData.emplace(
            mModelController.GetModel().CloneExistingLayer<LayerType::Ropes>(),
            mModelController.GetModel().GetDirtyState(),
            coords,
            mModelController.GetRopeElementIndexAt(coords),
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

    if (coords.IsInSize(mModelController.GetModel().GetShipSize())
        && !mModelController.GetRopeElementIndexAt(coords).has_value())
    {
        if (!mEngagementData->ExistingRopeElementIndex.has_value())
        {
            mModelController.AddRopeForEphemeralVisualization(
                mEngagementData->StartCoords,
                coords,
                GetMaterial(mEngagementData->Plane));
        }
        else
        {
            mModelController.MoveRopeEndpointForEphemeralVisualization(
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

    mModelController.RestoreRopesLayerForEphemeralVisualization(mEngagementData->OriginalLayerClone);

    mHasTempVisualization = false;
}

void RopePencilTool::CommmitAndStopEngagement(ShipSpaceCoordinates const & coords)
{
    assert(mEngagementData.has_value());

    assert(!mHasTempVisualization);

    //
    // Check conditions for doing action:
    //  - If same coords as startL NO
    //  - If outside ship rect : NO
    //  - Else: may release only if there's no other rope endpoint at that position
    // 

    if (coords.IsInSize(mModelController.GetModel().GetShipSize())
        && !mModelController.GetRopeElementIndexAt(coords).has_value()
        && coords != mEngagementData->StartCoords)
    {
        //
        // May release here
        //

        // Commit action
        {
            if (!mEngagementData->ExistingRopeElementIndex.has_value())
            {
                mModelController.AddRope(
                    mEngagementData->StartCoords,
                    coords,
                    GetMaterial(mEngagementData->Plane));
            }
            else
            {
                mModelController.MoveRopeEndpoint(
                    *mEngagementData->ExistingRopeElementIndex,
                    mEngagementData->StartCoords,
                    coords);
            }
        }

        // Mark layer as dirty
        SetLayerDirty(LayerType::Ropes);

        // Create undo action
        {
            PushUndoAction(
                _("Pencil Ropes"),
                mEngagementData->OriginalLayerClone.Buffer.GetSize() * sizeof(RopeElement),
                mEngagementData->OriginalDirtyState,
                [originalLayerClone = std::move(mEngagementData->OriginalLayerClone)](Controller & controller) mutable
                {
                    controller.RestoreLayerForUndo(std::move(originalLayerClone));
                });
        }
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();
}

void RopePencilTool::DrawOverlay(ShipSpaceCoordinates const & coords)
{
    //  - If outside ship rect : No overlay
    //  - Else if engaged : check if OK to release
    //      - May release only if there's no other rope endpoint at that position
    //  - Else(!engaged) : check if OK to engage
    //      - Always
    
    if (!coords.IsInSize(mModelController.GetModel().GetShipSize()))
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
            auto const existingRopeElementIndex = mModelController.GetRopeElementIndexAt(coords);
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

        mView.UploadCircleOverlay(
            coords,
            overlayMode);

        mHasOverlay = true;
    }
}

void RopePencilTool::HideOverlay()
{
    mView.RemoveCircleOverlay();

    mHasOverlay = false;
}

inline StructuralMaterial const * RopePencilTool::GetMaterial(MaterialPlaneType plane) const
{
    if (plane == MaterialPlaneType::Foreground)
    {
        return mWorkbenchState.GetRopesForegroundMaterial();
    }
    else
    {
        assert(plane == MaterialPlaneType::Background);

        return mWorkbenchState.GetRopesBackgroundMaterial();
    }
}

}