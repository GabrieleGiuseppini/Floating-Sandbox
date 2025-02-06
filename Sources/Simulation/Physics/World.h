/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Physics.h"

#include "../EventRecorder.h"
#include "../FishSpeciesDatabase.h"
#include "../NpcDatabase.h"
#include "../ShipDefinition.h"
#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"

#include <Render/RenderContext.h>

#include <Core/AABBSet.h>
#include <Core/GameChronometer.h>
#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/ImageData.h>
#include <Core/PerfStats.h>
#include <Core/ThreadManager.h>
#include <Core/Vectors.h>

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
        OceanFloorHeightMap && oceanFloorHeightMap,
        bool areCloudShadowsEnabled,
        FishSpeciesDatabase const & fishSpeciesDatabase,
        NpcDatabase const & npcDatabase,
        std::shared_ptr<SimulationEventDispatcher> simulationEventDispatcher,
        SimulationParameters const & simulationParameters,
        VisibleWorld const & visibleWorld);

    ShipId GetNextShipId() const;

    void AddShip(std::unique_ptr<Ship> ship);

    void Announce();

    void SetEventRecorder(EventRecorder * eventRecorder);

    void ReplayRecordedEvent(
        RecordedEvent const & event,
        SimulationParameters const & simulationParameters);

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

    inline void SetOceanFloorTerrain(OceanFloorHeightMap const & oceanFloorHeightMap)
    {
        mOceanFloor.SetHeightMap(oceanFloorHeightMap);
    }

    inline OceanFloorHeightMap const & GetOceanFloorHeightMap() const
    {
        return mOceanFloor.GetHeightMap();
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
        ExplosionType explosionType,
        SimulationParameters const & simulationParameters);

    //
    // Interactions
    //

    void PickConnectedComponentToMove(
        vec2f const & pickPosition,
        std::optional<GlobalConnectedComponentId> & connectedComponentId,
        SimulationParameters const & simulationParameters) const;

    void MoveBy(
        GlobalConnectedComponentId connectedComponentId,
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        SimulationParameters const & simulationParameters);

    void MoveBy(
        ShipId shipId,
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        SimulationParameters const & simulationParameters);

    void RotateBy(
        GlobalConnectedComponentId connectedComponentId,
        float angle,
        vec2f const & center,
        float inertialAngle,
        SimulationParameters const & simulationParameters);

    void RotateBy(
        ShipId shipId,
        float angle,
        vec2f const & center,
        float inertialAngle,
        SimulationParameters const & simulationParameters);

    void MoveGrippedBy(
        vec2f const & gripCenter,
        float const gripRadius,
        vec2f const & moveOffset,
        vec2f const & inertialWorldOffset,
        SimulationParameters const & simulationParameters);

    void RotateGrippedBy(
        vec2f const & gripCenter,
        float const gripRadius,
        float angle,
        float inertialAngle,
        SimulationParameters const & simulationParameters);

    void EndMoveGrippedBy(SimulationParameters const & simulationParameters);

    std::optional<GlobalElementId> PickObjectForPickAndPull(
        vec2f const & pickPosition,
        SimulationParameters const & simulationParameters);

    void Pull(
        GlobalElementId elementId,
        vec2f const & target,
        SimulationParameters const & simulationParameters);

    void DestroyAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        SessionId const & sessionId,
        SimulationParameters const & simulationParameters);

    void RepairAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        SequenceNumber repairStepId,
        SimulationParameters const & simulationParameters);

    bool SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        bool isFirstSegment,
        SimulationParameters const & simulationParameters);

    bool ApplyHeatBlasterAt(
        vec2f const & targetPos,
        HeatBlasterActionType action,
        float radius,
        SimulationParameters const & simulationParameters);

    bool ExtinguishFireAt(
        vec2f const & targetPos,
        float strengthMultiplier,
        float radius,
        SimulationParameters const & simulationParameters);

    void ApplyBlastAt(
        vec2f const & targetPos,
        float radius,
        float forceMultiplier,
        SimulationParameters const & simulationParameters);

    bool ApplyElectricSparkAt(
        vec2f const & targetPos,
        std::uint64_t counter,
        float lengthMultiplier,
        SimulationParameters const & simulationParameters);

    void ApplyRadialWindFrom(
        vec2f const & sourcePos,
        float preFrontRadius,
        float preFrontWindSpeed, // m/s
        float mainFrontRadius,
        float mainFrontWindSpeed, // m/s
        SimulationParameters const & simulationParameters);

    bool ApplyLaserCannonThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        float strength,
        SimulationParameters const & simulationParameters);

    void DrawTo(
        vec2f const & targetPos,
        float strengthFraction,
        SimulationParameters const & simulationParameters);

    void SwirlAt(
        vec2f const & targetPos,
        float strengthFraction,
        SimulationParameters const & simulationParameters);

    void TogglePinAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void RemoveAllPins();

    std::optional<ToolApplicationLocus> InjectPressureAt(
        vec2f const & targetPos,
        float pressureQuantityMultiplier,
        SimulationParameters const & simulationParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        SimulationParameters const & simulationParameters);

    void ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void ToggleFireExtinguishingBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void ToggleImpactBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    std::optional<bool> TogglePhysicsProbeAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void ToggleRCBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void ToggleTimerBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void DetonateRCBombs(
        SimulationParameters const & simulationParameters);

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
        SimulationParameters const & simulationParameters);

    bool RotThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        SimulationParameters const & simulationParameters);

    void ApplyThanosSnap(
        float centerX,
        float radius,
        float leftFrontX,
        float rightFrontX,
        bool isSparseMode,
        SimulationParameters const & simulationParameters);

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
        SimulationParameters const & simulationParameters);

    void TriggerTsunami();

    void TriggerRogueWave();

    void TriggerStorm();

    void TriggerLightning(SimulationParameters const & simulationParameters);

    void HighlightElectricalElement(GlobalElectricalElementId electricalElementId);

    void SetSwitchState(
        GlobalElectricalElementId electricalElementId,
        ElectricalState switchState,
        SimulationParameters const & simulationParameters);

    void SetEngineControllerState(
        GlobalElectricalElementId electricalElementId,
        float controllerValue,
        SimulationParameters const & simulationParameters);

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
        SimulationParameters const & simulationParameters) const;

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
        SimulationParameters const & simulationParameters);

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
        SimulationParameters const & simulationParameters,
        VisibleWorld const & visibleWorld,
        StressRenderModeType stressRenderMode,
        ThreadManager & threadManager,
        PerfStats & perfStats);

    void RenderUpload(
        SimulationParameters const & simulationParameters,
        RenderContext & renderContext);

private:

    // The current simulation time
    float mCurrentSimulationTime;

    // The game event handler
    std::shared_ptr<SimulationEventDispatcher> mSimulationEventHandler;

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
