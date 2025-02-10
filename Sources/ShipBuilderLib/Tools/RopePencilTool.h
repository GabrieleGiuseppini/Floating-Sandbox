/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/GameTypes.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class RopePencilTool : public Tool
{
public:

    RopePencilTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    virtual ~RopePencilTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override {}
    void OnShiftKeyUp() override {}
    void OnMouseLeft() override;

private:

    void OnMouseDown(MaterialPlaneType plane);

    void OnMouseUp();

    void Leave(bool doCommitIfEngaged);

    void StartEngagement(
        ShipSpaceCoordinates const & coords,
        MaterialPlaneType materialPlane);

    void DoTempVisualization(ShipSpaceCoordinates const & coords);

    void MendTempVisualization();

    bool CommmitAndStopEngagement();

    void DrawOverlay(ShipSpaceCoordinates const & coords);

    void HideOverlay();

    inline StructuralMaterial const * GetMaterial(MaterialPlaneType plane) const;

private:

    // True when we have temp visualization
    bool mHasTempVisualization;

    // True when we have uploaded an overlay
    bool mHasOverlay;

    struct EngagementData
    {
        // Original layer
        RopesLayerData OriginalLayerClone;

        // Original dirty state
        ModelDirtyState OriginalDirtyState;

        // Rope start position
        ShipSpaceCoordinates StartCoords;

        // Rope element index (if moving a rope endpoint), nullopt otherwise (if creating a rope)
        std::optional<size_t> ExistingRopeElementIndex;

        // Plane of the engagement
        MaterialPlaneType Plane;

        EngagementData(
            RopesLayerData && originalLayerClone,
            ModelDirtyState const & dirtyState,
            ShipSpaceCoordinates const & startCoords,
            std::optional<size_t> const & existingRopeElementIndex,
            MaterialPlaneType plane)
            : OriginalLayerClone(std::move(originalLayerClone))
            , OriginalDirtyState(dirtyState)
            , StartCoords(startCoords)
            , ExistingRopeElementIndex(existingRopeElementIndex)
            , Plane(plane)
        {}
    };

    // Engagement data - when set, it means we're engaged;
    // "being engaged" for this tool basically means that
    // the mouse button is down
    std::optional<EngagementData> mEngagementData;
};

}