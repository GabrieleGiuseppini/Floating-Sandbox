/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

#include "Tools/FloodTool.h"
#include "Tools/LineTool.h"
#include "Tools/PencilTool.h"
#include "Tools/RopeEraserTool.h"
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
        ShipSpaceSize(200, 100), // TODO: from preferences
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
    , mStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::ParticleMode)
    , mElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::ParticleMode)
    , mRopesLayerVisualizationMode(RopesLayerVisualizationModeType::LinesMode)
    , mTextureLayerVisualizationMode(TextureLayerVisualizationModeType::MatteMode)
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

    // Initialize layer visualizations
    InternalUpdateVisualizationModes();

    // Notify our initializations
    mUserInterface.OnPrimaryLayerChanged(mPrimaryLayer);
    mUserInterface.OnStructuralLayerVisualizationModeChanged(mStructuralLayerVisualizationMode);
    mUserInterface.OnElectricalLayerVisualizationModeChanged(mElectricalLayerVisualizationMode);
    mUserInterface.OnRopesLayerVisualizationModeChanged(mRopesLayerVisualizationMode);
    mUserInterface.OnTextureLayerVisualizationModeChanged(mTextureLayerVisualizationMode);
    mUserInterface.OnCurrentToolChanged(*mCurrentToolType);

    // Upload layers' visualizations
    mModelController->UploadVisualizations();
}

ShipDefinition Controller::MakeShipDefinition()
{
    auto const scopedToolResumeState = SuspendTool();

    assert(mModelController);
    assert(!mModelController->IsInEphemeralVisualization());

    return mModelController->MakeShipDefinition();
}

void Controller::SetShipProperties(
    std::optional<ShipMetadata> && metadata,
    std::optional<ShipPhysicsData> && physicsData,
    std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings)
{
    assert(mModelController);

    // Assuming at least one of the three was changed
    assert(metadata.has_value() || physicsData.has_value() || autoTexturizationSettings.has_value());

    //
    // Prepare undo entry
    //

    auto f = 
        [oldMetadata = mModelController->GetShipMetadata()
        , oldPhysicsData = mModelController->GetShipPhysicsData()
        , oldAutoTexturizationSettings = mModelController->GetShipAutoTexturizationSettings()]
        (Controller & controller) mutable
        {
            controller.RestoreShipPropertiesForUndo(
                std::move(oldMetadata),
                std::move(oldPhysicsData),
                std::move(oldAutoTexturizationSettings));
        };

    auto originalDirtyState = mModelController->GetModel().GetDirtyState();

    //
    // Set new properties
    //

    InternalSetShipProperties(
        std::move(metadata),
        std::move(physicsData),
        std::move(autoTexturizationSettings));

    // At least one of the three was changed
    mUserInterface.OnModelDirtyChanged();

    //
    // Store undo action
    //

    mUndoStack.Push(
        _("Properties"),
        256, // Arbitrary cost
        originalDirtyState,
        std::move(f));

    mUserInterface.OnUndoStackStateChanged();
}

void Controller::RestoreShipPropertiesForUndo(
    std::optional<ShipMetadata> && metadata,
    std::optional<ShipPhysicsData> && physicsData,
    std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings)
{
    assert(mModelController);

    InternalSetShipProperties(
        std::move(metadata),
        std::move(physicsData),
        std::move(autoTexturizationSettings));

    // At least one of the three was changed
    mUserInterface.OnModelDirtyChanged();
}

void Controller::ClearModelDirty()
{
    mModelController->ClearIsDirty();
    mUserInterface.OnModelDirtyChanged();
}

void Controller::RestoreDirtyState(Model::DirtyState && dirtyState)
{
    // Restore dirtyness
    mModelController->RestoreDirtyState(dirtyState);
    mUserInterface.OnModelDirtyChanged();
}

ModelValidationResults Controller::ValidateModel()
{
    auto const scopedToolResumeState = SuspendTool();

    assert(mModelController);
    assert(!mModelController->IsInEphemeralVisualization());

    return mModelController->ValidateModel();
}

