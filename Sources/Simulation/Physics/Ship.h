/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Physics.h"

#include "ShipElectricSparks.h"

#include "../EventRecorder.h"
#include "../MaterialDatabase.h"
#include "../ShipDefinition.h"
#include "../ShipOverlays.h"
#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"

#include <Render/RenderContext.h>

#include <Core/AABBSet.h>
#include <Core/Buffer.h>
#include <Core/GameTypes.h>
#include <Core/ImageData.h>
#include <Core/PerfStats.h>
#include <Core/RunningAverage.h>
#include <Core/ThreadManager.h>
#include <Core/Vectors.h>

#include <list>
#include <memory>
#include <optional>
#include <vector>

namespace Physics
{

class Ship final : public IShipPhysicsHandler
{
public:

    Ship(
        ShipId id,
        World & parentWorld,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<SimulationEventDispatcher> simulationEventDispatcher,
        Points && points,
        Springs && springs,
        Triangles && triangles,
        ElectricalElements && electricalElements,
        Frontiers && frontiers,
        RgbaImageData && interiorTextureImage);

    void Announce();

    ShipId GetId() const { return mId; }

    World const & GetParentWorld() const { return mParentWorld; }
    World & GetParentWorld() { return mParentWorld; }

    Geometry::AABBSet CalculateAABBs() const;

    PlaneId GetMaxPlaneId() const
    {
        return mMaxMaxPlaneId;
    }

    size_t GetPointCount() const { return mPoints.GetElementCount(); }

    Points const & GetPoints() const { return mPoints; }
    Points & GetPoints() { return mPoints; }

    Springs const & GetSprings() const { return mSprings; }

    Triangles const & GetTriangles() const { return mTriangles; }

    bool IsUnderwater(ElementIndex pointElementIndex) const
    {
        return mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex));
    }

    bool AreBombsInProximity(ElementIndex pointElementIndex) const
    {
        return mGadgets.AreBombsInProximity(mPoints.GetPosition(pointElementIndex));
    }

    void SetEventRecorder(EventRecorder * eventRecorder);

    bool ReplayRecordedEvent(
        RecordedEvent const & event,
        SimulationParameters const & simulationParameters);

    void Update(
        float currentSimulationTime,
		Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters,
        StressRenderModeType stressRenderMode,
        Geometry::AABBSet & externalAabbSet,
        ThreadManager & threadManager,
        PerfStats & perfStats);

    void UpdateEnd();

    void RenderUpload(RenderContext & renderContext);

