/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <ShipBuilderTypes.h>

#include <GameCore/GameTypes.h>

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
 */
class Tool
{
public:

    virtual ~Tool() = default;

    ToolType GetType() const
    {
        return mToolType;
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

protected:

    Tool(
        ToolType toolType,
        Controller & controller)
        : mToolType(toolType)
        , mController(controller)
    {}

    // Helpers

    void SetCursor(wxImage const & cursorImage);

    ShipSpaceCoordinates ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const;
    ImageCoordinates ScreenToTextureSpace(DisplayLogicalCoordinates const & displayCoordinates) const;

    std::optional<DisplayLogicalCoordinates> GetMouseCoordinatesIfInWorkCanvas() const;

    DisplayLogicalCoordinates GetCurrentMouseCoordinates() const;
    ShipSpaceCoordinates GetCurrentMouseCoordinatesInShipSpace() const;

protected:

    ToolType const mToolType;

    Controller & mController;
};

}