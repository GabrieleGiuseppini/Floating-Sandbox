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

#include <Game/ImageFileTools.h>

#include <cassert>

namespace ShipBuilder {

std::unique_ptr<Controller> Controller::CreateNew(
    std::string const & shipName,
    OpenGLManager & openGLManager,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ShipTexturizer const & shipTexturizer,
    ResourceLocator const & resourceLocator)
{
    auto modelController = ModelController::CreateNew(
        ShipSpaceSize(200, 100), // TODO: from preferences
        shipName,
        shipTexturizer);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            openGLManager,
            workbenchState,
            userInterface,
            resourceLocator));

    return controller;
}

std::unique_ptr<Controller> Controller::CreateForShip(
    ShipDefinition && shipDefinition,
    OpenGLManager & openGLManager,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ShipTexturizer const & shipTexturizer,
    ResourceLocator const & resourceLocator)
{
    auto modelController = ModelController::CreateForShip(
        std::move(shipDefinition),
        shipTexturizer);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            openGLManager,
            workbenchState,
            userInterface,
            resourceLocator));

    return controller;
}

Controller::Controller(
    std::unique_ptr<ModelController> modelController,
    OpenGLManager & openGLManager,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    ResourceLocator const & resourceLocator)
    : mView()
    , mModelController(std::move(modelController))
    , mUndoStack()
    , mWorkbenchState(workbenchState)
    , mUserInterface(userInterface)
    , mResourceLocator(resourceLocator)
    // State
    , mCurrentTool()
    , mLastToolTypePerLayer({ToolType::StructuralPencil, ToolType::ElectricalPencil, ToolType::RopePencil, std::nullopt})
{
    // We assume we start with at least a structural layer
    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    //
    // Create view
    //

    mView = std::make_unique<View>(
        mModelController->GetModel().GetShipSize(),
        mWorkbenchState.GetPrimaryVisualization(),
        mWorkbenchState.GetOtherVisualizationsOpacity(),
        mWorkbenchState.IsGridEnabled(),
        mUserInterface.GetDisplaySize(),
        mUserInterface.GetLogicalToPhysicalPixelFactor(),
        openGLManager,
        [this]()
        {
            mUserInterface.SwapRenderBuffers();
        },
        mResourceLocator);

    mView->UploadBackgroundTexture(
        ImageFileTools::LoadImageRgba(
            mResourceLocator.GetBitmapFilePath("shipbuilder_background")));

    // Set ideal zoom
    mView->SetZoom(mView->CalculateIdealZoom());

    //
    // Sync with UI
    //

    mUserInterface.OnViewModelChanged(mView->GetViewModel());
    mUserInterface.OnShipNameChanged(mModelController->GetModel());
    mUserInterface.OnShipSizeChanged(GetShipSize());
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());
    mUserInterface.OnUndoStackStateChanged(mUndoStack);
    
    //
    // Initialize visualization
    //

    // Initialize layer visualizations
    InternalReconciliateTextureVisualizationMode();
    InternalUpdateModelControllerVisualizationModes();

    // Upload layers' visualizations
    mModelController->UpdateVisualizations(*mView);

    // Refresh view
    mUserInterface.RefreshView();

    //
    // Create tool - iff mouse is (logically) in work canvas at this very moment
    //

    if (mUserInterface.IsMouseInWorkCanvas())
    {
        if (mWorkbenchState.GetCurrentToolType().has_value())
        {
            mCurrentTool = MakeTool(*mWorkbenchState.GetCurrentToolType());
        }
    }
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
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    //
    // Store undo action
    //

    mUndoStack.Push(
        _("Properties"),
        256, // Arbitrary cost
        originalDirtyState,
        std::move(f));

    mUserInterface.OnUndoStackStateChanged(mUndoStack);
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
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());
}

void Controller::ClearModelDirty()
{
    mModelController->ClearIsDirty();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());
}

void Controller::RestoreDirtyState(Model::DirtyState && dirtyState)
{
    // Restore dirtyness
    mModelController->RestoreDirtyState(dirtyState);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());
}

ModelValidationResults Controller::ValidateModel()
{
    auto const scopedToolResumeState = SuspendTool();

    assert(mModelController);
    assert(!mModelController->IsInEphemeralVisualization());

    return mModelController->ValidateModel();
}

