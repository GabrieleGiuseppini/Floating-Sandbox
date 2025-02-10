/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Core/GameTypes.h>

#include <wx/image.h>

#include <optional>

namespace ShipBuilder {

class Controller;

/*
 * Base class for all tools.
 *
 * Tools:
 * - "Extensions" of Controller
 * - Take a reference to Controller - entry point for almost anything, including model modification primitives
 * - Implement state machines for interactions, including visual notifications (pseudo-cursor, marching ants, paste mask, etc.)
 * - Receive input state events from Controller, and notifications of WorkbenchState changed
 * - Instruct View for tool interactions, e.g. tool visualizations (lines, paste mask, etc.)
 * - Publish notifications to IUserInterface, e.g. to capture/release mouse
 *
 * Generally, the state machine of tools wrt event handlers is:
 * - Cctor: if it's in canvas: start eph viz.
 * - Mouse down: if in eph viz: stop eph viz; begin engagement.
 * - Mouse up: if engaged: commit and end engagement; if it's in canvas: start eph viz.
 *      Note: no guarantee that Mouse up is always preceded by Mouse down - e.g. when down happened outside window.
 * - Mouse leave: if in eph viz: stop eph viz; if engaged: commit and end engagement.
 * - Mouse move: update eph viz and/or engagement.
 */
class Tool
{
public:

    virtual ~Tool() = default;

    ToolType GetType() const
    {
        return mToolType;
    }

    virtual ToolClass GetClass() const
    {
        // Default: not a special class
        return ToolClass::Other;
    }

    //
    // Event handlers
    //

    virtual void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) = 0;
    virtual void OnLeftMouseDown() = 0;
    virtual void OnLeftMouseUp() = 0;
    virtual void OnRightMouseDown() = 0;
    virtual void OnRightMouseUp() = 0;
    virtual void OnShiftKeyDown() = 0;
    virtual void OnShiftKeyUp() = 0;
    virtual void OnMouseLeft() = 0;

protected:

    Tool(
        ToolType toolType,
        Controller & controller)
        : mToolType(toolType)
        , mController(controller)
    {}

    // Helpers

    void SetCursor(wxImage const & cursorImage);

    DisplayLogicalCoordinates GetCurrentMouseCoordinates() const;
    std::optional<DisplayLogicalCoordinates> GetCurrentMouseCoordinatesIfInWorkCanvas() const;

    ShipSpaceCoordinates GetCurrentMouseShipCoordinates() const;
    ShipSpaceCoordinates GetCurrentMouseShipCoordinatesClampedToShip() const; // <w, h> are excluded
    ShipSpaceCoordinates GetCurrentMouseShipCoordinatesClampedToShip(DisplayLogicalCoordinates const & mouseCoordinates) const; // <w, h> are excluded
    std::optional<ShipSpaceCoordinates> GetCurrentMouseShipCoordinatesIfInShip() const; // <w, h> are excluded
    std::optional<ShipSpaceCoordinates> GetCurrentMouseShipCoordinatesIfInShip(DisplayLogicalCoordinates const & mouseCoordinates) const; // <w, h> are excluded
    std::optional<ShipSpaceCoordinates> GetCurrentMouseShipCoordinatesIfInWorkCanvas() const;

    ShipSpaceCoordinates ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const;
    ShipSpaceCoordinates ScreenToShipSpaceNearest(DisplayLogicalCoordinates const & displayCoordinates) const;
    ImageCoordinates ScreenToTextureSpace(LayerType layerType, DisplayLogicalCoordinates const & displayCoordinates) const;

protected:

    ToolType const mToolType;

    Controller & mController;
};

}