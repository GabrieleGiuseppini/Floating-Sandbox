/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "EventRecorder.h"
#include "FishSpeciesDatabase.h"
#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "PerfStats.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLocator.h"
#include "ShipDefinition.h"
#include "ShipTexturizer.h"
#include "VisibleWorld.h"

#include <GameCore/AABBSet.h>
#include <GameCore/GameChronometer.h>
#include <GameCore/ImageData.h>
#include <GameCore/TaskThreadPool.h>
#include <GameCore/Vectors.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

namespace Physics
{

class World
{
public:

    World(
        OceanFloorTerrain && oceanFloorTerrain,
        FishSpeciesDatabase const & fishSpeciesDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::shared_ptr<TaskThreadPool> taskThreadPool,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    std::tuple<ShipId, RgbaImageData> AddShip(
        ShipDefinition && shipDefinition,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        GameParameters const & gameParameters);

    void Announce();

    void SetEventRecorder(EventRecorder * eventRecorder);

    void ReplayRecordedEvent(
        RecordedEvent const & event,
        GameParameters const & gameParameters);

    float GetCurrentSimulationTime() const
    {
        return mCurrentSimulationTime;
    }

    size_t GetShipCount() const;

    size_t GetShipPointCount(ShipId shipId) const;

    vec2f GetShipSize(ShipId shipId) const;

    inline float GetOceanSurfaceHeightAt(float x) const
    {
        return mOceanSurface.GetHeightAt(x);
    }

    inline bool IsUnderwater(vec2f const & position) const
    {
        return position.y < GetOceanSurfaceHeightAt(position.x);
    }

    bool IsUnderwater(ElementId elementId) const;

    inline void DisplaceOceanSurfaceAt(
        float x,
        float yOffset)
    {
        mOceanSurface.DisplaceAt(x, yOffset);
    }

    inline float GetOceanFloorHeightAt(float x) const noexcept
    {
        return mOceanFloor.GetHeightAt(x);
    }

    inline bool IsUnderOceanFloor(float x, float y) const noexcept
    {
        return mOceanFloor.IsUnderOceanFloor(x, y);
    }

    inline vec2f GetOceanFloorNormalAt(float x) const noexcept
    {
        return mOceanFloor.GetNormalAt(x);
    }

    inline void DisplaceOceanFloorAt(
        float x,
        float yOffset)
    {
        mOceanFloor.DisplaceAt(x, yOffset);
    }

    inline void DisturbOceanAt(
        vec2f const & position,
        float fishScareRadius,
        std::chrono::milliseconds delay)
    {
        mFishes.DisturbAt(
            position,
            fishScareRadius,
            delay);
    }

    inline void DisturbOcean(std::chrono::milliseconds delay)
    {
        mFishes.TriggerWidespreadPanic(delay);
    }

    inline vec2f const & GetCurrentWindSpeed() const
    {
        return mWind.GetCurrentWindSpeed();
    }

    inline void SetOceanFloorTerrain(OceanFloorTerrain const & oceanFloorTerrain)
    {
        mOceanFloor.SetTerrain(oceanFloorTerrain);
    }

    inline OceanFloorTerrain const & GetOceanFloorTerrain() const
    {
        return mOceanFloor.GetTerrain();
    }


    //
    // Interactions
    //

    void ScareFish(
        vec2f const & position,
        float radius,
        std::chrono::milliseconds delay);

    void AttractFish(
        vec2f const & position,
        float radius,
        std::chrono::milliseconds delay);

    void PickPointToMove(
        vec2f const & pickPosition,
        std::optional<ElementId> & elementId,
        GameParameters const & gameParameters) const;

    void MoveBy(
        ElementId elementId,
        vec2f const & offset,
        vec2f const & inertialVelocity,
        GameParameters const & gameParameters);

    void MoveBy(
        ShipId shipId,
        vec2f const & offset,
        vec2f const & inertialVelocity,
        GameParameters const & gameParameters);

    void RotateBy(
        ElementId elementId,
        float angle,
        vec2f const & center,
        float inertialAngle,
        GameParameters const & gameParameters);

    void RotateBy(
        ShipId shipId,
        float angle,
        vec2f const & center,
        float inertialAngle,
        GameParameters const & gameParameters);

    std::optional<ElementId> PickObjectForPickAndPull(
        vec2f const & pickPosition,
        GameParameters const & gameParameters);

    void Pull(
        ElementId elementId,
        vec2f const & target,
        GameParameters const & gameParameters);

    void DestroyAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        GameParameters const & gameParameters);

    void RepairAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        RepairSessionId sessionId,
        RepairSessionStepId sessionStepId,
        GameParameters const & gameParameters);

    void SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        GameParameters const & gameParameters);

    bool ApplyHeatBlasterAt(
        vec2f const & targetPos,
        HeatBlasterActionType action,
        float radius,
        GameParameters const & gameParameters);

    bool ExtinguishFireAt(
        vec2f const & targetPos,
        float radius,
        GameParameters const & gameParameters);

    void DrawTo(
        vec2f const & targetPos,
        float strengthFraction,
        GameParameters const & gameParameters);

    void SwirlAt(
        vec2f const & targetPos,
        float strengthFraction,
        GameParameters const & gameParameters);

    void TogglePinAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool InjectBubblesAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        GameParameters const & gameParameters);

    void ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    std::optional<bool> TogglePhysicsProbeAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    void DetonateAntiMatterBombs();

    void AdjustOceanSurfaceTo(std::optional<vec2f> const & worldCoordinates);

    std::optional<bool> AdjustOceanFloorTo(
        float x1,
        float targetY1,
        float x2,
        float targetY2);

    bool ScrubThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        GameParameters const & gameParameters);

    bool RotThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        GameParameters const & gameParameters);

    void ApplyThanosSnap(
        float centerX,
        float radius,
        float leftFrontX,
        float rightFrontX,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    std::optional<ElementId> GetNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void QueryNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    std::optional<vec2f> FindSuitableLightningTarget() const;

    void ApplyLightning(
        vec2f const & targetPos,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void TriggerTsunami();

    void TriggerRogueWave();

    void TriggerStorm();

    void TriggerLightning();

    void HighlightElectricalElement(ElectricalElementId electricalElementId);

    void SetSwitchState(
        ElectricalElementId electricalElementId,
        ElectricalState switchState,
        GameParameters const & gameParameters);

    void SetEngineControllerState(
        ElectricalElementId electricalElementId,
        int telegraphValue,
        GameParameters const & gameParameters);

    void SetSilence(float silenceAmount);

    bool DestroyTriangle(ElementId triangleId);

    bool RestoreTriangle(ElementId triangleId);

public:

    void Update(
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld,
        Render::RenderContext & renderContext,
        PerfStats & perfStats);

    void RenderUpload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext,
        PerfStats & perfStats);

private:

    // The current simulation time
    float mCurrentSimulationTime;

    // The game event handler
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // The current event recorder (if any)
    EventRecorder * mEventRecorder;

    // The task thread pool that we use for concurrency
    std::shared_ptr<TaskThreadPool> mTaskThreadPool;

    // Repository
    std::vector<std::unique_ptr<Ship>> mAllShips;
    Stars mStars;
    Storm mStorm;
    Wind mWind;
    Clouds mClouds;
    OceanSurface mOceanSurface;
    OceanFloor mOceanFloor;
    Fishes mFishes;

    // The set of all AABB's in the world, updated at each
    // simulation cycle
    Geometry::AABBSet mAllAABBs;
};

}
