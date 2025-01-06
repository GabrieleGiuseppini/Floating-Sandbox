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
#include "NpcDatabase.h"
#include "PerfStats.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLocator.h"
#include "ShipDefinition.h"
#include "VisibleWorld.h"

#include <GameCore/AABBSet.h>
#include <GameCore/GameChronometer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ThreadManager.h>
#include <GameCore/Vectors.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace Physics
{

class World
{
public:

    World(
        OceanFloorTerrain && oceanFloorTerrain,
        bool areCloudShadowsEnabled,
        FishSpeciesDatabase const & fishSpeciesDatabase,
        NpcDatabase const & npcDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    ShipId GetNextShipId() const;

    void AddShip(std::unique_ptr<Ship> ship);

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

    Geometry::AABBSet GetAllShipAABBs() const
    {
        return mAllShipAABBs;
    }

    Npcs const & GetNpcs() const
    {
        assert(!!mNpcs);

        return *mNpcs;
    }

    Npcs & GetNpcs()
    {
        assert(!!mNpcs);

        return *mNpcs;
    }

    void SetAreCloudShadowsEnabled(bool value)
    {
        mClouds.SetShadowsEnabled(value);
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

    OceanSurface const & GetOceanSurface() const
    {
        return mOceanSurface;
    }

    bool IsUnderwater(GlobalElementId elementId) const;

    inline void DisplaceOceanSurfaceAt(
        float x,
        float yOffset)
    {
        mOceanSurface.DisplaceAt(x, yOffset);
    }

    OceanFloor const & GetOceanFloor() const
    {
        return mOceanFloor;
    }

    inline void SetOceanFloorTerrain(OceanFloorTerrain const & oceanFloorTerrain)
    {
        mOceanFloor.SetTerrain(oceanFloorTerrain);
    }

    inline OceanFloorTerrain const & GetOceanFloorTerrain() const
    {
        return mOceanFloor.GetTerrain();
    }

    // Km/h
    inline vec2f const & GetCurrentWindSpeed() const
    {
        return mWind.GetCurrentWindSpeed();
    }

    inline std::optional<Wind::RadialWindField> GetCurrentRadialWindField() const
    {
        return mWind.GetCurrentRadialWindField();
    }

    // Does secondary tasks after a blast has been applied to a ship or globally
    void OnBlast(
        ShipId shipId, // None if global
        vec2f const & centerPosition,
        float blastForceMagnitude, // N
        float blastForceRadius, // m
        float blastHeat, // KJ/s
        float blastHeatRadius, // m
        GameParameters const & gameParameters);

    //
    // Interactions
    //

    void PickConnectedComponentToMove(
        vec2f const & pickPosition,
        std::optional<GlobalConnectedComponentId> & connectedComponentId,
        GameParameters const & gameParameters) const;

    void MoveBy(
        GlobalConnectedComponentId connectedComponentId,
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        GameParameters const & gameParameters);

    void MoveBy(
        ShipId shipId,
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        GameParameters const & gameParameters);

    void RotateBy(
        GlobalConnectedComponentId connectedComponentId,
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

    void MoveGrippedBy(
        vec2f const & gripCenter,
        float const gripRadius,
        vec2f const & moveOffset,
        vec2f const & inertialWorldOffset,
        GameParameters const & gameParameters);

    void RotateGrippedBy(
        vec2f const & gripCenter,
        float const gripRadius,
        float angle,
        float inertialAngle,
        GameParameters const & gameParameters);

    void EndMoveGrippedBy(GameParameters const & gameParameters);

    std::optional<GlobalElementId> PickObjectForPickAndPull(
        vec2f const & pickPosition,
        GameParameters const & gameParameters);

    void Pull(
        GlobalElementId elementId,
        vec2f const & target,
        GameParameters const & gameParameters);

    void DestroyAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        SessionId const & sessionId,
        GameParameters const & gameParameters);

    void RepairAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        SequenceNumber repairStepId,
        GameParameters const & gameParameters);

    bool SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        bool isFirstSegment,
        GameParameters const & gameParameters);

    bool ApplyHeatBlasterAt(
        vec2f const & targetPos,
        HeatBlasterActionType action,
        float radius,
        GameParameters const & gameParameters);

    bool ExtinguishFireAt(
        vec2f const & targetPos,
        float strengthMultiplier,
        float radius,
        GameParameters const & gameParameters);

    void ApplyBlastAt(
        vec2f const & targetPos,
        float radius,
        float forceMultiplier,
        GameParameters const & gameParameters);

    bool ApplyElectricSparkAt(
        vec2f const & targetPos,
        std::uint64_t counter,
        float lengthMultiplier,
        GameParameters const & gameParameters);

    void ApplyRadialWindFrom(
        vec2f const & sourcePos,
        float preFrontRadius,
        float preFrontWindSpeed, // m/s
        float mainFrontRadius,
        float mainFrontWindSpeed, // m/s
        GameParameters const & gameParameters);

    bool ApplyLaserCannonThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        float strength,
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

    void RemoveAllPins();

    std::optional<ToolApplicationLocus> InjectPressureAt(
        vec2f const & targetPos,
        float pressureQuantityMultiplier,
        GameParameters const & gameParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        GameParameters const & gameParameters);

    void ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleFireExtinguishingBombAt(
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

    void DetonateRCBombs(
        GameParameters const & gameParameters);

    void DetonateAntiMatterBombs();

    void AdjustOceanSurfaceTo(
        vec2f const & worldCoordinates,
        float worldRadius);

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
        bool isSparseMode,
        GameParameters const & gameParameters);

    std::optional<GlobalElementId> GetNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void QueryNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void QueryNearestNpcAt(
        vec2f const & targetPos,
        float radius) const;

    std::optional<vec2f> FindSuitableLightningTarget() const;

    void ApplyLightning(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void TriggerTsunami();

    void TriggerRogueWave();

    void TriggerStorm();

    void TriggerLightning(GameParameters const & gameParameters);

    void HighlightElectricalElement(GlobalElectricalElementId electricalElementId);

    void SetSwitchState(
        GlobalElectricalElementId electricalElementId,
        ElectricalState switchState,
        GameParameters const & gameParameters);

    void SetEngineControllerState(
        GlobalElectricalElementId electricalElementId,
        float controllerValue,
        GameParameters const & gameParameters);

    void SetSilence(float silenceAmount);

    void ScareFish(
        vec2f const & position,
        float radius,
        std::chrono::milliseconds delay);

    void AttractFish(
        vec2f const & position,
        float radius,
        std::chrono::milliseconds delay);

    NpcKindType GetNpcKind(NpcId id);

    std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> BeginPlaceNewFurnitureNpc(
        std::optional<NpcSubKindIdType> subKind,
        vec2f const & position,
        bool doMoveWholeMesh);

    std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> BeginPlaceNewHumanNpc(
        std::optional<NpcSubKindIdType> subKind,
        vec2f const & position,
        bool doMoveWholeMesh);

    std::optional<PickedNpc> ProbeNpcAt(
        vec2f const & position,
        float radius,
        GameParameters const & gameParameters) const;

    std::vector<NpcId> ProbeNpcsInRect(
        vec2f const & corner1,
        vec2f const & corner2) const;

    void BeginMoveNpc(
        NpcId id,
        int particleOrdinal,
        bool doMoveWholeMesh);

    void BeginMoveNpcs(std::vector<NpcId> const & ids);

    void MoveNpcTo(
        NpcId id,
        vec2f const & position,
        vec2f const & offset,
        bool doMoveWholeMesh);

    void MoveNpcsBy(
        std::vector<NpcId> const & ids,
        vec2f const & stride);

    void EndMoveNpc(NpcId id);

    void CompleteNewNpc(NpcId id);

    void RemoveNpc(NpcId id);

    void RemoveNpcsInRect(
        vec2f const & corner1,
        vec2f const & corner2);

    void AbortNewNpc(NpcId id);

    std::tuple<std::optional<NpcId>, NpcCreationFailureReasonType> AddNpcGroup(
        NpcKindType kind,
        VisibleWorld const & visibleWorld,
        GameParameters const & gameParameters);

    void TurnaroundNpc(NpcId id);

    void TurnaroundNpcsInRect(
        vec2f const & corner1,
        vec2f const & corner2);

    std::optional<NpcId> GetSelectedNpc() const;

    void SelectFirstNpc();

    void SelectNextNpc();

    void SelectNpc(std::optional<NpcId> id);

    void HighlightNpcs(std::vector<NpcId> const & ids);

    void HighlightNpcsInRect(
        vec2f const & corner1,
        vec2f const & corner2);

    bool DestroyTriangle(GlobalElementId triangleId);

    bool RestoreTriangle(GlobalElementId triangleId);

public:

    void Update(
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld,
        StressRenderModeType stressRenderMode,
        ThreadManager & threadManager,
        PerfStats & perfStats);

    void RenderUpload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext);

private:

    // The current simulation time
    float mCurrentSimulationTime;

    // The game event handler
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // The current event recorder (if any)
    EventRecorder * mEventRecorder;

    // Repository
    std::vector<std::unique_ptr<Ship>> mAllShips;
    Stars mStars;
    Storm mStorm;
    Wind mWind;
    Clouds mClouds;
    OceanSurface mOceanSurface;
    OceanFloor mOceanFloor;
    Fishes mFishes;
    std::unique_ptr<Npcs> mNpcs; // Pointer simply because of #include dependencies

    // The set of all ship AABB's in the world, updated at each
    // simulation cycle and at each ship addition
    Geometry::AABBSet mAllShipAABBs;
};

}
