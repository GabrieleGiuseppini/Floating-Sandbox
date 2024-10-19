/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

#include "Tools/FloodTool.h"
#include "Tools/LineTool.h"
#include "Tools/MeasuringTapeTool.h"
#include "Tools/PasteTool.h"
#include "Tools/PencilTool.h"
#include "Tools/RopeEraserTool.h"
#include "Tools/RopePencilTool.h"
#include "Tools/SamplerTool.h"
#include "Tools/SelectionTool.h"
#include "Tools/StructuralRectangleTool.h"
#include "Tools/TextureEraserTool.h"
#include "Tools/TextureMagicWandTool.h"

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
        workbenchState.GetNewShipSize(),
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
    , mSelectionManager(userInterface)
    , mWorkbenchState(workbenchState)
    , mUserInterface(userInterface)
    , mResourceLocator(resourceLocator)
    // State
    , mCurrentTool()
    , mCurrentToolTypePerLayer({ToolType::StructuralPencil, ToolType::ElectricalPencil, ToolType::RopePencil, ToolType::ExteriorTextureEraser, ToolType::InteriorTextureEraser})
{
    //
    // Create view
    //

    mView = std::make_unique<View>(
        mModelController->GetShipSize(),
        mWorkbenchState.GetCanvasBackgroundColor(),
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
    mUserInterface.OnShipNameChanged(*mModelController);
    mUserInterface.OnShipScaleChanged(mModelController->GetShipMetadata().Scale);
    mUserInterface.OnShipSizeChanged(mModelController->GetShipSize());
    mUserInterface.OnLayerPresenceChanged(*mModelController);
    mUserInterface.OnModelDirtyChanged(*mModelController);
    mUserInterface.OnElectricalLayerInstancedElementSetChanged(mModelController->GetInstancedElectricalElementSet());
    mUserInterface.OnUndoStackStateChanged(mUndoStack);
    mUserInterface.OnSelectionChanged(mSelectionManager.GetSelection());

    //
    // Initialize visualization
    //

    // Switch primary viz to default if it's not compatible with current layer presence
    if (!mModelController->HasLayer(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization())))
    {
        InternalSelectPrimaryVisualization(WorkbenchState::GetDefaultPrimaryVisualization()); // Will also change tool
    }

    // Initialize layer visualizations
    InternalReconciliateExteriorTextureVisualizationMode();
    InternalUpdateModelControllerVisualizationModes();

    // Upload layers' visualizations
    mModelController->UpdateVisualizations(*mView);

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh view
    mUserInterface.RefreshView();

    //
    // Set tool to tool for current visualization
    //

    InternalSetCurrentTool(GetToolTypeForCurrentVisualization(), false);

    RefreshToolCoordinatesDisplay();
}

ShipDefinition Controller::MakeShipDefinition() const
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

    auto originalDirtyState = mModelController->GetDirtyState();

    //
    // Set new properties
    //

    InternalSetShipProperties(
        std::move(metadata),
        std::move(physicsData),
        std::move(autoTexturizationSettings));

    // At least one of the three was changed
    mUserInterface.OnModelDirtyChanged(*mModelController);

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
    mUserInterface.OnModelDirtyChanged(*mModelController);
}

void Controller::SetElectricalPanel(ElectricalPanel electricalPanel)
{
    assert(mModelController);

    //
    // Prepare undo entry
    //

    auto f = [oldElectricalPanel = mModelController->GetElectricalPanel()] (Controller & controller) mutable
    {
        controller.RestoreElectricalPanelForUndo(std::move(oldElectricalPanel));
    };

    auto originalDirtyState = mModelController->GetDirtyState();

    //
    // Set new panel
    //

    InternalSetElectricalPanel(std::move(electricalPanel));

    mModelController->SetLayerDirty(LayerType::Electrical);
    mUserInterface.OnModelDirtyChanged(*mModelController);

    //
    // Store undo action
    //

    mUndoStack.Push(
        _("Electrical Panel"),
        256, // Arbitrary cost
        originalDirtyState,
        std::move(f));

    mUserInterface.OnUndoStackStateChanged(mUndoStack);
}

void Controller::RestoreElectricalPanelForUndo(ElectricalPanel && electricalPanel)
{
    assert(mModelController);

    InternalSetElectricalPanel(std::move(electricalPanel));

    mUserInterface.OnModelDirtyChanged(*mModelController);
}

void Controller::ClearModelDirty()
{
    mModelController->ClearIsDirty();
    mUserInterface.OnModelDirtyChanged(*mModelController);
}

