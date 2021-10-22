/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

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
{
    // We assume we start with at least a structural layer
    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    // Tell view new workspace size
    mView.SetShipSize(mModelController->GetModel().GetShipSize());

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
    mModelController->NewStructuralLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    SelectPrimaryLayer(LayerType::Structural);

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::SetStructuralLayer(/*TODO*/)
{
    mModelController->SetStructuralLayer(/*TODO*/);

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerBufferRegion(
    StructuralLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    mModelController->StructuralRegionReplace(
        layerBufferRegion,
        { {0, 0}, layerBufferRegion.Size},
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
    mModelController->NewElectricalLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    SelectPrimaryLayer(LayerType::Electrical);

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    mModelController->SetElectricalLayer();

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RemoveElectricalLayer()
{
    mModelController->RemoveElectricalLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Electrical)
    {
        SelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerBufferRegion(
    ElectricalLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    // TODOHERE: copy from Structural
}

void Controller::NewRopesLayer()
{
    mModelController->NewRopesLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to this one
    SelectPrimaryLayer(LayerType::Ropes);

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::SetRopesLayer(/*TODO*/)
{
    mModelController->SetRopesLayer();

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RemoveRopesLayer()
{
    mModelController->RemoveRopesLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Ropes)
    {
        SelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerBufferRegion(
    RopesLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    // TODOHERE: copy from structural
}

void Controller::SetTextureLayer(/*TODO*/)
{
    mModelController->SetTextureLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RemoveTextureLayer()
{
    mModelController->RemoveTextureLayer();
    mUserInterface.OnLayerPresenceChanged();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Texture)
    {
        SelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerBufferRegion(
    TextureLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    // TODOHERE: copy from structural
}

void Controller::ResizeShip(ShipSpaceSize const & newSize)
{
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
    mPrimaryLayer = primaryLayer;

    // Reset current tool to none
    // TODO: might actually want to select the "primary tool" for the layer
    // (i.e. probably the pencil, in all cases)
    SetCurrentTool(std::nullopt);

    mUserInterface.OnPrimaryLayerChanged(mPrimaryLayer);

    // TODO: *update* layers visualization at controller
    // TODO: *upload* "                                "
    // TODO: view refresh
}

std::optional<ToolType> Controller::GetCurrentTool() const
{
    return mCurrentToolType;
}

void Controller::SetCurrentTool(std::optional<ToolType> tool)
{
    // Nuke current tool
    mCurrentToolType.reset();
    mCurrentTool.reset();

    if (tool.has_value())
    {
        mCurrentToolType = tool;
        mCurrentTool = MakeTool(*mCurrentToolType);
    }
    else
    {
        // Relinquish cursor
        mUserInterface.ResetToolCursor();
    }

    // Notify UI
    mUserInterface.OnCurrentToolChanged(mCurrentToolType);
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
    undoAction->ApplyAction(*this);

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

    // Tell tool about the new mouse (ship space) position
    if (mUserInterface.IsMouseInWorkCanvas()
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(mUserInterface.GetMouseCoordinates());
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::SetCamera(int camX, int camY)
{
    mView.SetCameraShipSpacePosition(ShipSpaceCoordinates(camX, camY));

    // Tell tool about the new mouse (ship space) position
    if (mUserInterface.IsMouseInWorkCanvas()
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(mUserInterface.GetMouseCoordinates());
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::ResetView()
{
    mView.SetZoom(0);
    mView.SetCameraShipSpacePosition(ShipSpaceCoordinates(0, 0));

    // Tell tool about the new mouse (ship space) position
    if (mUserInterface.IsMouseInWorkCanvas()
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(mUserInterface.GetMouseCoordinates());
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::OnWorkCanvasResized(DisplayLogicalSize const & newSize)
{
    mView.SetDisplayLogicalSize(newSize);

    // Tell tool about the new mouse (ship space) position
    if (mUserInterface.IsMouseInWorkCanvas()
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(mUserInterface.GetMouseCoordinates());
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

///////////////////////////////////////////////////////////////////////////////

void Controller::StopTool()
{
    mCurrentTool.reset();
}

void Controller::StartTool()
{
    if (mCurrentToolType.has_value())
    {
        mCurrentTool = MakeTool(*mCurrentToolType);
    }
}

std::unique_ptr<Tool> Controller::MakeTool(ToolType toolType)
{
    switch (toolType)
    {
        // TODOHERE: other tool types

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