std::unique_ptr<RgbaImageData> Controller::MakePreview() const
{
    auto const scopedToolResumeState = SuspendTool();

    return mModelController->MakePreview();
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
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz to this one
    if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::StructuralLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::StructuralLayer);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetStructuralLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
    // Update layer
    mModelController->SetStructuralLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz to this one
    if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::StructuralLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::StructuralLayer);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Structural);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreStructuralLayerRegionForUndo(
    StructuralLayerData && layerRegion,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    ShipSpaceRect const regionRect({ {0, 0}, layerRegion.Buffer.Size });

    mModelController->RestoreStructuralLayerRegion(
        std::move(layerRegion),
        regionRect,
        origin);

    // No need to update dirtyness, this is for undo

    // Refresh model visualization
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreStructuralLayerForUndo(StructuralLayerData && structuralLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreStructuralLayer(std::move(structuralLayer));

    // No need to update dirtyness, this is for undo

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::NewElectricalLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
    // Make new layer
    mModelController->NewElectricalLayer();
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz to this one
    if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::ElectricalLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::ElectricalLayer);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualization
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetElectricalLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
    // Update layer
    mModelController->SetElectricalLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary layer to this one
    if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::ElectricalLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::ElectricalLayer);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RemoveElectricalLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
    // Remove layer
    mModelController->RemoveElectricalLayer();
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz if it was this one
    if (mWorkbenchState.GetPrimaryVisualization() == VisualizationType::ElectricalLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::Game);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreElectricalLayerRegionForUndo(
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
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreElectricalLayerForUndo(std::unique_ptr<ElectricalLayerData> electricalLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreElectricalLayer(std::move(electricalLayer));

    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
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
        if (affectedRect.has_value())
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
                    controller.RestoreElectricalLayerRegionForUndo(std::move(clippedRegionClone), origin);
                });
            
            mUserInterface.OnUndoStackStateChanged(mUndoStack);
        }
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::NewRopesLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
    // Make new layer
    mModelController->NewRopesLayer();
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz to this one
    if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::RopesLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::RopesLayer);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetRopesLayer(/*TODO*/)
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: undo, copy from texture
    // Update layer
    mModelController->SetRopesLayer(/*TODO*/);
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz to this one
    if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::RopesLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::RopesLayer);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RemoveRopesLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    // TODO: do undo, copy from texture
    // Remove layer
    mModelController->RemoveRopesLayer();
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // Switch primary viz if it was this one
    if (mWorkbenchState.GetPrimaryVisualization() == VisualizationType::RopesLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::Game);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Ropes);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreRopesLayerForUndo(std::unique_ptr<RopesLayerData> ropesLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreRopesLayer(std::move(ropesLayer));

    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetTextureLayer(
    TextureLayerData && textureLayer,
    std::optional<std::string> textureArtCredits)
{
    auto const scopedToolResumeState = SuspendTool();

    // Update layer
    {
        // Get state snapshot
        auto originalDirtyStateClone = mModelController->GetModel().GetDirtyState();
        auto originalLayerClone = mModelController->GetModel().CloneTextureLayer();
        auto originalTextureArtCredits = mModelController->GetShipMetadata().ArtCredits;

        // Update layer
        mModelController->SetTextureLayer(std::move(textureLayer), std::move(textureArtCredits));
        mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

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

        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    // FUTUREWORK: disabled primary layer switch for this release, as there are no tools
    // and thus it's pointless
    ////// Switch primary viz to this one
    ////if (mPrimaryVisualization != VisualizationType::TextureLayer)
    ////{
    ////    InternalSelectPrimaryVisualization(VisualizationType::TextureLayer);
    ////}

    // Update visualization modes
    InternalReconciliateTextureVisualizationMode();
    InternalUpdateModelControllerVisualizationModes();

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
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
        mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

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

        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    // Switch primary viz if it was this one
    if (mWorkbenchState.GetPrimaryVisualization() == VisualizationType::TextureLayer)
    {
        InternalSelectPrimaryVisualization(VisualizationType::Game);
    }

    // Change texture visualization mode so that next time a texture
    // layer is present, we don't start in "none" mode
    if (mWorkbenchState.GetTextureLayerVisualizationMode() == TextureLayerVisualizationModeType::None)
    {
        mWorkbenchState.SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType::MatteMode); // New default for next layer
        mUserInterface.OnTextureLayerVisualizationModeChanged(TextureLayerVisualizationModeType::MatteMode);
    }

    // Update dirtyness
    mModelController->SetLayerDirty(LayerType::Texture);
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Update visualization modes
    InternalReconciliateTextureVisualizationMode();
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
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

    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalReconciliateTextureVisualizationMode();
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreAllLayersForUndo(
    ShipSpaceSize const & shipSize,
    StructuralLayerData && structuralLayer,
    std::unique_ptr<ElectricalLayerData> electricalLayer,
    std::unique_ptr<RopesLayerData> ropesLayer,
    std::unique_ptr<TextureLayerData> textureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    auto const scopedToolResumeState = SuspendTool();

    //
    // Model
    //

    mModelController->SetShipSize(shipSize);

    mModelController->RestoreStructuralLayer(std::move(structuralLayer));

    mModelController->RestoreElectricalLayer(std::move(electricalLayer));

    mModelController->RestoreRopesLayer(std::move(ropesLayer));

    mModelController->RestoreTextureLayer(
        std::move(textureLayer),
        std::move(originalTextureArtCredits));

    //
    // Finalize
    //

    // We (might) have changed the presence of layers
    mUserInterface.OnLayerPresenceChanged(mModelController->GetModel());

    // No need to update dirtyness, this is for undo

    // Notify view of (possibly) new size
    mView->SetShipSize(shipSize);
    mUserInterface.OnViewModelChanged(mView->GetViewModel());

    // Notify UI of (possibly) new ship size
    mUserInterface.OnShipSizeChanged(shipSize);

    // Update visualization modes
    InternalReconciliateTextureVisualizationMode();
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::AutoTrim()
{
    auto const scopedToolResumeState = SuspendTool();

    std::optional<ShipSpaceRect> const boundingRect = mModelController->CalculateBoundingBox();

    if (boundingRect.has_value())
    {
        InternalResizeShip(
            boundingRect->size,
            ShipSpaceCoordinates(
                -boundingRect->origin.x,
                -boundingRect->origin.y),
            _("Trim"));
    }
}

void Controller::Flip(DirectionType direction)
{
    auto const scopedToolResumeState = SuspendTool();

    InternalFlip<false>(direction);
}

void Controller::FlipForUndo(DirectionType direction)
{
    auto const scopedToolResumeState = SuspendTool();

    InternalFlip<true>(direction);
}

void Controller::ResizeShip(
    ShipSpaceSize const & newSize,
    ShipSpaceCoordinates const & originOffset)
{
    auto const scopedToolResumeState = SuspendTool();

    InternalResizeShip(
        newSize,
        originOffset,
        _("Resize Ship"));
}

void Controller::SelectPrimaryVisualization(VisualizationType primaryVisualization)
{
    if (primaryVisualization != mWorkbenchState.GetPrimaryVisualization())
    {
        {
            auto const scopedToolResumeState = SuspendTool();

            InternalSelectPrimaryVisualization(primaryVisualization);
        }

        // Refresh view
        mUserInterface.RefreshView();
    }
}

void Controller::SetGameVisualizationMode(GameVisualizationModeType mode)
{
    mWorkbenchState.SetGameVisualizationMode(mode);

    // Notify
    mUserInterface.OnGameVisualizationModeChanged(mode);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode)
{
    mWorkbenchState.SetStructuralLayerVisualizationMode(mode);

    // Notify
    mUserInterface.OnStructuralLayerVisualizationModeChanged(mode);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode)
{
    mWorkbenchState.SetElectricalLayerVisualizationMode(mode);

    // Notify
    mUserInterface.OnElectricalLayerVisualizationModeChanged(mode);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode)
{
    mWorkbenchState.SetRopesLayerVisualizationMode(mode);

    // Notify
    mUserInterface.OnRopesLayerVisualizationModeChanged(mode);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode)
{
    mWorkbenchState.SetTextureLayerVisualizationMode(mode);

    // Notify
    mUserInterface.OnTextureLayerVisualizationModeChanged(mode);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetOtherVisualizationsOpacity(float opacity)
{
    mWorkbenchState.SetOtherVisualizationsOpacity(opacity);

    // Notify
    mUserInterface.OnOtherVisualizationsOpacityChanged(opacity);

    // Update view
    mView->SetOtherVisualizationsOpacity(opacity);
    mUserInterface.RefreshView();
}

void Controller::EnableVisualGrid(bool doEnable)
{
    mWorkbenchState.EnableGrid(doEnable);

    // Notify
    mUserInterface.OnVisualGridEnablementChanged(doEnable);

    // Update view
    mView->EnableVisualGrid(doEnable);
    mUserInterface.RefreshView();
}

void Controller::SetCurrentTool(std::optional<ToolType> tool)
{
    if (tool != mWorkbenchState.GetCurrentToolType())
    {
        bool const hadTool = (mCurrentTool != nullptr);

        // Nuke current tool
        mWorkbenchState.SetCurrentToolType(std::nullopt);
        mCurrentTool.reset();

        InternalSetCurrentTool(tool);

        // Make new tool - unless we are suspended
        assert(mWorkbenchState.GetCurrentToolType() == tool);
        if (mWorkbenchState.GetCurrentToolType().has_value()
            && hadTool)
        {
            mCurrentTool = MakeTool(*mWorkbenchState.GetCurrentToolType());
        }
    }
}

void Controller::SetStructuralMaterial(MaterialPlaneType plane, StructuralMaterial const * material)
{
    switch (plane)
    {
        case MaterialPlaneType::Foreground:
        {
            mWorkbenchState.SetStructuralForegroundMaterial(material);
            break;
        }

        case MaterialPlaneType::Background:
        {
            mWorkbenchState.SetStructuralBackgroundMaterial(material);
            break;
        }
    }

    mUserInterface.OnStructuralMaterialChanged(plane, material);
}

void Controller::SetElectricalMaterial(MaterialPlaneType plane, ElectricalMaterial const * material)
{
    switch (plane)
    {
        case MaterialPlaneType::Foreground:
        {
            mWorkbenchState.SetElectricalForegroundMaterial(material);
            break;
        }

        case MaterialPlaneType::Background:
        {
            mWorkbenchState.SetElectricalBackgroundMaterial(material);
            break;
        }
    }

    mUserInterface.OnElectricalMaterialChanged(plane, material);
}

void Controller::SetRopeMaterial(MaterialPlaneType plane, StructuralMaterial const * material)
{
    switch (plane)
    {
        case MaterialPlaneType::Foreground:
        {
            mWorkbenchState.SetRopesForegroundMaterial(material);
            break;
        }

        case MaterialPlaneType::Background:
        {
            mWorkbenchState.SetRopesBackgroundMaterial(material);
            break;
        }
    }

    mUserInterface.OnRopesMaterialChanged(plane, material);
}

void Controller::UndoLast()
{
    auto const scopedToolResumeState = SuspendTool();

    // Apply action
    mUndoStack.PopAndApply(*this);

    // Update undo state
    mUserInterface.OnUndoStackStateChanged(mUndoStack);
}

void Controller::UndoUntil(size_t index)
{
    auto const scopedToolResumeState = SuspendTool();

    // Apply actions
    mUndoStack.RewindAndApply(index, *this);

    // Update undo state
    mUserInterface.OnUndoStackStateChanged(mUndoStack);
}

void Controller::Render()
{
    mView->Render();
}

void Controller::AddZoom(int deltaZoom)
{
    mView->SetZoom(mView->GetZoom() + deltaZoom);

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged(mView->GetViewModel());
    mUserInterface.RefreshView();
}

void Controller::SetCamera(int camX, int camY)
{
    mView->SetCameraShipSpacePosition(ShipSpaceCoordinates(camX, camY));

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged(mView->GetViewModel());
    mUserInterface.RefreshView();
}

void Controller::ResetView()
{
    mView->SetZoom(0);
    mView->SetCameraShipSpacePosition(ShipSpaceCoordinates(0, 0));

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    RefreshToolCoordinatesDisplay();
    mUserInterface.OnViewModelChanged(mView->GetViewModel());
    mUserInterface.RefreshView();
}

void Controller::OnWorkCanvasResized(DisplayLogicalSize const & newSize)
{
    // Tell view
    mView->SetDisplayLogicalSize(newSize);

    // Tell tool about the new mouse (ship space) position, but only
    // if the mouse is in the canvas
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates
        && mCurrentTool)
    {
        mCurrentTool->OnMouseMove(*mouseCoordinates);
    }

    // Tell UI
    mUserInterface.OnViewModelChanged(mView->GetViewModel());
}

void Controller::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
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
            mUserInterface.OnShipNameChanged(mModelController->GetModel());
        }
    }

    if (physicsData.has_value())
    {
        mModelController->SetShipPhysicsData(std::move(*physicsData));
    }

    if (autoTexturizationSettings.has_value())
    {
        mModelController->SetShipAutoTexturizationSettings(std::move(*autoTexturizationSettings));

        if (mWorkbenchState.GetGameVisualizationMode() == GameVisualizationModeType::AutoTexturizationMode)
        {
            // Redo game viz
            mModelController->ForceWholeGameVisualizationRefresh();

            // Refresh model visualizations
            mModelController->UpdateVisualizations(*mView);
            mUserInterface.RefreshView();
        }
    }
}

