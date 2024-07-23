/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "EventRecorder.h"
#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "PerfStats.h"
#include "RenderContext.h"
#include "ShipDefinition.h"
#include "ShipElectricSparks.h"
#include "ShipOverlays.h"

#include <GameCore/AABBSet.h>
#include <GameCore/Buffer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/RunningAverage.h>
#include <GameCore/ThreadManager.h>
#include <GameCore/Vectors.h>

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
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        Points && points,
        Springs && springs,
        Triangles && triangles,
        ElectricalElements && electricalElements,
        Frontiers && frontiers,
        RgbaImageData && interiorTextureImage);

    void Announce();

    inline ShipId GetId() const { return mId; }

    inline World const & GetParentWorld() const { return mParentWorld; }
    inline World & GetParentWorld() { return mParentWorld; }

    Geometry::AABBSet CalculateAABBs() const;

    PlaneId GetMaxPlaneId() const
    {
        return mMaxMaxPlaneId;
    }

    inline size_t GetPointCount() const { return mPoints.GetElementCount(); }

    inline Points const & GetPoints() const { return mPoints; }
    inline Points & GetPoints() { return mPoints; }

    inline Springs const & GetSprings() const { return mSprings; }

    inline Triangles const & GetTriangles() const { return mTriangles; }

    bool IsUnderwater(ElementIndex pointElementIndex) const
    {
        return mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex));
    }

    void SetEventRecorder(EventRecorder * eventRecorder);

    bool ReplayRecordedEvent(
        RecordedEvent const & event,
        GameParameters const & gameParameters);

    void Update(
        float currentSimulationTime,
		Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters,
        StressRenderModeType stressRenderMode,
        Geometry::AABBSet & externalAabbSet,
        ThreadManager & threadManager,
        PerfStats & perfStats);

    void RenderUpload(Render::RenderContext & renderContext);

