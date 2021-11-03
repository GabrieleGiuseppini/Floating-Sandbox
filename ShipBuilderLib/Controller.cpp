/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

#include "Tools/FloodTool.h"
#include "Tools/PencilTool.h"

#include <cassert>

namespace ShipBuilder {

std::unique_ptr<Controller> Controller::CreateNew(
    std::string const & shipName,
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ResourceLocator const & resourceLocator)
{
    auto modelController = ModelController::CreateNew(
        ShipSpaceSize(400, 200), // TODO: from preferences
        shipName,
        view);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            view,
            workbenchState,
            userInterface,
            resourceLocator));

    return controller;
}

std::unique_ptr<Controller> Controller::CreateForShip(
    ShipDefinition && shipDefinition,
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ResourceLocator const & resourceLocator)
{
    auto modelController = ModelController::CreateForShip(
        std::move(shipDefinition),
        view);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            view,
            workbenchState,
            userInterface,
            resourceLocator));

    return controller;
}

Controller::Controller(
    std::unique_ptr<ModelController> modelController,
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ResourceLocator const & resourceLocator)
    : mModelController(std::move(modelController))
    , mUndoStack()
    , mView(view)
    , mWorkbenchState(workbenchState)
    , mUserInterface(userInterface)
    , mResourceLocator(resourceLocator)
    // State
    , mPrimaryLayer(LayerType::Structural)
    , mCurrentToolType(ToolType::StructuralPencil)
    , mCurrentTool()
    , mLastToolTypePerLayer({ToolType::StructuralPencil, ToolType::ElectricalPencil, std::nullopt, std::nullopt})
{
    // We assume we start with at least a structural layer
    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    // Tell view new workspace size and settings
    mView.SetShipSize(mModelController->GetModel().GetShipSize());
    mView.SetPrimaryLayer(mPrimaryLayer);

    // Set ideal zoom
    mView.SetZoom(mView.CalculateIdealZoom());

    // Notify our initializations
    mUserInterface.OnPrimaryLayerChanged(mPrimaryLayer);
    mUserInterface.OnCurrentToolChanged(*mCurrentToolType);

    // Upload layers visualization
    mModelController->UploadVisualization();

    // Create tool (might upload dirty visualization already)
    mCurrentTool = MakeTool(*mCurrentToolType);
}

void Controller::ClearModelDirty()
{
    mModelController->ClearIsDirty();
    mUserInterface.OnModelDirtyChanged();
}