void Controller::InternalSelectPrimaryVisualization(VisualizationType primaryVisualization)
{
    //
    // No tool destroy/create
    // No visualization changes
    //

    assert(!mCurrentTool);
    assert(mWorkbenchState.GetPrimaryVisualization() != primaryVisualization);

    // Store new primary visualization
    mWorkbenchState.SetPrimaryVisualization(primaryVisualization);

    // Notify
    mUserInterface.OnPrimaryVisualizationChanged(primaryVisualization);

    // Select the last tool we have used for this new viz's layer
    // Note: stores new tool for new layer
    InternalSetCurrentTool(mLastToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(primaryVisualization))]);

    // Tell view
    mView->SetPrimaryVisualization(primaryVisualization);
}

void Controller::InternalReconciliateTextureVisualizationMode()
{
    if (!mModelController->GetModel().HasLayer(LayerType::Texture))
    {
        // If game visualization mode is the one only allowed with texture, 
        // change it to auto-texturization
        if (mWorkbenchState.GetGameVisualizationMode() == GameVisualizationModeType::TextureMode)
        {
            mWorkbenchState.SetGameVisualizationMode(GameVisualizationModeType::AutoTexturizationMode);
            mUserInterface.OnGameVisualizationModeChanged(GameVisualizationModeType::AutoTexturizationMode);
        }
    }
    else
    {
        // If game visualization mode is the one only allowed without texture, 
        // change it to texture
        if (mWorkbenchState.GetGameVisualizationMode() == GameVisualizationModeType::AutoTexturizationMode)
        {
            mWorkbenchState.SetGameVisualizationMode(GameVisualizationModeType::TextureMode);
            mUserInterface.OnGameVisualizationModeChanged(GameVisualizationModeType::TextureMode);
        }
    }
}