std::optional<ShipSpaceRect> Controller::CalculateBoundingBox() const
{
    auto const scopedToolResumeState = SuspendTool();

    return mModelController->CalculateBoundingBox();
}

void Controller::NewStructuralLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::SetStructuralLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerRegionForUndo(
    StructuralLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreStructuralLayerRegion(
        std::move(layerRegion),
        { {0, 0}, layerRegion.Buffer.Size},
        origin);

    // No need to update dirtyness, this is for undo

    // Refresh model visualization
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

RgbaImageData const & Controller::GetStructuralLayerVisualization()
{
    auto const scopedToolResumeState = SuspendTool();

    return mModelController->GetStructuralLayerVisualization();
}

void Controller::NewElectricalLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualization
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RemoveElectricalLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerRegionForUndo(
    ElectricalLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreElectricalLayerRegion(
        std::move(layerRegion),
        { {0, 0}, layerRegion.Buffer.Size },
        origin);

    // No need to update dirtyness, this is for undo

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::TrimElectricalParticlesWithoutSubstratum()
{
    auto const scopedToolResumeState = SuspendTool();

    // Trim
    {
        // Save state
        auto originalDirtyStateClone = mModelController->GetModel().GetDirtyState();
        auto originalLayerClone = mModelController->GetModel().CloneExistingLayer<LayerType::Electrical>();

        // Trim
        auto const affectedRect = mModelController->TrimElectricalParticlesWithoutSubstratum();

        // Create undo action, if needed
        if (affectedRect)
        {
            //
            // Create undo action
            //

            ElectricalLayerData clippedRegionClone = originalLayerClone.Clone(*affectedRect);

            mUndoStack.Push(
                _("Trim Electrical"),
                clippedRegionClone.Buffer.GetByteSize(),
                originalDirtyStateClone,
                [clippedRegionClone = std::move(clippedRegionClone), origin = affectedRect->origin](Controller & controller) mutable
                {
                    controller.RestoreLayerRegionForUndo(std::move(clippedRegionClone), origin);
                });
            
            mUserInterface.OnUndoStackStateChanged();
        }
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::NewRopesLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::SetRopesLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RemoveRopesLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
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

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RestoreLayerForUndo(RopesLayerData && layer)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreRopesLayer(std::move(layer));

    // No need to update dirtyness, this is for undo

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::SetTextureLayer(
    TextureLayerData && textureLayer,
    std::optional<std::string> textureArtCredits)
{
    auto const scopedToolResumeState = SuspendTool();

    // Update layer
    {
        // Save state
        auto originalDirtyStateClone = mModelController->GetModel().GetDirtyState();
        auto originalLayerClone = mModelController->GetModel().CloneTextureLayer();
        auto originalTextureArtCredits = mModelController->GetShipMetadata().ArtCredits;

        // Update layer
        mModelController->SetTextureLayer(std::move(textureLayer), std::move(textureArtCredits));
        mUserInterface.OnLayerPresenceChanged();

        //
        // Create undo action
        //

        mUndoStack.Push(
            _("Import Texture Layer"),
            originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone), originalTextureArtCredits = std::move(originalTextureArtCredits)](Controller & controller) mutable
            {
                controller.RestoreTextureLayerForUndo(
                    std::move(originalLayerClone), 
                    std::move(originalTextureArtCredits));
            });

        mUserInterface.OnUndoStackStateChanged();
    }

    // Switch primary layer to this one
    if (mPrimaryLayer != LayerType::Texture)
    {
        InternalSelectPrimaryLayer(LayerType::Texture);
    }

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RemoveTextureLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // Update layer
    {
        // Save state
        auto originalDirtyStateClone = mModelController->GetModel().GetDirtyState();
        auto originalLayerClone = mModelController->CloneTextureLayer();
        auto originalTextureArtCredits = mModelController->GetShipMetadata().ArtCredits;

        // Remove layer
        mModelController->RemoveTextureLayer();
        mUserInterface.OnLayerPresenceChanged();

        //
        // Create undo action
        //

        mUndoStack.Push(
            _("Remove Texture Layer"),
            originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone), originalTextureArtCredits = std::move(originalTextureArtCredits)](Controller & controller) mutable
            {
                controller.RestoreTextureLayerForUndo(
                    std::move(originalLayerClone), 
                    std::move(originalTextureArtCredits));
            });

        mUserInterface.OnUndoStackStateChanged();
    }

    // Switch primary layer to structural if it was this one
    if (mPrimaryLayer == LayerType::Texture)
    {
        InternalSelectPrimaryLayer(LayerType::Structural);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged();

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::RestoreTextureLayerForUndo(
    std::unique_ptr<TextureLayerData> textureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreTextureLayer(
        std::move(textureLayer), 
        std::move(originalTextureArtCredits));

    mUserInterface.OnLayerPresenceChanged();

    // No need to update dirtyness, this is for undo

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
}

void Controller::Flip(DirectionType direction)
{
    Flip<false>(direction);
}

void Controller::FlipForUndo(DirectionType direction)
{
    Flip<true>(direction);
}

void Controller::ResizeShip(ShipSpaceSize const & newSize)
{
    // TODO: reset tool
    // TODO: tell ModelController
    // TODO: update layers visualization
    // TODO: update dirtyness (of all present layers)
    // TODO: undo

    // Notify view of new size
    mView.SetShipSize(newSize);
    mUserInterface.OnViewModelChanged();
    mUserInterface.RefreshView();

    // Notify UI of new ship size
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

StructuralLayerVisualizationModeType Controller::GetStructuralLayerVisualizationMode() const
{
    return mStructuralLayerVisualizationMode;
}

void Controller::SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode)
{
    mStructuralLayerVisualizationMode = mode;

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Notify
    mUserInterface.OnStructuralLayerVisualizationModeChanged(mode);

    // Refresh view
    mUserInterface.RefreshView();
}

ElectricalLayerVisualizationModeType Controller::GetElectricalLayerVisualizationMode() const
{
    return mElectricalLayerVisualizationMode;
}

void Controller::SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode)
{
    mElectricalLayerVisualizationMode = mode;

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Notify
    mUserInterface.OnElectricalLayerVisualizationModeChanged(mode);

    // Refresh view
    mUserInterface.RefreshView();
}

RopesLayerVisualizationModeType Controller::GetRopesLayerVisualizationMode() const
{
    return mRopesLayerVisualizationMode;
}

void Controller::SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode)
{
    mRopesLayerVisualizationMode = mode;

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Notify
    mUserInterface.OnRopesLayerVisualizationModeChanged(mode);

    // Refresh view
    mUserInterface.RefreshView();
}