public:

    void Finalize();

    ///////////////////////////////////////////////////////////////
    // Interactions
    ///////////////////////////////////////////////////////////////

    std::optional<ConnectedComponentId> PickConnectedComponentToMove(
        vec2f const & pickPosition,
        GameParameters const & gameParameters) const;

    void MoveBy(
        ConnectedComponentId connectedComponentId,
        vec2f const & offset,
        vec2f const & inertialVelocity,
        GameParameters const & gameParameters);

    void MoveBy(
        vec2f const & offset,
        vec2f const & inertialVelocity,
        GameParameters const & gameParameters);

    void RotateBy(
        ConnectedComponentId connectedComponentId,
        float angle,
        vec2f const & center,
        float inertialAngle,
        GameParameters const & gameParameters);

    void RotateBy(
        float angle,
        vec2f const & center,
        float inertialAngle,
        GameParameters const & gameParameters);

    std::optional<ElementIndex> PickObjectForPickAndPull(
        vec2f const & pickPosition,
        GameParameters const & gameParameters);

    void Pull(
        ElementIndex pointElementIndex,
        vec2f const & target,
        GameParameters const & gameParameters);

    bool DestroyAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void RepairAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        SequenceNumber repairStepId,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    bool SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        bool isFirstSegment,
        float currentSimulationTime,
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

    void ApplyBlastAt(
        vec2f const & targetPos,
        float radius,
        float forceMultiplier,
        GameParameters const & gameParameters);

    bool ApplyElectricSparkAt(
        vec2f const & targetPos,
        std::uint64_t counter,
        float lengthMultiplier,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void ApplyRadialWindFrom(
        vec2f const & sourcePos,
        float preFrontRadius,
        float preFrontWindSpeed,
        float mainFrontRadius,
        float mainFrontWindSpeed,
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

    bool TogglePinAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void RemoveAllPins();

    std::optional<ToolApplicationLocus> InjectBubblesAt(
        vec2f const & targetPos,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    std::optional<ToolApplicationLocus> InjectPressureAt(
        vec2f const & targetPos,
        float pressureQuantityMultiplier,
        GameParameters const & gameParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        GameParameters const & gameParameters);

    bool ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    std::optional<bool> TogglePhysicsProbeAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void RemovePhysicsProbe();

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    void DetonateAntiMatterBombs();

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
        float currentSimulationTime,
        GameParameters const & gameParameters);

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
		GameParameters const & gameParameters);

    void HighlightElectricalElement(GlobalElectricalElementId electricalElementId);

    void SetSwitchState(
        GlobalElectricalElementId electricalElementId,
        ElectricalState switchState,
        GameParameters const & gameParameters);

    void SetEngineControllerState(
        GlobalElectricalElementId electricalElementId,
        float controllerValue,
        GameParameters const & gameParameters);

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
            Swirl,
            RadialWind
        };

        InteractionType Type;

        union ArgumentsUnion
        {
            struct BlastArguments
            {
                vec2f CenterPos;
                float Radius;
                float Magnitude;

                BlastArguments(
                    vec2f const & centerPos,
                    float radius,
                    float magnitude)
                    : CenterPos(centerPos)
                    , Radius(radius)
                    , Magnitude(magnitude)
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

            struct RadialWindArguments
            {
                vec2f SourcePos;
                float PreFrontRadius;
                float PreFrontWindForceMagnitude;
                float MainFrontRadius;
                float MainFrontWindForceMagnitude;

                RadialWindArguments(
                    vec2f sourcePos,
                    float preFrontRadius,
                    float preFrontWindForceMagnitude,
                    float mainFrontRadius,
                    float mainFrontWindForceMagnitude)
                    : SourcePos(sourcePos)
                    , PreFrontRadius(preFrontRadius)
                    , PreFrontWindForceMagnitude(preFrontWindForceMagnitude)
                    , MainFrontRadius(mainFrontRadius)
                    , MainFrontWindForceMagnitude(mainFrontWindForceMagnitude)
                {}
            };

            RadialWindArguments RadialWind;

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

            ArgumentsUnion(RadialWindArguments radialWind)
                : RadialWind(radialWind)
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

        Interaction(ArgumentsUnion::RadialWindArguments radialWind)
            : Type(InteractionType::RadialWind)
            , Arguments(radialWind)
        {
        }
    };

    std::list<Interaction> mQueuedInteractions;

    void ApplyBlastAt(Interaction::ArgumentsUnion::BlastArguments const & args);

    void DrawTo(Interaction::ArgumentsUnion::DrawArguments const & args);

    void Pull(Interaction::ArgumentsUnion::PullArguments const & args);

    void SwirlAt(Interaction::ArgumentsUnion::SwirlArguments const & args);

    void ApplyRadialWindFrom(Interaction::ArgumentsUnion::RadialWindArguments const & args);

private:

    /////////////////////////////////////////////////////////////////////////
    // Dynamics
    /////////////////////////////////////////////////////////////////////////

    // Mechanical

    void ApplyQueuedInteractionForces();

    void ApplyWorldForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        GameParameters const & gameParameters,
        Geometry::AABBSet & externalAabbSet);

    void ApplyWorldParticleForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        Buffer<float> & newCachedPointDepths,
        GameParameters const & gameParameters);

    template<bool DoDisplaceWater>
    void ApplyWorldSurfaceForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        Buffer<float> & newCachedPointDepths,
        GameParameters const & gameParameters,
        Geometry::AABBSet & externalAabbSet);

    void ApplyStaticPressureForces(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        GameParameters const & gameParameters);

    void ApplyStaticPressureForces(
        Frontiers::Frontier const & frontier,
        float effectiveAirDensity,
        float effectiveWaterDensity,
        GameParameters const & gameParameters);

    void RecalculateSpringRelaxationParallelism(size_t simulationParallelism, GameParameters const & gameParameters);
    void RecalculateSpringRelaxationSpringForcesParallelism(size_t simulationParallelism);
    void RecalculateSpringRelaxationIntegrationAndSeaFloorCollisionParallelism(size_t simulationParallelism, GameParameters const & gameParameters);

    void RunSpringRelaxationAndDynamicForcesIntegration(
        GameParameters const & gameParameters,
        ThreadManager & threadManager);

    void ApplySpringsForces(
        ElementIndex startSpringIndex,
        ElementIndex endSpringIndex,
        vec2f * restrict dynamicForceBuffer);

    inline void IntegrateAndResetDynamicForces(
        ElementIndex startPointIndex,
        ElementIndex endPointIndex,
        GameParameters const & gameParameters);

    inline float CalculateIntegrationVelocityFactor(float dt, GameParameters const & gameParameters) const;
    inline void IntegrateAndResetDynamicForces_1(ElementIndex startPointIndex, ElementIndex endPointIndex, GameParameters const & gameParameters);
    inline void IntegrateAndResetDynamicForces_2(ElementIndex startPointIndex, ElementIndex endPointIndex, GameParameters const & gameParameters);
    inline void IntegrateAndResetDynamicForces_3(ElementIndex startPointIndex, ElementIndex endPointIndex, GameParameters const & gameParameters);
    inline void IntegrateAndResetDynamicForces_4(ElementIndex startPointIndex, ElementIndex endPointIndex, GameParameters const & gameParameters);
    inline void IntegrateAndResetDynamicForces_N(size_t parallelism, ElementIndex startPointIndex, ElementIndex endPointIndex, GameParameters const & gameParameters);

    void HandleCollisionsWithSeaFloor(
        ElementIndex startPointIndex,
        ElementIndex endPointIndex,
        GameParameters const & gameParameters);

    void TrimForWorldBounds(GameParameters const & gameParameters);

    // Pressure and water

    void UpdatePressureAndWaterInflow(
        float effectiveAirDensity,
        float effectiveWaterDensity,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters,
        float & waterTakenInStep);

    void EqualizeInternalPressure(GameParameters const & gameParameters);

    void UpdateWaterVelocities(
        GameParameters const & gameParameters,
        float & waterSplashed);

    void UpdateSinking();

    // Electrical

    void RecalculateLightDiffusionParallelism(size_t simulationParallelism);

    void DiffuseLight(
        GameParameters const & gameParameters,
        ThreadManager & threadManager);

    // Heat

    void PropagateHeat(
        float currentSimulationTime,
        float dt,
		Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    // Misc

    void RotPoints(
        ElementIndex partition,
        ElementIndex partitionCount,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void UpdateStateMachines(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void UploadStateMachines(Render::RenderContext & renderContext);

private:

    /////////////////////////////////////////////////////////////////////////
    // Force Fields
    /////////////////////////////////////////////////////////////////////////

    void ApplyRadialSpaceWarpForceField(
        vec2f const & centerPosition,
        float radius,
        float radiusThickness,
        float strength);

    void ApplyImplosionForceField(
        vec2f const & centerPosition,
        float strength);

    void ApplyRadialExplosionForceField(
        vec2f const & centerPosition,
        float strength);

private:

    /////////////////////////////////////////////////////////////////////////
    // Misc
    /////////////////////////////////////////////////////////////////////////

    inline void UpdateForSimulationParallelism(
        GameParameters const & gameParameters,
        ThreadManager & threadManager);

    void RunConnectivityVisit();

    inline void SetAndPropagateResultantPointHullness(
        ElementIndex pointElementIndex,
        bool isHull);

    void DestroyConnectedTriangles(ElementIndex pointElementIndex);

    void DestroyConnectedTriangles(
        ElementIndex pointAElementIndex,
        ElementIndex pointBElementIndex);

    void AttemptPointRestore(ElementIndex pointElementIndex);

    void GenerateAirBubble(
        vec2f const & position,
        float depth,
        float temperature,
        float currentSimulationTime,
        PlaneId planeId,
        GameParameters const & gameParameters);

    void GenerateDebris(
        ElementIndex sourcePointElementIndex,
        StructuralMaterial const & debrisStructuralMaterial,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void GenerateSparklesForCut(
        ElementIndex springElementIndex,
        vec2f const & cutDirectionStartPos,
        vec2f const & cutDirectionEndPos,
        float currentSimulationTime,
        GameParameters const & gameParameters);

	void GenerateSparklesForLightning(
		ElementIndex pointElementIndex,
		float currentSimulationTime,
		GameParameters const & gameParameters);

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
        GameParameters const & gameParameters)
    {
        mPoints.Detach(
            pointIndex,
            detachVelocity,
            Points::DetachOptions::GenerateDebris
            | Points::DetachOptions::FireDestroyEvent,
            currentSimulationTime,
            gameParameters);
    }

private:

    /////////////////////////////////////////////////////////////////////////
    // IShipPhysicsHandler
    /////////////////////////////////////////////////////////////////////////

    virtual void HandlePointDetach(
        ElementIndex pointElementIndex,
        bool generateDebris,
        bool fireDestroyEvent,
        float currentSimulationTime,
        GameParameters const & gameParameters) override;

    virtual void HandlePointDamaged(ElementIndex pointElementIndex) override;

    virtual void HandleEphemeralParticleDestroy(
        ElementIndex pointElementIndex) override;

    virtual void HandlePointRestore(ElementIndex pointElementIndex) override;

    virtual void HandleSpringDestroy(
        ElementIndex springElementIndex,
        bool destroyAllTriangles,
        GameParameters const & gameParameters) override;

    virtual void HandleSpringRestore(
        ElementIndex springElementIndex,
        GameParameters const & gameParameters) override;

    virtual void HandleTriangleDestroy(ElementIndex triangleElementIndex) override;

    virtual void HandleTriangleRestore(ElementIndex triangleElementIndex) override;

    virtual void HandleElectricalElementDestroy(
        ElementIndex electricalElementIndex,
        ElementIndex pointIndex,
        ElectricalElementDestroySpecializationType specialization,
        float currentSimulationTime,
        GameParameters const & gameParameters) override;

    virtual void HandleElectricalElementRestore(ElementIndex electricalElementIndex) override;

    virtual void StartExplosion(
        float currentSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float blastRadius,
        float blastForce,
        float blastHeat,
        ExplosionType explosionType,
        GameParameters const & gameParameters) override;

    virtual void DoAntiMatterBombPreimplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        float radius,
        GameParameters const & gameParameters) override;

    virtual void DoAntiMatterBombImplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) override;

    virtual void DoAntiMatterBombExplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) override;

    virtual void HandleWatertightDoorUpdated(
        ElementIndex pointElementIndex,
        bool isOpen) override;

    virtual void HandleElectricSpark(
        ElementIndex pointElementIndex,
        float strength,
        float currentSimulationTime,
        GameParameters const & gameParameters) override;

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
        GameParameters const & gameParameters);

    inline void UploadExplosionStateMachine(
        ExplosionStateMachine const & explosionStateMachine,
        Render::RenderContext & renderContext);

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
        GameParameters const & gameParameters);

    bool RepairFromAttractor(
        ElementIndex attractorPointIndex,
        float repairStrength,
        SequenceNumber repairStepId,
        GameParameters const & gameParameters);

private:

    ShipId const mId;
    World & mParentWorld;
    MaterialDatabase const & mMaterialDatabase;
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;
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

    // Last-applied (interactive) wind field
    std::optional<WindField> mWindField;

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