/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopePencilTool.h"

#include <GameCore/GameGeometry.h>

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
        auto const positionApplicability = GetPositionApplicability(*mouseCoordinates);
        if (positionApplicability.has_value())
        {
            DrawOverlay(*mouseCoordinates, *positionApplicability);

            mUserInterface.RefreshView();
        }
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

    mModelController.UploadVisualization();
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

    // Do ephemeral visualization
    if (mEngagementData.has_value())
    {
        DoTempVisualization(mouseCoordinates);
    }

    // Do overlay
    auto const positionApplicability = GetPositionApplicability(mouseCoordinates);
    if (positionApplicability.has_value())
    {
        DrawOverlay(mouseCoordinates, *positionApplicability);
    }
    else if (mHasOverlay)
    {
        HideOverlay();
    }

    mModelController.UploadVisualization();
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

    mModelController.UploadVisualization();
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

    mModelController.UploadVisualization();
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

    mModelController.UploadVisualization();
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

    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

//////////////////////////////////////////////////////////////////////////////

void RopePencilTool::CheckEngagement(
    ShipSpaceCoordinates const & coords,
    MaterialPlaneType materialPlane)
{
    assert(!mEngagementData.has_value());

    if (mModelController.IsRopeEndpointAllowedAt(coords)
        && GetMaterial(materialPlane) != nullptr) // Do not engage with Clear material
    {
        mEngagementData.emplace(
            mModelController.GetModel().CloneLayer<LayerType::Ropes>(),
            mModelController.GetModel().GetDirtyState(),
            coords,
            materialPlane);
    }
}

void RopePencilTool::DoTempVisualization(ShipSpaceCoordinates const & coords)
{
    assert(!mHasTempVisualization);

    assert(mEngagementData.has_value());

    mModelController.AddRopeForEphemeralVisualization(
        mEngagementData->StartCoords,
        coords,
        GetMaterial(mEngagementData->Plane));

    mHasTempVisualization = true;
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

    if (mModelController.IsRopeEndpointAllowedAt(coords))
    {
        // Commit action
        {
            mModelController.AddRope(
                mEngagementData->StartCoords,
                coords,
                GetMaterial(mEngagementData->Plane));
        }

        // Create undo action
        {
            // TODOHERE
            ////auto undoAction = std::make_unique<LayerRegionUndoAction<typename LayerTypeTraits<TLayer>::layer_data_type>>(
            ////    IsEraser ? _("Eraser Tool") : _("Pencil Tool"),
            ////    mEngagementData->OriginalDirtyState,
            ////    std::move(clippedRegionClone),
            ////    mEngagementData->EditRegion->origin);

            ////PushUndoAction(std::move(undoAction));
        }
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();
}

void RopePencilTool::DrawOverlay(
    ShipSpaceCoordinates const & coords,
    bool isApplicablePosition)
{
    mView.UploadCircleOverlay(
        coords,
        isApplicablePosition ? View::OverlayMode::Default : View::OverlayMode::Error);

    mHasOverlay = true;
}

void RopePencilTool::HideOverlay()
{
    mView.RemoveCircleOverlay();

    mHasOverlay = false;
}

std::optional<bool> RopePencilTool::GetPositionApplicability(ShipSpaceCoordinates const & coords) const
{
    if (!coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        return std::nullopt;
    }

    return mModelController.IsRopeEndpointAllowedAt(coords);
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