TextureLayerVisualizationModeType Controller::GetTextureLayerVisualizationMode() const
{
    return mTextureLayerVisualizationMode;
}

void Controller::SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode)
{
    mTextureLayerVisualizationMode = mode;

    // Update layer visualization modes
    InternalUpdateVisualizationModes();

    // Notify
    mUserInterface.OnTextureLayerVisualizationModeChanged(mode);

    // Refresh view
    mUserInterface.RefreshView();
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

size_t Controller::GetUndoStackSize() const
{
    return mUndoStack.GetSize();
}

wxString const & Controller::GetUndoTitleAt(size_t index) const
{
    return mUndoStack.GetTitleAt(index);
}

void Controller::UndoLast()
{
    assert(CanUndo());

    auto const scopedToolResumeState = SuspendTool();

    // Apply action
    mUndoStack.PopAndApply(*this);

    // Update undo state
    mUserInterface.OnUndoStackStateChanged();
}

void Controller::UndoUntil(size_t index)
{
    assert(CanUndo());

    auto const scopedToolResumeState = SuspendTool();

    // Apply actions
    mUndoStack.RewindAndApply(index, *this);

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
    // Note: we don't tell view, as MainFrame is responsible for that

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    // Tell UI
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

void Controller::InternalSetShipProperties(
    std::optional<ShipMetadata> && metadata,
    std::optional<ShipPhysicsData> && physicsData,
    std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings)
{
    assert(mModelController);

    if (metadata.has_value())
    {
        bool const hasShipNameChanged =
            mModelController->GetShipMetadata().ShipName != metadata->ShipName;

        mModelController->SetShipMetadata(std::move(*metadata));

        if (hasShipNameChanged)
        {
            mUserInterface.OnShipNameChanged(mModelController->GetShipMetadata().ShipName);
        }
    }

    if (physicsData.has_value())
    {
        mModelController->SetShipPhysicsData(std::move(*physicsData));
    }

    if (autoTexturizationSettings.has_value())
    {
        mModelController->SetShipAutoTexturizationSettings(std::move(*autoTexturizationSettings));
    }
}

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

    // Tell view
    mView.SetPrimaryLayer(primaryLayer);
}

void Controller::InternalUpdateVisualizationModes()
{
    //
    // Here we orchestrate the viz mode that we want for the ModelController
    //

    // Structural

    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    mModelController->SetStructuralLayerVisualizationMode(mStructuralLayerVisualizationMode);

    // Electrical

    if (mModelController->GetModel().HasLayer(LayerType::Electrical))
    {
        mModelController->SetElectricalLayerVisualizationMode(mElectricalLayerVisualizationMode);
    }
    else
    {
        mModelController->SetElectricalLayerVisualizationMode(std::nullopt);
    }

    // Ropes

    if (mModelController->GetModel().HasLayer(LayerType::Ropes))
    {
        mModelController->SetRopesLayerVisualizationMode(mRopesLayerVisualizationMode);
    }
    else
    {
        mModelController->SetRopesLayerVisualizationMode(std::nullopt);
    }

    // Texture

    if (mModelController->GetModel().HasLayer(LayerType::Texture))
    {
        mModelController->SetTextureLayerVisualizationMode(mTextureLayerVisualizationMode);
    }
    else
    {
        mModelController->SetTextureLayerVisualizationMode(std::nullopt);
    }
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

Controller::ScopedToolResumeState Controller::SuspendTool() const
{
    return ScopedToolResumeState(
        *this,
        (const_cast<Controller *>(this))->InternalSuspendTool());
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

        case ToolType::ElectricalLine:
        {
            return std::make_unique<ElectricalLineTool>(
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

        case ToolType::StructuralLine:
        {
            return std::make_unique<StructuralLineTool>(
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

        case ToolType::RopeEraser:
        {
            return std::make_unique<RopeEraserTool>(
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

template<bool IsForUndo>
void Controller::Flip(DirectionType direction)
{
    auto const scopedToolResumeState = SuspendTool();

    if constexpr (!IsForUndo)
    {
        // Get dirty state
        Model::DirtyState const originalDirtyState = mModelController->GetModel().GetDirtyState();

        // Calculate undo title
        wxString undoTitle;
        if (direction == DirectionType::Horizontal)
            undoTitle = _("Flip H");
        else if (direction == DirectionType::Vertical)
            undoTitle = _("Flip V");
        else if (direction == (DirectionType::Horizontal | DirectionType::Vertical))
            undoTitle = _("Flip H+V");
        else
            assert(false);

        // Create undo
        mUndoStack.Push(
            undoTitle,
            1, // Arbitrary
            originalDirtyState,
            [direction](Controller & controller) mutable
            {
                controller.FlipForUndo(direction);
            });
        mUserInterface.OnUndoStackStateChanged();
    }

    // Flip
    mModelController->Flip(direction);

    if constexpr (!IsForUndo)
    {
        // Update dirtyness
        mModelController->SetAllLayersDirty();
        mUserInterface.OnModelDirtyChanged();
    }

    // Refresh model visualizations
    mModelController->UploadVisualizations();
    mUserInterface.RefreshView();
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