void Controller::RestoreDirtyState(ModelDirtyState && dirtyState)
{
    // Restore dirtyness
    mModelController->RestoreDirtyState(dirtyState);
    mUserInterface.OnModelDirtyChanged(*mModelController);
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

ModelValidationSession Controller::StartValidation() const
{
    auto scopedToolResumeState = SuspendTool();

    assert(mModelController);
    assert(!mModelController->IsInEphemeralVisualization());

    return mModelController->StartValidation(std::move(scopedToolResumeState));
}

void Controller::NewStructuralLayer()
{
    StructuralLayerData newStructuralLayer(
        mModelController->GetShipSize(),
        StructuralElement(nullptr)); // No material

    InternalSetLayer<LayerType::Structural>(
        _("New Structural Layer"),
        std::move(newStructuralLayer));
}

void Controller::SetStructuralLayer(
    wxString actionTitle,
    StructuralLayerData && structuralLayer)
{
    InternalSetLayer<LayerType::Structural>(
        actionTitle,
        std::move(structuralLayer));
}

void Controller::RestoreStructuralLayerRegionBackupForUndo(
    StructuralLayerData && layerRegionBackup,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreStructuralLayerRegionBackup(
        std::move(layerRegionBackup),
        origin);

    // No need to update dirtyness, this is for undo

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualization
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreStructuralLayerForUndo(std::unique_ptr<StructuralLayerData> structuralLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    WrapLikelyLayerPresenceChangingOperation<LayerType::Structural>(
        [this, structuralLayer = std::move(structuralLayer)]() mutable
        {
            mModelController->RestoreStructuralLayer(std::move(structuralLayer));
        });

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::NewElectricalLayer()
{
    ElectricalLayerData newElectricalLayer(
        mModelController->GetShipSize(),
        ElectricalElement(nullptr, NoneElectricalElementInstanceIndex)); // No material

    InternalSetLayer<LayerType::Electrical>(
        _("New Electrical Layer"),
        std::move(newElectricalLayer));
}

void Controller::SetElectricalLayer(
    wxString actionTitle,
    ElectricalLayerData && electricalLayer)
{
    InternalSetLayer<LayerType::Electrical>(
        actionTitle,
        std::move(electricalLayer));
}

void Controller::RemoveElectricalLayer()
{
    InternalRemoveLayer<LayerType::Electrical>();
}

void Controller::RestoreElectricalLayerRegionBackupForUndo(
    ElectricalLayerData && layerRegionBackup,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreElectricalLayerRegionBackup(
        std::move(layerRegionBackup),
        origin);

    // No need to update dirtyness, this is for undo

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Notify of (possible) change in electrical panel
    mUserInterface.OnElectricalLayerInstancedElementSetChanged(mModelController->GetInstancedElectricalElementSet());

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreElectricalLayerForUndo(std::unique_ptr<ElectricalLayerData> electricalLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    WrapLikelyLayerPresenceChangingOperation<LayerType::Electrical>(
        [this, electricalLayer = std::move(electricalLayer)]() mutable
        {
            mModelController->RestoreElectricalLayer(std::move(electricalLayer));
        });

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Notify of (possible) change in electrical panel
    mUserInterface.OnElectricalLayerInstancedElementSetChanged(mModelController->GetInstancedElectricalElementSet());

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
        auto originalDirtyStateClone = mModelController->GetDirtyState();
        auto originalLayerClone = mModelController->CloneExistingLayer<LayerType::Electrical>();

        // Trim
        auto const affectedRect = mModelController->TrimElectricalParticlesWithoutSubstratum();

        if (affectedRect.has_value())
        {
            // Create undo action

            ElectricalLayerData clippedRegionBackup = originalLayerClone.MakeRegionBackup(*affectedRect);
            auto const clipByteSize = clippedRegionBackup.Buffer.GetByteSize();

            mUndoStack.Push(
                _("Trim Electrical"),
                clipByteSize,
                originalDirtyStateClone,
                [clippedRegionBackup = std::move(clippedRegionBackup), origin = affectedRect->origin](Controller & controller) mutable
                {
                    controller.RestoreElectricalLayerRegionBackupForUndo(std::move(clippedRegionBackup), origin);
                });

            mUserInterface.OnUndoStackStateChanged(mUndoStack);

            LayerChangeEpilog({ LayerType::Electrical });
        }
    }
}

void Controller::NewRopesLayer()
{
    RopesLayerData newRopesLayer(mModelController->GetShipSize());

    InternalSetLayer<LayerType::Ropes>(
        _("New Ropes Layer"),
        std::move(newRopesLayer));
}

void Controller::SetRopesLayer(
    wxString actionTitle,
    RopesLayerData && ropesLayer)
{
    InternalSetLayer<LayerType::Ropes>(
        actionTitle,
        std::move(ropesLayer));
}

void Controller::RemoveRopesLayer()
{
    InternalRemoveLayer<LayerType::Ropes>();
}

void Controller::RestoreRopesLayerRegionBackupForUndo(
    RopesLayerData && layerRegionBackup,
    ShipSpaceCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreRopesLayerRegionBackup(
        std::move(layerRegionBackup),
        origin);

    // No need to update dirtyness, this is for undo

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();

}

void Controller::RestoreRopesLayerForUndo(std::unique_ptr<RopesLayerData> ropesLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    WrapLikelyLayerPresenceChangingOperation<LayerType::Ropes>(
        [this, ropesLayer = std::move(ropesLayer)]() mutable
        {
            mModelController->RestoreRopesLayer(std::move(ropesLayer));
        });

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetExteriorTextureLayer(
    wxString actionTitle,
    TextureLayerData && exteriorTextureLayer,
    std::optional<std::string> textureArtCredits)
{
    InternalSetLayer<LayerType::ExteriorTexture>(
        actionTitle,
        std::move(exteriorTextureLayer),
        std::move(textureArtCredits));
}

void Controller::RemoveExteriorTextureLayer()
{
    InternalRemoveLayer<LayerType::ExteriorTexture>();
}

void Controller::RestoreExteriorTextureLayerRegionBackupForUndo(
    TextureLayerData && layerRegionBackup,
    ImageCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreExteriorTextureLayerRegionBackup(
        std::move(layerRegionBackup),
        origin);

    // No need to update dirtyness, this is for undo

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreExteriorTextureLayerForUndo(
    std::unique_ptr<TextureLayerData> exteriorTextureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    auto const scopedToolResumeState = SuspendTool();

    WrapLikelyLayerPresenceChangingOperation<LayerType::ExteriorTexture>(
        [this, exteriorTextureLayer = std::move(exteriorTextureLayer), originalTextureArtCredits = std::move(originalTextureArtCredits)]() mutable
        {
            mModelController->RestoreExteriorTextureLayer(
                std::move(exteriorTextureLayer),
                std::move(originalTextureArtCredits));
        });

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetInteriorTextureLayer(
    wxString actionTitle,
    TextureLayerData && interiorTextureLayer)
{
    InternalSetLayer<LayerType::InteriorTexture>(
        actionTitle,
        std::move(interiorTextureLayer));
}

void Controller::RemoveInteriorTextureLayer()
{
    InternalRemoveLayer<LayerType::InteriorTexture>();
}

void Controller::RestoreInteriorTextureLayerRegionBackupForUndo(
    TextureLayerData && layerRegionBackup,
    ImageCoordinates const & origin)
{
    auto const scopedToolResumeState = SuspendTool();

    mModelController->RestoreInteriorTextureLayerRegionBackup(
        std::move(layerRegionBackup),
        origin);

    // No need to update dirtyness, this is for undo

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreInteriorTextureLayerForUndo(std::unique_ptr<TextureLayerData> interiorTextureLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    WrapLikelyLayerPresenceChangingOperation<LayerType::InteriorTexture>(
        [this, interiorTextureLayer = std::move(interiorTextureLayer)]() mutable
        {
            mModelController->RestoreInteriorTextureLayer(std::move(interiorTextureLayer));
        });

    // No need to update dirtyness, this is for undo

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::RestoreAllLayersForUndo(
    ShipSpaceSize const & shipSize,
    std::unique_ptr<StructuralLayerData> structuralLayer,
    std::unique_ptr<ElectricalLayerData> electricalLayer,
    std::unique_ptr<RopesLayerData> ropesLayer,
    std::unique_ptr<TextureLayerData> exteriorTextureLayer,
    std::optional<std::string> originalTextureArtCredits,
    std::unique_ptr<TextureLayerData> interiorTextureLayer)
{
    auto const scopedToolResumeState = SuspendTool();

    //
    // Model
    //

    mModelController->SetShipSize(shipSize);

    WrapLikelyLayerPresenceChangingOperation<LayerType::Structural>(
        [this, structuralLayer = std::move(structuralLayer)]() mutable
        {
            mModelController->RestoreStructuralLayer(std::move(structuralLayer));
        });

    WrapLikelyLayerPresenceChangingOperation<LayerType::Electrical>(
        [this, electricalLayer = std::move(electricalLayer)]() mutable
        {
            mModelController->RestoreElectricalLayer(std::move(electricalLayer));
        });

    WrapLikelyLayerPresenceChangingOperation<LayerType::Ropes>(
        [this, ropesLayer = std::move(ropesLayer)]() mutable
        {
            mModelController->RestoreRopesLayer(std::move(ropesLayer));
        });

    WrapLikelyLayerPresenceChangingOperation<LayerType::ExteriorTexture>(
        [this, exteriorTextureLayer = std::move(exteriorTextureLayer), originalTextureArtCredits = std::move(originalTextureArtCredits)]() mutable
        {
            mModelController->RestoreExteriorTextureLayer(
                std::move(exteriorTextureLayer),
                std::move(originalTextureArtCredits));
        });

    WrapLikelyLayerPresenceChangingOperation<LayerType::InteriorTexture>(
        [this, interiorTextureLayer = std::move(interiorTextureLayer)]() mutable
        {
            mModelController->RestoreInteriorTextureLayer(std::move(interiorTextureLayer));
        });

    //
    // Finalize
    //

    // No need to update dirtyness, this is for undo

    // Notify view of (possibly) new size
    mView->SetShipSize(shipSize);
    mUserInterface.OnViewModelChanged(mView->GetViewModel());

    // Notify UI of (possibly) new ship size
    mUserInterface.OnShipSizeChanged(shipSize);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::Restore(GenericUndoPayload && undoPayload)
{
    // No layer-presence changing operations
    mModelController->Restore(std::move(undoPayload));

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::Copy() const
{
    // Note: no need to suspend tool, as Selection tool has no eph viz
    assert(!mCurrentTool || mCurrentTool->GetClass() == ToolClass::Selection);

    assert(mSelectionManager.GetSelection().has_value());
    auto const selectionRegion = *(mSelectionManager.GetSelection());

    std::optional<LayerType> const layerSelection = mWorkbenchState.GetSelectionIsAllLayers()
        ? std::nullopt // All layers
        : std::optional<LayerType>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization())); // Currently-selected layer

    InternalCopySelectionToClipboard(selectionRegion, layerSelection);
}

void Controller::Cut()
{
    assert(mSelectionManager.GetSelection().has_value());
    auto const selectionRegion = *(mSelectionManager.GetSelection()); // Get selection before we remove tool

    auto const scopedToolResumeState = SuspendTool();

    std::optional<LayerType> const layerSelection = mWorkbenchState.GetSelectionIsAllLayers()
        ? std::nullopt // All layers
        : std::optional<LayerType>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization())); // Currently-selected layer

    // Copy to clipboard
    InternalCopySelectionToClipboard(selectionRegion, layerSelection);

    // Erase region
    GenericUndoPayload undoPayload = mModelController->EraseRegion(selectionRegion, layerSelection);

    // Store Undo
    size_t const undoPayloadCost = undoPayload.GetTotalCost();
    mUndoStack.Push(
        _("Cut"),
        undoPayloadCost,
        mModelController->GetDirtyState(),
        [undoPayload = std::move(undoPayload)](Controller & controller) mutable
        {
            controller.Restore(std::move(undoPayload));
        });

    mUserInterface.OnUndoStackStateChanged(mUndoStack);

    LayerChangeEpilog(mModelController->CalculateAffectedLayers(layerSelection));
}

void Controller::Paste()
{
    //
    // Clone clipboard
    //

    assert(!mWorkbenchState.GetClipboardManager().IsEmpty());

    ShipLayers clipboardClone = mWorkbenchState.GetClipboardManager().GetContent()->Clone();

    //
    // Nuke current tool
    //

    mCurrentTool.reset();

    //
    // Decide which of the layer variants to choose:
    //  - First layer present in clipboard
    //  - If clipboard has layer for current viz, wins
    //

    std::optional<LayerType> bestLayer;

    LayerType currentVizLayer = VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization());

    if (clipboardClone.StructuralLayer)
    {
        if (!bestLayer || currentVizLayer == LayerType::Structural)
        {
            bestLayer = LayerType::Structural;
        }
    }

    if (clipboardClone.ElectricalLayer)
    {
        if (!bestLayer || currentVizLayer == LayerType::Electrical)
        {
            bestLayer = LayerType::Electrical;
        }
    }

    if (clipboardClone.RopesLayer)
    {
        if (!bestLayer || currentVizLayer == LayerType::Ropes)
        {
            bestLayer = LayerType::Ropes;
        }
    }

    if (clipboardClone.ExteriorTextureLayer)
    {
        if (!bestLayer || currentVizLayer == LayerType::ExteriorTexture)
        {
            bestLayer = LayerType::ExteriorTexture;
        }
    }

    if (clipboardClone.InteriorTextureLayer)
    {
        if (!bestLayer || currentVizLayer == LayerType::InteriorTexture)
        {
            bestLayer = LayerType::InteriorTexture;
        }
    }

    assert(bestLayer);

    //
    // If chosen layer is not current viz's, change viz - WITHOUT setting tool
    //

    if (*bestLayer != currentVizLayer)
    {
        switch (*bestLayer)
        {
            case LayerType::Structural:
            {
                InternalSelectPrimaryVisualization(VisualizationType::Game); // Arbitrary
                break;
            }

            case LayerType::Electrical:
            {
                InternalSelectPrimaryVisualization(VisualizationType::ElectricalLayer);
                break;
            }

            case LayerType::Ropes:
            {
                InternalSelectPrimaryVisualization(VisualizationType::RopesLayer);
                break;
            }

            case LayerType::ExteriorTexture:
            {
                InternalSelectPrimaryVisualization(VisualizationType::ExteriorTextureLayer);
                break;
            }

            case LayerType::InteriorTexture:
            {
                InternalSelectPrimaryVisualization(VisualizationType::InteriorTextureLayer);
                break;
            }
        }
    }

    //
    // Instantiate and set tool, making sure it does not become "the" tool
    // for the current viz mode
    //

    switch (*bestLayer)
    {
        case LayerType::Structural:
        {
            mCurrentTool = std::make_unique<StructuralPasteTool>(
                std::move(clipboardClone),
                mWorkbenchState.GetPasteIsTransparent(),
                *this,
                mResourceLocator);

            break;
        }

        case LayerType::Electrical:
        {
            mCurrentTool = std::make_unique<ElectricalPasteTool>(
                std::move(clipboardClone),
                mWorkbenchState.GetPasteIsTransparent(),
                *this,
                mResourceLocator);

            break;
        }

        case LayerType::Ropes:
        {
            mCurrentTool = std::make_unique<RopePasteTool>(
                std::move(clipboardClone),
                mWorkbenchState.GetPasteIsTransparent(),
                *this,
                mResourceLocator);

            break;
        }

        case LayerType::ExteriorTexture:
        {
            mCurrentTool = std::make_unique<ExteriorTexturePasteTool>(
                std::move(clipboardClone),
                mWorkbenchState.GetPasteIsTransparent(),
                *this,
                mResourceLocator);

            break;
        }

        case LayerType::InteriorTexture:
        {
            mCurrentTool = std::make_unique<InteriorTexturePasteTool>(
                std::move(clipboardClone),
                mWorkbenchState.GetPasteIsTransparent(),
                *this,
                mResourceLocator);

            break;
        }
    }

    assert(mCurrentTool);

    // Notify new tool
    mUserInterface.OnCurrentToolChanged(mCurrentTool->GetType(), true);
}

void Controller::SetPasteIsTransparent(bool isTransparent)
{
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.SetIsTransparent(isTransparent);
}

void Controller::PasteRotate90CW()
{
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.Rotate90CW();
}

void Controller::PasteRotate90CCW()
{
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.Rotate90CCW();
}

void Controller::PasteFlipH()
{
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.FlipH();
}

void Controller::PasteFlipV()
{
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.FlipV();
}

void Controller::PasteCommit()
{
    // Commit
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.Commit();

    // Nuke tool and restore previous tool
    InternalSetCurrentTool(mCurrentToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))], true);
}

void Controller::PasteAbort()
{
    // Abort
    PasteTool & pasteTool = GetCurrentToolAs<PasteTool>(ToolClass::Paste);
    pasteTool.Abort();

    // Nuke tool and restore previous tool
    InternalSetCurrentTool(mCurrentToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))], true);
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