void Controller::InternalUpdateModelControllerVisualizationModes()
{
    //
    // Here we orchestrate the viz mode that we want for the ModelController
    //

    // Game

    mModelController->SetGameVisualizationMode(mWorkbenchState.GetGameVisualizationMode());

    // Structural

    assert(mModelController->GetModel().HasLayer(LayerType::Structural));

    mModelController->SetStructuralLayerVisualizationMode(mWorkbenchState.GetStructuralLayerVisualizationMode());

    // Electrical

    if (mModelController->GetModel().HasLayer(LayerType::Electrical))
    {
        mModelController->SetElectricalLayerVisualizationMode(mWorkbenchState.GetElectricalLayerVisualizationMode());
    }
    else
    {
        mModelController->SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::None);
    }

    // Ropes

    if (mModelController->GetModel().HasLayer(LayerType::Ropes))
    {
        mModelController->SetRopesLayerVisualizationMode(mWorkbenchState.GetRopesLayerVisualizationMode());
    }
    else
    {
        mModelController->SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType::None);
    }

    // Texture

    if (mModelController->GetModel().HasLayer(LayerType::Texture))
    {
        mModelController->SetTextureLayerVisualizationMode(mWorkbenchState.GetTextureLayerVisualizationMode());
    }
    else
    {
        mModelController->SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType::None);
    }
}