void Controller::NewStructuralLayer()
{
    // Remove tool
    StopTool();

    // Make new layer
    mModelController->NewStructuralLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Structural)
    {
        InternalSelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::SetStructuralLayer(/*TODO*/)
{
    // Remove tool
    StopTool();

    // Update layer
    mModelController->SetStructuralLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Structural)
    {
        InternalSelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RestoreLayerRegion(
    StructuralLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    // Note: this is invoked from undo's double-dispatch, that's why it doesn't worry about tool

    mModelController->RestoreStructuralLayer(
        std::move(layerRegion),
        { {0, 0}, layerRegion.Buffer.Size},
        origin);

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::NewElectricalLayer()
{
    // Remove tool
    StopTool();

    // Make new layer
    mModelController->NewElectricalLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Electrical)
    {
        InternalSelectPrimaryLayer(LayerType::Electrical);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    // Remove tool
    StopTool();

    // Update layer
    mModelController->SetElectricalLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Electrical)
    {
        InternalSelectPrimaryLayer(LayerType::Electrical);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RemoveElectricalLayer()
{
    // Remove tool
    StopTool();

    // Remove layer
    mModelController->RemoveElectricalLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Electrical)
    {
        InternalSelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RestoreLayerRegion(
    ElectricalLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    // Note: this is invoked from undo's double-dispatch, that's why it doesn't worry about tool

    mModelController->RestoreElectricalLayer(
        std::move(layerRegion),
        { {0, 0}, layerRegion.Buffer.Size },
        origin);

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::NewRopesLayer()
{
    // Remove tool
    StopTool();

    // Make new layer
    mModelController->NewRopesLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Ropes)
    {
        InternalSelectPrimaryLayer(LayerType::Ropes);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::SetRopesLayer(/*TODO*/)
{
    // Remove tool
    StopTool();

    // Update layer
    mModelController->SetRopesLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Ropes)
    {
        InternalSelectPrimaryLayer(LayerType::Ropes);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RemoveRopesLayer()
{
    // Remove tool
    StopTool();

    // Remove layer
    mModelController->RemoveRopesLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Ropes)
    {
        InternalSelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RestoreLayerRegion(
    RopesLayerData const & layerRegion,
    ShipSpaceCoordinates const & origin)
{
    // TODOHERE: copy from structural
}

void Controller::SetTextureLayer(/*TODO*/)
{
    // Remove tool
    StopTool();

    // Update layer
    mModelController->SetTextureLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Texture)
    {
        InternalSelectPrimaryLayer(LayerType::Texture);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RemoveTextureLayer()
{
    // Remove tool
    StopTool();

    // Remove layer
    mModelController->RemoveTextureLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Texture)
    {
        InternalSelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

    // Restart tool
    StartTool();
}

void Controller::RestoreLayerRegion(
    TextureLayerData const & layerRegion,
    ShipSpaceCoordinates const & origin)
{
    // TODOHERE: copy from structural
}

void Controller::ResizeShip(ShipSpaceSize const & newSize)
{
    // TODO: reset tool
    // TODO: tell ModelController
    // TODO: update layers visualization
    // TODO: update dirtyness (of all present layers)

    // Notify view of new size
    // Note: might cause a view model change that would not be
    // notified via OnViewModelChanged
    mView.SetShipSize(newSize);

    mUserInterface.OnShipSizeChanged(newSize);
}

LayerType Controller::GetPrimaryLayer() const
{
    return mPrimaryLayer;
}

void Controller::SelectPrimaryLayer(LayerType primaryLayer)
{
    if (primaryLayer != mPrimaryLayer)
    {
        StopTool();

        InternalSelectPrimaryLayer(primaryLayer);

        StartTool();

        // Refresh view
        mUserInterface.RefreshView();
    }
}

void Controller::SetOtherLayersOpacity(float opacity)
{
    mView.SetOtherLayersOpacity(opacity);

    mUserInterface.RefreshView();
}

std::optional<ToolType> Controller::GetCurrentTool() const
{
    return mCurrentToolType;
}

void Controller::SetCurrentTool(std::optional<ToolType> tool)
{
    if (tool != mCurrentToolType)
    {
        // Nuke current tool
        mCurrentToolType.reset();
        mCurrentTool.reset();

        InternalSetCurrentTool(tool);

        // Make new tool
        assert(mCurrentToolType == tool);
        if (mCurrentToolType.has_value())
        {
            mCurrentTool = MakeTool(*mCurrentToolType);
        }
    }
}

bool Controller::CanUndo() const
{
    return !mUndoStack.IsEmpty();
}

void Controller::Undo()
{
    assert(CanUndo());

    // Remove tool
    StopTool();

    // Apply action
    auto undoAction = mUndoStack.Pop();
    undoAction->ApplyAndConsume(*this);

    // Restore dirtyness
    mModelController->RestoreDirtyState(undoAction->GetOriginalDirtyState());
    mUserInterface.OnModelDirtyChanged();

    // Update undo state
    mUserInterface.OnUndoStackStateChanged();

    // Restart tool
    StartTool();
}

void Controller::AddZoom(int deltaZoom)
{
    mView.SetZoom(mView.GetZoom() + deltaZoom);

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::SetCamera(int camX, int camY)
{
    mView.SetCameraShipSpacePosition(ShipSpaceCoordinates(camX, camY));

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas // TODOHERE, and if the scrollbars (possible
    // origins of this call) have not captured the mouse
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::ResetView()
{
    mView.SetZoom(0);
    mView.SetCameraShipSpacePosition(ShipSpaceCoordinates(0, 0));

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::OnWorkCanvasResized(DisplayLogicalSize const & newSize)
{
    mView.SetDisplayLogicalSize(newSize);

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    mUserInterface.OnViewModelChanged();
}

void Controller::EnableVisualGrid(bool doEnable)
{
    mView.EnableVisualGrid(doEnable);
    mUserInterface.RefreshView();
}

void Controller::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnMouseMove(mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
}

void Controller::OnLeftMouseDown()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnLeftMouseDown();
    }
}

void Controller::OnLeftMouseUp()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnLeftMouseUp();
    }
}

void Controller::OnRightMouseDown()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnRightMouseDown();
    }
}

void Controller::OnRightMouseUp()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnRightMouseUp();
    }
}

void Controller::OnShiftKeyDown()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnShiftKeyDown();
    }
}

void Controller::OnShiftKeyUp()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnShiftKeyUp();
    }
}

void Controller::OnUncapturedMouseOut()
{
    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnUncapturedMouseOut();
    }

    // Tell UI
    mUserInterface.OnToolCoordinatesChanged(std::nullopt);
}