void Controller::Rotate90(RotationDirectionType direction)
{
    auto const scopedToolResumeState = SuspendTool();

    InternalRotate90<false>(direction);
}

void Controller::Rotate90ForUndo(RotationDirectionType direction)
{
    auto const scopedToolResumeState = SuspendTool();

    InternalRotate90<true>(direction);
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

void Controller::LayerChangeEpilog(std::vector<LayerType> dirtyLayers)
{
    if (!dirtyLayers.empty())
    {
        //
        // This change is final (as opposed to ephemeral)
        //

        for (LayerType const dirtyLayer : dirtyLayers)
        {
            // Mark layer as dirty
            mModelController->SetLayerDirty(dirtyLayer);

            if (dirtyLayer == LayerType::Electrical)
            {
                // Notify of (possible) change in electrical panel
                mUserInterface.OnElectricalLayerInstancedElementSetChanged(mModelController->GetInstancedElectricalElementSet());
            }
        }

        // Notify dirty changes
        mUserInterface.OnModelDirtyChanged(*mModelController);
    }

    // Notify macro properties
    NotifyModelMacroPropertiesUpdated();

    // Refresh visualization
    assert(mView);
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SelectAll()
{
    if (!mCurrentTool || mCurrentTool->GetClass() != ToolClass::Selection)
    {
        //
        // Change/set current tool to selection tool
        //

        ToolType toolType;
        switch (VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))
        {
            case LayerType::Electrical:
            {
                toolType = ToolType::ElectricalSelection;
                break;
            }

            case LayerType::Ropes:
            {
                toolType = ToolType::RopeSelection;
                break;
            }

            case LayerType::Structural:
            {
                toolType = ToolType::StructuralSelection;
                break;
            }

            case LayerType::ExteriorTexture:
            {
                toolType = ToolType::ExteriorTextureSelection;
                break;
            }

            case LayerType::InteriorTexture:
            {
                toolType = ToolType::InteriorTextureSelection;
                break;
            }

            default:
            {
                assert(false);
                toolType = ToolType::StructuralPencil;
                break;
            }
        }

        InternalSetCurrentTool(toolType, true);
    }

    GetCurrentToolAs<SelectionTool>(ToolClass::Selection).SelectAll();
}

void Controller::Deselect()
{
    GetCurrentToolAs<SelectionTool>(ToolClass::Selection).Deselect();
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

void Controller::SetExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType mode)
{
    mWorkbenchState.SetExteriorTextureLayerVisualizationMode(mode);

    // Notify
    mUserInterface.OnExteriorTextureLayerVisualizationModeChanged(mode);

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    // Refresh model visualizations
    mModelController->UpdateVisualizations(*mView);
    mUserInterface.RefreshView();
}

void Controller::SetInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType mode)
{
    mWorkbenchState.SetInteriorTextureLayerVisualizationMode(mode);

    // Notify
    mUserInterface.OnInteriorTextureLayerVisualizationModeChanged(mode);

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

void Controller::EnableWaterlineMarkers(bool doEnable)
{
    // Storage
    mWorkbenchState.EnableWaterlineMarkers(doEnable);

    // Notify UI
    mUserInterface.OnVisualWaterlineMarkersEnablementChanged(doEnable);

    // Upload markers
    auto const modelMacroProperties = mModelController->GetModelMacroProperties();
    if (mWorkbenchState.IsWaterlineMarkersEnabled() && modelMacroProperties.CenterOfMass.has_value())
    {
        mView->UploadWaterlineMarker(
            *modelMacroProperties.CenterOfMass,
            View::WaterlineMarkerType::CenterOfMass);
    }
    else
    {
        mView->RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfMass);
    }

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

void Controller::TryUndoLast()
{
    if (!mUndoStack.IsEmpty())
    {
        UndoLast();
    }
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

void Controller::BroadcastSampledInformationUpdatedAt(std::optional<ShipSpaceCoordinates> const & coordinates, LayerType layer) const
{
    assert(mModelController);

    std::optional<SampledInformation> sampledInformation;

    if (coordinates.has_value()
        && coordinates->IsInSize(mModelController->GetShipSize()))
    {
        sampledInformation = mModelController->SampleInformationAt(*coordinates, layer);
    }

    mUserInterface.OnSampledInformationUpdated(sampledInformation);
}

void Controller::BroadcastSampledInformationUpdatedNone() const
{
    mUserInterface.OnSampledInformationUpdated(std::nullopt);
}

void Controller::SetCurrentTool(ToolType tool)
{
    InternalSetCurrentTool(tool, true);
}

void Controller::SetNewShipSize(ShipSpaceSize size)
{
    mWorkbenchState.SetNewShipSize(size);
}

void Controller::SetCanvasBackgroundColor(rgbColor const & color)
{
    mWorkbenchState.SetCanvasBackgroundColor(color);
    mView->SetCanvasBackgroundColor(color);
    mUserInterface.RefreshView();
}

void Controller::SetStructuralMaterial(StructuralMaterial const * material, MaterialPlaneType plane)
{
    mWorkbenchState.SetStructuralMaterial(material, plane);
    mUserInterface.OnStructuralMaterialChanged(material, plane);
}

void Controller::SetElectricalMaterial(ElectricalMaterial const * material, MaterialPlaneType plane)
{
    mWorkbenchState.SetElectricalMaterial(material, plane);
    mUserInterface.OnElectricalMaterialChanged(material, plane);
}

void Controller::SetRopeMaterial(StructuralMaterial const * material, MaterialPlaneType plane)
{
    mWorkbenchState.SetRopesMaterial(material, plane);
    mUserInterface.OnRopesMaterialChanged(material, plane);
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
}

void Controller::OnUncapturedMouseOut()
{
    if (mCurrentTool)
    {
        mCurrentTool->OnMouseLeft();
    }
}

void Controller::OnMouseCaptureLost()
{
    // Reset tool
    InternalResetTool();
}

///////////////////////////////////////////////////////////////////////////////

template<LayerType TLayerType, typename ... TArgs>
void Controller::InternalSetLayer(wxString actionTitle, TArgs&& ... args)
{
    auto const scopedToolResumeState = SuspendTool();

    //
    // Do layer-specific work
    //

    std::optional<VisualizationType> newVisualizationType;

    if constexpr (TLayerType == LayerType::Electrical)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::Electrical>(actionTitle);

        // Switch visualization mode to this new one, if needed
        if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::ElectricalLayer)
        {
            newVisualizationType = VisualizationType::ElectricalLayer;
        }

        // Set layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this, args = std::make_tuple(std::forward<TArgs>(args)...)]() mutable
            {
                std::apply(
                    [this](auto&& ... args)
                    {
                        mModelController->SetElectricalLayer(std::forward<TArgs>(args)...);
                    },
                    std::move(args));
            });
    }
    else if constexpr (TLayerType == LayerType::Ropes)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::Ropes>(actionTitle);

        // Switch visualization mode to this new one, if needed
        if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::RopesLayer)
        {
            newVisualizationType = VisualizationType::RopesLayer;
        }

        // Set layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this, args = std::make_tuple(std::forward<TArgs>(args)...)]() mutable
            {
                std::apply(
                    [this](auto&& ... args)
                    {
                        mModelController->SetRopesLayer(std::forward<TArgs>(args)...);
                    },
                    std::move(args));
            });
    }
    else if constexpr (TLayerType == LayerType::Structural)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::Structural>(actionTitle);

        // Switch visualization mode to this new one, if needed
        if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::StructuralLayer)
        {
            newVisualizationType = VisualizationType::StructuralLayer;
        }

        // Set layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this, args = std::make_tuple(std::forward<TArgs>(args)...)]() mutable
            {
                std::apply(
                    [this](auto&& ... args)
                    {
                        mModelController->SetStructuralLayer(std::forward<TArgs>(args)...);
                    },
                    std::move(args));
            });
    }
    else if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::ExteriorTexture>(actionTitle);

        // Switch visualization mode to this new one, if needed
        if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::ExteriorTextureLayer)
        {
            newVisualizationType = VisualizationType::ExteriorTextureLayer;
        }

        // Set layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this, args = std::make_tuple(std::forward<TArgs>(args)...)]() mutable
            {
                std::apply(
                    [this](auto&& ... args)
                    {
                        mModelController->SetExteriorTextureLayer(std::forward<TArgs>(args)...);
                    },
                    std::move(args));
            });
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture);

        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::InteriorTexture>(actionTitle);

        // Switch visualization mode to this new one, if needed
        if (mWorkbenchState.GetPrimaryVisualization() != VisualizationType::InteriorTextureLayer)
        {
            newVisualizationType = VisualizationType::InteriorTextureLayer;
        }

        // Set layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this, args = std::make_tuple(std::forward<TArgs>(args)...)]() mutable
            {
                std::apply(
                    [this](auto&& ... args)
                    {
                        mModelController->SetInteriorTextureLayer(std::forward<TArgs>(args)...);
                    },
                    std::move(args));
            });
    }

    // Switch primary viz
    if (newVisualizationType.has_value())
    {
        InternalSelectPrimaryVisualization(*newVisualizationType);
    }

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    LayerChangeEpilog({ TLayerType });
}

