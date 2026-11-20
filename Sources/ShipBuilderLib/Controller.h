/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "LayerSelector.h"
#include "ModelController.h"
#include "ModelValidationSession.h"
#include "OpenGLManager.h"
#include "SelectionManager.h"
#include "ShipBuilderTypes.h"
#include "UndoStack.h"
#include "View.h"
#include "WorkbenchState.h"
#include "Tools/Tool.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/ShipDefinition.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/Finalizer.h>

#include <map>
#include <memory>
#include <optional>
#include <string>

namespace ShipBuilder {

/*
 * This class implements the core of the ShipBuilder logic. It orchestrates interactions between
 * UI, View, and Model.
 *
 * It is the only actor that acts on the ModelController, and the only actor that acts on the UI.
 *
 * - Owns ModelController and View
 * - Main Frame calls into Controller for each user interaction, including button clicks
 *      - Controller->Main Frame callbacks via IUserInterface
 * - Maintains UI state (e.g. grid toggle, visible layers), instructing View
 * - Uses Undo stack (not individual entries), which is owned by caller, and orchestrates undo stack visualization with IUserInterface
 * - Maintains interaction state, implemented via Tools
 * - Owns SelectionManager pseudo-tool
 * - Owns ClipboardManager pseudo-tool
 */
class Controller
{
public:

    static std::unique_ptr<Controller> CreateNew(
        std::string const & shipName,
        OpenGLManager & openGLManager,
        WorkbenchState & workbenchState,
        UndoStack & undoStack,
        IUserInterface & userInterface,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & resourceLocator);

    static std::unique_ptr<Controller> CreateForShip(
        ShipDefinition && shipDefinition,
        OpenGLManager & openGLManager,
        WorkbenchState & workbenchState,
        UndoStack & undoStack,
        IUserInterface & userInterface,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & resourceLocator);

    IModelObservable const & GetModelObservable() const
    {
        assert(mModelController);
        return *mModelController;
    }

    ModelController & GetModelController()
    {
        assert(mModelController);
        return *mModelController;
    }

    IUserInterface & GetUserInterface()
    {
        return mUserInterface;
    }

    View & GetView()
    {
        assert(mView);
        return *mView;
    }

    WorkbenchState & GetWorkbenchState()
    {
        return mWorkbenchState;
    }

    //
    // Ship
    //

    ShipDefinition MakeShipDefinition() const;

    void SetShipProperties(
        std::optional<ShipMetadata> && metadata,
        std::optional<ShipPhysicsData> && physicsData,
        std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings);

    void RestoreShipPropertiesForUndo(
        std::optional<ShipMetadata> && metadata,
        std::optional<ShipPhysicsData> && physicsData,
        std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings);

    void SetElectricalPanel(ElectricalPanel electricalPanel);

    void RestoreElectricalPanelForUndo(ElectricalPanel && electricalPanel);

    void ClearModelDirty();

    void RestoreDirtyState(ModelDirtyState && dirtyState);

    std::unique_ptr<RgbaImageData> MakePreview() const;

    std::optional<ShipSpaceRect> CalculateBoundingBox() const;

    ModelValidationSession StartValidation() const;

    //
    // Layer editing
    //

    // Structural layer

    void NewStructuralLayer();
    void SetStructuralLayer(
        wxString actionTitle,
        StructuralLayerData && structuralLayer);
    void RestoreStructuralLayerPresenceForUndo(std::unique_ptr<StructuralLayerData> structuralLayer); // Exposed to Undo callbacks

    // Electrical layer

    void NewElectricalLayer();
    void SetElectricalLayer(
        wxString actionTitle,
        ElectricalLayerData && electricalLayer);
    void RemoveElectricalLayer();
    void RestoreElectricalLayerPresenceForUndo(std::unique_ptr<ElectricalLayerData> electricalLayer); // Exposed to Undo callbacks
    void TrimElectricalParticlesWithoutSubstratum();

    // Ropes layer

    void NewRopesLayer();
    void SetRopesLayer(
        wxString actionTitle,
        RopesLayerData && ropesLayer);
    void RemoveRopesLayer();
    void RestoreRopesLayerPresenceForUndo(std::unique_ptr<RopesLayerData> ropesLayer); // Exposed to Undo callbacks

    // Exterior Texture layer

    void SetExteriorTextureLayer(
        wxString actionTitle,
        TextureLayerData && exteriorTextureLayer,
        std::optional<std::string> textureArtCredits);
    void RemoveExteriorTextureLayer();
    void RestoreExteriorTextureLayerPresenceForUndo( // Exposed to Undo callbacks
        std::unique_ptr<TextureLayerData> exteriorTextureLayer,
        std::optional<std::string> originalTextureArtCredits);

    // Interior Texture layer

