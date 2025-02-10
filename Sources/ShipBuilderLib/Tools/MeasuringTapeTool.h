/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>

#include <memory>

namespace ShipBuilder {

class MeasuringTapeTool : public Tool
{
public:

    MeasuringTapeTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    virtual ~MeasuringTapeTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override;

private:

    void Leave();

    void StartEngagement(ShipSpaceCoordinates const & coords);

    void DoAction(ShipSpaceCoordinates const & coords);

    void StopEngagement();

    void UpdateOverlay(ShipSpaceCoordinates const & coords);

    void HideOverlay();

private:

    bool mIsShiftDown;

    // When set, we have an overlay
    bool mHasOverlay;

    struct EngagementData
    {
        // Start position
        ShipSpaceCoordinates StartCoords;

        EngagementData(ShipSpaceCoordinates const & startCoords)
            : StartCoords(startCoords)
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;
};

}