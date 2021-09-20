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
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ResourceLocator const & resourceLocator)
{
    auto modelController = ModelController::CreateNew(
        ShipSpaceSize(400, 200), // TODO: from preferences
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
    /* TODO: loaded ship ,*/
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ResourceLocator const & resourceLocator)
{
    auto modelController = ModelController::CreateForShip(
        /* TODO: loaded ship ,*/
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
    , mView(view)
    , mWorkbenchState(workbenchState)
    , mUserInterface(userInterface)
    , mResourceLocator(resourceLocator)
    , mInputState()
    // State
    , mPrimaryLayer(LayerType::Structural)
    , mCurrentTool(MakeTool(ToolType::StructuralPencil))
{
    // Notify view of workspace size
    mView.SetShipSize(mModelController->GetModel().GetShipSize());

    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    mUserInterface.OnPrimaryLayerChanged(mPrimaryLayer);
    mUserInterface.OnCurrentToolChanged(mCurrentTool->GetType());
}

void Controller::NewStructuralLayer()
{
    mModelController->NewStructuralLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());

    // Switch primary layer to this one
    SelectPrimaryLayer(LayerType::Structural);
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

    // Switch primary layer to this one
    SelectPrimaryLayer(LayerType::Electrical);
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    mModelController->SetElectricalLayer();

    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::RemoveElectricalLayer()
{
    mModelController->RemoveElectricalLayer();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Electrical)
    {
        SelectPrimaryLayer(LayerType::Structural);
    }

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::NewRopesLayer()
{
    mModelController->NewRopesLayer();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());

    // Switch primary layer to this one
    SelectPrimaryLayer(LayerType::Ropes);
}

void Controller::SetRopesLayer(/*TODO*/)
{
    mModelController->SetRopesLayer();

    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::RemoveRopesLayer()
{
    mModelController->RemoveRopesLayer();

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Ropes)
    {
        SelectPrimaryLayer(LayerType::Structural);
    }

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

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Texture)
    {
        SelectPrimaryLayer(LayerType::Structural);
    }

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel().GetIsDirty());
}

void Controller::ResizeShip(ShipSpaceSize const & newSize)
{
    // TODO: tell ModelController

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
}

std::optional<ToolType> Controller::GetCurrentTool() const
{
    return mCurrentTool
        ? mCurrentTool->GetType()
        : std::optional<ToolType>();
}

void Controller::SetCurrentTool(std::optional<ToolType> tool)
{
    // Nuke current tool
    mCurrentTool.reset();

    if (tool.has_value())
    {
        mCurrentTool = MakeTool(*tool);
    }
    else
    {
        // Relinquish cursor
        mUserInterface.ResetToolCursor();
    }

    // Notify UI
    mUserInterface.OnCurrentToolChanged(tool);
}

void Controller::AddZoom(int deltaZoom)
{
    mView.SetZoom(mView.GetZoom() + deltaZoom);

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::SetCamera(int camX, int camY)
{
    mView.SetCameraShipSpacePosition(ShipSpaceCoordinates(camX, camY));

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();
}

void Controller::ResetView()
{
    mView.SetZoom(0);
    mView.SetCameraShipSpacePosition(ShipSpaceCoordinates(0, 0));

    RefreshToolCoordinatesDisplay();
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

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnMouseMove(mInputState);
    }

    RefreshToolCoordinatesDisplay();
}

void Controller::OnLeftMouseDown()
{
    // Update input state
    mInputState.IsLeftMouseDown = true;

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnLeftMouseDown(mInputState);
    }
}

void Controller::OnLeftMouseUp()
{
    // Update input state
    mInputState.IsLeftMouseDown = false;

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnLeftMouseUp(mInputState);
    }
}

void Controller::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnRightMouseDown(mInputState);
    }
}

void Controller::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnRightMouseUp(mInputState);
    }
}

void Controller::OnShiftKeyDown()
{
    // Update input state
    mInputState.IsShiftKeyDown = true;

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnShiftKeyDown(mInputState);
    }
}

void Controller::OnShiftKeyUp()
{
    // Update input state
    mInputState.IsShiftKeyDown = false;

    // Forward to tool
    if (mCurrentTool)
    {
        mCurrentTool->OnShiftKeyUp(mInputState);
    }
}

void Controller::OnMouseOut()
{
    // Reset tool
    if (mCurrentTool)
    {
        mCurrentTool->Reset();
    }

    // Tell UI
    mUserInterface.OnToolCoordinatesChanged(std::nullopt);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Tool> Controller::MakeTool(ToolType toolType)
{
    switch (toolType)
    {
        // TODOHERE: other tool types

        case ToolType::StructuralPencil:
        {
            return std::make_unique<StructuralPencilTool>(
                *mModelController,
                mWorkbenchState,
                mUserInterface,
                mView,
                mResourceLocator);
        }
    }
}

void Controller::RefreshToolCoordinatesDisplay()
{
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mView.ScreenToShipSpace(mInputState.MousePosition);

    // Check if within ship canvas
    if (mouseShipSpaceCoordinates.IsInRect(mModelController->GetModel().GetShipSize()))
    {
        mUserInterface.OnToolCoordinatesChanged(mouseShipSpaceCoordinates);
    }
    else
    {
        mUserInterface.OnToolCoordinatesChanged(std::nullopt);
    }
}

}