public:

    void Finalize();

    ///////////////////////////////////////////////////////////////
    // Interactions
    ///////////////////////////////////////////////////////////////

    std::optional<ConnectedComponentId> PickConnectedComponentToMove(
        vec2f const & pickPosition,
        SimulationParameters const & simulationParameters) const;

    void MoveBy(
        ConnectedComponentId connectedComponentId,
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        SimulationParameters const & simulationParameters);

    void MoveBy(
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        SimulationParameters const & simulationParameters);

    void RotateBy(
        ConnectedComponentId connectedComponentId,
        float angle,
        vec2f const & center,
        float inertialAngle,
        SimulationParameters const & simulationParameters);

    void RotateBy(
        float angle,
        vec2f const & center,
        float inertialAngle,
        SimulationParameters const & simulationParameters);

    void MoveGrippedBy(
        vec2f const & gripCenter,
        float const gripRadius,
        vec2f const & moveOffset,
        vec2f const & inertialVelocity,
        SimulationParameters const & simulationParameters);

    void RotateGrippedBy(
        vec2f const & gripCenter,
        float const gripRadius,
        float angle,
        float inertialAngle,
        SimulationParameters const & simulationParameters);

    void EndMoveGrippedBy(SimulationParameters const & simulationParameters);

    std::optional<ElementIndex> PickObjectForPickAndPull(
        vec2f const & pickPosition,
        SimulationParameters const & simulationParameters);

    void Pull(
        ElementIndex pointElementIndex,
        vec2f const & target,
        SimulationParameters const & simulationParameters);

    bool DestroyAt(
        vec2f const & targetPos,
        float radius,
        SessionId const & sessionId,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void RepairAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        SequenceNumber repairStepId,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    bool SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        bool isFirstSegment,
        float currentSimulationTime,
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
        float forceMagnitude,
        SimulationParameters const & simulationParameters);

    bool ApplyElectricSparkAt(
        vec2f const & targetPos,
        std::uint64_t counter,
        float lengthMultiplier,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    bool ApplyLaserCannonThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        float strength,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void DrawTo(
        vec2f const & targetPos,
        float strength);

    void SwirlAt(
        vec2f const & targetPos,
        float strength);

    bool TogglePinAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void RemoveAllPins();

    std::optional<ToolApplicationLocus> InjectBubblesAt(
        vec2f const & targetPos,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    std::optional<ToolApplicationLocus> InjectPressureAt(
        vec2f const & targetPos,
        float pressureQuantityMultiplier,
        SimulationParameters const & simulationParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        SimulationParameters const & simulationParameters);

    bool ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    bool ToggleFireExtinguishingBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    bool ToggleImpactBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    std::optional<bool> TogglePhysicsProbeAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void RemovePhysicsProbe();

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    bool ToggleTimerBombAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters);

    void DetonateRCBombs(
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void DetonateAntiMatterBombs();

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
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    ElementIndex GetNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    bool QueryNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

	std::optional<vec2f> FindSuitableLightningTarget() const;

	void ApplyLightning(
		vec2f const & targetPos,
		float currentSimulationTime,
		SimulationParameters const & simulationParameters);

    void HighlightElectricalElement(GlobalElectricalElementId electricalElementId);

    void SetSwitchState(
        GlobalElectricalElementId electricalElementId,
        ElectricalState switchState,
        SimulationParameters const & simulationParameters);

    void SetEngineControllerState(
        GlobalElectricalElementId electricalElementId,
        float controllerValue,
        SimulationParameters const & simulationParameters);

    void SpawnAirBubble(
        vec2f const & position,
        float finalScale, // Relative to texture's world dimensions
        float temperature,
        float currentSimulationTime,
        PlaneId planeId,
        SimulationParameters const & simulationParameters);

    bool DestroyTriangle(ElementIndex triangleIndex);

    bool RestoreTriangle(ElementIndex triangleIndex);

private:

    // Queued interactions

    struct Interaction
    {
        enum class InteractionType
        {
            Blast,
            Draw,
            Pull,
            Swirl
        };

        InteractionType Type;

        union ArgumentsUnion
        {
            struct BlastArguments
            {
                vec2f CenterPos;
                float Radius;
                float ForceMagnitude;

                BlastArguments(
                    vec2f const & centerPos,
                    float radius,
                    float forceMagnitude)
                    : CenterPos(centerPos)
                    , Radius(radius)
                    , ForceMagnitude(forceMagnitude)
                {}
            };

            BlastArguments Blast;

            struct DrawArguments
            {
                vec2f CenterPos;
                float Strength;

                DrawArguments(
                    vec2f const & centerPos,
                    float strength)
                    : CenterPos(centerPos)
                    , Strength(strength)
                {}
            };

            DrawArguments Draw;

            struct PullArguments
            {
                ElementIndex PointIndex;
                vec2f TargetPos;
                float Stiffness;

                PullArguments(
                    ElementIndex pointIndex,
                    vec2f const & targetPos,
                    float stiffness)
                    : PointIndex(pointIndex)
                    , TargetPos(targetPos)
                    , Stiffness(stiffness)
                {}
            };

            PullArguments Pull;

            struct SwirlArguments
            {
                vec2f CenterPos;
                float Strength;

                SwirlArguments(
                    vec2f const & centerPos,
                    float strength)
                    : CenterPos(centerPos)
                    , Strength(strength)
                {}
            };

            SwirlArguments Swirl;

            ArgumentsUnion(BlastArguments blast)
                : Blast(blast)
            {}

            ArgumentsUnion(DrawArguments draw)
                : Draw(draw)
            {}

            ArgumentsUnion(PullArguments pull)
                : Pull(pull)
            {}

            ArgumentsUnion(SwirlArguments swirl)
                : Swirl(swirl)
            {}

        } Arguments;

        Interaction(ArgumentsUnion::BlastArguments blast)
            : Type(InteractionType::Blast)
            , Arguments(blast)
        {
        }

        Interaction(ArgumentsUnion::DrawArguments draw)
            : Type(InteractionType::Draw)
            , Arguments(draw)
        {
        }

        Interaction(ArgumentsUnion::PullArguments pull)
            : Type(InteractionType::Pull)
            , Arguments(pull)
        {
        }

        Interaction(ArgumentsUnion::SwirlArguments swirl)
            : Type(InteractionType::Swirl)
            , Arguments(swirl)
        {
        }
    };

    std::list<Interaction> mQueuedInteractions;

    void ApplyBlastAt(Interaction::ArgumentsUnion::BlastArguments const & args, SimulationParameters const & simulationParameters);

    void DrawTo(Interaction::ArgumentsUnion::DrawArguments const & args);

    void Pull(Interaction::ArgumentsUnion::PullArguments const & args);

    void SwirlAt(Interaction::ArgumentsUnion::SwirlArguments const & args);

private:

    /////////////////////////////////////////////////////////////////////////
    // Dynamics
    /////////////////////////////////////////////////////////////////////////

    // Mechanical

    void ApplyQueuedInteractionForces(SimulationParameters const & simulationParameters);

    void ApplyWorldForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        SimulationParameters const & simulationParameters,
        Geometry::AABBSet & externalAabbSet);

    void ApplyWorldParticleForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        Buffer<float> & newCachedPointDepths,
        SimulationParameters const & simulationParameters);

    template<bool DoDisplaceWater>
    void ApplyWorldSurfaceForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        Buffer<float> & newCachedPointDepths,
        SimulationParameters const & simulationParameters,
        Geometry::AABBSet & externalAabbSet);

    void ApplyStaticPressureForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        SimulationParameters const & simulationParameters);

    void ApplyStaticPressureForces(
        Frontiers::Frontier const & frontier,
        float effectiveAirDensity,
        float effectiveWaterDensity,
        SimulationParameters const & simulationParameters);

    void RecalculateSpringRelaxationParallelism(size_t simulationParallelism, SimulationParameters const & simulationParameters);
    void RecalculateSpringRelaxationSpringForcesParallelism(size_t simulationParallelism);
    void RecalculateSpringRelaxationIntegrationAndSeaFloorCollisionParallelism(size_t simulationParallelism, SimulationParameters const & simulationParameters);

    void RunSpringRelaxationAndDynamicForcesIntegration(
        SimulationParameters const & simulationParameters,
        ThreadManager & threadManager);

    void ApplySpringsForces(
        ElementIndex startSpringIndex,
        ElementIndex endSpringIndex,
        vec2f * restrict dynamicForceBuffer);

    inline void IntegrateAndResetDynamicForces(
        ElementIndex startPointIndex,
        ElementIndex endPointIndex,
        SimulationParameters const & simulationParameters);

    inline float CalculateIntegrationVelocityFactor(float dt, SimulationParameters const & simulationParameters) const;
    inline void IntegrateAndResetDynamicForces_1(ElementIndex startPointIndex, ElementIndex endPointIndex, SimulationParameters const & simulationParameters);
    inline void IntegrateAndResetDynamicForces_2(ElementIndex startPointIndex, ElementIndex endPointIndex, SimulationParameters const & simulationParameters);
    inline void IntegrateAndResetDynamicForces_3(ElementIndex startPointIndex, ElementIndex endPointIndex, SimulationParameters const & simulationParameters);
    inline void IntegrateAndResetDynamicForces_4(ElementIndex startPointIndex, ElementIndex endPointIndex, SimulationParameters const & simulationParameters);
    inline void IntegrateAndResetDynamicForces_N(size_t parallelism, ElementIndex startPointIndex, ElementIndex endPointIndex, SimulationParameters const & simulationParameters);

    void HandleCollisionsWithSeaFloor(
        ElementIndex startPointIndex,
        ElementIndex endPointIndex,
        SimulationParameters const & simulationParameters);

    void TrimForWorldBounds(SimulationParameters const & simulationParameters);

    // Pressure and water

    void UpdatePressureAndWaterInflow(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters,
        float & waterTakenInStep);

    void EqualizeInternalPressure(SimulationParameters const & simulationParameters);

    void UpdateWaterVelocities(
        SimulationParameters const & simulationParameters,
        float & waterSplashed);

    void UpdateSinking(float currentSimulationTime);

    // Electrical

    void RecalculateLightDiffusionParallelism(size_t simulationParallelism);

    void DiffuseLight(
        SimulationParameters const & simulationParameters,
        ThreadManager & threadManager);

    // Heat

    void PropagateHeat(
        float currentSimulationTime,
        float dt,
		Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters);

    // Misc

    void RotPoints(
        ElementIndex partition,
        ElementIndex partitionCount,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void UpdateStateMachines(
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void UploadStateMachines(RenderContext & renderContext);

private:

private:

    /////////////////////////////////////////////////////////////////////////
    // Misc
    /////////////////////////////////////////////////////////////////////////

    inline void UpdateForSimulationParallelism(
        SimulationParameters const & simulationParameters,
        ThreadManager & threadManager);

    void RunConnectivityVisit();

    inline void SetAndPropagateResultantPointHullness(
        ElementIndex pointElementIndex,
        bool isHull);

    void DestroyConnectedTriangles(ElementIndex pointElementIndex);

    void DestroyConnectedTriangles(
        ElementIndex pointAElementIndex,
        ElementIndex pointBElementIndex);

    void AttemptPointRestore(
        ElementIndex pointElementIndex,
        float currentSimulationTime);

    void InternalSpawnAirBubble(
        vec2f const & position,
        float depth,
        float finalScale, // Relative to texture's world dimensions
        float temperature,
        float currentSimulationTime,
        PlaneId planeId,
        SimulationParameters const & simulationParameters);

    void InternalSpawnDebris(
        ElementIndex sourcePointElementIndex,
        StructuralMaterial const & debrisStructuralMaterial,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void InternalSpawnSparklesForCut(
        ElementIndex springElementIndex,
        vec2f const & cutDirectionStartPos,
        vec2f const & cutDirectionEndPos,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

	void InternalSpawnSparklesForLightning(
		ElementIndex pointElementIndex,
		float currentSimulationTime,
		SimulationParameters const & simulationParameters);

    inline size_t GetPointConnectedComponentSize(ElementIndex pointIndex) const noexcept
    {
        auto const connCompId = mPoints.GetConnectedComponentId(pointIndex);
        if (NoneConnectedComponentId == connCompId)
            return 0;

        return mConnectedComponentSizes[static_cast<size_t>(connCompId)];
    }

    inline void DetachPointForDestroy(
        ElementIndex pointIndex,
        vec2f const & detachVelocity,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters)
    {
        mPoints.Detach(
            pointIndex,
            detachVelocity,
            Points::DetachOptions::GenerateDebris
            | Points::DetachOptions::FireDestroyEvent,
            currentSimulationTime,
            simulationParameters);
    }

public:

    /////////////////////////////////////////////////////////////////////////
    // IShipPhysicsHandler
    /////////////////////////////////////////////////////////////////////////

    void HandlePointDetach(
        ElementIndex pointElementIndex,
        bool generateDebris,
        bool fireDestroyEvent,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

    void HandlePointDamaged(ElementIndex pointElementIndex) override;

    void HandleEphemeralParticleDestroy(
        ElementIndex pointElementIndex) override;

    void HandlePointRestore(
        ElementIndex pointElementIndex,
        float currentSimulationTime) override;

    void HandleSpringDestroy(
        ElementIndex springElementIndex,
        bool destroyAllTriangles,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

    void HandleSpringRestore(
        ElementIndex springElementIndex,
        SimulationParameters const & simulationParameters) override;

    void HandleTriangleDestroy(ElementIndex triangleElementIndex) override;

    void HandleTriangleRestore(ElementIndex triangleElementIndex) override;

    void HandleElectricalElementDestroy(
        ElementIndex electricalElementIndex,
        ElementIndex pointIndex,
        ElectricalElementDestroySpecializationType specialization,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

    void HandleElectricalElementRestore(ElementIndex electricalElementIndex) override;

    void StartExplosion(
        float currentSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float blastForce,
        float blastForceRadius,
        float blastHeat,
        float blastHeatRadius,
        float renderRadiusOffset,
        ExplosionType explosionType,
        SimulationParameters const & simulationParameters) override;

    void DoAntiMatterBombPreimplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        float radius,
        SimulationParameters const & simulationParameters) override;

    void DoAntiMatterBombImplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        SimulationParameters const & simulationParameters) override;

    void DoAntiMatterBombExplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        SimulationParameters const & simulationParameters) override;

    void HandleWatertightDoorUpdated(
        ElementIndex pointElementIndex,
        bool isOpen) override;

    void HandleElectricSpark(
        ElementIndex pointElementIndex,
        float strength,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

private:

#ifdef _DEBUG
    void VerifyInvariants();
#endif

private:

    /////////////////////////////////////////////////////////////////////////
    // State machines
    /////////////////////////////////////////////////////////////////////////

    enum class StateMachineType
    {
        Explosion
    };

    struct StateMachine
    {
    public:

        StateMachineType const Type;

        StateMachine(StateMachineType type)
            : Type(type)
        {}

        virtual ~StateMachine()
        {}
    };

    struct ExplosionStateMachine;

    inline bool UpdateExplosionStateMachine(
        ExplosionStateMachine & explosionStateMachine,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    template<bool DoExtinguishFire, bool DoDetachNearestPoint>
    inline void InternalUpdateExplosionStateMachine(
        ExplosionStateMachine & explosionStateMachine,
        float explosionBlastForceProgress,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    inline void UploadExplosionStateMachine(
        ExplosionStateMachine const & explosionStateMachine,
        RenderContext & renderContext);

    std::list<std::unique_ptr<StateMachine>> mStateMachines;

private:

    /////////////////////////////////////////////////////////////////////////
    // Interaction Helpers
    /////////////////////////////////////////////////////////////////////////

    void StraightenOneSpringChains(ElementIndex pointIndex);

    void StraightenTwoSpringChains(ElementIndex pointIndex);

    bool TryRepairAndPropagateFromPoint(
        ElementIndex startingPointIndex,
        vec2f const & targetPos,
        float squareSearchRadius,
        SequenceNumber repairStepId,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    bool RepairFromAttractor(
        ElementIndex attractorPointIndex,
        float repairStrength,
        SequenceNumber repairStepId,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

private:

    ShipId const mId;
    World & mParentWorld;
    MaterialDatabase const & mMaterialDatabase;
    std::shared_ptr<SimulationEventDispatcher> mSimulationEventHandler;
    EventRecorder * mEventRecorder;

    // All the ship elements - never removed, the repositories maintain their own size forever
    Points mPoints;
    Springs mSprings;
    Triangles mTriangles;
    ElectricalElements mElectricalElements;
    Frontiers mFrontiers;
    RgbaImageData mInteriorTextureImage;

    // Pinned points
    PinnedPoints mPinnedPoints;

    // Gadgets
    Gadgets mGadgets;

    // Electric sparks
    ShipElectricSparks mElectricSparks;

    // Overlays
    ShipOverlays mOverlays;

    // The current simulation sequence number
    SequenceNumber mCurrentSimulationSequenceNumber;

    // The current connectivity visit sequence number
    SequenceNumber mCurrentConnectivityVisitSequenceNumber;

    // The max plane ID we have seen - ever
    PlaneId mMaxMaxPlaneId;

    // The current electrical connectivity visit sequence number
    SequenceNumber mCurrentElectricalVisitSequenceNumber;

    // The number of points in each connected component
    std::vector<size_t> mConnectedComponentSizes;

    // Flag remembering whether the structure of the ship (i.e. the connectivity between elements)
    // has changed since the last step.
    // When this flag is set, we'll re-detect connected components and planes, and re-upload elements
    // to the rendering context
    bool mIsStructureDirty;

    // Counts of elements currently broken - updated each time an element is broken
    // or restored
    ElementCount mDamagedPointsCount;
    ElementCount mBrokenSpringsCount;
    ElementCount mBrokenTrianglesCount;

    // Sinking detection
    bool mIsSinking;

    // Water splashes
    RunningAverage<30> mWaterSplashedRunningAverage;

    // Last luminiscence adjustment that we've run the light diffusion algorithm with;
    // used to avoid running diffusion when luminiscence adjustment is zero and we've
    // already ran once with zero (so to zero out buffer)
    float mLastLuminiscenceAdjustmentDiffused;

    // Normally at 1.0, set to 0.0 during repair to turn off updates that hinder the
    // repair process
    float mRepairGracePeriodMultiplier;

    // Index of last-queried point - used as an aid to debugging
    ElementIndex mutable mLastQueriedPointIndex;

    // Counter of created bubble ephemeral particles
    std::uint64_t mAirBubblesCreatedCount;

    // The last thread pool simulation parallelism we've seen; used to
    // detect changes
    size_t mCurrentSimulationParallelism;

    //
    // Spring relaxation
    //

    // The spring relaxation tasks
    std::vector<typename ThreadPool::Task> mSpringRelaxationSpringForcesTasks;
    std::vector<typename ThreadPool::Task> mSpringRelaxationIntegrationTasks;
    std::vector<typename ThreadPool::Task> mSpringRelaxationIntegrationAndSeaFloorCollisionTasks;

    //
    // Static pressure
    //

    struct StaticPressureOnPoint
    {
        ElementIndex PointIndex;
        vec2f ForceVector;
        vec2f TorqueArm;

        StaticPressureOnPoint(
            ElementIndex pointIndex,
            vec2f const & forceVector,
            vec2f const & torqueArm)
            : PointIndex(pointIndex)
            , ForceVector(forceVector)
            , TorqueArm(torqueArm)
        {}
    };

    // Buffer of StaticPressureOnPoint structs, aiding static pressure calculations.
    //
    // Note: index in this buffer is _not_ point index, this is simply a container.
    // Note: may be populated for the same point multiple times, once for each crossing of
    // the frontier through that point.
    Buffer<StaticPressureOnPoint> mStaticPressureBuffer;

    // For statistics
    float mStaticPressureNetForceMagnitudeSum;
    float mStaticPressureNetForceMagnitudeCount;
    float mStaticPressureIterationsPercentagesSum;
    float mStaticPressureIterationsCount;

    //
    // Light diffusion
    //

    // The light diffusion tasks
    std::vector<typename ThreadPool::Task> mLightDiffusionTasks;

    //
    // Render members
    //

    // The debug ship render mode that was in effect the last time we've uploaded elements;
    // used to detect changes and eventually re-upload
    std::optional<DebugShipRenderModeType> mLastUploadedDebugShipRenderMode;

    // Initial indices of the triangles for each plane ID;
    // last extra element contains total number of triangles
    std::vector<size_t> mPlaneTriangleIndicesToRender;
};

}