    void SetInteriorTextureLayer(
        wxString actionTitle,
        TextureLayerData && interiorTextureLayer);
    void RemoveInteriorTextureLayer();
    void RestoreInteriorTextureLayerPresenceForUndo(std::unique_ptr<TextureLayerData> interiorTextureLayer); // Exposed to Undo callbacks

    //
    // Misc editing
    //

    void Copy() const;

    void Cut();

    void Paste();

    void SetPasteIsTransparent(bool value);
    void PasteRotate90CW();
    void PasteRotate90CCW();
    void PasteFlipH();
    void PasteFlipV();
    void PasteCommit();
    void PasteAbort();

    void AutoTrim();

    void Flip(DirectionType direction);
    void FlipForUndo(DirectionType direction);

    void Rotate90(RotationDirectionType direction);
    void Rotate90ForUndo(RotationDirectionType direction);

    void ResizeShip(
        ShipSpaceSize const & newSize,
        ShipSpaceCoordinates const & originOffset);

    template<LayerType TLayerType, typename TMaterialUndoAction>
    void ReplaceCustomMaterials(
        std::map<typename LayerTypeTraits<TLayerType>::material_type const *, typename LayerTypeTraits<TLayerType>::material_type const *> const & replacementMap,
        TMaterialUndoAction && materialUndoAction,
        wxString && undoActionName)
    {
        auto layerDirtyStateClone = mModelController->GetDirtyState();

        if (mModelController->HasLayer(TLayerType))
        {
            auto const scopedToolResumeState = SuspendTool();

            //
            // Take clone of current layer
            //

            auto layerClone = mModelController->CopyShipSpaceWhole(LayerSelector{ TLayerType });

            //
            // Do operation
            //

            std::optional<ShipSpaceRect> affectedRegion = mModelController->ReplaceCustomMaterials<TLayerType>(replacementMap);

            //
            // Create undo
            //

            if (affectedRegion.has_value())
            {
                // Clip region
                layerClone = ModelController::MakeShipBodyRegionClip(layerClone, *affectedRegion);

                // Store
                auto const undoCost = layerClone.GetTotalCost();
                StoreUndoAction(
                    undoActionName,
                    undoCost,
                    layerDirtyStateClone,
                    [layerClone = std::move(layerClone), materialUndoAction = std::move(materialUndoAction)](Controller & controller) mutable
                    {
                        controller.RestoreForUndo(std::move(layerClone));
                        materialUndoAction();
                    });
            }
            else
            {
                // Material not in use, store just material action undo
                StoreUndoAction(
                    undoActionName,
                    1, // Nominal
                    layerDirtyStateClone,
                    [materialUndoAction = std::move(materialUndoAction)](Controller &) mutable
                    {
                        materialUndoAction();
                    });
            }

            //
            // Finalize
            //

            if (affectedRegion.has_value())
            {
                LayerChangeEpilog({ TLayerType });
            }
        }
        else
        {
            // Material not in use, store just material action undo
            StoreUndoAction(
                undoActionName,
                1, // Nominal
                layerDirtyStateClone,
                [materialUndoAction = std::move(materialUndoAction)](Controller &) mutable
                {
                    materialUndoAction();
                });
        }
    }

    void SelectAll();

    void Deselect();

    // Invoked for changes to any layer, including ephemeral viz changes (in which case
    // no layer gets dirty)
    void LayerChangeEpilog(LayerSelector const & dirtyLayers = {}); // Empty when for eph viz

    //
    // Undo
    //

    // Convenience helper for one single region
    void RestoreForUndo(ShipSpaceBodyRegions::ShipSpaceBodyRegion && undoPayload)
    {
        RestoreForUndo(ShipSpaceBodyRegions::Make(std::move(undoPayload)));
    }
    void RestoreForUndo(ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion && undoPayload)
    {
        RestoreForUndo(ExteriorTextureSpaceBodyRegions::Make(std::move(undoPayload)));
    }
    void RestoreForUndo(InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion && undoPayload)
    {
        RestoreForUndo(InteriorTextureSpaceBodyRegions::Make(std::move(undoPayload)));
    }

    // Main restore functionality
    // Exposed to Undo callbacks
    template<typename TUndoPayload>
    void RestoreForUndo(TUndoPayload && undoPayload)
    {
        // No layer-presence changing operations
        mModelController->RestoreForUndo(std::move(undoPayload));

        // Notify macro properties
        NotifyModelMacroPropertiesUpdated();

        // Refresh model visualizations
        mModelController->UpdateVisualizations(*mView, mGameAssetManager);
        mUserInterface.RefreshView();
    }

    // Special-case restore
    // Exposed to Undo callbacks
    void RestoreAllForUndo(
        ShipSpaceSize const & shipSize,
        std::unique_ptr<StructuralLayerData> structuralLayer,
        std::unique_ptr<ElectricalLayerData> electricalLayer,
        std::unique_ptr<RopesLayerData> ropesLayer,
        std::unique_ptr<TextureLayerData> exteriorTextureLayer,
        std::optional<std::string> originalExteriorTextureArtCredits,
        std::unique_ptr<TextureLayerData> interiorTextureLayer);

