/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopeEraserTool.h"

#include "Controller.h"

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

RopeEraserTool::RopeEraserTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::RopeEraser,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mOriginalLayerClone(modelController.GetModel().CloneExistingLayer<LayerType::Ropes>())
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("eraser_cursor", 8, 27, resourceLocator));

    // Check if we draw the overlay right away
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DrawOverlay(*mouseCoordinates);

        mModelController.UploadVisualizations(mView);
        mUserInterface.RefreshView();
    }
}

RopeEraserTool::~RopeEraserTool()
{
    // Remove our overlay, if any
    if (mHasOverlay)
    {
        HideOverlay();

        mModelController.UploadVisualizations(mView);
        mUserInterface.RefreshView();
    }
}

void RopeEraserTool::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    if (mEngagementData.has_value())
    {
        // Do action
        DoAction(mouseCoordinates);

        // No need to do eph viz when engaged
    }
    else
    {
        // Draw overlay
        DrawOverlay(mouseCoordinates);
    }

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
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

    // Do action
    DoAction(mUserInterface.GetMouseCoordinates());

    // No need to do eph viz when engaged

    mModelController.UploadVisualizations(mView);
    mUserInterface.RefreshView();
}

void RopeEraserTool::OnMouseUp()
{
    // Check if should stop engagement
    if (mEngagementData.has_value())
    {
        assert(!mHasOverlay);

        // Disengage
        StopEngagement();

        // Restart overlay
        DrawOverlay(mUserInterface.GetMouseCoordinates());

        assert(mHasOverlay);

        mModelController.UploadVisualizations(mView);
        mUserInterface.RefreshView();
    }
}

void RopeEraserTool::StartEngagement()
{
    assert(!mHasOverlay);

    assert(!mEngagementData.has_value());

    mEngagementData.emplace(mModelController.GetModel().GetDirtyState());
}

void RopeEraserTool::DoAction(ShipSpaceCoordinates const & coords)
{
    assert(!mHasOverlay);

    assert(mEngagementData.has_value());

    bool const hasErased = mModelController.EraseRopeAt(coords);
    if (hasErased)
    {
        mEngagementData->HasEdited = true;
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

        PushUndoAction(
            _("Eraser Ropes"),
            mOriginalLayerClone.Buffer.GetSize() * sizeof(RopeElement),
            mEngagementData->OriginalDirtyState,
            [originalLayerClone = std::move(mOriginalLayerClone)](Controller & controller) mutable
            {
                controller.RestoreLayerForUndo(std::move(originalLayerClone));
            });

        // Take new orig clone
        mOriginalLayerClone = mModelController.GetModel().CloneExistingLayer<LayerType::Ropes>();
    }

    // Stop engagement
    mEngagementData.reset();
}

void RopeEraserTool::DrawOverlay(ShipSpaceCoordinates const & coords)
{
    mView.UploadCircleOverlay(
        coords,
        mModelController.GetRopeElementIndexAt(coords).has_value()
        ? View::OverlayMode::Default
        : View::OverlayMode::Error);

    mHasOverlay = true;
}

void RopeEraserTool::HideOverlay()
{
    assert(mHasOverlay);

    mView.RemoveCircleOverlay();

    mHasOverlay = false;
}

}