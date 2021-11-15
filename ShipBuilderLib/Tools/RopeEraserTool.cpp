/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopeEraserTool.h"

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
    , mOriginalLayerClone(modelController.GetModel().CloneLayer<LayerType::Ropes>())
    , mHasEphemeralVisualization(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("eraser_cursor", 8, 27, resourceLocator));

    // Check if we need to do a eph viz right away
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DoEphemeralVisualization(*mouseCoordinates);

        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
}

RopeEraserTool::~RopeEraserTool()
{
    // Mend our ephemeral visualization, if any
    if (mHasEphemeralVisualization)
    {
        MendEphemeralVisualization();

        assert(!mHasEphemeralVisualization);

        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
}

void RopeEraserTool::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Mend our ephemeral visualization, if any
    if (mHasEphemeralVisualization)
    {
        MendEphemeralVisualization();

        assert(!mHasEphemeralVisualization);
    }

    if (mEngagementData.has_value())
    {
        // Do action
        DoAction(mouseCoordinates);

        // No need to do eph viz when engaged
    }
    else
    {
        // Do eph viz
        DoEphemeralVisualization(mouseCoordinates);
    }

    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

void RopeEraserTool::OnLeftMouseDown()
{
    // Mend our ephemeral visualization, if any
    if (mHasEphemeralVisualization)
    {
        MendEphemeralVisualization();

        assert(!mHasEphemeralVisualization);
    }

    // Engage
    StartEngagement();

    // Do action
    DoAction(mUserInterface.GetMouseCoordinates());

    // No need to do eph viz when engaged

    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

void RopeEraserTool::OnLeftMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasEphemeralVisualization)
    {
        MendEphemeralVisualization();

        assert(!mHasEphemeralVisualization);
    }

    // Disengage
    StopEngagement();

    // Do eph viz
    assert(!mEngagementData.has_value());
    DoEphemeralVisualization(mUserInterface.GetMouseCoordinates());

    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

void RopeEraserTool::OnRightMouseDown()
{
    // Mend our ephemeral visualization, if any
    if (mHasEphemeralVisualization)
    {
        MendEphemeralVisualization();

        assert(!mHasEphemeralVisualization);
    }

    // Engage
    StartEngagement();

    // Do action
    DoAction(mUserInterface.GetMouseCoordinates());

    // No need to do eph viz when engaged

    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

void RopeEraserTool::OnRightMouseUp()
{
    // Mend our ephemeral visualization, if any
    if (mHasEphemeralVisualization)
    {
        MendEphemeralVisualization();

        assert(!mHasEphemeralVisualization);
    }

    // Disengage
    StopEngagement();

    // Do eph viz
    assert(!mEngagementData.has_value());
    DoEphemeralVisualization(mUserInterface.GetMouseCoordinates());

    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

//////////////////////////////////////////////////////////////////////////////

void RopeEraserTool::DoEphemeralVisualization(ShipSpaceCoordinates const & coords)
{
    assert(!mHasEphemeralVisualization);

    assert(!mEngagementData.has_value());

    mModelController.EraseRopeAtForEphemeralVisualization(coords);

    mView.UploadCircleOverlay(
        coords,
        View::OverlayMode::Default);

    mHasEphemeralVisualization = true;
}

void RopeEraserTool::MendEphemeralVisualization()
{
    assert(mHasEphemeralVisualization);

    assert(!mEngagementData.has_value());

    mModelController.RestoreRopesLayerForEphemeralVisualization(mOriginalLayerClone);

    mView.RemoveCircleOverlay();

    mHasEphemeralVisualization = false;
}

void RopeEraserTool::StartEngagement()
{
    assert(!mHasEphemeralVisualization);

    assert(!mEngagementData.has_value());

    mEngagementData.emplace(mModelController.GetModel().GetDirtyState());
}

void RopeEraserTool::DoAction(ShipSpaceCoordinates const & coords)
{
    assert(!mHasEphemeralVisualization);

    assert(mEngagementData.has_value());

    bool const hasErased = mModelController.EraseRopeAt(coords);
    if (hasErased)
    {
        mEngagementData->HasEdited = true;
    }
}

void RopeEraserTool::StopEngagement()
{
    assert(!mHasEphemeralVisualization);

    assert(mEngagementData.has_value());

    if (mEngagementData->HasEdited)
    {
        //
        // Create undo action
        //

        size_t const cost = mOriginalLayerClone.Buffer.size() * sizeof(RopeElement);

        auto undoAction = std::make_unique<WholeLayerUndoAction<RopesLayerData>>(
            _("Ropes Eraser"),
            mEngagementData->OriginalDirtyState,
            std::move(mOriginalLayerClone),
            cost);

        PushUndoAction(std::move(undoAction));

        // Take new orig clone
        mOriginalLayerClone = mModelController.GetModel().CloneLayer<LayerType::Ropes>();
    }

    // Stop engagement
    mEngagementData.reset();
}

}