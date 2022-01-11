/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "ModelController.h"
#include "OpenGLManager.h"
#include "UndoStack.h"
#include "ShipBuilderTypes.h"
#include "View.h"
#include "WorkbenchState.h"
#include "Tools/Tool.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipDefinition.h>
#include <Game/ShipTexturizer.h>

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
        ResourceLocator const & resourceLocator);

    static std::unique_ptr<Controller> CreateForShip(
        ShipDefinition && shipDefinition,
        OpenGLManager & openGLManager,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ShipTexturizer const & shipTexturizer,
        ResourceLocator const & resourceLocator);

    ShipDefinition MakeShipDefinition();

    ShipSpaceSize const & GetShipSize() const
    {
        assert(mModelController);
        return mModelController->GetModel().GetShipSize();
    }

    ShipMetadata const & GetShipMetadata() const
    {
        assert(mModelController);
        return mModelController->GetModel().GetShipMetadata();
    }

    ShipPhysicsData const & GetShipPhysicsData() const
    {
        assert(mModelController);
        return mModelController->GetModel().GetShipPhysicsData();
    }

    std::optional<ShipAutoTexturizationSettings> const & GetShipAutoTexturizationSettings() const
    {
        assert(mModelController);
        return mModelController->GetModel().GetShipAutoTexturizationSettings();
    }

    void SetShipProperties(
        std::optional<ShipMetadata> && metadata,
        std::optional<ShipPhysicsData> && physicsData,
        std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings);

    void RestoreShipPropertiesForUndo(
        std::optional<ShipMetadata> && metadata,
        std::optional<ShipPhysicsData> && physicsData,
        std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings);

    bool HasModelLayer(LayerType layer) const
    {
        assert(mModelController);
        return mModelController->GetModel().HasLayer(layer);
    }

    bool IsModelDirty() const
    {
        assert(mModelController);
        return mModelController->GetModel().GetIsDirty();
    }

    bool IsModelDirty(LayerType layer) const
    {
        assert(mModelController);
        return mModelController->GetModel().GetIsDirty(layer);
    }

    void ClearModelDirty();

    void RestoreDirtyState(Model::DirtyState && dirtyState);

    std::unique_ptr<RgbaImageData> MakePreview() const;

    std::optional<ShipSpaceRect> CalculateBoundingBox() const;

    ModelValidationResults ValidateModel();

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);
    void RestoreStructuralLayerRegionForUndo(
        StructuralLayerData && layerRegion,
        ShipSpaceCoordinates const & origin);
    void RestoreStructuralLayerForUndo(StructuralLayerData && structuralLayer);

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();
    void RestoreElectricalLayerRegionForUndo(
        ElectricalLayerData && layerRegion,
        ShipSpaceCoordinates const & origin);
    void RestoreElectricalLayerForUndo(std::unique_ptr<ElectricalLayerData> electricalLayer);
    void TrimElectricalParticlesWithoutSubstratum();

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();
    void RestoreRopesLayerForUndo(std::unique_ptr<RopesLayerData> ropesLayer);

    void SetTextureLayer(
        TextureLayerData && textureLayer,
        std::optional<std::string> textureArtCredits);
    void RemoveTextureLayer();
    void RestoreTextureLayerForUndo(
        std::unique_ptr<TextureLayerData> textureLayer,
        std::optional<std::string> originalTextureArtCredits);

    void RestoreAllLayersForUndo(
        ShipSpaceSize const & shipSize,
        StructuralLayerData && structuralLayer,
        std::unique_ptr<ElectricalLayerData> electricalLayer,
        std::unique_ptr<RopesLayerData> ropesLayer,
        std::unique_ptr<TextureLayerData> textureLayer,
        std::optional<std::string> originalTextureArtCredits);

    void AutoTrim();

    void Flip(DirectionType direction);
    void FlipForUndo(DirectionType direction);

    void ResizeShip(
        ShipSpaceSize const & newSize,
        ShipSpaceCoordinates const & originOffset);

    void SelectPrimaryVisualization(VisualizationType primaryVisualization);

    void SetGameVisualizationMode(GameVisualizationModeType mode);
    void SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode);
    void SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode);
    void SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode);

    TextureLayerVisualizationModeType GetTextureLayerVisualizationMode() const;
    void SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode);

    float GetOtherVisualizationsOpacity() const;
    void SetOtherVisualizationsOpacity(float opacity);

    bool IsVisualGridEnabled() const;
    void EnableVisualGrid(bool doEnable);

    std::optional<ToolType> GetCurrentTool() const;
    void SetCurrentTool(std::optional<ToolType> tool);

    void SetStructuralMaterial(MaterialPlaneType plane, StructuralMaterial const * material);
    void SetElectricalMaterial(MaterialPlaneType plane, ElectricalMaterial const * material);
    void SetRopeMaterial(MaterialPlaneType plane, StructuralMaterial const * material);

    void UndoLast();
    void UndoUntil(size_t index);

    void Render();

    void AddZoom(int deltaZoom);
    void SetCamera(int camX, int camY);
    void ResetView();

    void OnWorkCanvasResized(DisplayLogicalSize const & newSize);

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

    //
    // Tool resume state, RAII
    //

    struct [[nodiscard]] ScopedToolResumeState
    {
        ScopedToolResumeState(
            Controller const & controller,
            bool doResumeTool)
            : mController(controller)
            , mDoResumeTool(doResumeTool)
        {
            LogMessage("ScopedToolResumeState::cctor(doResume=", doResumeTool, ")");
        }

        ~ScopedToolResumeState()
        {
            LogMessage("ScopedToolResumeState::dctor(doResume=", mDoResumeTool, ")");

            if (mDoResumeTool)
            {
                (const_cast<Controller &>(mController)).InternalResumeTool();
            }
        }

    private:
        Controller const & mController;
        bool const mDoResumeTool;
    };

private:

    Controller(
        std::unique_ptr<ModelController> modelController,
        OpenGLManager & openGLManager,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    void InternalSetShipProperties(
        std::optional<ShipMetadata> && metadata,
        std::optional<ShipPhysicsData> && physicsData,
        std::optional<std::optional<ShipAutoTexturizationSettings>> && autoTexturizationSettings);

    void InternalSelectPrimaryVisualization(VisualizationType primaryVisualization);

    void InternalReconciliateTextureVisualizationMode();

    void InternalUpdateModelControllerVisualizationModes();

    void InternalSetCurrentTool(std::optional<ToolType> toolType);

    ScopedToolResumeState SuspendTool() const;
    bool InternalSuspendTool();
    void InternalResumeTool();
    void InternalResetTool();

    std::unique_ptr<Tool> MakeTool(ToolType toolType);

    void InternalResizeShip(
        ShipSpaceSize const & newSize,
        ShipSpaceCoordinates const & originOffset,
        wxString const & actionName);

    template<bool IsForUndo>
    void InternalFlip(DirectionType direction);

    void RefreshToolCoordinatesDisplay();

private:

    std::unique_ptr<View> mView;
    std::unique_ptr<ModelController> mModelController;
    UndoStack mUndoStack;
    WorkbenchState & mWorkbenchState;
    IUserInterface & mUserInterface;

    ResourceLocator const & mResourceLocator;

    //
    // State
    //

    std::unique_ptr<Tool> mCurrentTool;

    // The last tool that was used for each layer
    std::array<std::optional<ToolType>, LayerCount> mLastToolTypePerLayer;
};

}