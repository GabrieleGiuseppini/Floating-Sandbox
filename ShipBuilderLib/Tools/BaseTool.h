/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "InputState.h"
#include "IUserInterface.h"
#include "ModelController.h"
#include "SelectionManager.h"
#include "ShipBuilderTypes.h"
#include "View.h"

namespace ShipBuilder {

/*
 * Base class for all tools.
 *
 * Tools:
 * - "Extensions" of Controller
 * - Implement state machines for interactions, including visual notifications (marching ants, paste mask, etc.)
 * - Take references to WorkbenchState and SelectionManager (at tool initialization time)
 *     - SelectionManager so that Selection tool can save selection to it
 * - Receive input state events from Controller, and notifications of WorkbenchState changed
 * - Take references to View and ModelController
 * - Modify Model via ModelController
 * - Instruct View for tool interactions, e.g. tool visualizations (lines, paste mask, etc.)
 * - Have also reference to IUserInterface, e.g. to capture/release mouse
 */
class BaseTool
{
public:

    virtual ~BaseTool() = default;

    //
    // Event handlers
    //

    virtual void OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition) = 0;
    virtual void OnLeftMouseDown() = 0;
    virtual void OnLeftMouseUp() = 0;
    virtual void OnRightMouseDown() = 0;
    virtual void OnRightMouseUp() = 0;
    virtual void OnShiftKeyDown() = 0;
    virtual void OnShiftKeyUp() = 0;
    virtual void OnMouseOut() = 0;

protected:

    BaseTool(
        ToolType toolType,
        ModelController & modelController,
        SelectionManager & selectionManager,
        IUserInterface & userInterface,
        View & view)
        : mToolType(toolType)
        , mModelController(modelController)
        , mSelectionManager(selectionManager)
        , mUserInterface(userInterface)
        , mView(view)
    {}

    void ScrollIntoViewIfNeeded(DisplayLogicalCoordinates const & mouseScreenPosition)
    {
        mUserInterface.ScrollIntoViewIfNeeded(mouseScreenPosition);
    }

protected:

    ToolType const mToolType;

    ModelController & mModelController;
    SelectionManager & mSelectionManager;
    IUserInterface & mUserInterface;
    View & mView;
};

}