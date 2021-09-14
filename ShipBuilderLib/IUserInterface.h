/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <optional>

namespace ShipBuilder {

/*
 * Interface of MainFrame that is seen by Controller and underneath.
 */
struct IUserInterface
{
public:

    virtual void RefreshView() = 0;

    // Notifies of a (possible) change in the presence of a layer
    virtual void OnLayerPresenceChanged() = 0;

    // Notifies of a (possible) change in the primary layer
    virtual void OnPrimaryLayerChanged(LayerType primaryLayer) = 0;

    // Notifies of a (possible) change in the dirtiness of the model
    virtual void OnModelDirtyChanged(bool isDirty) = 0;

    // Notifies of a change in the size of the model
    virtual void OnWorkSpaceSizeChanged(WorkSpaceSize const & workSpaceSize) = 0;

    // Notifies of a change in the view model geometry
    virtual void OnViewModelChanged() = 0;

    // Notifies of a change in any member of the workbench state
    virtual void OnWorkbenchStateChanged() = 0;

    // Notifies of a change in the currently-selected tool
    virtual void OnCurrentToolChanged(std::optional<ToolType> tool) = 0;

    // Notifies of a change in the tool coordinates to display
    virtual void OnToolCoordinatesChanged(std::optional<WorkSpaceCoordinates> coordinates) = 0;

    // Scrolls the work canvas to ensure the specified logical coordinates are visible
    virtual void ScrollIntoViewIfNeeded(DisplayLogicalCoordinates const & workCanvasDisplayLogicalCoordinates) = 0;
};

}