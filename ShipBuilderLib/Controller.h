/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "ModelController.h"
#include "UndoStack.h"
#include "ShipBuilderTypes.h"
#include "View.h"
#include "WorkbenchState.h"
#include "Tools/Tool.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipDefinition.h>

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
 * - Owns ModelController, takes reference to View
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
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    static std::unique_ptr<Controller> CreateForShip(
        ShipDefinition && shipDefinition,
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    ModelController const & GetModelController() const
    {
        return *mModelController;
    }

    void ClearModelDirty();

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);
    void RestoreLayerRegion(
        StructuralLayerData && layerRegion,
        ShipSpaceCoordinates const & origin);

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();
    void RestoreLayerRegion(
        ElectricalLayerData && layerRegion,
        ShipSpaceCoordinates const & origin);

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();
    void RestoreLayerRegion(
        RopesLayerData const & layerRegion,
        ShipSpaceCoordinates const & origin);

    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();
    void RestoreLayerRegion(
        TextureLayerData const & layerRegion,
        ShipSpaceCoordinates const & origin);

    void ResizeShip(ShipSpaceSize const & newSize);

    LayerType GetPrimaryLayer() const;
    void SelectPrimaryLayer(LayerType primaryLayer);

    std::optional<ToolType> GetCurrentTool() const;
    void SetCurrentTool(std::optional<ToolType> tool);

    bool CanUndo() const;
    void Undo();

    void AddZoom(int deltaZoom);
    void SetCamera(int camX, int camY);
    void ResetView();

    void OnWorkCanvasResized(DisplayLogicalSize const & newSize);

    void EnableVisualGrid(bool doEnable);

    void OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates);
    void OnLeftMouseDown();
    void OnLeftMouseUp();
    void OnRightMouseDown();
    void OnRightMouseUp();
    void OnShiftKeyDown();
    void OnShiftKeyUp();
    void OnUncapturedMouseOut();
    void OnMouseCaptureLost();

private:

    Controller(
        std::unique_ptr<ModelController> modelController,
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    void StopTool();

    void StartTool();

    std::unique_ptr<Tool> MakeTool(ToolType toolType);

    void RefreshToolCoordinatesDisplay();

private:

    View & mView;
    std::unique_ptr<ModelController> mModelController;
    UndoStack mUndoStack;
    WorkbenchState & mWorkbenchState;
    IUserInterface & mUserInterface;

    ResourceLocator const & mResourceLocator;

    //
    // State
    //

    LayerType mPrimaryLayer;

    std::optional<ToolType> mCurrentToolType;
    std::unique_ptr<Tool> mCurrentTool;
};

}