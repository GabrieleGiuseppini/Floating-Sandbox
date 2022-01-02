/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ShipBuilderTypes.h"
#include "UndoStack.h"

#include <wx/image.h>
#include <wx/string.h>

#include <optional>

namespace ShipBuilder {

/*
 * Interface of MainFrame that is seen by Controller and underneath.
 */
struct IUserInterface
{
public:

    virtual void RefreshView() = 0;

    // Notifies of a change in the view model geometry
    virtual void OnViewModelChanged() = 0;

    // Notifies of a change in the name of the ship
    virtual void OnShipNameChanged(std::string const & newName) = 0;

    // Notifies of a change in the size of the model
    virtual void OnShipSizeChanged(ShipSpaceSize const & shipSpaceSize) = 0;

    // Notifies of a (possible) change in the presence of a layer
    virtual void OnLayerPresenceChanged() = 0;

    // Notifies of a (possible) change in the primary visualization
    virtual void OnPrimaryVisualizationChanged(VisualizationType primaryVisualization) = 0;

    // Notifies of a (possible) change in a visualization mode
    virtual void OnGameVisualizationModeChanged(GameVisualizationModeType mode) = 0;
    virtual void OnStructuralLayerVisualizationModeChanged(StructuralLayerVisualizationModeType mode) = 0;
    virtual void OnElectricalLayerVisualizationModeChanged(ElectricalLayerVisualizationModeType mode) = 0;
    virtual void OnRopesLayerVisualizationModeChanged(RopesLayerVisualizationModeType mode) = 0;
    virtual void OnTextureLayerVisualizationModeChanged(TextureLayerVisualizationModeType mode) = 0;

    // Notifies of a (possible) change in the dirtiness of the model
    virtual void OnModelDirtyChanged() = 0;

    // Notifies of a change in any member of the workbench state
    virtual void OnWorkbenchStateChanged() = 0;

    // Notifies of a change in the currently-selected tool
    virtual void OnCurrentToolChanged(std::optional<ToolType> tool) = 0;

    // Notifies of a change in the state of the undo stack
    virtual void OnUndoStackStateChanged() = 0;

    // Notifies of a change in the tool coordinates to display
    virtual void OnToolCoordinatesChanged(std::optional<ShipSpaceCoordinates> coordinates) = 0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    virtual void OnError(wxString const & errorMessage) const = 0;

    virtual ShipSpaceCoordinates GetMouseCoordinates() const = 0;

    virtual std::optional<ShipSpaceCoordinates> GetMouseCoordinatesIfInWorkCanvas() const = 0;

    virtual void SetToolCursor(wxImage const & cursorImage) = 0;

    virtual void ResetToolCursor() = 0;

    // Scrolls the work canvas to ensure the specified logical coordinates are visible
    virtual void ScrollIntoViewIfNeeded(DisplayLogicalCoordinates const & workCanvasDisplayLogicalCoordinates) = 0;
};

}