void Controller::InternalSetCurrentTool(std::optional<ToolType> toolType)
{
    //
    // No tool destroy/create
    // No visualization changes
    //

    assert(!mCurrentTool);

    // Set current tool
    mWorkbenchState.SetCurrentToolType(toolType);

    // Notify
    mUserInterface.OnCurrentToolChanged(toolType);

    if (!toolType.has_value())
    {
        // Relinquish cursor
        mUserInterface.ResetToolCursor();
    }

    // Remember new tool as the last tool of this primary visualization's layer
    mLastToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))] = toolType;
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
    mCurrentTool.reset();

    if (mWorkbenchState.GetCurrentToolType().has_value())
    {
        mCurrentTool = MakeTool(*mWorkbenchState.GetCurrentToolType());
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
                *mView,
                mResourceLocator);
        }

        case ToolType::ElectricalLine:
        {
            return std::make_unique<ElectricalLineTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::ElectricalPencil:
        {
            return std::make_unique<ElectricalPencilTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::StructuralEraser:
        {
            return std::make_unique<StructuralEraserTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::StructuralFlood:
        {
            return std::make_unique<StructuralFloodTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::StructuralLine:
        {
            return std::make_unique<StructuralLineTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::StructuralPencil:
        {
            return std::make_unique<StructuralPencilTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::RopePencil:
        {
            return std::make_unique<RopePencilTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }

        case ToolType::RopeEraser:
        {
            return std::make_unique<RopeEraserTool>(
                *mModelController,
                mUndoStack,
                mWorkbenchState,
                mUserInterface,
                *mView,
                mResourceLocator);
        }
    }

    assert(false);
    return nullptr;
}

void Controller::InternalResizeShip(
    ShipSpaceSize const & newSize,
    ShipSpaceCoordinates const & originOffset,
    wxString const & actionName)
{
    // Store undo
    {
        // Get dirty state
        Model::DirtyState const originalDirtyState = mModelController->GetModel().GetDirtyState();

        // Clone all layers
        auto structuralLayerClone = mModelController->CloneStructuralLayer();
        auto electricalLayerClone = mModelController->CloneElectricalLayer();
        auto ropesLayerClone = mModelController->CloneRopesLayer();
        auto textureLayerClone = mModelController->CloneTextureLayer();
        auto textureArtCreditsClone = mModelController->GetShipMetadata().ArtCredits;

        // Calculate cost
        size_t const totalCost =
            structuralLayerClone.Buffer.GetByteSize()
            + (electricalLayerClone ? electricalLayerClone->Buffer.GetByteSize() : 0)
            + (ropesLayerClone ? ropesLayerClone->Buffer.GetSize() * sizeof(RopeElement) : 0)
            + (textureLayerClone ? textureLayerClone->Buffer.GetByteSize() : 0);

        // Create undo

        mUndoStack.Push(
            actionName,
            totalCost,
            originalDirtyState,
            [shipSize = mModelController->GetModel().GetShipSize()
            , structuralLayerClone = std::move(structuralLayerClone)
            , electricalLayerClone = std::move(electricalLayerClone)
            , ropesLayerClone = std::move(ropesLayerClone)
            , textureLayerClone = std::move(textureLayerClone)
            , textureArtCreditsClone = std::move(textureArtCreditsClone)](Controller & controller) mutable
            {
                controller.RestoreAllLayersForUndo(
                    shipSize,
                    std::move(structuralLayerClone),
                    std::move(electricalLayerClone),
                    std::move(ropesLayerClone),
                    std::move(textureLayerClone),
                    std::move(textureArtCreditsClone));
            });

        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    // Resize
    mModelController->ResizeShip(newSize, originOffset);

    // Update dirtyness
    mModelController->SetAllPresentLayersDirty();
    mUserInterface.OnModelDirtyChanged(mModelController->GetModel());

    // Notify view of new size
    mView->SetShipSize(newSize);
    mUserInterface.OnViewModelChanged(mView->GetViewModel());

    // Notify UI of new ship size
    mUserInterface.OnShipSizeChanged(newSize);

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

template<bool IsForUndo>
void Controller::InternalFlip(DirectionType direction)
{
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

        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    // Flip
    mModelController->Flip(direction);

    if constexpr (!IsForUndo)
    {
        // Update dirtyness
        mModelController->SetAllLayersDirty();
        mUserInterface.OnModelDirtyChanged(mModelController->GetModel());
    }

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RefreshToolCoordinatesDisplay()
{
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mView->ScreenToShipSpace(mUserInterface.GetMouseCoordinates());

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