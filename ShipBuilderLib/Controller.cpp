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
        WorkSpaceSize(400, 200), // TODO: from preferences
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
    mView.SetWorkSpaceSize(mModelController->GetModel().GetWorkSpaceSize());

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
    // Nuke old tool
    mCurrentTool.reset();

    // Make new tool
    if (tool.has_value())
    {
        mCurrentTool = MakeTool(*tool);
    }

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

    // TODOHERE
    // TODO: should we detect in<->out transitions and tell tool?

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController->GetModel().GetWorkSpaceSize()))
    {
        if (mCurrentTool)
        {
            mCurrentTool->OnMouseMove(mInputState);
        }
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

    // TODO: should we do this only if in rect?
    if (mCurrentTool)
    {
        mCurrentTool->OnLeftMouseDown(mInputState);
    }
}

void Controller::OnLeftMouseUp()
{
    // Update input state
    mInputState.IsLeftMouseDown = false;

    // TODO: should we do this only if in rect?
    if (mCurrentTool)
    {
        mCurrentTool->OnLeftMouseUp(mInputState);
    }
}

void Controller::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // TODO: should we do this only if in rect?
    if (mCurrentTool)
    {
        mCurrentTool->OnRightMouseDown(mInputState);
    }
}

void Controller::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    // TODO: should we do this only if in rect?
    if (mCurrentTool)
    {
        mCurrentTool->OnRightMouseUp(mInputState);
    }
}

void Controller::OnShiftKeyDown()
{
    // Update input state
    mInputState.IsShiftKeyDown = true;

    // TODO: should we do this only if in rect?
    if (mCurrentTool)
    {
        mCurrentTool->OnShiftKeyDown(mInputState);
    }
}

void Controller::OnShiftKeyUp()
{
    // Update input state
    mInputState.IsShiftKeyDown = false;

    // TODO: should we do this only if in rect?
    if (mCurrentTool)
    {
        mCurrentTool->OnShiftKeyUp(mInputState);
    }
}

void Controller::OnMouseOut()
{
    // Reset tool
    {
        auto const oldTool = GetCurrentTool();
        mCurrentTool.reset();
        if (oldTool.has_value())
        {
            mCurrentTool = MakeTool(*oldTool);
        }
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