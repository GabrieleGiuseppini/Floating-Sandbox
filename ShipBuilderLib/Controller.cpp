/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

#include "Tools/FloodTool.h"
#include "Tools/PencilTool.h"
#include "Tools/RopePencilTool.h"

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
    , mLastToolTypePerLayer({ToolType::StructuralPencil, ToolType::ElectricalPencil, ToolType::RopePencil, std::nullopt})
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
}

ShipDefinition Controller::MakeShipDefinition()
{
    auto const scopedToolResumeState = SuspendTool();

    assert(mModelController);
    assert(!mModelController->IsInEphemeralVisualization());

    return mModelController->MakeShipDefinition();
}

void Controller::ClearModelDirty()
{
    mModelController->ClearIsDirty();
    mUserInterface.OnModelDirtyChanged();
}

ModelValidationResults Controller::ValidateModel()
{
    auto const scopedToolResumeState = SuspendTool();

    assert(mModelController);
    assert(!mModelController->IsInEphemeralVisualization());

    return mModelController->ValidateModel();
}

void Controller::NewStructuralLayer()
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::SetStructuralLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::RestoreLayerRegion(
    StructuralLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

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
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::RemoveElectricalLayer()
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::RestoreLayerRegion(
    ElectricalLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

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

void Controller::TrimElectricalParticlesWithoutSubstratum()
{
    auto const scopedToolResumeState = SuspendTool();

    // Trim
    {
        // Save state
        auto originalDirtyStateClone = mModelController->GetModel().GetDirtyState();
        auto originalLayerClone = mModelController->GetModel().CloneLayer<LayerType::Electrical>();

        // Trim
        auto const affectedRect = mModelController->TrimElectricalParticlesWithoutSubstratum();

        // Create undo action, if needed
        if (affectedRect)
        {
            //
           // Create undo action
           //

            auto clippedRegionClone = originalLayerClone.Clone(*affectedRect);

            auto undoAction = std::make_unique<LayerRegionUndoAction<ElectricalLayerData>>(
                _("Trim Electrical"),
                originalDirtyStateClone,
                std::move(clippedRegionClone),
                affectedRect->origin);

            mUndoStack.Push(std::move(undoAction));
            mUserInterface.OnUndoStackStateChanged();
        }
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();
}

void Controller::NewRopesLayer()
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::SetRopesLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::RemoveRopesLayer()
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::RestoreLayer(RopesLayerData && layer)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreRopesLayer(std::move(layer));

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualization
    mModelController->UploadVisualization();
    mUserInterface.RefreshView();

}

void Controller::SetTextureLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

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
}

void Controller::RemoveTextureLayer()
{
    auto const scopedToolResumeState = SuspendTool();

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
        {
            auto const scopedToolResumeState = SuspendTool();

            InternalSelectPrimaryLayer(primaryLayer);
        }

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
        bool const hadTool = (mCurrentTool != nullptr);

        // Nuke current tool
        mCurrentToolType.reset();
        mCurrentTool.reset();

        InternalSetCurrentTool(tool);

        // Make new tool - unless we are suspended
        assert(mCurrentToolType == tool);
        if (mCurrentToolType.has_value()
            && hadTool)
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

    auto const scopedToolResumeState = SuspendTool();

    // Apply action
    auto undoAction = mUndoStack.Pop();
    undoAction->ApplyAndConsume(*this);

    // Restore dirtyness
    mModelController->RestoreDirtyState(undoAction->GetOriginalDirtyState());
    mUserInterface.OnModelDirtyChanged();

    // Update undo state
    mUserInterface.OnUndoStackStateChanged();
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

void Controller::OnUncapturedMouseIn()
{
    InternalResumeTool();
}

void Controller::OnUncapturedMouseOut()
{
    InternalSuspendTool();

    // Tell UI
    mUserInterface.OnToolCoordinatesChanged(std::nullopt);
}

void Controller::OnMouseCaptureLost()
{
    // Reset tool
    InternalResetTool();
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

Controller::ScopedToolResumeState Controller::SuspendTool()
{
    return ScopedToolResumeState(
        *this,
        InternalSuspendTool());
}

bool Controller::InternalSuspendTool()
{
    bool const doResume = (mCurrentTool != nullptr);

    mCurrentTool.reset();
    // Leave mCurrentToolType as-is

    return doResume;
}

void Controller::InternalResumeTool()
{
    assert(!mCurrentTool);

    if (mCurrentToolType.has_value())
    {
        mCurrentTool = MakeTool(*mCurrentToolType);
    }
}

void Controller::InternalResetTool()
{
    InternalSuspendTool();
    InternalResumeTool();
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

        case ToolType::RopePencil:
        {
            return std::make_unique<RopePencilTool>(
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