void Controller::OnMouseCaptureLost()
{
    // Reset tool
    StopTool();
    StartTool();
}

///////////////////////////////////////////////////////////////////////////////

void Controller::InternalSelectPrimaryLayer(LayerType primaryLayer)
{
    //
    // No tool destroy/create
    // No visualization changes
    //

    assert(!mCurrentTool);
    assert(mPrimaryLayer != primaryLayer);

    // Store new primary layer
    mPrimaryLayer = primaryLayer;

    // Select the last tool we have used for this new layer
    // Note: stores new tool for new primary layer
    InternalSetCurrentTool(mLastToolTypePerLayer[static_cast<int>(primaryLayer)]);

    // Notify
    mUserInterface.OnPrimaryLayerChanged(primaryLayer);

    // TODO: *update* layers visualization at controller
    // TODO: *upload* "                                "

    // Tell view
    mView.SetPrimaryLayer(primaryLayer);
}

void Controller::InternalSetCurrentTool(std::optional<ToolType> toolType)
{
    //
    // No tool destroy/create
    // No visualization changes
    //

    assert(!mCurrentTool);
    assert(mCurrentToolType != toolType);

    // Set current tool
    mCurrentToolType = toolType;

    if (!toolType.has_value())
    {
        // Relinquish cursor
        mUserInterface.ResetToolCursor();
    }

    // Remember new tool as the last tool of this primary layer
    mLastToolTypePerLayer[static_cast<int>(mPrimaryLayer)] = toolType;

    // Notify UI
    mUserInterface.OnCurrentToolChanged(mCurrentToolType);
}

void Controller::StopTool()
{
    mCurrentTool.reset();
}

void Controller::StartTool()
{
    assert(!mCurrentTool);

    if (mCurrentToolType.has_value())
    {
        mCurrentTool = MakeTool(*mCurrentToolType);
    }
}

std::unique_ptr<Tool> Controller::MakeTool(ToolType toolType)
{
    switch (toolType)
    {
        case ToolType::ElectricalEraser:
        {
            return std::make_unique<ElectricalEraserTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }

        case ToolType::ElectricalFlood:
        {
            return std::make_unique<ElectricalFloodTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }

        case ToolType::ElectricalPencil:
        {
            return std::make_unique<ElectricalPencilTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }

        case ToolType::StructuralEraser:
        {
            return std::make_unique<StructuralEraserTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }

        case ToolType::StructuralFlood:
        {
            return std::make_unique<StructuralFloodTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }

        case ToolType::StructuralPencil:
        {
            return std::make_unique<StructuralPencilTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }
    }

    assert(false);
    return nullptr;
}

void Controller::RefreshToolCoordinatesDisplay()
{
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mUserInterface.GetMouseCoordinates();

    // Check if within ship canvas
    if (mouseShipSpaceCoordinates.IsInSize(mModelController->GetModel().GetShipSize()))
    {
        mUserInterface.OnToolCoordinatesChanged(mouseShipSpaceCoordinates);
    }
    else
    {
        mUserInterface.OnToolCoordinatesChanged(std::nullopt);
    }
}

}