template<LayerType TLayerType>
void Controller::InternalRemoveLayer()
{
    auto const scopedToolResumeState = SuspendTool();

    //
    // Do layer-specific work
    //

    if constexpr (TLayerType == LayerType::Electrical)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::Electrical>(_("Remove Electrical Layer"));

        // Remove layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this]()
            {
                mModelController->RemoveElectricalLayer();
            });
    }
    else if constexpr (TLayerType == LayerType::Ropes)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::Ropes>(_("Remove Ropes Layer"));

        // Remove layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this]()
            {
                mModelController->RemoveRopesLayer();
            });
    }
    else if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::ExteriorTexture>(_("Remove Exterior Layer"));

        // Remove layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this]()
            {
                mModelController->RemoveExteriorTextureLayer();
            });
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture); // No "remove" layer for structural

        // Create undo action
        InternalPushUndoForWholeLayer<LayerType::InteriorTexture>(_("Remove Interior Layer"));

        // Remove layer
        WrapLikelyLayerPresenceChangingOperation<TLayerType>(
            [this]()
            {
                mModelController->RemoveInteriorTextureLayer();
            });
    }

    // Update visualization modes
    InternalUpdateModelControllerVisualizationModes();

    LayerChangeEpilog({ TLayerType });
}

template<LayerType TLayerType>
void Controller::InternalPushUndoForWholeLayer(wxString const & title)
{
    assert(!mCurrentTool); // Tools are suspended

    // Get dirty state snapshot
    auto originalDirtyStateClone = mModelController->GetDirtyState();

    // Create undo action
    if constexpr (TLayerType == LayerType::Electrical)
    {
        auto originalLayerClone = mModelController->CloneElectricalLayer();
        auto const cloneByteSize = originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0;

        mUndoStack.Push(
            title,
            cloneByteSize,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone)](Controller & controller) mutable
            {
                controller.RestoreElectricalLayerForUndo(std::move(originalLayerClone));
            });
    }
    else if constexpr (TLayerType == LayerType::Ropes)
    {
        auto originalLayerClone = mModelController->CloneRopesLayer();
        auto const cloneByteSize = originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0;

        // Create undo action
        mUndoStack.Push(
            title,
            cloneByteSize,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone)](Controller & controller) mutable
            {
                controller.RestoreRopesLayerForUndo(std::move(originalLayerClone));
            });
    }
    else if constexpr (TLayerType == LayerType::Structural)
    {
        auto originalLayerClone = mModelController->CloneStructuralLayer();
        auto const cloneByteSize = originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0;

        // Create undo action
        mUndoStack.Push(
            title,
            cloneByteSize,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone)](Controller & controller) mutable
            {
                controller.RestoreStructuralLayerForUndo(std::move(originalLayerClone));
            });
    }
    else if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        auto originalLayerClone = mModelController->CloneExteriorTextureLayer();
        auto const cloneByteSize = originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0;
        auto originalTextureArtCredits = mModelController->GetShipMetadata().ArtCredits;

        // Create undo action
        mUndoStack.Push(
            title,
            cloneByteSize,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone), originalTextureArtCredits = std::move(originalTextureArtCredits)](Controller & controller) mutable
            {
                controller.RestoreExteriorTextureLayerForUndo(
                    std::move(originalLayerClone),
                    std::move(originalTextureArtCredits));
            });
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture);

        auto originalLayerClone = mModelController->CloneInteriorTextureLayer();
        auto const cloneByteSize = originalLayerClone ? originalLayerClone->Buffer.GetByteSize() : 0;

        // Create undo action
        mUndoStack.Push(
            title,
            cloneByteSize,
            originalDirtyStateClone,
            [originalLayerClone = std::move(originalLayerClone)](Controller & controller) mutable
            {
                controller.RestoreInteriorTextureLayerForUndo(std::move(originalLayerClone));
            });
    }

    // Notify undo stack
    mUserInterface.OnUndoStackStateChanged(mUndoStack);
}

