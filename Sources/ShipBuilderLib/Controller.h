/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GenericUndoPayload.h"
#include "IUserInterface.h"
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
 * - Maintains Undo stack (not individual entries), and orchestrates undo stack visualization with IUserInterface
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
        IUserInterface & userInterface,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & resourceLocator);

    static std::unique_ptr<Controller> CreateForShip(
        ShipDefinition && shipDefinition,
        OpenGLManager & openGLManager,
        WorkbenchState & workbenchState,
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
    void RestoreStructuralLayerRegionBackupForUndo(
        StructuralLayerData && layerRegionBackup,
        ShipSpaceCoordinates const & origin);
    void RestoreStructuralLayerForUndo(std::unique_ptr<StructuralLayerData> structuralLayer);

    // Electrical layer

    void NewElectricalLayer();
    void SetElectricalLayer(
        wxString actionTitle,
        ElectricalLayerData && electricalLayer);
    void RemoveElectricalLayer();
    void RestoreElectricalLayerRegionBackupForUndo(
        ElectricalLayerData && layerRegionBackup,
        ShipSpaceCoordinates const & origin);
    void RestoreElectricalLayerForUndo(std::unique_ptr<ElectricalLayerData> electricalLayer);
    void TrimElectricalParticlesWithoutSubstratum();

    // Ropes layer

    void NewRopesLayer();
    void SetRopesLayer(
        wxString actionTitle,
        RopesLayerData && ropesLayer);
    void RemoveRopesLayer();
    void RestoreRopesLayerRegionBackupForUndo(
        RopesLayerData && layerRegionBackup,
        ShipSpaceCoordinates const & origin);
    void RestoreRopesLayerForUndo(std::unique_ptr<RopesLayerData> ropesLayer);

    // Exterior Texture layer

    void SetExteriorTextureLayer(
        wxString actionTitle,
        TextureLayerData && exteriorTextureLayer,
        std::optional<std::string> textureArtCredits);
    void RemoveExteriorTextureLayer();
    void RestoreExteriorTextureLayerRegionBackupForUndo(
        TextureLayerData && layerRegionBackup,
        ImageCoordinates const & origin);
    void RestoreExteriorTextureLayerForUndo(
        std::unique_ptr<TextureLayerData> exteriorTextureLayer,
        std::optional<std::string> originalTextureArtCredits);

    // Interior Texture layer

    void SetInteriorTextureLayer(
        wxString actionTitle,
        TextureLayerData && interiorTextureLayer);
    void RemoveInteriorTextureLayer();
    void RestoreInteriorTextureLayerRegionBackupForUndo(
        TextureLayerData && layerRegionBackup,
        ImageCoordinates const & origin);
    void RestoreInteriorTextureLayerForUndo(std::unique_ptr<TextureLayerData> interiorTextureLayer);

    //
    // Misc editing
    //

    void RestoreAllLayersForUndo(
        ShipSpaceSize const & shipSize,
        std::unique_ptr<StructuralLayerData> structuralLayer,
        std::unique_ptr<ElectricalLayerData> electricalLayer,
        std::unique_ptr<RopesLayerData> ropesLayer,
        std::unique_ptr<TextureLayerData> exteriorTextureLayer,
        std::optional<std::string> originalExteriorTextureArtCredits,
        std::unique_ptr<TextureLayerData> interiorTextureLayer);

    void Restore(GenericUndoPayload && undoPayload);

    void Copy() const;

    void Cut();

    void Paste();

    void SetPasteIsTransparent(bool isTransparent);
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

    // Invoked for changes to any layer, including ephemeral viz changes (in which case
    // no layer gets dirty)
    void LayerChangeEpilog(std::vector<LayerType> dirtyLayers = {});

    void SelectAll();

    void Deselect();

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
    // Undo
    //

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
    void SetDoTextureAlignmentOptimization(bool value);
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
        IUserInterface & userInterface,
        GameAssetManager const & resourceLocator);

    template<LayerType TLayerType, typename ... TArgs>
    void InternalSetLayer(wxString actionTitle, TArgs&& ... args);

    template<LayerType TLayerType>
    void InternalRemoveLayer();

    template<LayerType TLayerType>
    void InternalPushUndoForWholeLayer(wxString const & title);

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
        std::optional<LayerType> const & layerSelection) const;

    void NotifyModelMacroPropertiesUpdated();

    void RefreshToolCoordinatesDisplay();

private:

    std::unique_ptr<View> mView;
    std::unique_ptr<ModelController> mModelController;
    UndoStack mUndoStack;
    SelectionManager mSelectionManager;
    WorkbenchState & mWorkbenchState;
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