/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <memory>

namespace ShipBuilder {

class MeasuringTapeTool : public Tool
{
public:

    MeasuringTapeTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);

    virtual ~MeasuringTapeTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;

private:

    void StartEngagement(ShipSpaceCoordinates const & coords);

    void DoAction(ShipSpaceCoordinates const & coords);

    void StopEngagement();

    void DrawOverlay(ShipSpaceCoordinates const & coords);

    void HideOverlay();

private:

    bool mIsShiftDown;

    // When set, we have an overlay
    bool mHasOverlay;

    struct EngagementData
    {
        // Start position
        ShipSpaceCoordinates StartCoords;

        // V or H lock
        bool IsLocked;

        EngagementData(ShipSpaceCoordinates const & startCoords)
            : StartCoords(startCoords)
            , IsLocked(false)
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;
};

}