template<LayerType TLayerType, typename TFunctor>
void Controller::WrapLikelyLayerPresenceChangingOperation(TFunctor operation)
{
    assert(!mCurrentTool); // Tools are suspended

    bool const oldIsLayerPresent = mModelController->HasLayer(TLayerType);

    operation();

    bool const newIsLayerPresent = mModelController->HasLayer(TLayerType);

    if (oldIsLayerPresent != newIsLayerPresent)
    {
        // Notify layer presence changed
        mUserInterface.OnLayerPresenceChanged(*mModelController);

        if constexpr (TLayerType == LayerType::ExteriorTexture)
        {
            // Make sure current game viz mode is consistent with presence of texture layer
            InternalReconciliateExteriorTextureVisualizationMode();
        }

        if (!newIsLayerPresent)
        {
            //
            // Deal with layer removal - need to ensure consistency
            //

            // Switch primary viz to default if it was about this layer
            if (VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()) == TLayerType)
            {
                InternalSelectPrimaryVisualization(WorkbenchState::GetDefaultPrimaryVisualization());
            }

            if constexpr (TLayerType == LayerType::ExteriorTexture)
            {
                // Change texture visualization mode if it's currently "None", so that next time a texture
                // layer is present, we don't start in "none" mode
                if (mWorkbenchState.GetExteriorTextureLayerVisualizationMode() == ExteriorTextureLayerVisualizationModeType::None)
                {
                    mWorkbenchState.SetExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType::MatteMode); // New default for next layer
                    mUserInterface.OnExteriorTextureLayerVisualizationModeChanged(ExteriorTextureLayerVisualizationModeType::MatteMode);
                }
            }
            else if constexpr (TLayerType == LayerType::InteriorTexture)
            {
                // Change texture visualization mode if it's currently "None", so that next time a texture
                // layer is present, we don't start in "none" mode
                if (mWorkbenchState.GetInteriorTextureLayerVisualizationMode() == InteriorTextureLayerVisualizationModeType::None)
                {
                    mWorkbenchState.SetInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType::MatteMode); // New default for next layer
                    mUserInterface.OnInteriorTextureLayerVisualizationModeChanged(InteriorTextureLayerVisualizationModeType::MatteMode);
                }
            }
        }
        else
        {
            // Note: we do nothing if, instead, we've just *added* the layer - we let the caller decide
            // what to do on that, as it's not about consistency
        }
    }
}

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

        bool const hasShipScaleChanged =
            mModelController->GetShipMetadata().Scale != metadata->Scale;

        mModelController->SetShipMetadata(std::move(*metadata));

        if (hasShipNameChanged)
        {
            mUserInterface.OnShipNameChanged(*mModelController);
        }

        if (hasShipScaleChanged)
        {
            mUserInterface.OnShipScaleChanged(mModelController->GetShipMetadata().Scale);
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

void Controller::InternalSetElectricalPanel(ElectricalPanel && electricalPanel)
{
    assert(mModelController);

    mModelController->SetElectricalPanel(std::move(electricalPanel));
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

    // Tell view
    mView->SetPrimaryVisualization(primaryVisualization);
}

void Controller::InternalReconciliateExteriorTextureVisualizationMode()
{
    if (!mModelController->HasLayer(LayerType::ExteriorTexture))
    {
        // If game visualization mode is the one only allowed with texture,
        // change it to auto-texturization
        if (mWorkbenchState.GetGameVisualizationMode() == GameVisualizationModeType::ExteriorTextureMode)
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
            mWorkbenchState.SetGameVisualizationMode(GameVisualizationModeType::ExteriorTextureMode);
            mUserInterface.OnGameVisualizationModeChanged(GameVisualizationModeType::ExteriorTextureMode);
        }
    }
}

