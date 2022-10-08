/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <memory>

namespace ShipBuilder {

class RopeEraserTool : public Tool
{
public:

    RopeEraserTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);

    virtual ~RopeEraserTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override {}
    void OnShiftKeyUp() override {}
    void OnMouseLeft() override;

private:

    void OnMouseDown();

    void OnMouseUp();

    void StartEngagement();

    void DoAction(ShipSpaceCoordinates const & coords);

    void StopEngagement();

    void Leave(bool doCommitIfEngaged);

    void DrawOverlay(ShipSpaceCoordinates const & coords);

    void HideOverlay();

private:

    // Original layer
    RopesLayerData mOriginalLayerClone;

    // When set, we have an overlay
    bool mHasOverlay;

    struct EngagementData
    {
        // Dirty state
        ModelDirtyState OriginalDirtyState;

        // Set to true if we've really edited anything
        bool HasEdited;

        EngagementData(ModelDirtyState const & dirtyState)
            : OriginalDirtyState(dirtyState)
            , HasEdited(false)
        {}
    };

    // Engagement data - when set, it means we're engaged;
    // "being engaged" for this tool basically means that 
    // the mouse button is down
    std::optional<EngagementData> mEngagementData;
};

}