    template<typename ... TArgs>
    void StoreUndoAction(TArgs&& ... args)
    {
        mUndoStack.Push(std::forward<TArgs>(args)...);
        mUserInterface.OnUndoStackStateChanged(mUndoStack);
    }

    void TryUndoLast();
    void UndoLast();
    void UndoUntil(size_t index);

    //
    // Visualization management
    //

    void SelectPrimaryVisualization(VisualizationType primaryVisualization);

    void SetGameVisualizationMode(GameVisualizationModeType mode);
    void SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode);
    void SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode);
    void SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode);
    void SetExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType mode);
    void SetInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType mode);

    void SetOtherVisualizationsOpacity(float opacity);

    void EnableWaterlineMarkers(bool doEnable);
    void EnableVisualGrid(bool doEnable);

    //
    // View
    //

    void Render();

    void AddZoom(int deltaZoom);
    void SetCamera(int camX, int camY);
    void ResetView();

    void OnWorkCanvasResized(DisplayLogicalSize const & newSize);

    //
    // Services to tools
    //

    void BroadcastSampledInformationUpdatedAt(std::optional<ShipSpaceCoordinates> const & coordinates, LayerType layer) const;
    void BroadcastSampledInformationUpdatedNone() const;

    //
    // Misc
    //

    void SetCurrentTool(ToolType tool);

    void SetNewShipSize(ShipSpaceSize size);
    void SetCanvasBackgroundColor(rgbColor const & color);

    void SetStructuralMaterial(StructuralMaterial const * material, MaterialPlaneType plane);
    void SetElectricalMaterial(ElectricalMaterial const * material, MaterialPlaneType plane);
    void SetRopeMaterial(StructuralMaterial const * material, MaterialPlaneType plane);

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates);
    void OnLeftMouseDown();
    void OnLeftMouseUp();
    void OnRightMouseDown();
    void OnRightMouseUp();
    void OnShiftKeyDown();
    void OnShiftKeyUp();
    void OnUncapturedMouseIn();
    void OnUncapturedMouseOut();
    void OnMouseCaptureLost();

private:

    Controller(
        std::unique_ptr<ModelController> modelController,
        OpenGLManager & openGLManager,
        WorkbenchState & workbenchState,
        UndoStack & undoStack,
        IUserInterface & userInterface,
        GameAssetManager const & resourceLocator);

    template<LayerType TLayerType, typename ... TArgs>
    void InternalSetLayer(wxString actionTitle, TArgs&& ... args);

    template<LayerType TLayerType>
    void InternalRemoveLayer();

    template<LayerType TLayerType>
    void InternalPushUndoForLayerPresenceChange(wxString const & title);

    template<LayerType TLayerType, typename TFunctor>
    void WrapLikelyLayerPresenceChangingOperation(TFunctor operation);

    //

    void InternalSetShipProperties(
        std::optional<ShipMetadata> && metadata,
        std::optional<ShipPhysicsData> && physicsData,
        std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings);

    void InternalSetElectricalPanel(ElectricalPanel && electricalPanel);

    void InternalSelectPrimaryVisualization(VisualizationType primaryVisualization);

    void InternalReconciliateExteriorTextureVisualizationMode();

    void InternalUpdateModelControllerVisualizationModes();

    ToolType GetToolTypeForCurrentVisualization();

    void InternalSetCurrentTool(ToolType tool, bool isFromUser);
    Finalizer SuspendTool() const;
    bool InternalSuspendTool();
    void InternalResumeTool();
    void InternalResetTool();

    std::unique_ptr<Tool> MakeTool(ToolType toolType);

    template<typename TTool>
    TTool & GetCurrentToolAs(ToolClass toolClass);

    void InternalResizeShip(
        ShipSpaceSize const & newSize,
        ShipSpaceCoordinates const & originOffset,
        wxString const & actionName);

    template<bool IsForUndo>
    void InternalFlip(DirectionType direction);

    template<bool IsForUndo>
    void InternalRotate90(RotationDirectionType direction);

    void InternalCopySelectionToClipboard(
        ShipSpaceRect const & selectionRegion,
        LayerSelector const & layerSelection) const;

    void NotifyModelMacroPropertiesUpdated();

    void RefreshToolCoordinatesDisplay();

private:

    std::unique_ptr<View> mView;
    std::unique_ptr<ModelController> mModelController;
    SelectionManager mSelectionManager;
    WorkbenchState & mWorkbenchState;
    UndoStack & mUndoStack;
    IUserInterface & mUserInterface;

    GameAssetManager const & mGameAssetManager;

    //
    // State
    //

    std::unique_ptr<Tool> mCurrentTool;

    // The tool active for each layer
    std::array<ToolType, LayerCount> mCurrentToolTypePerLayer;
};

}