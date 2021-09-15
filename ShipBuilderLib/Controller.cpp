/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

#include <cassert>

namespace ShipBuilder {

std::unique_ptr<Controller> Controller::CreateNew(
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface)
{
    auto modelController = ModelController::CreateNew(
        WorkSpaceSize(400, 200), // TODO: from preferences
        view);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            view,
            workbenchState,
            userInterface));

    return controller;
}

std::unique_ptr<Controller> Controller::CreateForShip(
    /* TODO: loaded ship ,*/
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface)
{
    auto modelController = ModelController::CreateForShip(
        /* TODO: loaded ship ,*/
        view);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            view,
            workbenchState,
            userInterface));

    return controller;
}

Controller::Controller(
    std::unique_ptr<ModelController> modelController,
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface)
    : mModelController(std::move(modelController))
    , mView(view)
    , mWorkbenchState(workbenchState)
    , mUserInterface(userInterface)
    , mInputState()
    // State
    , mPrimaryLayer(LayerType::Structural)
    // TODO: current tool is "none"
{
    // Notify view of workspace size
    mView.SetWorkSpaceSize(mModelController->GetModel().GetWorkSpaceSize());

    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    mUserInterface.OnPrimaryLayerChanged(mPrimaryLayer);
    mUserInterface.OnCurrentToolChanged(std::nullopt);
}

void Controller::NewStructuralLayer()
{
    mModelController->NewStructuralLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::SetStructuralLayer(/*TODO*/)
{
    mModelController->SetStructuralLayer();

    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::NewElectricalLayer()
{
    mModelController->NewElectricalLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    mModelController->SetElectricalLayer();

    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::RemoveElectricalLayer()
{
    mModelController->RemoveElectricalLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::NewRopesLayer()
{
    mModelController->NewRopesLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::SetRopesLayer(/*TODO*/)
{
    mModelController->SetRopesLayer();

    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::RemoveRopesLayer()
{
    mModelController->RemoveRopesLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::NewTextureLayer()
{
    mModelController->NewTextureLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::SetTextureLayer(/*TODO*/)
{
    mModelController->SetTextureLayer();

    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::RemoveTextureLayer()
{
    mModelController->RemoveTextureLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::ResizeWorkspace(WorkSpaceSize const & newSize)
{
    // TODO: tell ModelController

    // Notify view of new workspace size
    // Note: might cause a view model change that would not be
    // notified via OnViewModelChanged
    mView.SetWorkSpaceSize(newSize);

    mUserInterface.OnWorkSpaceSizeChanged(newSize);
}

LayerType Controller::GetPrimaryLayer() const
{
    return mPrimaryLayer;
}

void Controller::SelectPrimaryLayer(LayerType primaryLayer)
{
    mPrimaryLayer = primaryLayer;

    // Reset current tool
    SetTool(std::nullopt);

    mUserInterface.OnPrimaryLayerChanged(mPrimaryLayer);
}

std::optional<ToolType> Controller::GetTool() const
{
    // TODOHERE
    return std::nullopt;
}

void Controller::SetTool(std::optional<ToolType> tool)
{
    // TODOHERE

    // Notify UI
    mUserInterface.OnCurrentToolChanged(tool);
}

void Controller::AddZoom(int deltaZoom)
{
    mView.SetZoom(mView.GetZoom() + deltaZoom);

    RefreshToolCoordinateDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::SetCamera(int camX, int camY)
{
    mView.SetCameraWorkSpacePosition(WorkSpaceCoordinates(camX, camY));

    RefreshToolCoordinateDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::ResetView()
{
    mView.SetZoom(0);
    mView.SetCameraWorkSpacePosition(WorkSpaceCoordinates(0, 0));

    RefreshToolCoordinateDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::OnWorkCanvasResized(DisplayLogicalSize const & newSize)
{
    mView.SetDisplayLogicalSize(newSize);
    mUserInterface.OnViewModelChanged();
}

void Controller::OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition)
{
    // Update input state
    mInputState.PreviousMousePosition = mInputState.MousePosition;
    mInputState.MousePosition = mouseScreenPosition;

    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(mInputState.MousePosition);

    // TODO: should we detect in<->out transitions and tell tool?

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController->GetModel().GetWorkSpaceSize()))
    {
        // TODO: FW to tool
    }
    else
    {
        // TODO: what to tell tool? Should we detect in<->out transitions?
    }

    RefreshToolCoordinateDisplay();
}

void Controller::OnLeftMouseDown()
{
    // Update input state
    mInputState.IsLeftMouseDown = true;

    // TODO: FW to tool
}

void Controller::OnLeftMouseUp()
{
    // Update input state
    mInputState.IsLeftMouseDown = false;

    // TODO: FW to tool
}

void Controller::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // TODO: FW to tool
}

void Controller::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    // TODO: FW to tool
}

void Controller::OnShiftKeyDown()
{
    // Update input state
    mInputState.IsShiftKeyDown = true;

    // TODO: FW to tool
}

void Controller::OnShiftKeyUp()
{
    // Update input state
    mInputState.IsShiftKeyDown = false;

    // TODO: FW to tool
}

void Controller::OnMouseOut()
{
    // TODO: reset tool

    // Tell UI
    mUserInterface.OnToolCoordinatesChanged(std::nullopt);
}

///////////////////////////////////////////////////////////////////////////////

void Controller::RefreshToolCoordinateDisplay()
{
    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(mInputState.MousePosition);

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController->GetModel().GetWorkSpaceSize()))
    {
        mUserInterface.OnToolCoordinatesChanged(mouseWorkSpaceCoordinates);
    }
    else
    {
        mUserInterface.OnToolCoordinatesChanged(std::nullopt);
    }
}

}