void Controller::InternalUpdateModelControllerVisualizationModes()
{
    //
    // Here we orchestrate the viz modes that we want for the ModelController
    //

    // Game

    mModelController->SetGameVisualizationMode(mWorkbenchState.GetGameVisualizationMode());

    // Structural

    if (mModelController->HasLayer(LayerType::Structural))
    {
        mModelController->SetStructuralLayerVisualizationMode(mWorkbenchState.GetStructuralLayerVisualizationMode());
    }
    else
    {
        mModelController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::None);
    }

    // Electrical

    if (mModelController->HasLayer(LayerType::Electrical))
    {
        mModelController->SetElectricalLayerVisualizationMode(mWorkbenchState.GetElectricalLayerVisualizationMode());
    }
    else
    {
        mModelController->SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::None);
    }

    // Ropes

    if (mModelController->HasLayer(LayerType::Ropes))
    {
        mModelController->SetRopesLayerVisualizationMode(mWorkbenchState.GetRopesLayerVisualizationMode());
    }
    else
    {
        mModelController->SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType::None);
    }

    // Exterior Texture

    if (mModelController->HasLayer(LayerType::ExteriorTexture))
    {
        mModelController->SetExteriorTextureLayerVisualizationMode(mWorkbenchState.GetExteriorTextureLayerVisualizationMode());
    }
    else
    {
        mModelController->SetExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType::None);
    }

    // Interior Texture

    if (mModelController->HasLayer(LayerType::InteriorTexture))
    {
        mModelController->SetInteriorTextureLayerVisualizationMode(mWorkbenchState.GetInteriorTextureLayerVisualizationMode());
    }
    else
    {
        mModelController->SetInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType::None);
    }
}

