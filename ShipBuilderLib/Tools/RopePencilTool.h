/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				202111-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>
#include <GameCore/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class RopePencilTool : public Tool
{
public:

    RopePencilTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);

    ~RopePencilTool();

    void OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override {}
    void OnShiftKeyUp() override {}

private:

    void CheckEngagement(
        ShipSpaceCoordinates const & coords,
        StrongTypedBool<struct IsRightMouseButton> isRightButton);

    void DoEdit(ShipSpaceCoordinates const & coords);

    void CommmitAndStopEngagement();

    void DrawOverlay(
        ShipSpaceCoordinates const & coords,
        bool isApplicablePosition);

    void HideOverlay();

    std::optional<bool> GetPositionApplicability(ShipSpaceCoordinates const & coords) const;

private:

    // True when we have uploaded an overlay
    bool mHasOverlay;

    struct EngagementData
    {
        // Original layer
        RopesLayerData OriginalLayerClone;

        // Original dirty state
        Model::DirtyState OriginalDirtyState;

        // Line start
        ShipSpaceCoordinates StartCoords;

        EngagementData(
            RopesLayerData && originalLayerClone,
            Model::DirtyState const & dirtyState,
            ShipSpaceCoordinates const & startCoords)
            : OriginalLayerClone(std::move(originalLayerClone))
            , OriginalDirtyState(dirtyState)
            , StartCoords(startCoords)
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;
};

}