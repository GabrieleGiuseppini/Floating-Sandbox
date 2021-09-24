/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "InputState.h"
#include "IUserInterface.h"
#include "ModelController.h"
#include "ShipBuilderTypes.h"
#include "View.h"
#include "WorkbenchState.h"
#include "Tools/Tool.h"

#include <Game/LayerBuffers.h>
#include <Game/ResourceLocator.h>

#include <memory>
#include <optional>

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
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    static std::unique_ptr<Controller> CreateForShip(
        /* TODO: loaded ship ,*/
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    ModelController const & GetModelController() const
    {
        return *mModelController;
    }

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);
    void RestoreLayerBufferRegion(
        StructuralLayerBuffer const & layerBufferRegion,
        ShipSpaceCoordinates const & origin);

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();
    void RestoreLayerBufferRegion(
        ElectricalLayerBuffer const & layerBufferRegion,
        ShipSpaceCoordinates const & origin);

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();
    void RestoreLayerBufferRegion(
        RopesLayerBuffer const & layerBufferRegion,
        ShipSpaceCoordinates const & origin);

    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();
    void RestoreLayerBufferRegion(
        TextureLayerBuffer const & layerBufferRegion,
        ShipSpaceCoordinates const & origin);

    void ResizeShip(ShipSpaceSize const & newSize);

    LayerType GetPrimaryLayer() const;
    void SelectPrimaryLayer(LayerType primaryLayer);

    std::optional<ToolType> GetCurrentTool() const;
    void SetCurrentTool(std::optional<ToolType> tool);

    void AddZoom(int deltaZoom);
    void SetCamera(int camX, int camY);
    void ResetView();

    void OnWorkCanvasResized(DisplayLogicalSize const & newSize);

    void OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition);
    void OnLeftMouseDown();
    void OnLeftMouseUp();
    void OnRightMouseDown();
    void OnRightMouseUp();
    void OnShiftKeyDown();
    void OnShiftKeyUp();
    void OnMouseOut();

private:

    Controller(
        std::unique_ptr<ModelController> modelController,
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        ResourceLocator const & resourceLocator);

    std::unique_ptr<Tool> MakeTool(ToolType toolType);

    void RefreshToolCoordinatesDisplay();

private:

    View & mView;
    std::unique_ptr<ModelController> mModelController;

    WorkbenchState & mWorkbenchState;
    IUserInterface & mUserInterface;

    ResourceLocator const & mResourceLocator;

    // Input state
    InputState mInputState;

    //
    // State
    //

    LayerType mPrimaryLayer;

    std::unique_ptr<Tool> mCurrentTool;
};

}