ToolType Controller::GetToolTypeForCurrentVisualization()
{
    return mCurrentToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))];
}

void Controller::InternalSetCurrentTool(
    ToolType tool,
    bool isFromUser)
{
    if (!mCurrentTool || tool != mCurrentTool->GetType())
    {
        // Nuke current tool (if)
        mCurrentTool.reset();

        // Make new tool
        mCurrentTool = MakeTool(tool);

        // Notify new tool
        mUserInterface.OnCurrentToolChanged(tool, isFromUser);

        // Set new tool as the current tool of this primary visualization's layer - unless it's the Paste tool,
        // in which case we allow the previous tool for this viz layer to be resumed after the Paste tool is suspended
        assert(mCurrentTool->GetClass() != ToolClass::Paste);
        if (mCurrentTool->GetClass() != ToolClass::Paste)
        {
            mCurrentToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))] = tool;
        }
    }
}

Finalizer Controller::SuspendTool() const
{
    LogMessage("Controller::SuspendTool()");

    // Suspend tool
    bool const doResumeTool = (const_cast<Controller *>(this))->InternalSuspendTool();

    // Create finalizer
    return Finalizer(
        [doResumeTool, this]()
        {
            LogMessage("Controller::SuspendTool::Finalizer::dctor(doResume=", doResumeTool, ")");
            if (doResumeTool)
            {
                (const_cast<Controller *>(this))->InternalResumeTool();
            }
        });
}

bool Controller::InternalSuspendTool()
{
    bool const doResume = (mCurrentTool != nullptr);

    mCurrentTool.reset();

    return doResume;
}

void Controller::InternalResumeTool()
{
    assert(!mCurrentTool); // Should be here only when there's no current tool
    mCurrentTool.reset();

    // Restart last tool for current layer
    InternalSetCurrentTool(mCurrentToolTypePerLayer[static_cast<size_t>(VisualizationToLayer(mWorkbenchState.GetPrimaryVisualization()))], false);
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
                *this,
                mResourceLocator);
        }

        case ToolType::ElectricalLine:
        {
            return std::make_unique<ElectricalLineTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::ElectricalPencil:
        {
            return std::make_unique<ElectricalPencilTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::ElectricalSampler:
        {
            return std::make_unique<ElectricalSamplerTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralEraser:
        {
            return std::make_unique<StructuralEraserTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralFlood:
        {
            return std::make_unique<StructuralFloodTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralLine:
        {
            return std::make_unique<StructuralLineTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralMeasuringTapeTool:
        {
            return std::make_unique<MeasuringTapeTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralPencil:
        {
            return std::make_unique<StructuralPencilTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralSampler:
        {
            return std::make_unique<StructuralSamplerTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::RopePencil:
        {
            return std::make_unique<RopePencilTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::RopeEraser:
        {
            return std::make_unique<RopeEraserTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::RopeSampler:
        {
            return std::make_unique<RopeSamplerTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::ExteriorTextureEraser:
        {
            return std::make_unique<ExteriorTextureEraserTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::ExteriorTextureMagicWand:
        {
            return std::make_unique<ExteriorTextureMagicWandTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::InteriorTextureEraser:
        {
            return std::make_unique<InteriorTextureEraserTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::InteriorTextureMagicWand:
        {
            return std::make_unique<InteriorTextureMagicWandTool>(
                *this,
                mResourceLocator);
        }

        case ToolType::StructuralSelection:
        {
            return std::make_unique<StructuralSelectionTool>(
                *this,
                mSelectionManager,
                mResourceLocator);
        }

        case ToolType::ElectricalSelection:
        {
            return std::make_unique<ElectricalSelectionTool>(
                *this,
                mSelectionManager,
                mResourceLocator);
        }

        case ToolType::RopeSelection:
        {
            return std::make_unique<RopeSelectionTool>(
                *this,
                mSelectionManager,
                mResourceLocator);
        }

        case ToolType::ExteriorTextureSelection:
        {
            return std::make_unique<ExteriorTextureSelectionTool>(
                *this,
                mSelectionManager,
                mResourceLocator);
        }

        case ToolType::InteriorTextureSelection:
        {
            return std::make_unique<InteriorTextureSelectionTool>(
                *this,
                mSelectionManager,
                mResourceLocator);
        }

        case ToolType::StructuralPaste:
        case ToolType::ElectricalPaste:
        case ToolType::RopePaste:
        case ToolType::ExteriorTexturePaste:
        case ToolType::InteriorTexturePaste:
        {
            // We should never be invoked for this tool
            assert(false);
            break;
        }

        case ToolType::StructuralRectangle:
        {
            return std::make_unique<StructuralRectangleTool>(
                *this,
                mResourceLocator);
        }
    }

    assert(false);
    return nullptr;
}

template<typename TTool>
TTool & Controller::GetCurrentToolAs(ToolClass toolClass)
{
    assert(mCurrentTool);
    assert(mCurrentTool->GetClass() == toolClass);
    (void)toolClass;
    return dynamic_cast<TTool &>(*mCurrentTool);
}

void Controller::InternalResizeShip(
    ShipSpaceSize const & newSize,
    ShipSpaceCoordinates const & originOffset,
    wxString const & actionName)
{
    // Store undo
    {
        // Get dirty state
        ModelDirtyState const originalDirtyState = mModelController->GetDirtyState();

        // Clone all layers
        auto structuralLayerClone = mModelController->CloneStructuralLayer();
        auto electricalLayerClone = mModelController->CloneElectricalLayer();
        auto ropesLayerClone = mModelController->CloneRopesLayer();
        auto exteriorTextureLayerClone = mModelController->CloneExteriorTextureLayer();
        auto textureArtCreditsClone = mModelController->GetShipMetadata().ArtCredits;
        auto interiorTextureLayerClone = mModelController->CloneInteriorTextureLayer();

        // Calculate cost
        size_t const totalCost =
            (structuralLayerClone ? structuralLayerClone->Buffer.GetByteSize() : 0)
            + (electricalLayerClone ? electricalLayerClone->Buffer.GetByteSize() : 0)
            + (ropesLayerClone ? ropesLayerClone->Buffer.GetByteSize() : 0)
            + (exteriorTextureLayerClone ? exteriorTextureLayerClone->Buffer.GetByteSize() : 0)
            + (interiorTextureLayerClone ? interiorTextureLayerClone->Buffer.GetByteSize() : 0);

        // Create undo

        mUndoStack.Push(
            actionName,
            totalCost,
            originalDirtyState,
            [shipSize = mModelController->GetShipSize()
            , structuralLayerClone = std::move(structuralLayerClone)
            , electricalLayerClone = std::move(electricalLayerClone)
            , ropesLayerClone = std::move(ropesLayerClone)
            , exteriorTextureLayerClone = std::move(exteriorTextureLayerClone)
            , textureArtCreditsClone = std::move(textureArtCreditsClone)
            , interiorTextureLayerClone = std::move(interiorTextureLayerClone)](Controller & controller) mutable
            {
                controller.RestoreAllLayersForUndo(
                    shipSize,
                    std::move(structuralLayerClone),
                    std::move(electricalLayerClone),
                    std::move(ropesLayerClone),
                    std::move(exteriorTextureLayerClone),
                    std::move(textureArtCreditsClone),
                    std::move(interiorTextureLayerClone));
            });

        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    // Resize
    mModelController->ResizeShip(newSize, originOffset);

    // Notify view of new size
    mView->SetShipSize(newSize);
    mUserInterface.OnViewModelChanged(mView->GetViewModel());

    // Notify UI of new ship size
    mUserInterface.OnShipSizeChanged(newSize);

    LayerChangeEpilog(mModelController->GetAllPresentLayers());
}

template<bool IsForUndo>
void Controller::InternalFlip(DirectionType direction)
{
    if constexpr (!IsForUndo)
    {
        // Get dirty state
        ModelDirtyState const originalDirtyState = mModelController->GetDirtyState();

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

    LayerChangeEpilog(IsForUndo
        ? std::vector<LayerType>()
        : mModelController->GetAllPresentLayers());
}

template<bool IsForUndo>
void Controller::InternalRotate90(RotationDirectionType direction)
{
    if constexpr (!IsForUndo)
    {
        // Get dirty state
        ModelDirtyState const originalDirtyState = mModelController->GetDirtyState();

        // Calculate undo title and anti-rotation
        wxString undoTitle;
        RotationDirectionType antiRotation;
        if (direction == RotationDirectionType::Clockwise)
        {
            undoTitle = _("Rotate CW");
            antiRotation = RotationDirectionType::CounterClockwise;
        }
        else
        {
            assert(direction == RotationDirectionType::CounterClockwise);

            undoTitle = _("Rotate CCW");
            antiRotation = RotationDirectionType::Clockwise;
        }

        // Create undo

        mUndoStack.Push(
            undoTitle,
            1, // Arbitrary
            originalDirtyState,
            [antiRotation](Controller & controller) mutable
            {
                controller.Rotate90ForUndo(antiRotation);
            });

        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    // Rotate
    mModelController->Rotate90(direction);

    // Notify view of new size
    mView->SetShipSize(mModelController->GetShipSize());
    mUserInterface.OnViewModelChanged(mView->GetViewModel());

    // Notify UI of new ship size
    mUserInterface.OnShipSizeChanged(mModelController->GetShipSize());

    LayerChangeEpilog(IsForUndo
        ? std::vector<LayerType>()
        : mModelController->GetAllPresentLayers());
}

void Controller::InternalCopySelectionToClipboard(
    ShipSpaceRect const & selectionRegion,
    std::optional<LayerType> const & layerSelection) const
{
    // Get region from model controller
    ShipLayers layersRegion = mModelController->Copy(
        selectionRegion,
        layerSelection);

    // Store region in clipboard manager
    mWorkbenchState.GetClipboardManager().SetContent(std::move(layersRegion));
}

void Controller::NotifyModelMacroPropertiesUpdated()
{
    auto const & modelMacroProperties = mModelController->GetModelMacroProperties();

    // Notify UI
    mUserInterface.OnModelMacroPropertiesUpdated(modelMacroProperties);

    // Upload marker - if applicable
    if (mWorkbenchState.IsWaterlineMarkersEnabled())
    {
        if (modelMacroProperties.CenterOfMass.has_value())
        {
            mView->UploadWaterlineMarker(
                *modelMacroProperties.CenterOfMass,
                View::WaterlineMarkerType::CenterOfMass);
        }
        else
        {
            mView->RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfMass);
        }
    }
}

void Controller::RefreshToolCoordinatesDisplay()
{
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mView->ScreenToShipSpace(mUserInterface.GetMouseCoordinates());

    // Check if within ship canvas
    auto const & shipSize = mModelController->GetShipSize();
    if (mouseShipSpaceCoordinates.IsInSize(shipSize))
    {
        mUserInterface.OnToolCoordinatesChanged(mouseShipSpaceCoordinates, shipSize);
    }
    else
    {
        mUserInterface.OnToolCoordinatesChanged(std::nullopt, shipSize);
    }
}

}