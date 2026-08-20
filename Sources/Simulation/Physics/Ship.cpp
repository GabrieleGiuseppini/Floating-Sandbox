/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "Ship_StateMachines.h"

#include <Core/AABB.h>
#include <Core/Algorithms.h>
#include <Core/Conversions.h>
#include <Core/GameChronometer.h>
#include <Core/GameDebug.h>
#include <Core/GameMath.h>
#include <Core/GameRandomEngine.h>
#include <Core/Log.h>
#include <Core/SysSpecifics.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <queue>
#include <set>

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Low-frequency updates scheduling
//
// While most physics updates run for every simulation step (i.e. for each frame), a few
// more expensive ones run only every nth step. In order to improve omogeneity of runtime,
// we distribute all of these low-frequency updates across the low-frequency period.
//
// We have the following:
// CombustionStateMachineSlow x 4
// DecayPoints x 4
// SpringDecayAndTemperature x 4
// UpdateSinking x 1

static int constexpr CombustionStateMachineSlowStep1 = 2;
static int constexpr SpringDecayAndTemperatureStep1 = 5;
static int constexpr DecayPointsStep1 = 8;
static int constexpr CombustionStateMachineSlowStep2 = 11;
static int constexpr SpringDecayAndTemperatureStep2 = 14;
static int constexpr DecayPointsStep2 = 17;
static int constexpr UpdateSinkingStep = 18;
static int constexpr CombustionStateMachineSlowStep3 = 20;
static int constexpr SpringDecayAndTemperatureStep3 = 23;
static int constexpr DecayPointsStep3 = 26;
static int constexpr CombustionStateMachineSlowStep4 = 29;
static int constexpr SpringDecayAndTemperatureStep4 = 32;
static int constexpr DecayPointsStep4 = 35;

static_assert(DecayPointsStep4 < SimulationParameters::ParticleUpdateLowFrequencyPeriod);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Physics {

//   SSS    H     H  IIIIIII  PPPP
// SS   SS  H     H     I     P   PP
// S        H     H     I     P    PP
// SS       H     H     I     P   PP
//   SSS    HHHHHHH     I     PPPP
//      SS  H     H     I     P
//       S  H     H     I     P
// SS   SS  H     H     I     P
//   SSS    H     H  IIIIIII  P

Ship::Ship(
    ShipId id,
    FloatSize const & worldSize,
    World & parentWorld,
    MaterialDatabase const & materialDatabase,
    SimulationEventDispatcher & simulationEventDispatcher,
    Points && points,
    Springs && springs,
    Triangles && triangles,
    ElectricalElements && electricalElements,
    Frontiers && frontiers,
    RgbaImageData && interiorTextureImage)
    : mId(id)
    , mWorldSize(worldSize)
    , mParentWorld(parentWorld)
    , mMaterialDatabase(materialDatabase)
    , mSimulationEventHandler(simulationEventDispatcher)
    , mEventRecorder(nullptr)
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mFrontiers(std::move(frontiers))
    , mInteriorTextureImage(std::move(interiorTextureImage))
    , mPinnedPoints(
        mParentWorld,
        mSimulationEventHandler,
        mPoints)
    , mGadgets(
        mParentWorld,
        mId,
        mSimulationEventHandler,
        *this,
        mPoints,
        mSprings)
    , mElectricSparks(
        *this,
        mPoints,
        mSprings)
    , mOverlays()
    , mCurrentSimulationSequenceNumber()
    , mCurrentConnectivityVisitSequenceNumber()
    , mMaxMaxPlaneId(0)
    , mCurrentElectricalVisitSequenceNumber()
    , mConnectedComponentSizes()
    , mIsStructureDirty(true)
    , mDamagedPointsCount(0)
    , mBrokenSpringsCount(0)
    , mBrokenTrianglesCount(0)
    , mIsSinking(false)
    , mWaterSplashedRunningAverage()
    , mIsLightBufferPopulated(false)
    , mRepairGracePeriodMultiplier(1.0f)
    , mAirBubblesCreatedCount(0)
    , mCurrentSimulationParallelism(0) // We'll detect a difference on first run
    , mCurrentSpringRelaxationParallelComputationMode() // We'll detect a difference on first run
    // Static pressure
    , mStaticPressureBuffer(mPoints.GetAlignedShipPointCount())
    , mStaticPressureNetForceMagnitudeSum(0.0f)
    , mStaticPressureNetForceMagnitudeCount(0.0f)
    , mStaticPressureIterationsPercentagesSum(0.0f)
    , mStaticPressureIterationsCount(0.0f)
    // Decay
    , mCurrentRotAcceler8r(std::numeric_limits<float>::lowest())
    , mCurrentRustAcceler8r(std::numeric_limits<float>::lowest())
    , mCurrentAlgaeGrowthAcceler8r(std::numeric_limits<float>::lowest())
    // Debug
    , mLastQueriedPointIndex(NoneElementIndex)
    // Render
    , mLastUploadedDebugShipRenderMode()
    , mPlaneTriangleIndicesToRender()
{
    mPlaneTriangleIndicesToRender.reserve(mTriangles.GetElementCount());

    // Set handlers
    mPoints.RegisterShipPhysicsHandler(this);
    mSprings.RegisterShipPhysicsHandler(this);
    mTriangles.RegisterShipPhysicsHandler(this);
    mElectricalElements.RegisterShipPhysicsHandler(this);

    // Finalize
    Finalize();
}

void Ship::Announce()
{
    // Announce instanced electrical elements
    mElectricalElements.AnnounceInstancedElements();
}

Geometry::ShipAABBSet Ship::CalculateExternalAABBs() const
{
    Geometry::ShipAABBSet allExternalAABBs;

    for (FrontierId frontierId : mFrontiers.GetFrontierIds())
    {
        auto & frontier = mFrontiers.GetFrontier(frontierId);
        if (frontier.Type == FrontierType::External)
        {
            Geometry::ShipAABB aabb;

            ElementIndex const frontierStartEdge = frontier.StartingEdgeIndex;
            for (ElementIndex edgeIndex = frontierStartEdge; /*checked in loop*/; /*advanced in loop*/)
            {
                auto const & frontierEdge = mFrontiers.GetFrontierEdge(edgeIndex);

                auto const pointPosition = mPoints.GetPosition(frontierEdge.PointAIndex);
                aabb.ExtendTo(pointPosition);

                // Advance
                edgeIndex = frontierEdge.NextEdgeIndex;
                if (edgeIndex == frontierStartEdge)
                    break;
            }

            aabb.FrontierEdgeCount = static_cast<float>(frontier.Size);

            allExternalAABBs.Add(aabb);
        }
    }

    return allExternalAABBs;
}

Geometry::AABB Ship::CalculateParticleAABB() const
{
    return mPoints.CalculateAABB();
}

void Ship::SetEventRecorder(EventRecorder * eventRecorder)
{
    mEventRecorder = eventRecorder;
}

bool Ship::ReplayRecordedEvent(
    RecordedEvent const & event,
    SimulationParameters const & simulationParameters)
{
    if (event.GetType() == RecordedEvent::RecordedEventType::PointDetachForDestroy)
    {
        RecordedPointDetachForDestroyEvent const & detachEvent = static_cast<RecordedPointDetachForDestroyEvent const &>(event);

        DetachPointForDestroy(
            detachEvent.GetPointIndex(),
            detachEvent.GetDetachVelocity(),
            detachEvent.GetSimulationTime(),
            simulationParameters);
    }

    return false;
}

void Ship::Update(
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters,
    StressRenderModeType stressRenderMode,
    Geometry::ShipAABBSet & externalAabbSet, // output
    ThreadManager & threadManager,
    PerfStats & perfStats)
{
#ifdef FS_PROFILE_SHIP_UPDATE
    auto const updateStartTimestamp = GameChronometer::Now();
#endif

    /////////////////////////////////////////////////////////////////
    //         This is where most of the magic happens             //
    /////////////////////////////////////////////////////////////////

    std::vector<ThreadPool::Task> parallelTasks;

    mDebugVectors.clear();

    /////////////////////////////////////////////////////////////////
    // At this moment:
    //  - Particle positions are within world boundaries
    //  - Particle non-spring forces contain (some of) interaction-provided forces
    /////////////////////////////////////////////////////////////////

    // Get the current wall clock time
    auto const currentWallClockTime = GameWallClock::GetInstance().Now();
    auto const currentWallClockTimeFloat = GameWallClock::GetInstance().AsFloat(currentWallClockTime);

    // Advance the current simulation sequence
    ++mCurrentSimulationSequenceNumber;

#ifdef _DEBUG
    VerifyInvariants();
#endif

    ///////////////////////////////////////////////////////////////////
    // Process eventual parameter changes
    ///////////////////////////////////////////////////////////////////

    mPoints.UpdateForSimulationParameters(
        simulationParameters);

    mSprings.UpdateForSimulationParameters(
        simulationParameters,
        mPoints);

    mElectricalElements.UpdateForSimulationParameters(
        simulationParameters);

    UpdateForSimulationParameters(
        threadManager.GetSimulationThreadPool(),
        simulationParameters);

    ///////////////////////////////////////////////////////////////////
    // Calculate some widely-used physical constants
    ///////////////////////////////////////////////////////////////////

    float const effectiveAirDensity = Formulae::CalculateAirDensity(
        simulationParameters.AirTemperature + stormParameters.AirTemperatureDelta,
        simulationParameters);

    float const effectiveWaterDensity = Formulae::CalculateWaterDensity(
        simulationParameters.WaterTemperature,
        simulationParameters);

    ///////////////////////////////////////////////////////////////////
    // Recalculate current masses and everything else that derives from them
    ///////////////////////////////////////////////////////////////////

    // - Inputs: Water, AugmentedMaterialMass
    // - Outputs: Mass
    mPoints.UpdateMasses(simulationParameters);

    ///////////////////////////////////////////////////////////////////
    // Run spring relaxation iterations, together with integration
    // and ocean floor collision handling
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    auto startTimestamp1 = GameChronometer::Now();
#endif

    {
        auto const springsStartTime = GameChronometer::Now();

        RunSpringRelaxation(threadManager, simulationParameters);

        perfStats.Update<PerfMeasurement::TotalShipsSpringsUpdate>(GameChronometer::Now() - springsStartTime);
    }

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedSpringRelaxation = GameChronometer::Now() - startTimestamp1;
#endif

    ///////////////////////////////////////////////////////////////////
    // Produce silt clouds, if any
    ///////////////////////////////////////////////////////////////////

    for (auto const & siltImpact : mSiltImpacts)
    {
        InternalSpawnSiltCloud(siltImpact, currentSimulationTime, simulationParameters);
    }

    ///////////////////////////////////////////////////////////////////
    // Trim for world bounds
    ///////////////////////////////////////////////////////////////////

    // - Inputs: Position
    // - Outputs: Position, Velocity
    TrimForWorldBounds(simulationParameters);

    // We're done with changing positions for the rest of the Update() loop
#ifdef _DEBUG
    mPoints.Diagnostic_ClearDirtyPositions();
#endif

    ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////
    // From now on, we only work with forces and never update positions
    ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // Update strain for all springs - may cause springs to break,
    // rerouting frontiers
    //
    // Note: also calculates cached vectorial info for each spring
    ///////////////////////////////////////////////////////////////////

    if (stressRenderMode != StressRenderModeType::None)
    {
        mPoints.ResetStress();
    }

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    // - Inputs: P.Position, S.SpringDeletion, S.RestLength, S.BreakingElongation
    // - Outputs: S.Destroy(), P.Stress, S.CachedVectorialInfo
    // - Fires events, updates frontiers
    mSprings.UpdateForStrainsAndCacheSpringVectors(
        currentSimulationTime,
        simulationParameters,
        mPoints,
        stressRenderMode);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedUpdateForStress = GameChronometer::Now() - startTimestamp1;
#endif

    ///////////////////////////////////////////////////////////////////
    // Apply world forces
    //
    // Re-initializes static forces, now that they have been integrated.
    //
    // Also calculates cached depths, and updates frontiers' AABBs and
    // geometric centers - hence needs to come _after _ UpdateForStrains().
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    ApplyWorldForces(
        effectiveAirDensity,
        effectiveWaterDensity,
        currentSimulationTime,
        simulationParameters,
        externalAabbSet);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedWorldForces = GameChronometer::Now() - startTimestamp1;
#endif

    // Cached depths are valid from now on --------------------------->

    ///////////////////////////////////////////////////////////////////
    // Apply interaction forces that have been queued before this
    // step
    ///////////////////////////////////////////////////////////////////

    ApplyQueuedInteractionForces(simulationParameters);


    ///////////////////////////////////////////////////////////////////
    // Decay points
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    // - Inputs: Position, Water, IsLeaking
    // - Output: Decay

    if (mCurrentSimulationSequenceNumber.IsStepOf(DecayPointsStep1, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        DecayPoints(
            0, 4,
            currentSimulationTime,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(DecayPointsStep2, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        DecayPoints(
            1, 4,
            currentSimulationTime,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(DecayPointsStep3, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        DecayPoints(
            2, 4,
            currentSimulationTime,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(DecayPointsStep4, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        DecayPoints(
            3, 4,
            currentSimulationTime,
            simulationParameters);
    }

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedDecayPoints = GameChronometer::Now() - startTimestamp1;
#endif

    /////////////////////////////////////////////////////////////////
    // Update gadgets
    /////////////////////////////////////////////////////////////////

    // Might cause explosions; might cause elements to be detached/destroyed
    // (which would flag our structure as dirty)
    mGadgets.Update(
        currentWallClockTime,
        currentSimulationTime,
        stormParameters,
        simulationParameters);

    ///////////////////////////////////////////////////////////////////
    // Update state machines - may generate ephemeral particles
    ///////////////////////////////////////////////////////////////////

    // - Outputs:   Non-spring forces, temperature
    //              Point Detach, Debris generation
    UpdateStateMachines(currentSimulationTime, simulationParameters);

    /////////////////////////////////////////////////////////////////
    // Update water dynamics - may generate ephemeral particles
    /////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    //
    // Update intake of pressure and water
    //

    {
        float waterTakenInStep = 0.f;

        // - Inputs: P.Position, P.Water, P.IsLeaking, P.Temperature, P.PlaneId
        // - Outputs: P.InternalPressure, P.Water, P.CumulatedIntakenWater
        // - Creates ephemeral particles
        UpdatePressureAndWaterInflow(
            effectiveAirDensity,
            effectiveWaterDensity,
            currentSimulationTime,
            stormParameters,
            simulationParameters,
            waterTakenInStep);

        // Notify intaken water
        mSimulationEventHandler.OnWaterTaken(waterTakenInStep);
    }

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedWaterDynamics = GameChronometer::Now() - startTimestamp1;
#endif

    ///////////////////////////////
    // Parallel run 1 START
    ///////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    GameChronometer::duration elapsedWaterDiffusion;
    GameChronometer::duration elapsedEqualizeInternalPressure;
    GameChronometer::duration elapsedStaticPressure;
    GameChronometer::duration elapsedHeatPropagation;
#endif

    assert(parallelTasks.empty());

    parallelTasks.emplace_back(
        [&]()
        {
            //
            // Diffuse water (Cost: 14)
            //

#ifdef FS_PROFILE_SHIP_UPDATE
            auto startTimestamp2 = GameChronometer::Now();
#endif

            float waterSplashedInStep = 0.f;

            // - Inputs: Position, Water, WaterVelocity, WaterMomentum, ConnectedSprings
            // - Outpus: Water, WaterVelocity, WaterMomentum
            // TODOTEST
            //UpdateWaterVelocities(simulationParameters, waterSplashedInStep);
            // TODO: update comment above adding air pressure, and forces if we end up doing it here
            //UpdateWaterAndAirPressure_NewtonRhapson(simulationParameters, waterSplashedInStep);
            //UpdateWaterAndAirPressure_NewtonRhapson_2(simulationParameters, waterSplashedInStep);
            //UpdateWaterAndAirPressure_NewtonRhapson_2_TwoStep(simulationParameters, waterSplashedInStep);
            UpdateWaterAndAirPressure_NewtonRhapson_2_TwoStep_NewMomenta(simulationParameters, waterSplashedInStep);
            //UpdateWaterAndAirPressure_GaussSeidel_1(simulationParameters, waterSplashedInStep);
            //UpdateWaterAndAirPressure_GaussSeidel_2(simulationParameters, waterSplashedInStep);

            // Notify
            mSimulationEventHandler.OnWaterSplashed(waterSplashedInStep);

#ifdef FS_PROFILE_SHIP_UPDATE
            elapsedWaterDiffusion = GameChronometer::Now() - startTimestamp2;
#endif
        });

    parallelTasks.emplace_back(
        [&]()
        {
            //
            // Equalize internal pressure (Cost: 1.5)
            //

#ifdef FS_PROFILE_SHIP_UPDATE
            auto startTimestamp2 = GameChronometer::Now();
#endif

            // - Inputs: InternalPressure, ConnectedSprings
            // - Outpus: InternalPressure
            EqualizeInternalPressure(simulationParameters);

#ifdef FS_PROFILE_SHIP_UPDATE
            elapsedEqualizeInternalPressure = GameChronometer::Now() - startTimestamp2;
#endif

            //
            // Apply static pressure forces (Cost: 10)
            //

#ifdef FS_PROFILE_SHIP_UPDATE
            startTimestamp2 = GameChronometer::Now();
#endif

            if (simulationParameters.StaticPressureForceAdjustment > 0.0f)
            {
                // - Inputs: frontiers, P.Position, P.InternalPressure
                // - Outputs: P.DynamicForces
                ApplyStaticPressureForces(
                    effectiveAirDensity,
                    effectiveWaterDensity,
                    simulationParameters);
            }

#ifdef FS_PROFILE_SHIP_UPDATE
            elapsedStaticPressure = GameChronometer::Now() - startTimestamp2;
#endif

            //
            // Propagate heat (Cost: 4)
            //

#ifdef FS_PROFILE_SHIP_UPDATE
            startTimestamp2 = GameChronometer::Now();
#endif

            // - Inputs: P.Position, P.Temperature, P.ConnectedSprings, P.Water
            // - Outputs: P.Temperature
            PropagateHeat(
                currentSimulationTime,
                SimulationParameters::SimulationStepTimeDuration<float>,
                stormParameters,
                simulationParameters);

#ifdef FS_PROFILE_SHIP_UPDATE
            elapsedHeatPropagation = GameChronometer::Now() - startTimestamp2;
#endif
        });

    threadManager.GetSimulationThreadPool().RunAndClear(parallelTasks);

    // Publish static pressure stats
    mSimulationEventHandler.OnStaticPressureUpdated(
        mStaticPressureNetForceMagnitudeCount != 0.0f ? mStaticPressureNetForceMagnitudeSum / mStaticPressureNetForceMagnitudeCount : 0.0f,
        mStaticPressureIterationsCount != 0.0f ? mStaticPressureIterationsPercentagesSum / mStaticPressureIterationsCount : 0.0f);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedParallel1 = GameChronometer::Now() - startTimestamp1;
#endif

    ///////////////////////////////
    // Parallel run 1 END
    ///////////////////////////////

    //
    // Run sinking/unsinking detection
    //

    if (mCurrentSimulationSequenceNumber.IsStepOf(UpdateSinkingStep, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        UpdateSinking(currentSimulationTime);
    }

#ifdef _DEBUG
    Verify(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    //
    // Update electrical dynamics
    //

    // Generate a new visit sequence number
    ++mCurrentElectricalVisitSequenceNumber;

    mElectricalElements.Update(
        currentWallClockTime,
        currentSimulationTime,
        mCurrentElectricalVisitSequenceNumber,
        mPoints,
        mSprings,
        effectiveAirDensity,
        effectiveWaterDensity,
        stormParameters,
        simulationParameters);

    //
    // Diffuse light
    //

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    // - Inputs: P.Position, P.PlaneId, EL.AvailableLight
    //      - EL.AvailableLight depends on electricals which depend on water
    // - Outputs: P.Light
    DiffuseLight(
        simulationParameters,
        threadManager);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedLightDiffusion = GameChronometer::Now() - startTimestamp1;
#endif

    //
    // Update slow combustion state machine
    //

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowStep1, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mPoints.UpdateCombustionLowFrequency(
            0,
            4,
            currentWallClockTimeFloat,
            currentSimulationTime,
            stormParameters,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowStep2, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mPoints.UpdateCombustionLowFrequency(
            1,
            4,
            currentWallClockTimeFloat,
            currentSimulationTime,
            stormParameters,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowStep3, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mPoints.UpdateCombustionLowFrequency(
            2,
            4,
            currentWallClockTimeFloat,
            currentSimulationTime,
            stormParameters,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowStep4, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mPoints.UpdateCombustionLowFrequency(
            3,
            4,
            currentWallClockTimeFloat,
            currentSimulationTime,
            stormParameters,
            simulationParameters);
    }

    //
    // Update fast combustion state machine
    //

    mPoints.UpdateCombustionHighFrequency(
        currentSimulationTime,
        SimulationParameters::SimulationStepTimeDuration<float>,
        mParentWorld.GetCurrentWindSpeed(),
        mParentWorld.GetCurrentRadialWindField(),
        simulationParameters);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedCombustion = GameChronometer::Now() - startTimestamp1;
#endif

    //
    // Update highlights
    //

    mPoints.UpdateHighlights(currentWallClockTimeFloat);

    //
    // Update electric sparks
    //

    mElectricSparks.Update();

    ///////////////////////////////////////////////////////////////////
    // Update spring parameters
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    if (mCurrentSimulationSequenceNumber.IsStepOf(SpringDecayAndTemperatureStep1, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mSprings.UpdateForDecayAndTemperature(
            0, 4,
            mPoints);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(SpringDecayAndTemperatureStep2, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mSprings.UpdateForDecayAndTemperature(
            1, 4,
            mPoints);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(SpringDecayAndTemperatureStep3, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mSprings.UpdateForDecayAndTemperature(
            2, 4,
            mPoints);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(SpringDecayAndTemperatureStep4, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mSprings.UpdateForDecayAndTemperature(
            3, 4,
            mPoints);
    }

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedUpdateSpringParameters = GameChronometer::Now() - startTimestamp1;
#endif

    ///////////////////////////////////////////////////////////////////
    // Update ephemeral particles
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    mPoints.UpdateEphemeralParticles(
        currentSimulationTime,
        simulationParameters);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedUpdateEphemeralParticles = GameChronometer::Now() - startTimestamp1;
#endif

    ///////////////////////////////////////////////////////////////////
    // Update cleanup
    ///////////////////////////////////////////////////////////////////

    // This one we clear here, so the NPC update - which comes next - populates
    // it for use in the next simulation step
    mPoints.ResetTransientAdditionalMasses();

    ///////////////////////////////////////////////////////////////////
    // Diagnostics
    ///////////////////////////////////////////////////////////////////

#ifdef _DEBUG

    Verify(!mPoints.Diagnostic_ArePositionsDirty());

    VerifyInvariants();

#endif

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const updateEndTimestamp = GameChronometer::Now();

    static std::chrono::microseconds springRelaxationTotal{0};
    static std::chrono::microseconds updateForStressTotal{0};
    static std::chrono::microseconds decayPointsTotal{0};
    static std::chrono::microseconds worldForcesTotal{0};
    static std::chrono::microseconds waterDynamicsTotal{0};
    static std::chrono::microseconds parallel1Total{0};
    static std::chrono::microseconds lightDiffusionTotal{0};
    static std::chrono::microseconds combustionTotal{0};
    static std::chrono::microseconds updateSpringParametersTotal{0};
    static std::chrono::microseconds waterDiffusionTotal{0};
    static std::chrono::microseconds equalizeInternalPressureTotal{0};
    static std::chrono::microseconds staticPressureTotal{0};
    static std::chrono::microseconds heatPropagationTotal{0};
    static std::chrono::microseconds ephemeralParticlesTotal{0};
    static std::chrono::microseconds totalUpdateTotal{0};
    static int profilingFrameCounter = 0;

    springRelaxationTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedSpringRelaxation);
    updateForStressTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedUpdateForStress);
    decayPointsTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedDecayPoints);
    worldForcesTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedWorldForces);
    waterDynamicsTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedWaterDynamics);
    parallel1Total += std::chrono::duration_cast<std::chrono::microseconds>(elapsedParallel1);
    lightDiffusionTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedLightDiffusion);
    combustionTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedCombustion);
    updateSpringParametersTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedUpdateSpringParameters);
    waterDiffusionTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedWaterDiffusion);
    equalizeInternalPressureTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedEqualizeInternalPressure);
    staticPressureTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedStaticPressure);
    heatPropagationTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedHeatPropagation);
    ephemeralParticlesTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedUpdateEphemeralParticles);
    totalUpdateTotal += std::chrono::duration_cast<std::chrono::microseconds>(updateEndTimestamp - updateStartTimestamp);
    ++profilingFrameCounter;

    if (0 == (profilingFrameCounter % 40))
    {
        LogMessage("*** Ship update: springRelax=", springRelaxationTotal.count() / profilingFrameCounter / 1000.0f,
                   " updateForStress=", updateForStressTotal.count() / profilingFrameCounter / 1000.0f,
                   " decayPoints=", decayPointsTotal.count() / profilingFrameCounter / 1000.0f,
                   " worldForces=", worldForcesTotal.count() / profilingFrameCounter / 1000.0f,
                   " waterDynamics=", waterDynamicsTotal.count() / profilingFrameCounter / 1000.0f,
                   " parallel1=", parallel1Total.count() / profilingFrameCounter / 1000.0f,
                   " (waterDiffusion=", waterDiffusionTotal.count() / profilingFrameCounter / 1000.0f,
                   " equalizeInternalPressure=", equalizeInternalPressureTotal.count() / profilingFrameCounter / 1000.0f,
                   " staticPressure=", staticPressureTotal.count() / profilingFrameCounter / 1000.0f,
                   " heatPropagation=", heatPropagationTotal.count() / profilingFrameCounter / 1000.0f, ")",
                   " lightDiffusion=", lightDiffusionTotal.count() / profilingFrameCounter / 1000.0f,
                   " combustion=", combustionTotal.count() / profilingFrameCounter / 1000.0f,
                   " updateSpringParameters=", updateSpringParametersTotal.count() / profilingFrameCounter / 1000.0f,
                   " ephemeralParticles=", ephemeralParticlesTotal.count() / profilingFrameCounter / 1000.0f,
                   " total: ", totalUpdateTotal.count() / profilingFrameCounter / 1000.0f, "ms");

        springRelaxationTotal = std::chrono::microseconds(0);
        updateForStressTotal = std::chrono::microseconds(0);
        decayPointsTotal = std::chrono::microseconds(0);
        worldForcesTotal = std::chrono::microseconds(0);
        waterDynamicsTotal = std::chrono::microseconds(0);
        parallel1Total = std::chrono::microseconds(0);
        lightDiffusionTotal = std::chrono::microseconds(0);
        combustionTotal = std::chrono::microseconds(0);
        updateSpringParametersTotal = std::chrono::microseconds(0);
        waterDiffusionTotal = std::chrono::microseconds(0);
        equalizeInternalPressureTotal = std::chrono::microseconds(0);
        staticPressureTotal = std::chrono::microseconds(0);
        heatPropagationTotal = std::chrono::microseconds(0);
        ephemeralParticlesTotal = std::chrono::microseconds(0);
        totalUpdateTotal = std::chrono::microseconds(0);
        profilingFrameCounter = 0;
    }
#endif
}

void Ship::UpdateEnd()
{
    // Continue recovering from a repair
    if (mRepairGracePeriodMultiplier != 1.0f)
    {
        mRepairGracePeriodMultiplier += 0.2f * (1.0f - mRepairGracePeriodMultiplier);
        if (std::abs(1.0f - mRepairGracePeriodMultiplier) < 0.02f)
        {
            mRepairGracePeriodMultiplier = 1.0f;
        }
    }

    // Reset electrification (was needed by NPCs)
    mPoints.ResetIsElectrifiedBuffer();
}

void Ship::RenderUpload(RenderContext & renderContext)
{
    //
    // Run all tasks that need to run when connectivity has changed
    // (i.e. when the connected components have changed, e.g. because
    // of particle or spring deletion)
    //
    // Note: we have to do this here, at render time rather than
    // at update time, because the structure might have been dirtied
    // by an interactive tool while the game is paused
    //

    if (mIsStructureDirty)
    {
        // Re-calculate connected components
        RunConnectivityVisit();

        // Notify electrical elements
        mElectricalElements.OnPhysicalStructureChanged(mPoints);

        // Notify NPCs
        mParentWorld.GetNpcs().OnShipConnectivityChanged(mId);
    }

    //
    // Initialize upload
    //

    auto & shipRenderContext = renderContext.GetShipRenderContext(mId);

    shipRenderContext.UploadStart(mMaxMaxPlaneId);

    //////////////////////////////////////////////////////////////////////////////

    //
    // Upload points's immutable and mutable attributes
    //

    mPoints.UploadAttributes(
        mId,
        renderContext);

    //
    // Upload elements, if needed
    //

    if (mIsStructureDirty
        || !mLastUploadedDebugShipRenderMode
        || *mLastUploadedDebugShipRenderMode != renderContext.GetDebugShipRenderMode())
    {
        shipRenderContext.UploadElementsStart();

        //
        // Upload point elements (either orphaned only or all, depending
        // on the debug render mode)
        //

        mPoints.UploadNonEphemeralPointElements(
            mId,
            renderContext);

        //
        // Upload spring elements (including ropes) (edge or all, depending
        // on the debug render mode)
        //

        mSprings.UploadElements(
            mId,
            renderContext);

        //
        // Upload triangles, but only if structure is dirty
        // (we can't upload more frequently as mPlaneTriangleIndicesToRender is a one-time use)
        //

        if (mIsStructureDirty)
        {
            assert(mPlaneTriangleIndicesToRender.size() >= 1);

            shipRenderContext.UploadElementTrianglesStart(mPlaneTriangleIndicesToRender.back());

            mTriangles.UploadElements(
                mId,
                mPlaneTriangleIndicesToRender,
                mPoints,
                renderContext);

            shipRenderContext.UploadElementTrianglesEnd();
        }

        shipRenderContext.UploadElementsEnd();
    }

    //
    // Upload stressed springs
    //
    // We do this regardless of whether or not elements are dirty,
    // as the set of stressed springs is bound to change from frame to frame
    //

    shipRenderContext.UploadElementStressedSpringsStart();

    if (renderContext.GetShowStressedSprings())
    {
        mSprings.UploadStressedSpringElements(
            mId,
            renderContext);
    }

    shipRenderContext.UploadElementStressedSpringsEnd();

    //
    // Upload electrical elements
    //

    mElectricalElements.Upload(
        shipRenderContext,
        mPoints);

    //
    // Upload electric sparks
    //

    mElectricSparks.Upload(
        mPoints,
        mId,
        renderContext);

    //
    // Upload frontiers
    //

    mFrontiers.Upload(
        mId,
        renderContext);

    //
    // Upload flames
    //

    shipRenderContext.UploadFlamesStart(mPoints.GetBurningPointCount() + mParentWorld.GetNpcs().GetFlameCount(mId));

    mPoints.UploadFlames(shipRenderContext);
    mParentWorld.GetNpcs().UploadFlames(mId, shipRenderContext);

    shipRenderContext.UploadFlamesEnd();

    //
    // Upload gadgets
    //

    mGadgets.Upload(
        mId,
        renderContext);

    //
    // Upload pinned points
    //

    mPinnedPoints.Upload(
        mId,
        renderContext);

    //
    // Upload ephemeral points and textures
    //

    mPoints.UploadEphemeralParticles(
        mId,
        renderContext);

    //
    // Upload highlights
    //

    mPoints.UploadHighlights(
        mId,
        renderContext);

    //
    // Upload vector fields
    //

    mPoints.UploadVectors(
        mId,
        renderContext);

    if (!mDebugVectors.empty())
    {
        shipRenderContext.UploadVectorsStart(mDebugVectors.size(), vec4f(0.8f, 0.0f, 0.0f, 1.0f));

        for (auto const & [p, v] : mDebugVectors)
        {
            shipRenderContext.UploadVector(
                p,
                static_cast<float>(mMaxMaxPlaneId),
                v,
                50.0f);
        }

        shipRenderContext.UploadVectorsEnd();
    }

    //
    // Upload state machines
    //

    UploadStateMachines(renderContext);

    //
    // Upload overlays
    //

    mOverlays.Upload(
        mId,
        renderContext);

    //////////////////////////////////////////////////////////////////////////////

    //
    // Finalize upload
    //

    shipRenderContext.UploadEnd();

    //
    // Reset render state
    //

    mIsStructureDirty = false;
    mLastUploadedDebugShipRenderMode = renderContext.GetDebugShipRenderMode();
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

void Ship::Finalize()
{
    //
    // 1. Propagate (ship) point materials' hullness
    //

    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        if (mPoints.GetStructuralMaterial(pointIndex).IsHull)
        {
            SetAndPropagateResultantPointHullness(pointIndex, true);
        }
    }

    //
    // 2. Do a first connectivity pass (for the first Update)
    //

    RunConnectivityVisit();
}

///////////////////////////////////////////////////////////////////////////////////
// Mechanical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::ApplyQueuedInteractionForces(SimulationParameters const & simulationParameters)
{
    for (auto const & interaction : mQueuedInteractions)
    {
        switch (interaction.Type)
        {
            case Interaction::InteractionType::AntiGravityField:
            {
                ApplyAntiGravityField(interaction.Arguments.AntiGravityField, simulationParameters);

                break;
            }

            case Interaction::InteractionType::Blast:
            {
                ApplyBlastAt(interaction.Arguments.Blast, simulationParameters);

                break;
            }

            case Interaction::InteractionType::Draw:
            {
                DrawTo(interaction.Arguments.Draw);

                break;
            }

            case Interaction::InteractionType::Pull:
            {
                Pull(interaction.Arguments.Pull);

                break;
            }

            case Interaction::InteractionType::Swirl:
            {
                SwirlAt(interaction.Arguments.Swirl);

                break;
            }

            case Interaction::InteractionType::Tornado:
            {
                ApplyTornado(interaction.Arguments.Tornado, simulationParameters);

                break;
            }
        }
    }

    mQueuedInteractions.clear();
}

void Ship::ApplyWorldForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters,
    Geometry::ShipAABBSet & externalAabbSet) // output
{
    // New buffer to which new cached depths will be written to
    std::shared_ptr<Buffer<float>> newCachedPointDepths = mPoints.AllocateWorkBufferFloat();

    //
    // Particle forces
    //

    ApplyWorldParticleForces(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, simulationParameters);

    //
    // Surface forces
    //

    if (simulationParameters.DoDisplaceWater)
        ApplyWorldSurfaceForces<true>(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, currentSimulationTime, simulationParameters, externalAabbSet);
    else
        ApplyWorldSurfaceForces<false>(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, currentSimulationTime, simulationParameters, externalAabbSet);

    // Commit new particle depth buffer
    mPoints.SwapCachedDepthBuffer(*newCachedPointDepths);
}

void Ship::ApplyWorldParticleForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    Buffer<float> & newCachedPointDepths,
    SimulationParameters const & simulationParameters)
{
    // Global wind force
    vec2f const globalWindForce = Formulae::WindSpeedToForceDensity(
        Conversions::KmhToMs(mParentWorld.GetCurrentWindSpeed()),
        effectiveAirDensity);

    // Abovewater points feel this amount of air drag, due to friction
    float const airFrictionDragCoefficient =
        SimulationParameters::AirFrictionDragCoefficient
        * simulationParameters.AirFrictionDragAdjustment;

    // Underwater points feel this amount of water drag, due to friction
    float const waterFrictionDragCoefficient =
        SimulationParameters::WaterFrictionDragCoefficient
        * simulationParameters.WaterFrictionDragAdjustment;

    OceanSurface const & oceanSurface = mParentWorld.GetOceanSurface();

    float * const restrict newCachedPointDepthsBuffer = newCachedPointDepths.data();
    vec2f * const restrict staticForcesBuffer = mPoints.GetStaticForceBufferAsVec2();

    //
    // 1. Various world forces
    //

    for (auto pointIndex : mPoints)
    {
        auto const & pointPosition = mPoints.GetPosition(pointIndex);

        vec2f staticForce = vec2f::zero();

        //
        // Calculate and store depth
        //

        newCachedPointDepthsBuffer[pointIndex] = oceanSurface.GetDepth(pointPosition);

        //
        // Calculate above/under-water coefficient
        //
        // 0.0: above water
        // 1.0: under water
        // in-between: smooth air-water interface (nature abhors discontinuities)
        //

        float const airWaterInterfaceInverseWidth = mPoints.GetAirWaterInterfaceInverseWidth(pointIndex);
        float const uwCoefficient = Clamp(newCachedPointDepthsBuffer[pointIndex] * airWaterInterfaceInverseWidth, 0.0f, 1.0f);

        //
        // Apply gravity
        //

        staticForce +=
            SimulationParameters::Gravity
            * mPoints.GetMass(pointIndex); // Material + Augmentation + Water

        //
        // Apply water/air buoyancy
        //

        // Calculate upward push of water/air mass
        auto const & buoyancyCoefficients = mPoints.GetBuoyancyCoefficients(pointIndex);
        float const buoyancyPush =
            buoyancyCoefficients.Coefficient1
            + buoyancyCoefficients.Coefficient2 * mPoints.GetTemperature(pointIndex);

        // Apply buoyancy
        staticForce.y +=
            buoyancyPush
            * Mix(effectiveAirDensity, effectiveWaterDensity, uwCoefficient);

        //
        // Apply friction drag
        //
        // We use a linear law for simplicity.
        //
        // With a linear law, we know that the force will never overcome the current velocity
        // as long as m > (C * dt) (~=0.0016 for water drag), which is a mass we won't have in our system (air is 1.2754);
        // hence we don't care here about capping the force to prevent overcoming accelerations.
        //

        staticForce +=
            -mPoints.GetVelocity(pointIndex)
            * Mix(airFrictionDragCoefficient, waterFrictionDragCoefficient, uwCoefficient);

        //
        // Global (linear) wind force
        //

        // Note: should be based on relative velocity, but we simplify here for performance reasons
        staticForce +=
            globalWindForce
            * mPoints.GetMaterialWindReceptivity(pointIndex)
            * (1.0f - uwCoefficient); // Only above-water (modulated)

        staticForcesBuffer[pointIndex] = staticForce; // Here we _initialize_ static forces
    }

    //
    // 2. Radial wind field, if any
    //

    auto const & radialWindField = mParentWorld.GetCurrentRadialWindField();
    if (radialWindField.has_value())
    {
        for (auto pointIndex : mPoints)
        {
            // Only above-water points
            if (newCachedPointDepthsBuffer[pointIndex] <= 0.0f)
            {
                vec2f const pointPosition = mPoints.GetPosition(pointIndex);
                vec2f const displacement = pointPosition - radialWindField->SourcePos;
                float const radius = displacement.length();
                if (radius < radialWindField->PreFrontRadius) // Within sphere
                {
                    // Calculate force magnitude
                    float windForceMagnitude;
                    if (radius < radialWindField->MainFrontRadius)
                    {
                        windForceMagnitude = radialWindField->MainFrontWindForceMagnitude;
                    }
                    else
                    {
                        windForceMagnitude = radialWindField->PreFrontWindForceMagnitude;
                    }

                    // Calculate force
                    vec2f const force =
                        displacement.normalise_approx(radius)
                        * windForceMagnitude
                        * mPoints.GetMaterialWindReceptivity(pointIndex);

                    // Apply force
                    staticForcesBuffer[pointIndex] += force;
                }
            }
        }
    }
}

template<bool DoDisplaceWater>
void Ship::ApplyWorldSurfaceForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    Buffer<float> & newCachedPointDepths,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters,
    Geometry::ShipAABBSet & externalAabbSet) // output
{
    float totalWaterDisplacementMagnitude = 0.0f;

    //
    // Drag constants
    //

    // Abovewater points feel this amount of air drag, due to pressure
    float const airPressureDragCoefficient =
        SimulationParameters::AirPressureDragCoefficient
        * simulationParameters.AirPressureDragAdjustment
        * (effectiveAirDensity / SimulationParameters::AirMass);

    // Underwater points feel this amount of water drag, due to pressure
    float const waterPressureDragCoefficient =
        SimulationParameters::WaterPressureDragCoefficient
        * simulationParameters.WaterPressureDragAdjustment
        * (effectiveWaterDensity / SimulationParameters::WaterMass);

    //
    // Water impact constants
    //

    float const waterImpactForceCoefficient =
        simulationParameters.WaterImpactForceAdjustment
        * (effectiveWaterDensity / SimulationParameters::WaterMass); // Denser water, denser impact

    //
    // Push Velocity -> Water Displacement mapping constants
    //

    float constexpr WdmX0 = 1.0f; // Vertical velocity at which displacement transitions from quadratic to linear
    float constexpr WdmY0 = 0.16f; // Displacement magnitude at x0

    // Linear portion
    float const wdmLinearSlope = SimulationParameters::SimulationStepTimeDuration<float> *6.0f; // Magic number

    // Quadratic portion: y = ax^2 + bx, with constraints:
    //  y(0) = 0
    //  y'(x0) = slope
    //  y(x0) = y0
    float const wdmQuadraticA = (wdmLinearSlope * WdmX0 - WdmY0) / (WdmX0 * WdmX0);
    float const wdmQuadraticB = 2.0f * WdmY0 / WdmX0 - wdmLinearSlope;

    //
    // Water foam
    //

    float const minAbsDisplacementForWaterFoam = (simulationParameters.WaterFoamSensitivityAdjustment > 0.0f)
        ? 0.065f / simulationParameters.WaterFoamSensitivityAdjustment // Magic
        : std::numeric_limits<float>::max();

    struct WaterFoam
    {
        vec2f Position;
        float VerticalDirection;
        float Strength;
        PlaneId Plane;

        WaterFoam()
            : Position()
            , VerticalDirection(0.0f)
            , Strength(0.0f)
            , Plane(NonePlaneId)
        { }

        WaterFoam(
            vec2f const & position,
            float verticalDirection,
            float strength,
            PlaneId plane)
            : Position(position)
            , VerticalDirection(verticalDirection)
            , Strength(strength)
            , Plane(plane)
        { }
    };

    WaterFoam strongestWaterFoam;

    //
    // Water splashes
    //

    float const minAbsDisplacementForWaterSplash = (simulationParameters.WaterSplashSensitivityAdjustment > 0.0f)
        ? 0.2f / simulationParameters.WaterSplashSensitivityAdjustment // Magic
        : std::numeric_limits<float>::max();

    struct WaterSplash
    {
        vec2f Position;
        vec2f SpawnDirection;
        float Strength;
        PlaneId Plane;

        WaterSplash()
            : Position()
            , SpawnDirection()
            , Strength(0.0f)
            , Plane(NonePlaneId)
        {
        }

        WaterSplash(
            vec2f const & position,
            vec2f const & spawnDirection,
            float strength,
            PlaneId plane)
            : Position(position)
            , SpawnDirection(spawnDirection)
            , Strength(strength)
            , Plane(plane)
        {
        }
    };

    WaterSplash strongestWaterSplash;

    //
    // Lift: Cl * rho * V^2 * A / 2
    //
    //  - Cl: from material
    //  - Rho: from water/air (though we only use air density, as underwater lift would be massive)
    //  - V: point velocity along edge, absolute (we pretend a double-profile wing, otherwise airplanes cannot fly in both flip directions)
    //  - A: 400 square meter (calculated empirically)
    //

    float constexpr WingSurface = 400.0f;

    float const liftForceFactor_Rho_A_Half_Adj =
        effectiveAirDensity
        * WingSurface
        / 2.0f
        * simulationParameters.LiftForceAdjustment;

    //
    // Visit all frontiers
    //

    for (FrontierId frontierId : mFrontiers.GetFrontierIds())
    {
        // Initialize AABB and geometric center
        Geometry::ShipAABB aabb;
        vec2f geometricCenter = vec2f::zero();

        auto & frontier = mFrontiers.GetFrontier(frontierId);

        // We only apply velocity drag and lift, and displace water for *external* frontiers,
        // not for internal ones
        if (frontier.Type == FrontierType::External)
        {
            //
            // Visit all edges of this frontier
            //

            assert(frontier.Size >= 3);

            ElementIndex const startEdgeIndex = frontier.StartingEdgeIndex;

            // Take previous point
            auto const & previousFrontierEdge = mFrontiers.GetFrontierEdge(startEdgeIndex);
            vec2f previousPointPosition = mPoints.GetPosition(previousFrontierEdge.PointAIndex);

            // Take this point
            auto const & thisFrontierEdge = mFrontiers.GetFrontierEdge(previousFrontierEdge.NextEdgeIndex);
            ElementIndex thisPointIndex = thisFrontierEdge.PointAIndex;
            vec2f thisPointPosition = mPoints.GetPosition(thisPointIndex);

#ifdef _DEBUG
            size_t visitedPoints = 0;
#endif

            ElementIndex const edgeVisitStartEdgeIndex = thisFrontierEdge.NextEdgeIndex;

            for (ElementIndex nextEdgeIndex = edgeVisitStartEdgeIndex; /*checked in loop*/; /*advanced in loop*/)
            {

#ifdef _DEBUG
                ++visitedPoints;
#endif

                // Update AABB and geometric center with this point
                aabb.ExtendTo(thisPointPosition);
                geometricCenter += thisPointPosition;

                // Get next edge and point
                auto const & nextFrontierEdge = mFrontiers.GetFrontierEdge(nextEdgeIndex);
                ElementIndex const nextPointIndex = nextFrontierEdge.PointAIndex;
                vec2f const nextPointPosition = mPoints.GetPosition(nextPointIndex);

                // Get point depth (positive at greater depths, negative over-water)
                float const thisPointDepth = newCachedPointDepths[thisPointIndex];

                // UW coefficient: 0.0 if point in air, 1.0 if under water, smooth in-between
                float const uwCoefficient = Clamp(thisPointDepth, 0.0f, 1.0f);

                // Get point velocity
                vec2f const thisPointVelocity = mPoints.GetVelocity(thisPointIndex);

                // Edge direction - calculated between p1 and p3
                vec2f const edgeDir = (nextPointPosition - previousPointPosition).normalise();

                // Magnitude of the edge velocity along the edge; positive when following
                // frontier's CW order, zero when perpendicular to edge
                float const edgeVelocityAlongEdge = thisPointVelocity.dot(edgeDir);

                // Normal to edge - calculated between p1 and p3; points outside
                vec2f const edgeNormal = edgeDir.to_perpendicular();

                // Magnitude of the edge velocity along the edge normal; positive when pointing out,
                // zero when parallel to edge
                float const edgeVelocityAlongEdgeNormal = thisPointVelocity.dot(edgeNormal);

                //
                // Drag force
                //
                // We would like to use a square law (i.e. drag force proportional to square
                // of velocity), but then particles at high velocities become subject to
                // enormous forces, which, for small masses - such as cloth - mean astronomical
                // accelerations.
                //
                // We have to recourse then, again, to a linear law:
                //
                // F = - C * |V| * cos(a) * Nn
                //
                //      cos(a) == cos(angle between velocity and surface normal) == Vn dot Nn
                //
                // With this law, a particle's velocity is overcome by the drag force when its
                // mass is <= C * dt, i.e. ~78Kg with water drag. Since this mass we do have in our sytem,
                // we have to cap the force to prevent velocity overcome.
                //

                // Cap it to the same direction as velocity, to avoid suction force
                // (i.e. drag force attracting surface facing opposite of velocity)
                float const edgeVelocityCapped = std::max(
                    edgeVelocityAlongEdgeNormal,
                    0.0f);

                // Max drag force magnitude: m * (V dot Nn) / dt
                float const maxDragForceMagnitude =
                    mPoints.GetMass(thisPointIndex) * edgeVelocityCapped
                    / SimulationParameters::SimulationStepTimeDuration<float>;

                // Calculate drag coefficient: air or water, with soft transition
                // to avoid discontinuities in drag force close to the air-water interface
                float const dragCoefficient = Mix(
                    airPressureDragCoefficient,
                    waterPressureDragCoefficient,
                    uwCoefficient);

                // Calculate magnitude of drag force (opposite sign), capped by max drag force
                //  - C * |V| * cos(a) == - C * |V| * (Vn dot Nn) == -C * (V dot Nn)
                float const dragForceMagnitude = std::min(
                    dragCoefficient * edgeVelocityCapped,
                    maxDragForceMagnitude);

                //
                // Impact force
                //
                // Impact force is proportional to kinetic energy, and we only apply it
                // when there's a discontinuity in the "underwaterness" of a frontier
                // particle, i.e. when this is the first frame in which the particle
                // gets underwater.
                //
                // For the impact force we consider the particle's velocity projection
                // along the normal to the ocean surface at this point
                //

                float const pointVelocityMagnitude = thisPointVelocity.length();
                vec2f const pushDir = thisPointVelocity.normalise(pointVelocityMagnitude);

                vec2f const oceanSurfaceNormal = mParentWorld.GetOceanSurface().GetNormalAt(thisPointPosition.x); // Points up
                float const impactVelocity = edgeVelocityCapped * std::abs(pushDir.dot(oceanSurfaceNormal));

                float const kineticEnergy =
                    impactVelocity * impactVelocity
                    * mPoints.GetMass(thisPointIndex);

                float const waterImpactForceMagnitude =
                    std::min(kineticEnergy, 100000000.0f) // Cap it to prevent gigantous forces
                    * waterImpactForceCoefficient
                    * Step(mPoints.GetCachedDepth(thisPointIndex), 0.0f) * Step(0.0f, newCachedPointDepths[thisPointIndex]);

                //
                // Apply drag and impact forces
                //

                mPoints.AddStaticForce(
                    thisPointIndex,
                    -edgeNormal * (dragForceMagnitude + waterImpactForceMagnitude));

                //
                // Water displacement
                //
                // * The magnitude of water displacement is proportional to the square root of
                //   the kinetic energy of the particle, thus it is *linearly* proportional to the
                //   particle's velocity
                //      * However, in order to generate visible waves also for very small velocities,
                //        we want the contribution of small velocities to be more than linear wrt
                //        the contribution of higher velocities, and so we'll be using a piecewise
                //        function: quadratic for small velocities, and linear for higher
                // * For a better effect (homogeneous and spawned by all materials), we ignore the
                //   dependency on the particle's mass
                // * The deeper the particle is, the less it contributes to displacement
                //

                if constexpr (DoDisplaceWater)
                {
                    //
                    // Goals:
                    // - a. Impact displacement: (proportional to, and sign of) vertical component of edge "push"
                    //    - For simplicity: independent from ocean surface normal at that point
                    // - b. Straight vertical keel moving along ocean surface (ship marching ahead): must generate waves and foam
                    //    - If keel is oblique : see a., pushing up or down
                    //    - In other words : when push surface is full but orthogonal to water surface, vertical component must
                    //      still be != 0 - provided it has velocity
                    // - c. Laminar vertical (straight vertical Titanic keel rocking on flat surface): must generate small waves and foam
                    //    - In other words: when push surface is zero, must still push something - provided it has velocity
                    //

                    //
                    // Impl:
                    //
                    // - An edge surface generates a "push" in the direction of the edge velocity; after empirical analysis, we do not consider
                    //   the magnitude as proportional to the surface area as seen from the edge's velocity axis
                    //      - In other words: not proportional to the velocity direction _dot_ edge's normal
                    //      - Would be zero for a velocity in the same direction as the edge surface (laminar vertical case), but to
                    //        meet goal c., we would clamp the angle, but experiments suggest to let the angle completely go
                    // - The actual displacement is always vertical (OceanSurface implementation constraint), and its magnitude is
                    //   proportional to the vertical component of the push vector
                    //      - Would be zero when push vector is perfectly horizontal, but to meet goal b., we clamp the angle
                    //      - And to avoid displacements along a horizontal keel traveling parallel to the water surface,
                    //        we also consider the angle of the push direction wrt the edge surface: we don't clamp
                    //        if it's parallel
                    //

                    // Note: if we get spurious foam at rest, clamp point velocity to ~0.5/0.6,
                    // which seems to be the vibration speed of the mesh

                    //
                    // Push velocity magnitude
                    //

                    // Clamp edge velocity to prevent ocean surface instabilities with extremely high
                    // velocities
                    float const absPointVelocityMagnitudeCapped = std::min(
                        std::abs(pointVelocityMagnitude),
                        10000.0f); // Magic number

                    //
                    // Displacement magnitude
                    //

                    // Transform push velocity (absolute) into a displacement magnitude (absolute)
                    float const linearAbsDisplacementMagnitude = WdmY0 + wdmLinearSlope * (absPointVelocityMagnitudeCapped - WdmX0);
                    float const quadraticAbsDisplacementMagnitude =
                        wdmQuadraticA * absPointVelocityMagnitudeCapped * absPointVelocityMagnitudeCapped
                        + wdmQuadraticB * absPointVelocityMagnitudeCapped;

                    //
                    // Depth attenuation: tapers down displacement the deeper the point is
                    //

                    // Depth at which the point stops contributing: rises with impact velocity along vertical, and asymmetric wrt sinking or rising
                    float const absVerticalPointVelocityMagnitudeCapped = absPointVelocityMagnitudeCapped * std::abs(pushDir.y);
                    float constexpr MaxVerticalVel = 35.0f;
                    float const maxDepth =
                        (0.5f + LinearStep(0.0f, MaxVerticalVel, absVerticalPointVelocityMagnitudeCapped) * 0.5f)
                        * (pushDir.y <= 0.0f ? 12.0f : 4.0f); // Keep up-push low or else bodies keep jumping up and down forever

                    // Linear attenuation up to maxDepth
                    float const depthAttenuation = 1.0f - LinearStep(0.0f, maxDepth, thisPointDepth); // Tapers down contribution the deeper the point is

                    //
                    // Displacement angle: due to the fact that we displace the ocean surface vertically, here we calculate the
                    // vertical component of the displacement, clamping it however to ensure goal b.
                    //
                    // With the clamp, we technically consider a push as vertical depending on either:
                    //  - The verticality of the push direction itself (obviously)
                    //  - The aligment of the push direction wrt the frontier edge , which maximizes pressure and thus
                    //    vertical (escaping) displacement
                    // There's those two maxima, and a minimum when both the push is parallel to the frontier edge,
                    // and the displacement vector is fully horizontal; for example, along the underwater keel
                    // marching ahead
                    // Push angle factor (push multiplier that takes into account the visible surface)
                    float const minDisplacementAngleVerticalFactor = 0.5f * std::abs(pushDir.dot(edgeNormal));
                    float const displacementAngleVerticalFactor = std::max(std::abs(pushDir.y), minDisplacementAngleVerticalFactor) * Sign(pushDir.y);

                    //
                    // Final displacement
                    //

                    float const displacement =
                        (absPointVelocityMagnitudeCapped < WdmX0 ? quadraticAbsDisplacementMagnitude : linearAbsDisplacementMagnitude)
                        * 0.4f // Magic magnitude adjustment
                        * depthAttenuation
                        * Step(0.0f, thisPointDepth) // No displacement for above-water points
                        * displacementAngleVerticalFactor; // Take vertical component, adjusting sign

                    float const absDisplacement = std::abs(displacement);

                    float const oceanSurfaceDisplacement = displacement * simulationParameters.WaterDisplacementWaveHeightAdjustment;
                    mParentWorld.DisplaceOceanSurfaceAt(thisPointPosition.x, oceanSurfaceDisplacement);
                    totalWaterDisplacementMagnitude += std::abs(oceanSurfaceDisplacement);

                    //
                    // Water foam
                    //

                    if (absDisplacement > minAbsDisplacementForWaterFoam // Both upwards and downwards
                        && thisPointDepth < 1.5f) // Only spawn foam on the surface
                    {
                        float const strength = absDisplacement - minAbsDisplacementForWaterFoam;
                        if (strength > strongestWaterFoam.Strength)
                        {
                            strongestWaterFoam = WaterFoam(
                                thisPointPosition,
                                Sign(edgeVelocityAlongEdgeNormal),
                                strength,
                                mPoints.GetPlaneId(thisPointIndex));
                        }
                    }

                    //
                    // Water splashes
                    //

                    if (displacement < -minAbsDisplacementForWaterSplash // Only downwards
                        && thisPointDepth < 2.0f) // Only spawn splashes on the surface
                    {
                        float const strength = -displacement - minAbsDisplacementForWaterSplash;
                        assert(strength > 0.0f);
                        if (strength > strongestWaterSplash.Strength)
                        {
                            strongestWaterSplash = WaterSplash(
                                thisPointPosition,
                                oceanSurfaceNormal, // Points up
                                strength,
                                mPoints.GetPlaneId(thisPointIndex));
                        }
                    }
                }

                //
                // Lift: Cl * V^2 * (Rho * A / 2)
                //

                // Cap velocity as a proxy to cap lift force, to avoid massive lift forces that
                // would makes structures explode when velocity is reached suddenly
                float const edgeVelocityAlongEdgeSquareCapped = std::min(edgeVelocityAlongEdge * edgeVelocityAlongEdge, 30.0f * 30.0f);

                float const liftForce =
                    mPoints.GetStructuralMaterial(thisPointIndex).LiftCoefficient
                    * edgeVelocityAlongEdgeSquareCapped
                    * liftForceFactor_Rho_A_Half_Adj;

                // Add force, towards the interior - we assume lift materials are placed on the bottom surface if we want an upward lift
                mPoints.AddStaticForce(
                    thisPointIndex,
                    -edgeNormal * liftForce);

                //
                // Advance edge in the frontier visit
                //

                nextEdgeIndex = nextFrontierEdge.NextEdgeIndex;
                if (nextEdgeIndex == edgeVisitStartEdgeIndex)
                    break;

                previousPointPosition = thisPointPosition;
                thisPointPosition = nextPointPosition;
                thisPointIndex = nextPointIndex;
            }

#ifdef _DEBUG
            assert(visitedPoints == frontier.Size);
#endif
        }
        else
        {
            //
            // Simply update AABB and geometric center
            //

            ElementIndex const frontierStartEdge = frontier.StartingEdgeIndex;

            for (ElementIndex edgeIndex = frontierStartEdge; /*checked in loop*/; /*advanced in loop*/)
            {
                auto const & frontierEdge = mFrontiers.GetFrontierEdge(edgeIndex);

                // Update AABB and geometric center with this point
                auto const pointPosition = mPoints.GetPosition(frontierEdge.PointAIndex);
                aabb.ExtendTo(pointPosition);
                geometricCenter += pointPosition;

                // Advance
                edgeIndex = frontierEdge.NextEdgeIndex;
                if (edgeIndex == frontierStartEdge)
                    break;
            }
        }

        //
        // Finalize AABB and geometric center update
        //

        aabb.FrontierEdgeCount = static_cast<float>(frontier.Size);

        geometricCenter /= static_cast<float>(frontier.Size);

        // Store AABB and geometric center in frontier
        frontier.AABB = aabb;
        frontier.GeometricCenterPosition = geometricCenter;

        // Store AABB in AABB set, but only if external
        if (frontier.Type == FrontierType::External)
        {
            externalAabbSet.Add(aabb);
        }
    }

    if constexpr (DoDisplaceWater)
    {
        mSimulationEventHandler.OnWaterDisplaced(totalWaterDisplacementMagnitude);

        if (strongestWaterFoam.Strength > 0.0f)
        {
            assert(strongestWaterFoam.Plane != NonePlaneId);

            InternalSpawnWaterFoam(
                strongestWaterFoam.Position,
                strongestWaterFoam.VerticalDirection,
                strongestWaterFoam.Strength,
                strongestWaterFoam.Plane,
                newCachedPointDepths,
                currentSimulationTime,
                simulationParameters);
        }

        if (strongestWaterSplash.Strength > 0.0f)
        {
            assert(strongestWaterSplash.Plane != NonePlaneId);

            InternalSpawnWaterSplash(
                strongestWaterSplash.Position,
                strongestWaterSplash.SpawnDirection,
                strongestWaterSplash.Strength,
                strongestWaterSplash.Plane,
                newCachedPointDepths,
                currentSimulationTime,
                simulationParameters);
        }
    }
}

void Ship::ApplyStaticPressureForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    SimulationParameters const & simulationParameters)
{
    //
    // At this moment, dynamic forces are all zero - we are the first populating those
    //

    assert(std::all_of(
        mPoints.GetDynamicForceBuffer0AsVec2(),
        mPoints.GetDynamicForceBuffer0AsVec2() + mPoints.GetAlignedShipPointCount(),
        [](vec2f const & v)
        {
            return v == vec2f::zero();
        }));

    // Initialize stats
    mStaticPressureNetForceMagnitudeSum = 0.0f;
    mStaticPressureNetForceMagnitudeCount = 0.0f;
    mStaticPressureIterationsPercentagesSum = 0.0f;
    mStaticPressureIterationsCount = 0.0f;

    // Visit all frontiers and apply static pressure forces on each
    for (FrontierId const frontierId : mFrontiers.GetFrontierIds())
    {
        auto const & frontier = mFrontiers.GetFrontier(frontierId);

        // Only consider external frontiers
        if (frontier.Type == FrontierType::External)
        {
            ApplyStaticPressureForces(
                frontier,
                effectiveAirDensity,
                effectiveWaterDensity,
                simulationParameters);
        }
    }
}

namespace {

    static inline float CalculateSiltPressureDamping(vec2f const & position, OceanFloor const & oceanFloor)
    {
        auto const siltY = oceanFloor.GetSiltHeightAt(position.x);

        // Nature abhors discontinuity
        return LinearStep(0.0f, 1.0f, position.y - siltY);
    }
}

void Ship::ApplyStaticPressureForces(
    Frontiers::Frontier const & frontier,
    float effectiveAirDensity,
    float effectiveWaterDensity,
    SimulationParameters const & simulationParameters)
{
    //
    // The hydrostatic pressure force acting on point P, between edges
    // E1 and E2, is:
    //
    //      F(P) = F(E1)/2 + F(E2)/2
    //
    // The hydrostatic pressure force acting on edge Ei is:
    //
    //      F(Ei) = -Ni * D * Mw * G * |Ei|
    //
    // Where Ni is the normal to Ei, D is the depth (which we take constant
    // per-frontier so to not produce buoyancy forces), Mw * G is the weight
    // of water, and |Ei| accounts for wider edges being subject to more pressure.
    //
    //
    // We will rewrite F(Ei) as:
    //
    //      F(Ei) = -Perp(Ei) * ForceStem
    //
    // And thus:
    //
    //      F(P)  = (-Perp(E1) -Perp(E2)) * ForceStem / 2
    //

    //
    // Given the discrete nature of the simulation, this implementation unfortunately results in residual unbalanced forces when all forces
    // are applied to the body. Such residual forces can take two forms: residual net force and/or torque, and large vibrations of the particles
    // in the mesh.
    //
    // These imperfections become seriously noticeable when pressure differentials (external-internal) are huge, which mostly happens at
    // great depths. It shows up as rotations, translations, contorsions, and still parts picking up silt from the bottom.
    //
    // In order to counteract these imperfections, we employ various mechanisms:
    // - We strive to ensure zero net-force and net-curl on the whole body; we do this
    //   by iteratively lowering forces according to their contribution to net-force and net-curl;
    // - We zero pressure against particles that are buried or close to silt;
    // - We scale down forces applied to small structures.
    //

    // Minimum size of a frontier to get some pressure. We do this early check to avoid
    // spending time when there's a zillion of frontiers, as in that case each frontier
    // would be small
    size_t constexpr MinFrontierSize = 40;
    if (frontier.Size < MinFrontierSize)
    {
        return;
    }

    // Notes:
    //  - We use the frontiers' gemetric centers as the place that depth is calculated at;
    //    as a consequence, if the ship is interactively moved or rotated, the centers
    //    that we use here are stale. Not a big deal...
    //    Outside of these "moving" interactions, the centers we use here would also be
    //    inconsistent with the current positions because of integration during dynamic
    //    iterations; for this reason, hydrostatic pressures are integrated only during
    //    the *first* dynamic iteration.
    //
    vec2f const & geometricCenterPosition = frontier.GeometricCenterPosition;
    float const oceanSurfaceY = mParentWorld.GetOceanSurface().GetHeightAt(geometricCenterPosition.x);
    float const depth = oceanSurfaceY - geometricCenterPosition.y;

    //
    // The forces that we calculate here are pressure differentials (ext - int), normalized to the expected
    // max differential which is the external pressure calculated at the center of the body.
    // At the end of the algorithm we will recover them by multiplying with the actual external pressure.
    //

    float const totalExternalPressure = Formulae::CalculateTotalPressureAt(
        geometricCenterPosition.y,
        oceanSurfaceY,
        effectiveAirDensity,
        effectiveWaterDensity,
        simulationParameters);

    assert(totalExternalPressure != 0.0f); // Air pressure is never zero

    // Counterbalance adjustment: a "trick" to reduce the effect of inner pressure on the external pressure
    // applied to the hull, so to generate higher hydrostatic forces - at lower depths. Basically to make
    // completely-flooded structures still implode.
    //
    // We want factor=1 above-water, and lower than 1 but converging to 1 the deeper we go;
    // to avoid discontinuities though, we lower it in a narrow band underwater
    float constexpr SmoothToZeroDepth = 50.0f;
    float const pressureCounterbalanceAdjustmentFactor = (depth < SmoothToZeroDepth)
        ? 1.0f - LinearStep(0.0f, SmoothToZeroDepth, depth)
        : LinearStep(SmoothToZeroDepth, SimulationParameters::HalfMaxWorldHeight, depth);

    float const forceNormalizationFactor = 1.0f / totalExternalPressure * pressureCounterbalanceAdjustmentFactor;

    //
    // 1. Calculate geometry of forces and populate interim buffer
    //
    // Here we calculate the *perpendicular* to each edge, rather than the normal, in order
    // to take into account the length of the edge, as the pressure force on an edge is
    // proportional to its "area" (length)
    //

    mStaticPressureBuffer.clear();

    // Note: these track *normalized* forces
    vec2f netForce = vec2f::zero();
    float netTorque = 0.0f;

    //
    // Visit all edges
    //
    //               thisPoint
    //                   V
    // ...---*---edge1---*---edge2---*---nextEdge---....
    //

    ElementIndex edge1Index = frontier.StartingEdgeIndex;
    ElementIndex prevPointIndex = mFrontiers.GetFrontierEdge(edge1Index).PointAIndex;
    vec2f prevPointPosition = mPoints.GetPosition(prevPointIndex);
    float prevPointSiltDamping = CalculateSiltPressureDamping(prevPointPosition, mParentWorld.GetOceanFloor());

    ElementIndex edge2Index = mFrontiers.GetFrontierEdge(edge1Index).NextEdgeIndex;
    ElementIndex thisPointIndex = mFrontiers.GetFrontierEdge(edge2Index).PointAIndex;
    vec2f thisPointPosition = mPoints.GetPosition(thisPointIndex);
    float thisPointSiltDamping = CalculateSiltPressureDamping(thisPointPosition, mParentWorld.GetOceanFloor());

    vec2f edge1PerpVector = -(thisPointPosition - prevPointPosition).to_perpendicular();

    int neighboringHullPointsCount =
        (mPoints.GetIsHull(prevPointIndex) ? 1 : 0)
        + (mPoints.GetIsHull(thisPointIndex) ? 1 : 0);

#ifdef _DEBUG
    ElementCount visitedPoints = 0;
#endif

    ElementIndex const startEdgeIndex = mFrontiers.GetFrontierEdge(edge2Index).NextEdgeIndex;

    for (ElementIndex nextEdgeIndex = startEdgeIndex; /*checked in loop*/; /*advanced in loop*/)
    {
#ifdef _DEBUG
        ++visitedPoints;
#endif
        auto const & nextEdge = mFrontiers.GetFrontierEdge(nextEdgeIndex);
        ElementIndex const nextPointIndex = nextEdge.PointAIndex;
        vec2f nextPointPosition = mPoints.GetPosition(nextPointIndex);
        float nextPointSiltDamping = CalculateSiltPressureDamping(nextPointPosition, mParentWorld.GetOceanFloor());

        vec2f edge2PerpVector = -(nextPointPosition - thisPointPosition).to_perpendicular();

        neighboringHullPointsCount += (mPoints.GetIsHull(nextPointIndex) ? 1 : 0);
        if (neighboringHullPointsCount == 3) // Avoid applying force to one or two isolated hull particles, allows for more stability of wretched wrecks
        {
            // Calculate normalized pressure force: we want the force vector
            // to be zero when internal pressure == external pressure.
            // Note that will be negative when internal>external - outward force!
            float const normalizedForceMagnitude = 1.0f - mPoints.GetInternalPressure(thisPointIndex) * forceNormalizationFactor;

            // Calculate static pressure force, and torque on whole body
            vec2f const forceVector =
                (edge1PerpVector + edge2PerpVector) / 2.0f * normalizedForceMagnitude
                * prevPointSiltDamping * thisPointSiltDamping * nextPointSiltDamping;

            vec2f const torqueArm = mPoints.GetPosition(thisPointIndex) - geometricCenterPosition;

            // EXPERIMENTAL: thickness adjustment, reduces some crazy clouds but does not make them
            // disappear completely, and given its price, we've turned it off for now.
            //////
            ////// Trick: avoid applying forces on the sides of a thin structure;
            ////// if we did, we'd cause contorsions of those poor structures
            //////

            ////if (normalizedForceMagnitude > 0.0f) // Only act on inward pressure
            ////{
            ////    // Calculate bulkiness heuristic: thickness of structure in the direction
            ////    // of force vector

            ////    vec2f const forceDir = forceVector.normalise();

            ////    int distanceToOppositeEdge = 0;
            ////    int constexpr MaxDistanceToOppositeEdge = 3; // Limit search space
            ////    ElementIndex currentVisitPointIndex = thisPointIndex;
            ////    for (; distanceToOppositeEdge < MaxDistanceToOppositeEdge; ++distanceToOppositeEdge)
            ////    {
            ////        vec2f const & currentVisitPointPosition = mPoints.GetPosition(currentVisitPointIndex);

            ////        // Find best spring
            ////        float bestProjection = 0.9063f; // cos(25): the angle between two adjacent springs is 45, half of it is the worst case scenario for a direction between two springs
            ////        ElementIndex bestNextPointIndex = NoneElementIndex;
            ////        for (auto const & connectedSpring : mPoints.GetConnectedSprings(currentVisitPointIndex).ConnectedSprings)
            ////        {
            ////            // Make sure there's a structure here
            ////            if (mSprings.GetCoveringTrianglesCount(connectedSpring.SpringIndex) > 0)
            ////            {
            ////                auto const springDir = (mPoints.GetPosition(connectedSpring.OtherEndpointIndex) - currentVisitPointPosition).normalise();
            ////                auto const projection = springDir.dot(forceDir);
            ////                if (projection >= bestProjection)
            ////                {
            ////                    bestProjection = projection;
            ////                    bestNextPointIndex = connectedSpring.OtherEndpointIndex;
            ////                }
            ////            }
            ////        }

            ////        if (bestNextPointIndex == NoneElementIndex)
            ////        {
            ////            // No luck, visit stops here
            ////            break;
            ////        }

            ////        // Advance
            ////        currentVisitPointIndex = bestNextPointIndex;
            ////    }

            ////    // Calculate scaling factor s:
            ////    //  - distance = 0, 1: s = 0.0
            ////    //  - distance = 2:s = 0.1
            ////    //  - distance = 3+: s = 1.0

            ////    assert(distanceToOppositeEdge <= MaxDistanceToOppositeEdge);
            ////    static float BulkinessScalingFactorsByDistance[MaxDistanceToOppositeEdge + 1] = { 0.0f, 0.0f, 0.1f, 1.0f };

            ////    forceVector *= BulkinessScalingFactorsByDistance[distanceToOppositeEdge];
            ////}

            // Store force
            mStaticPressureBuffer.emplace_back(
                thisPointIndex,
                forceVector,
                torqueArm);

            // Update resultant total force and torque
            netForce += forceVector;
            netTorque += torqueArm.cross(forceVector);
        }

        // Advance
        nextEdgeIndex = nextEdge.NextEdgeIndex;

        // Check whether we're done
        if (nextEdgeIndex == startEdgeIndex)
            break;

        neighboringHullPointsCount -= (mPoints.GetIsHull(prevPointIndex) ? 1 : 0);

        prevPointIndex = thisPointIndex;
        prevPointPosition = thisPointPosition;
        prevPointSiltDamping = thisPointSiltDamping;
        thisPointIndex = nextPointIndex;
        thisPointPosition = nextPointPosition;
        thisPointSiltDamping = nextPointSiltDamping;

        edge1PerpVector = edge2PerpVector;
    }

#ifdef _DEBUG
    assert(visitedPoints == frontier.Size);
#endif

    //
    // 2. Equalize forces to ensure they are zero-sum and zero-curl
    //
    // We do this via iterative optimization: at each iteration, we pick
    // the particle that has the most potential to affect the net force
    // and/or the net torque by getting its force reduced (via "lambda",
    // the force multiplicative factor).
    //
    // Note that this is basically a gradient descent search.
    //

    ElementCount iter;
    for (iter = 0; iter < frontier.Size; ++iter) // Heuristical max visit iterations, to ensure no super-long loops
    {
        // Check if we've reached a "minimum" that we're happy with
        if (netForce.length() < 0.5f
            && std::abs(netTorque) < 0.5f)
        {
            break;
        }

        float constexpr QuantizationRadius = 0.1f;

        // Find best particle
        std::optional<size_t> bestHPIndex;
        float bestLambda = 0.0f;
        if (netForce.length() >= std::abs(netTorque))
        {
            //
            // Find best lambda that minimizes the net force and, in case of a tie, the net torque as well
            //

            float minNetForceMagnitude = std::numeric_limits<float>::max();
            float minNetTorqueMagnitude = std::numeric_limits<float>::max();
            for (size_t hpi = 0; hpi < mStaticPressureBuffer.GetCurrentPopulatedSize(); ++hpi)
            {
                auto const & hp = mStaticPressureBuffer[hpi];

                vec2f const & thisForce = hp.ForceVector;

                if (thisForce != vec2f::zero())
                {
                    // Find lambda that minimizes magnitude of force:
                    //      Magnitude(l) = |NetForce(l)| = |NetForcePrev + ThisForce*l|
                    //      dMagnitude(l)/dl = 2*l*(ThisForce.x^2 + ThisForce.y^2) + 2*(NetForcePrev.x*ThisForce.x + NetForcePrev.y*ThisForce.y)
                    //      dMagnitude(l)/dl = 0 => l = NetForcePrev.dot(ThisForce) / |ThisForce|^2
                    float const lambdaFRaw = -(netForce - thisForce).dot(thisForce) / thisForce.squareLength();
                    if (lambdaFRaw < 1.0f) // Ensure it's a change wrt now, and that we don't amplify existing forces
                    {
                        float const lambda = std::max(lambdaFRaw, 0.0f);

                        // Remember best
                        float const newNetForceMagnitude = (netForce - thisForce * (1.0f - lambda)).length();
                        float const thisTorque = hp.TorqueArm.cross(thisForce);
                        float const newNetTorqueMagnitude = std::abs(netTorque - thisTorque * (1.0f - lambda));
                        if (newNetForceMagnitude < minNetForceMagnitude - QuantizationRadius
                            || (newNetForceMagnitude < minNetForceMagnitude + QuantizationRadius && newNetTorqueMagnitude < minNetTorqueMagnitude))
                        {
                            minNetForceMagnitude = newNetForceMagnitude;
                            minNetTorqueMagnitude = newNetTorqueMagnitude;
                            bestHPIndex = hpi;
                            bestLambda = lambda;
                        }
                    }
                }
            }
        }
        else
        {
            //
            // Find best lambda that minimizes the net torque and, in case of a tie, the net force as well
            //

            float minNetForceMagnitude = std::numeric_limits<float>::max();
            float minNetTorqueMagnitude = std::numeric_limits<float>::max();
            for (size_t hpi = 0; hpi < mStaticPressureBuffer.GetCurrentPopulatedSize(); ++hpi)
            {
                auto const & hp = mStaticPressureBuffer[hpi];

                vec2f const & thisForce = hp.ForceVector;
                float const thisTorque = hp.TorqueArm.cross(thisForce);

                if (thisTorque != 0.0f)
                {
                    // Calculate lambda at which netTorque is zero:
                    //      NetTorque(l) = NetTorquePrev + l*ThisTorque
                    //      NetTorque(l) = 0 => l = -NetTorquePrev/ThisTorque
                    float const lambdaTRaw = -(netTorque - thisTorque) / thisTorque;
                    if (lambdaTRaw < 1.0f) // Ensure it's a change wrt now, and that we don't amplify existing forces
                    {
                        float const lambda = std::max(lambdaTRaw, 0.0f);

                        // Remember best
                        float const newNetForceMagnitude = (netForce - thisForce * (1.0f - lambda)).length();
                        float const newNetTorqueMagnitude = std::abs(netTorque - thisTorque * (1.0f - lambda));
                        if (newNetTorqueMagnitude < minNetTorqueMagnitude - QuantizationRadius
                            || (newNetTorqueMagnitude < minNetTorqueMagnitude + QuantizationRadius && newNetForceMagnitude < minNetForceMagnitude))
                        {
                            minNetForceMagnitude = newNetForceMagnitude;
                            minNetTorqueMagnitude = newNetTorqueMagnitude;
                            bestHPIndex = hpi;
                            bestLambda = lambda;
                        }
                    }
                }
            }
        }

        if (!bestHPIndex.has_value()
            || bestLambda > 0.999f) // Infinitesimal change which provides *most magnitude change* won't change much
        {
            // Couldn't find a minimizer, stop
            break;
        }

        vec2f const thisForce = mStaticPressureBuffer[*bestHPIndex].ForceVector;
        float const thisTorque = mStaticPressureBuffer[*bestHPIndex].TorqueArm.cross(thisForce);

        // Adjust force vector of optimal particle
        mStaticPressureBuffer[*bestHPIndex].ForceVector *= bestLambda;

        // Update net force and torque
        //
        // Note: we've debugged these, they end up being exactly identical to newly-calculated finals
        netForce -= thisForce * (1.0f - bestLambda);
        netTorque -= thisTorque * (1.0f - bestLambda);
    }

    // Update stats (aggregate across all frontiers)
    mStaticPressureNetForceMagnitudeSum += netForce.length();
    mStaticPressureNetForceMagnitudeCount += 1.0f;
    mStaticPressureIterationsPercentagesSum += static_cast<float>(iter + 1) / static_cast<float>(frontier.Size);
    mStaticPressureIterationsCount += 1.0f;

    //
    // 3. Apply forces as dynamic forces - so they only apply to current positions
    //    (i.e. first iteration of integration and not any subsequent ones),
    //    as these forces are very sensitive to their position, and would generate
    //    phantom forces and torques otherwise
    //

    // Trick: avoid applying too much pressure to small frontiers, as
    // huge forces on small frontiers are difficult to round out.
    //
    // Empirical table:
    //  235=1
    //  180=1
    //   47~=0
    //   13=0
    float const pressureBulkinessMultiplier = LinearStep(
        static_cast<float>(MinFrontierSize),
        180.0f,
        static_cast<float>(frontier.Size));

    float const forceMultiplier =
        totalExternalPressure // Force vector is normalized to external pressure, and remember: it includes contribution from internal pressure
        * pressureBulkinessMultiplier
        * simulationParameters.StaticPressureForceAdjustment
        * mRepairGracePeriodMultiplier; // Static pressure hinders the repair process

    size_t const particleCount = mStaticPressureBuffer.GetCurrentPopulatedSize();
    for (size_t hpi = 0; hpi < particleCount; ++hpi)
    {
        mPoints.AddDynamicForce0(
            mStaticPressureBuffer[hpi].PointIndex,
            mStaticPressureBuffer[hpi].ForceVector * forceMultiplier);
    }
}

void Ship::TrimForWorldBounds(SimulationParameters const & simulationParameters)
{
    float constexpr MaxWorldLeft = -SimulationParameters::HalfMaxWorldWidth;
    float constexpr MaxWorldRight = SimulationParameters::HalfMaxWorldWidth;

    float constexpr MaxWorldTop = SimulationParameters::HalfMaxWorldHeight;
    float constexpr MaxWorldBottom = -SimulationParameters::HalfMaxWorldHeight;

    // Elasticity of the bounce against world boundaries
    //  - We use the ocean floor bedrock's elasticity for convenience
    float const elasticity = simulationParameters.OceanFloorBedrockElasticityCoefficient * simulationParameters.ElasticityAdjustment;

    // We clamp velocity to damp system instabilities at extreme events
    static constexpr float MaxBounceVelocity = 150.0f; // Magic number

    // Visit all points
    vec2f * const restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * const restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();
    size_t const count = mPoints.GetBufferElementCount();
    for (size_t p = 0; p < count; ++p)
    {
        auto const & pos = positionBuffer[p];

        if (pos.x < MaxWorldLeft)
        {
            // Simulate bounce, bounded
            positionBuffer[p].x = std::min(MaxWorldLeft + elasticity * (MaxWorldLeft - pos.x), 0.0f);

            // Bounce bounded
            velocityBuffer[p].x = std::min(-velocityBuffer[p].x, MaxBounceVelocity);
        }
        else if (pos.x > MaxWorldRight)
        {
            // Simulate bounce, bounded
            positionBuffer[p].x = std::max(MaxWorldRight - elasticity * (pos.x - MaxWorldRight), 0.0f);

            // Bounce bounded
            velocityBuffer[p].x = std::max(-velocityBuffer[p].x, -MaxBounceVelocity);
        }

        if (pos.y > MaxWorldTop)
        {
            // Simulate bounce, bounded
            positionBuffer[p].y = std::max(MaxWorldTop - elasticity * (pos.y - MaxWorldTop), 0.0f);

            // Bounce bounded
            velocityBuffer[p].y = std::max(-velocityBuffer[p].y, -MaxBounceVelocity);
        }
        else if (pos.y < MaxWorldBottom)
        {
            // Simulate bounce, bounded
            positionBuffer[p].y = std::min(MaxWorldBottom + elasticity * (MaxWorldBottom - pos.y), 0.0f);

            // Bounce bounded
            velocityBuffer[p].y = std::min(-velocityBuffer[p].y, MaxBounceVelocity);
        }

        assert(positionBuffer[p].x >= MaxWorldLeft);
        assert(positionBuffer[p].x <= MaxWorldRight);
        assert(positionBuffer[p].y >= MaxWorldBottom);
        assert(positionBuffer[p].y <= MaxWorldTop);
    }

#ifdef _DEBUG
    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

///////////////////////////////////////////////////////////////////////////////////
// Pressure and water Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdatePressureAndWaterInflow(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters,
    float & waterTakenInStep)
{
    //
    // Intake/outtake air and water into/from all the leaking nodes (structural or forced)
    //
    // Ephemeral points are never leaking, hence we ignore them
    //

    // Multiplier to get internal pressure delta from water delta
    float const volumetricWaterPressure = Formulae::CalculateVolumetricWaterPressure(simulationParameters.WaterTemperature, simulationParameters);

    // Equivalent depth of a point when it's exposed to rain
    float const rainEquivalentWaterHeight =
        stormParameters.RainQuantity // m/h
        / 3600.0f // -> m/s
        * SimulationParameters::SimulationStepTimeDuration<float> // -> m/step
        * simulationParameters.RainFloodAdjustment;

    float const waterPumpPowerMultiplier =
        simulationParameters.WaterPumpPowerAdjustment
        * (simulationParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    bool const doGenerateAirBubbles = (simulationParameters.AirBubblesDensity != 0.0f);

    float const cumulatedOutflownAirPressureThresholdForAirBubbles =
        SimulationParameters::AirBubblesDensityToCumulatedOutflownAirPressure(simulationParameters.AirBubblesDensity);

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        // This is one of the few cases in which we prefer branching over calculating
        // for all points, mostly because we expect a tiny fraction of all points to
        // be leaking at any moment
        auto const & pointCompositeLeaking = mPoints.GetLeakingComposite(pointIndex);
        if (pointCompositeLeaking.IsCumulativelyLeaking)
        {
            // Point could be structurally hull or not; could be leaking if it's a watertight door

            float const pointDepth = mPoints.GetCachedDepth(pointIndex);

            // External water height
            //
            // We also incorporate rain in the sources of external water height:
            // - If point is below water surface: external water height is due to depth
            // - If point is above water surface: external water height is due to rain
            float const externalWaterHeight = std::max(
                pointDepth + 0.1f, // Magic number to force flotsam to take some water in and eventually sink
                rainEquivalentWaterHeight); // At most is one meter, so does not interfere with underwater pressure

            // Internal water height
            float const internalWaterHeight = mPoints.GetWater(pointIndex);

            // Quantities exchanged
            float totalPointDeltaWater = 0.0f; // TODO: needed?
            float totalPointDeltaAirPressureLost = 0.0f;
            float totalPointDeltaWaterForStepTotal = 0.0f; // For total returned - discounts orhpaned points' structural

            if (pointCompositeLeaking.LeakingSources.StructuralLeak != 0.0f)
            {
                //
                // 1. Update water due to structural leaks (holes)
                //

                {
                    //
                    // 1.1) Calculate velocity of incoming water, based off Bernoulli's equation applied to point:
                    //  v**2/2 + Dp/density = c (assuming y of incoming water does not change along the intake)
                    //      With: Dp = delta pressure = |total external pressure - total internal pressure|
                    //
                    // However, given the simulation's propensity to compress air too much, we lower water intake
                    // by only considering the external _water_ pressure (i.e. ignoring atmospheric pressure), also
                    // simplifying the math along the way.
                    //
                    // With this simplification, Dp = Dh * density * g, with Dh = |external water height - internal water height|.
                    // Considering that at equilibrium we have v=0 and Dp=0, then c is 0, and thus velocity becomes:
                    //  v = +/- sqrt(2*g*|Dh|)
                    //




                    // TODOTEST: ORIG
                    //float incomingWaterVelocity_Structural;
                    //if (externalWaterHeight >= internalWaterHeight)
                    //{
                    //    // Incoming water
                    //    incomingWaterVelocity_Structural = sqrtf(2.0f * SimulationParameters::GravityMagnitude * (externalWaterHeight - internalWaterHeight));
                    //}
                    //else
                    //{
                    //    // Outgoing water
                    //    incomingWaterVelocity_Structural = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * (internalWaterHeight - externalWaterHeight));
                    //}


                    // TODOTEST: NEW
                    float const externalTotalPressure =
                        Formulae::PressureToEquivalentWaterHeight(
                            Formulae::CalculateTotalPressureAt(
                                mPoints.GetPosition(pointIndex).y,
                                mPoints.GetPosition(pointIndex).y + pointDepth, // oceanSurfaceY
                                effectiveAirDensity,
                                effectiveWaterDensity,
                                simulationParameters),
                            effectiveWaterDensity);

                    float const internalTotalPressure = mPoints.GetWater(pointIndex) + mPoints.GetAirPressure(pointIndex);

                    float incomingWaterVelocity_Structural;
                    if (externalTotalPressure >= internalTotalPressure)
                    {
                        // Incoming water
                        if (pointDepth > 0.0f)
                            incomingWaterVelocity_Structural = sqrtf(2.0f * SimulationParameters::GravityMagnitude * (externalTotalPressure - internalTotalPressure));
                        else
                            incomingWaterVelocity_Structural = 0.0f;
                    }
                    else
                    {
                        // Outgoing water
                        incomingWaterVelocity_Structural = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * (internalTotalPressure - externalTotalPressure));
                    }













                    //
                    // 1.2) In/Outtake water according to velocity:
                    // - During dt, we move a volume of water Vw equal to A*v*dt; the equivalent change in water
                    //   height is thus Vw/A, i.e. v*dt
                    //

                    float deltaWater_Structural =
                        incomingWaterVelocity_Structural
                        * SimulationParameters::SimulationStepTimeDuration<float>
                        // TODOTEST
                        //* mPoints.GetMaterialWaterIntake(pointIndex)
                        * simulationParameters.WaterIntakeAdjustment;

                    //
                    // 1.3) Update water
                    //

                    if (deltaWater_Structural < 0.0f)
                    {
                        // Outgoing water

                        // Make sure we don't over-drain the point
                        deltaWater_Structural = std::max(-mPoints.GetWater(pointIndex), deltaWater_Structural);

                        // TODOTEST
                        ////// Honor the water retention of this material
                        ////deltaWater_Structural *= mPoints.GetMaterialWaterRestitution(pointIndex);
                    }

                    // Adjust water
                    mPoints.SetWater(
                        pointIndex,
                        mPoints.GetWater(pointIndex) + deltaWater_Structural);

                    // Update total delta water
                    totalPointDeltaWater += deltaWater_Structural;
                    if (!mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.empty())
                    {
                        // Only count water taken if this point has a spring, to avoid counting
                        // water and generating bubbles for orphaned particles
                        // (note that leaking points have no connected triangles)
                        totalPointDeltaWaterForStepTotal += deltaWater_Structural;
                    }
                }

                //
                // 2. Update air pressure due to structural leaks (holes)
                //

                {
                    //
                    // - If underwater: leaves completely
                    // - If abovewater: enters/leaves and (air/total) pressure outside <> air pressure inside
                    //

                    // External air pressure in equivalent water height
                    float const externalAirPressure = (pointDepth >= 0.0f)
                        ? 0.0f // Device to force all air to be espelled when underwater
                        : Formulae::PressureToEquivalentWaterHeight(
                            Formulae::CalculateAirColumnPressureAt(
                                mPoints.GetPosition(pointIndex).y,
                                effectiveAirDensity,
                                simulationParameters),
                            effectiveWaterDensity);

                    // Internal pressure in equivalent water height
                    float const internalAirPressure = mPoints.GetAirPressure(pointIndex);

                    if (internalAirPressure >= externalAirPressure // Always if underwater
                        || pointDepth < 0.0f) // Air can only come in if there's air outside
                    {
                        // Assuming that external pressure is an infinite reservoir,
                        // we converge internal pressure to the external
                        // See TODO for rate here
                        float const newAirPressure = externalAirPressure;

                        totalPointDeltaAirPressureLost += mPoints.GetAirPressure(pointIndex) - newAirPressure;

                        mPoints.SetAirPressure(
                            pointIndex,
                            newAirPressure);
                    }
                }
            }

            float const waterPumpForce = pointCompositeLeaking.LeakingSources.WaterPumpForce;
            if (waterPumpForce != 0.0f)
            {
                //
                // 3) Update water due to forced leaks (pumps)
                //    (positive is incoming)
                //

                float deltaWater_Forced = 0.0f;
                if (waterPumpForce > 0.0f)
                {
                    // Inward pump: only works if underwater
                    deltaWater_Forced = (externalWaterHeight > 0.0f)
                        ? waterPumpForce * waterPumpPowerMultiplier // No need to cap as sea is infinite
                        : 0.0f;
                }
                else
                {
                    // Outward pump: only works if water inside
                    deltaWater_Forced = (internalWaterHeight > 0.0f)
                        ? waterPumpForce * waterPumpPowerMultiplier // We'll cap it
                        : 0.0f;
                }

                // Make sure we don't over-drain the point
                deltaWater_Forced = std::max(-mPoints.GetWater(pointIndex), deltaWater_Forced);

                // Adjust water
                mPoints.SetWater(
                    pointIndex,
                    mPoints.GetWater(pointIndex) + deltaWater_Forced);

                // Update total delta water
                totalPointDeltaWater += deltaWater_Forced;
                totalPointDeltaWaterForStepTotal += deltaWater_Forced;

                //
                // 4) Update pressure due to forced leaks (pumps)
                //    (positive is incoming)
                //
                //    Forced delta pressure depends on (effective) forced delta water only
                //

                float const deltaPressure_Forced = deltaWater_Forced * volumetricWaterPressure;

                mPoints.SetInternalPressure(
                    pointIndex,
                    std::max(mPoints.GetInternalPressure(pointIndex) + deltaPressure_Forced, 0.0f)); // Make sure we don't over-drain the point
            }

            //
            // 5) Check if it's time to produce air bubbles
            //

            float newPointCumulatedOutflownAirPressure = mPoints.GetCumulatedOutflownAirPressure(pointIndex) + totalPointDeltaAirPressureLost;
            if (newPointCumulatedOutflownAirPressure > cumulatedOutflownAirPressureThresholdForAirBubbles)
            {
                // Generate air bubbles - but not on ropes as that looks awful
                if (doGenerateAirBubbles
                    && !mPoints.IsRope(pointIndex)
                    && pointDepth >= 0.0f) // TODOHERE: see notes on pressure lost / underwater
                {
                    InternalSpawnAirBubble(
                        mPoints.GetPosition(pointIndex),
                        pointDepth,
                        SimulationParameters::ShipAirBubbleFinalScale,
                        mPoints.GetTemperature(pointIndex),
                        mPoints.GetPlaneId(pointIndex),
                        currentSimulationTime,
                        simulationParameters);
                }

                // Consume all cumulated air pressure lost
                newPointCumulatedOutflownAirPressure = 0.0f;
            }

            mPoints.SetCumulatedOutflownAirPressure(pointIndex, newPointCumulatedOutflownAirPressure);

            // Adjust total water taken during this step, but not counting
            // ropes, to prevent "rushing water" sound from playing for
            // ropes, and also to prevent rope-only ships from playing
            // "farewell"
            if (!mPoints.IsRope(pointIndex))
            {
                waterTakenInStep += totalPointDeltaWaterForStepTotal;
            }
        }
    }
}

void Ship::EqualizeInternalPressure(SimulationParameters const & /*simulationParameters*/)
{
    // Local cache of indices of other endpoints
    FixedSizeVector<ElementIndex, SimulationParameters::MaxSpringsPerPoint> otherEndpoints;

    //
    // For each (non-ephemeral) point, equalize its internal pressure with its
    // neighbors
    //

    float * restrict internalPressureBufferData = mPoints.GetInternalPressureBufferAsFloat();
    bool const * restrict isHullBufferData = mPoints.GetIsHullBuffer();

    for (auto pointIndex : mPoints.RawShipPoints()) // No need to visit ephemeral points as they have no springs
    {
        if (!isHullBufferData[pointIndex])
        {
            //
            // Non-hull particle: flow its surplus pressure to its neighbors
            //

            float const internalPressure = internalPressureBufferData[pointIndex];

            //
            // 1. Calculate average internal pressure among this particle and all its neighbors that have
            // lower internal pressure
            //

            float averageInternalPressure = internalPressure;
            float targetEndpointsCount = 1.0f;

            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                ElementIndex const otherEndpointIndex = cs.OtherEndpointIndex;

                // We only consider outgoing pressure, not towards hull points
                float const otherEndpointInternalPressure = internalPressureBufferData[otherEndpointIndex];
                if (internalPressure > otherEndpointInternalPressure
                    && !isHullBufferData[otherEndpointIndex])
                {
                    averageInternalPressure += otherEndpointInternalPressure;
                    targetEndpointsCount += 1.0f;

                    otherEndpoints.emplace_back(otherEndpointIndex);
                }
            }

            averageInternalPressure /= targetEndpointsCount;

            //
            // 2. Distribute surplus pressure
            //

            internalPressureBufferData[pointIndex] = averageInternalPressure;

            for (auto const & otherEndpointIndex : otherEndpoints)
            {
                internalPressureBufferData[otherEndpointIndex] = averageInternalPressure;
            }

            otherEndpoints.clear();
        }
        else
        {
            //
            // Hull particle: set its internal pressure to the average internal pressure
            // of all its non-hull neighbors
            //

            float averageInternalPressure = 0.0f;
            float neighborsCount = 0.0f;

            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                ElementIndex const otherEndpointIndex = cs.OtherEndpointIndex;
                if (!isHullBufferData[otherEndpointIndex])
                {
                    averageInternalPressure += internalPressureBufferData[otherEndpointIndex];
                    neighborsCount += 1.0f;
                }
            }

            if (neighborsCount != 0.0f)
            {
                internalPressureBufferData[pointIndex] = averageInternalPressure / neighborsCount;
            }
        }
    }
}

void Ship::UpdateWaterVelocities(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move each spring's outgoing water momentum to
    // its destination point
    //
    // Implementation of https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // Calculate water momenta
    mPoints.UpdateWaterMomentaFromVelocities();

    // Source and result water buffers
    auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
    float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total weight
    float totalOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-oldPointWaterBufferData[pointIndex] * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif

    //
    // Visit all non-ephemeral points and move water and its momenta
    //
    // No need to visit ephemeral points as they have no springs
    //

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        //
        // 1) Calculate water momenta along *all* springs connected to this point,
        //    including impermeable ones - as we'll eventually bounce back along those
        //

        // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
        // and gravity were exaggerated
        //
        // WV[t] = WV[t-1] + alpha * Bernoulli
        //
        // WaterCrazyness=0   -> alpha=1
        // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
        // WaterCrazyness=1   -> alpha=Wh
        float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

#if !FS_IS_PLATFORM_MOBILE()
        pointSplashNeighbors = 0.0f;
        pointSplashFreeNeighbors = 0.0f;
#endif

        totalOutboundWaterFlowWeight = 0.0f;

        size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Normalized spring vector, oriented point -> other endpoint
            vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

            // Component of the point's own water velocity along the spring
            float const pointWaterVelocityAlongSpring =
                oldPointWaterVelocityBufferData[pointIndex]
                .dot(springNormalizedVector);

            //
            // Calulate Bernoulli's velocity gained along this spring, from this point to
            // the other endpoint
            //

            // Pressure difference (positive implies point -> other endpoint flow)
            float const dw = oldPointWaterBufferData[pointIndex] - oldPointWaterBufferData[cs.OtherEndpointIndex];

            // Gravity potential difference (positive implies point -> other endpoint flow)
            float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

            // Calculate gained water velocity along this spring, from point to other endpoint
            // (Bernoulli, 1738)
            float bernoulliVelocityAlongSpring;
            float const dwy = dw + dy;
            if (dwy >= 0.0f)
            {
                // Gained velocity goes from point to other endpoint
                bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
            }
            else
            {
                // Gained velocity goes from other endpoint to point
                bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
            }

            // Resultant scalar velocity along spring; outbound only, as
            // if this were inbound it wouldn't result in any movement of the point's
            // water between these two springs. Morevoer, Bernoulli's velocity injected
            // along this spring will be picked up later also by the other endpoint,
            // and at that time it would move water if it agrees with its velocity
            float const springOutboundScalarWaterVelocity = std::max(
                pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                0.0f);

            // Store weight along spring, scaling for the greater distance traveled along
            // diagonal springs
            springOutboundWaterFlowWeights[s] =
                springOutboundScalarWaterVelocity
                / mSprings.GetFactoryRestLength(cs.SpringIndex);

            // Resultant outbound velocity along spring
            springOutboundWaterVelocities[s] =
                springNormalizedVector
                * springOutboundScalarWaterVelocity;

            // Update total outbound flow weight
            totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];

#if !FS_IS_PLATFORM_MOBILE()
            //
            // Update splash neighbors counts
            //

            pointSplashFreeNeighbors +=
                mSprings.GetWaterPermeability(cs.SpringIndex)
                * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

            pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif
        }

        //
        // 2) Calculate normalization factor for water flows:
        //    the quantity of water along a spring is proportional to the weight of the spring
        //    (resultant velocity along that spring), and the sum of all outbound water flows must
        //    match the water currently at the point times the water speed fraction and the adjustment
        //

        assert(totalOutboundWaterFlowWeight >= 0.0f);

        float waterQuantityNormalizationFactor = 0.0f;
        if (totalOutboundWaterFlowWeight != 0.0f)
        {
            waterQuantityNormalizationFactor =
                oldPointWaterBufferData[pointIndex]
                * mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment
                / totalOutboundWaterFlowWeight;
        }

        //
        // 3) Move water along all springs according to their flows,
        //    and update destination's momenta accordingly
        //

#if !FS_IS_PLATFORM_MOBILE()
        // Kinetic energy lost at this point
        pointKineticEnergyLoss = 0.0f;
#endif

        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Calculate quantity of water directed outwards
            float const springOutboundQuantityOfWater =
                springOutboundWaterFlowWeights[s]
                * waterQuantityNormalizationFactor;

            assert(springOutboundQuantityOfWater >= 0.0f);

            if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
            {
                //
                // Water - and momentum - move from point to endpoint
                //

                // Move water quantity
                newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                newPointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                // Remove "old momentum" (old velocity) from point
                newPointWaterMomentumBufferData[pointIndex] -=
                    oldPointWaterVelocityBufferData[pointIndex]
                    * springOutboundQuantityOfWater;

                // Add "new momentum" (old velocity + velocity gained) to other endpoint
                newPointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update point's kinetic energy loss:
                // splintered water colliding with whole other endpoint
                //

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();
                float mb = oldPointWaterBufferData[cs.OtherEndpointIndex];
                float vb = oldPointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                float vf = 0.0f;
                if (ma + mb != 0.0f)
                    vf = (ma * va + mb * vb) / (ma + mb);

                float deltaKa =
                    0.5f
                    * ma
                    * (va * va - vf * vf);

                // Note: deltaKa might be negative, in which case deltaKb would have been
                // more positive (perfectly inelastic -> deltaK == max); we will pickup
                // deltaKb later
                pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
            }
            else
            {
                // Wall hit

                // Deleted springs are removed from points' connected springs
                assert(!mSprings.IsDeleted(cs.SpringIndex));

                //
                // New momentum (old velocity + velocity gained) bounces back
                // (and zeroes outgoing), assuming perfectly inelastic collision
                //
                // No changes to other endpoint
                //

                newPointWaterMomentumBufferData[pointIndex] -=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update point's kinetic energy loss:
                // entire splintered water
                //

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();

                float deltaKa =
                    0.5f
                    * ma
                    * va * va;

                assert(deltaKa >= 0.0f);
                pointKineticEnergyLoss += deltaKa;
#endif
            }
        }

#if !FS_IS_PLATFORM_MOBILE()
        //
        // 4) Update water splash
        //

        if (pointSplashNeighbors != 0.0f)
        {
            // Water splashed is proportional to kinetic energy loss that took
            // place near free points (i.e. not drowned by water)
            waterSplashed +=
                pointKineticEnergyLoss
                * pointSplashFreeNeighbors
                / pointSplashNeighbors;
        }
#endif
    }

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif


    //
    // Transforming momenta into velocities
    //

    mPoints.UpdateWaterVelocitiesFromMomenta();




    //
    // TODOTEST: readings
    //

    std::vector<PressureReading> readings;

    //ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8150;
    //ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 738;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        ElementIndex prevPointIndex = PressureCrossCutReadingsStartPointIndex;
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                0.0f,
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y });

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            prevPointIndex = pointIndex;
            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);

    // TODOTEST
    // Calculate total water after
    float totalWaterPost = 0.0f;
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        totalWaterPost += mPoints.GetWater(pointIndex);
    }
    mSimulationEventHandler.OnCustomProbe("Total W In", totalWaterPost);
}

void Ship::UpdateWaterAndAirPressure_NewtonRhapson(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move water and air along its connected springs,
    // based on pressure differentials and water momenta (https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/)
    //
    // Model is tanks, connected at bottom (for water moves) and at top (for air moves)
    //    - Hence, water moves are governed by pressures at bottom, which are air pressure + water pressure, and water momenta
    //    - Hence, air moves are governed by pressures at top, which are air pressure
    //      - But compressibility of air plays a role - i.e.water volumes plays a role
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // TODOTEST: moved into loop
    //// Calculate water momenta
    //mPoints.UpdateWaterMomentaFromVelocities();

    //// Source and result water buffers
    //auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    //float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
    //float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    //vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    //vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    //// Source and result air buffers
    //auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
    //float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
    //float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total water flow weight
    float totalOutboundWaterFlowWeight;
    float maxOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    // Weights of outbound air flows along each spring, only permeable ones;
    // set to zero for springs whose resultant scalar air flows are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundAirFlowWeights;

    // Total air flow weight
    float totalOutboundAirFlowWeight;
    float maxOutboundAirFlowWeight;
    float totalOutboundAirFlowWeightSquared = 0.0f;

    auto const squeezeAir = [&](float air, float water)
        {
            // TODOTEST: original
            //float const availableAirVolume = 1.0f / (1.0f + water);

            // TODOTEST: capped
            //float const availableAirVolume = std::max(1.0f - water, 0.1f);

            // TODOTEST: with slider
            return air * (1.0f + water * simulationParameters.ElectricalElementHeatProducedAdjustment);
        };

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-mPoints.GetWater(pointIndex) * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif

    //
    // Visit all non-ephemeral points and:
    //  - Move water and its momenta according to momenta and pressure differentials
    //  - Move air (pressure) according to pressure differentials (and volumetric bias)
    //
    // No need to visit ephemeral points as they have no springs
    //

    // TODOTEST
    //int constexpr NumberOfIterations = 4;
    //int constexpr NumberOfIterations = 2;
    int constexpr NumberOfIterations = 1;
    //int constexpr NumberOfIterations = 32;
    for (int iter = 0; iter < NumberOfIterations; ++iter)
    {


        // TODOTEST: moved from outside into loop
        // Calculate water momenta
        mPoints.UpdateWaterMomentaFromVelocities();



        // Source and result water buffers
        auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
        float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
        float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
        vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
        vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

        // Source and result air buffers
        auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
        //float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
        //float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();
        float const * oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
        float * newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();


        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("Start W: ", oldPointWaterBufferData[mLastQueriedPointIndex], "  Start A: ", oldPointAirPressureBufferData[mLastQueriedPointIndex]);
        }
        float todoTotalWOut = 0.0f;
        float todoTotalAOut = 0.0f;


        for (auto pointIndex : mPoints.RawShipPoints())
        {
            //
            // 1a) Calculate water momenta along *all* springs connected to this point,
            //     including impermeable ones - as we'll eventually bounce back along those
            // 1b) Calculate air pressure transfers along travelable springs connected to this point
            //

            // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
            // and gravity were exaggerated
            //
            // WV[t] = WV[t-1] + alpha * Bernoulli
            //
            // WaterCrazyness=0   -> alpha=1
            // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
            // WaterCrazyness=1   -> alpha=Wh
            float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

            // Total pressure at bottom of this point/tank
            // TODOTEST
            //float const oldThisPointTotalPressureAtBottom = oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex];

            // Volume at this tank that is available for air;
            // given that plain water would cause non-linearities, we make
            // air volume go to zero only asymptotically
            // TODOTEST
            //float const oldThisPointAvailableAirVolume = 1.0f / (1.0f + oldPointWaterBufferData[pointIndex]);

#if !FS_IS_PLATFORM_MOBILE()
            pointSplashNeighbors = 0.0f;
            pointSplashFreeNeighbors = 0.0f;
#endif

            totalOutboundWaterFlowWeight = 0.0f;
            maxOutboundWaterFlowWeight = 0.0f;
            totalOutboundAirFlowWeight = 0.0f;
            maxOutboundAirFlowWeight = 0.0f;
            totalOutboundAirFlowWeightSquared = 0.0f;

            size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                // Upness: TODOHERE -- 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector

                // TODOTEST
                //float const springUpness = springNormalizedVector.y;

                // TODOTEST: step
                //float const springUpness = Step(0.0f, springNormalizedVector.y);
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: 0->1 smooth
                //float const springUpness = (1.0f + springNormalizedVector.y) / 2.0f;
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: -1->1 smooth
                //float const springUpness = springNormalizedVector.y;
                //float const springDownness = -springUpness;

                // TODOTEST: 0->1->1 smooth
                //float const springUpness = std::min(springNormalizedVector.y + 1.0f, 1.0f);
                //float const springDownness = std::min(1.0f - springNormalizedVector.y, 1.0f);

                // TODOTEST: 0->0->1 smooth
                float const springUpness = std::max(springNormalizedVector.y, 0.0f);
                float const springDownness = std::max(-springNormalizedVector.y, 0.0f);

                //
                // Water
                //
                // Moves according to water momentum + pressure differentials
                //    - Source pressure is water pressure + air pressure
                //    - Destination pressure:
                //      - When diffusing up: water pressure + air pressure
                //      - When diffusing down: water pressure - air pressure (Rayleigh–Taylor instability: water is not stopped by air below - actually drawn down)
                //

                // Component of the point's own water velocity along the spring
                float const pointWaterVelocityAlongSpring =
                    oldPointWaterVelocityBufferData[pointIndex]
                    .dot(springNormalizedVector);

                //
                // Calulate Bernoulli's velocity gained along this spring, from this point to
                // the other endpoint
                //

                // TODOOLD
                // Pressure difference (positive implies point -> other endpoint flow)
                // Bias with air below (Rayleigh–Taylor instability):
                //  - Going up: this total_pressure - other total_pressure
                //  - Going down: this total_pressure - (other water_pressure - other air_pressure)
                //float const dw =
                //    oldThisPointTotalPressureAtBottom
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);


                // TODOTEST
                //float const dwUp = oldPointWaterBufferData[pointIndex] - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);
                //float const dwDown = (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex]) - oldPointWaterBufferData[cs.OtherEndpointIndex];


                // TODOTEST
                //float const dwUp = oldPointWaterBufferData[pointIndex] - (oldPointWaterBufferData[cs.OtherEndpointIndex] + squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]));
                //float const dwDown = (oldPointWaterBufferData[pointIndex] + squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex])) - oldPointWaterBufferData[cs.OtherEndpointIndex];
                //float const dw =
                //    (dwUp * springUpness + dwDown * springDownness)
                //    * mSprings.GetWaterPermeability(cs.SpringIndex); // Enforce no delta-pressure with (dry) wall

                // New factor 1: elegant
                float const dw =
                    (
                        oldPointWaterBufferData[pointIndex] - oldPointWaterBufferData[cs.OtherEndpointIndex]
                        + squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex]) * springDownness
                        - squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]) * springUpness
                    )
                    * mSprings.GetWaterPermeability(cs.SpringIndex); // Enforce no delta-pressure with (dry) wall


                // Gravity potential difference (positive implies point -> other endpoint flow)
                float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

                // Calculate gained water velocity along this spring, from point to other endpoint
                // (Bernoulli, 1738)
                //
                // We add pressure and heights as pressure is in "height equivalent units"
                float bernoulliVelocityAlongSpring;
                float const dwy = dw + dy;
                if (dwy >= 0.0f)
                {
                    // Gained velocity goes from point to other endpoint
                    bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
                }
                else
                {
                    // Gained velocity goes from other endpoint to point
                    bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
                }

                // Resultant scalar velocity along spring; outbound only, as
                // if this were inbound it wouldn't result in any movement of the point's
                // water between these two springs. Morevoer, Bernoulli's velocity injected
                // along this spring will be picked up later also by the other endpoint,
                // and at that time it would move water if it agrees with its velocity
                float const springOutboundScalarWaterVelocity = std::max(
                    pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                    0.0f);

                // Store weight along spring, as quantity of water (& pressure) moved by velocity;
                // scaling for the greater distance traveled along diagonal springs
                springOutboundWaterFlowWeights[s] =
                    springOutboundScalarWaterVelocity * SimulationParameters::SimulationStepTimeDuration<float> * oldPointWaterBufferData[pointIndex]
                    / mSprings.GetFactoryRestLength(cs.SpringIndex);

                // Resultant outbound velocity along spring
                springOutboundWaterVelocities[s] =
                    springNormalizedVector
                    * springOutboundScalarWaterVelocity;

                // Update total outbound flow weight
                totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];
                maxOutboundWaterFlowWeight = std::max(maxOutboundWaterFlowWeight, springOutboundWaterFlowWeights[s]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  W Out: springOutboundWaterFlowWeights=", springOutboundWaterFlowWeights[s], " dw=", dw, " springUpness=", springUpness, " springDownness=", springDownness);
                    LogMessage("         pThis=", oldPointWaterBufferData[pointIndex] + squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex]) * springDownness,
                               " pOther=", oldPointWaterBufferData[cs.OtherEndpointIndex] + squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]) * springUpness,
                               " bVel=", bernoulliVelocityAlongSpring, " wVel=", pointWaterVelocityAlongSpring);
                }

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update splash neighbors counts
                //

                pointSplashFreeNeighbors +=
                    mSprings.GetWaterPermeability(cs.SpringIndex)
                    * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

                pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif

                //
                // Air
                //
                // Moves according to pressure differentials:
                //    - Air-Air: moves to reach average air pressure
                //    - Water at this squeezes more air out
                //    - When traveling down, encounters resistance from water at other
                //    - When traveling up, TODOHERE
                //
                // Simple air pressure increase at a tank should force water to move out of it at water's turn
                //    - Hopefully downward because of Bernoulli/gravity and Rayleigh–Taylor
                //

                // TODOTEST
                //float const equilibriumAirPressure = (oldPointAirPressureBufferData[pointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]) / 2.0f;

                //float constexpr SqueezeFactor = 0.05f;
                //float airPressureMoved =
                //    oldPointAirPressureBufferData[pointIndex]
                //    - equilibriumAirPressure * ((1.0f - SqueezeFactor) + SqueezeFactor * oldThisPointAvailableAirVolume); // Move it more if current tank has little volume left (squeeze effect)

                //// If going down, encounter resistance from water at destination (inverse squeezing)
                //// If going up, TODOHERE
                //float const oldOtherPointAvailableAirVolume = 1.0f / (1.0f + oldPointWaterBufferData[cs.OtherEndpointIndex]);
                //airPressureMoved = airPressureMoved * (1.0f - (1.0f - oldOtherPointAvailableAirVolume) * (1.0f - springUpness));




                // TODOHERE: come up with formula for delta_pressure => pressure move, and then calc delta_pressure based on squeezes (*)
                // - Orifice Flow Equation (v = C \sqrt{\frac{2 \Delta P}{\rho}}\)
                // - Make sure no air pressure is consumed/created; publish total


                // TODOTEST
                //float const thisAirPSqueezed = squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex]);
                //float const dAirUp = thisAirPSqueezed - oldPointAirPressureBufferData[cs.OtherEndpointIndex];

                //float const otherAirPSqueezed = squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]);
                //float const dAirDown = oldPointAirPressureBufferData[pointIndex] - otherAirPSqueezed;

                //float const dAir = dAirUp * springUpness + dAirDown * springDownness;


                // New factor 1: inelegant
                //float const dAir1 =
                //    squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex])
                //    - Mix(
                //        squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]),
                //        oldPointAirPressureBufferData[cs.OtherEndpointIndex],
                //        std::max(springNormalizedVector.y, 0.0f));
                //float const dAir =
                //    dAir1
                //    * Mix(
                //        1.0f,
                //        std::max(1.0f - oldPointWaterBufferData[cs.OtherEndpointIndex], 0.0f),
                //        std::max(-springNormalizedVector.y, 0.0f));

                //// New factor 2: elegant
                //float const dAir =
                //    squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex] * springUpness)
                //    - squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex] * springDownness);

                // Simply all squeezed, with omega
                float const omegaThis = 1.0f - std::min(oldPointWaterBufferData[pointIndex] * springDownness, 1.0f); // When up, always 1.0
                float const omegaOther = 1.0f - std::min(oldPointWaterBufferData[cs.OtherEndpointIndex] * springUpness, 1.0f); // When down, always 1.0
                float const dAir =
                    squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex]) * omegaThis
                    - squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]) * omegaOther;

                float airMoved;
                if (dAir >= 0.0f)
                {
                    // Outbound
                    airMoved = dAir / 2.0f;
                }
                else
                {
                    // Not its turn
                    airMoved = 0.0f;
                }

                // Store weight along spring, scaling for the greater distance traveled along
                // diagonal springs
                springOutboundAirFlowWeights[s] =
                    airMoved
                    / mSprings.GetFactoryRestLength(cs.SpringIndex)
                    * mSprings.GetWaterPermeability(cs.SpringIndex); // Only along permeable springs

                // Update total outbound flow weight
                totalOutboundAirFlowWeight += springOutboundAirFlowWeights[s];
                maxOutboundAirFlowWeight = std::max(maxOutboundAirFlowWeight, springOutboundAirFlowWeights[s]);
                totalOutboundAirFlowWeightSquared += springOutboundAirFlowWeights[s] * springOutboundAirFlowWeights[s];


                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    //LogMessage("  A Out: springUpness=", springUpness, " springDownness=", springDownness," dAirUp=", dAirUp, " dAirDown=", dAirDown, " springOutboundAirFlowWeights=", springOutboundAirFlowWeights[s]);
                    LogMessage("  A Out: springOutboundAirFlowWeights=", springOutboundAirFlowWeights[s]);
                }
            }

            //
            // 2a) Calculate normalization factors for water flows:
            //    the quantity of water along a spring is proportional to the weight of the spring
            //    (resultant velocity along that spring), and the sum of all outbound flows must
            //    not exceed the water currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundWaterFlowWeight >= 0.0f);
            assert(maxOutboundWaterFlowWeight >= 0.0f);

            float waterQuantityNormalizationFactor = 0.0f;
            if (totalOutboundWaterFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //waterQuantityNormalizationFactor = std::min(
                //    // TODOTEST
                //    //(oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment),
                //    (oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST: new norm factor
                //waterQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointWaterBufferData[pointIndex] * mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment)
                //    / totalOutboundWaterFlowWeight;

                // TODOTEST: max norm factor
                maxOutboundWaterFlowWeight = std::min(maxOutboundWaterFlowWeight, oldPointWaterBufferData[pointIndex]);
                waterQuantityNormalizationFactor = std::min(
                    (maxOutboundWaterFlowWeight / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("W: normFactor=", waterQuantityNormalizationFactor, " (oldWater=", oldPointWaterBufferData[pointIndex], " alpha=", (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment), " tot=", totalOutboundWaterFlowWeight, ")");
                }
            }

            // TODOTEST
            waterQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 2b) Calculate normalization factors for air flows:
            //    the quantity of air along a spring is proportional to the weight of the spring
            //    (pressure flow along that spring), and the sum of all outbound flows must not
            //    exceed the air pressure currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundAirFlowWeight >= 0.0f);
            assert(maxOutboundAirFlowWeight >= 0.0f);

            float airPressureQuantityNormalizationFactor = 0.0f;
            if (totalOutboundAirFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //airPressureQuantityNormalizationFactor = std::min(
                //    (oldPointAirPressureBufferData[pointIndex] / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST
                //airPressureQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment)
                //    / totalOutboundAirFlowWeight;

                // TODOTEST: quadratic norm factor
                //airPressureQuantityNormalizationFactor =
                //    std::min(
                //        1.0f / totalOutboundAirFlowWeight,
                //        oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment / totalOutboundAirFlowWeightSquared);

                // TODOTEST: max factor
                maxOutboundAirFlowWeight = std::min(maxOutboundAirFlowWeight, oldPointAirPressureBufferData[pointIndex]);
                airPressureQuantityNormalizationFactor = std::min(
                    (maxOutboundAirFlowWeight / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("A: normFactor=", airPressureQuantityNormalizationFactor, " (oldAir=", oldPointAirPressureBufferData[pointIndex], " alpha=", simulationParameters.AirDiffusionSpeedAdjustment, " tot=", totalOutboundAirFlowWeight, ")");
                }
            }

            // TODOTEST
            airPressureQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 3) Move water/air along all springs according to their flows,
            //    and update destination's momenta accordingly
            //

#if !FS_IS_PLATFORM_MOBILE()
        // Kinetic energy lost at this point
            pointKineticEnergyLoss = 0.0f;
#endif

            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];


                //
                // Water
                //

                // Calculate quantity of water directed outwards
                float const springOutboundQuantityOfWater =
                    springOutboundWaterFlowWeights[s]
                    * waterQuantityNormalizationFactor;

                assert(springOutboundQuantityOfWater >= 0.0f);

                if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
                {
                    //
                    // Water - and momentum - move from point to endpoint
                    //

                    // TODOTEST
                    if (pointIndex == mLastQueriedPointIndex)
                    {
                        LogMessage("  W: springOutboundQuantityOfWater=", springOutboundQuantityOfWater, " (w=", springOutboundWaterFlowWeights[s], " norm=", waterQuantityNormalizationFactor, ")");
                        todoTotalWOut += springOutboundQuantityOfWater;
                    }

                    // Move water quantity
                    newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                    newPointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                    // Remove "old momentum" (old velocity) from point
                    newPointWaterMomentumBufferData[pointIndex] -=
                        oldPointWaterVelocityBufferData[pointIndex]
                        * springOutboundQuantityOfWater;

                    // Add "new momentum" (old velocity + velocity gained) to other endpoint
                    newPointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                        springOutboundWaterVelocities[s]
                        * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // splintered water colliding with whole other endpoint
                    //

                    // Normalized spring vector, oriented point -> other endpoint
                    vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                        ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                        : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();
                    float mb = oldPointWaterBufferData[cs.OtherEndpointIndex];
                    float vb = oldPointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                    float vf = 0.0f;
                    if (ma + mb != 0.0f)
                        vf = (ma * va + mb * vb) / (ma + mb);

                    float deltaKa =
                        0.5f
                        * ma
                        * (va * va - vf * vf);

                    // Note: deltaKa might be negative, in which case deltaKb would have been
                    // more positive (perfectly inelastic -> deltaK == max); we will pickup
                    // deltaKb later
                    pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
                }
                else
                {
                    // Wall hit

                    // Deleted springs are removed from points' connected springs
                    assert(!mSprings.IsDeleted(cs.SpringIndex));

                    //
                    // New momentum (old velocity + velocity gained) bounces back
                    // (and zeroes outgoing), assuming perfectly inelastic collision
                    //
                    // No changes to other endpoint
                    //

                    newPointWaterMomentumBufferData[pointIndex] -=
                        springOutboundWaterVelocities[s]
                        * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // entire splintered water
                    //

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();

                    float deltaKa =
                        0.5f
                        * ma
                        * va * va;

                    assert(deltaKa >= 0.0f);
                    pointKineticEnergyLoss += deltaKa;
#endif
                }


                //
                // Air
                //

                // Calculate quantity of air pressure directed outwards,
                // being careful not to overdrain the point
                float const springOutboundQuantityOfAirPressure = std::min(
                    springOutboundAirFlowWeights[s] * airPressureQuantityNormalizationFactor,
                    newPointAirPressureBufferData[pointIndex]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A: springOutboundQuantityOfAirPressure=", springOutboundQuantityOfAirPressure, " (w=", springOutboundAirFlowWeights[s], " norm=", airPressureQuantityNormalizationFactor, ")");
                    todoTotalAOut += springOutboundQuantityOfAirPressure;
                }

                assert(springOutboundQuantityOfAirPressure >= 0.0f);
                assert(springOutboundQuantityOfAirPressure <= newPointAirPressureBufferData[pointIndex]);

                //
                // Air pressure moves from point to endpoint
                //

                newPointAirPressureBufferData[pointIndex] -= springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[pointIndex] >= 0.0f);
                newPointAirPressureBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[cs.OtherEndpointIndex] >= 0.0f);
            }

#if !FS_IS_PLATFORM_MOBILE()
            //
            // 4) Update water splash
            //

            if (pointSplashNeighbors != 0.0f)
            {
                // Water splashed is proportional to kinetic energy loss that took
                // place near free points (i.e. not drowned by water)
                waterSplashed +=
                    pointKineticEnergyLoss
                    * pointSplashFreeNeighbors
                    / pointSplashNeighbors;
            }
#endif
        }



        // TODOTEST: moved into this loop from outside
        //
        // Transforming momenta into velocities
        //

        mPoints.UpdateWaterVelocitiesFromMomenta();


        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("Total W Out: ", todoTotalWOut, "   Total A Out: ", todoTotalAOut);
        }

    } // Iter loop

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif

    // TODOTEST
    //
    // Damp water velocities
    //

    float todoTotalAir = 0.0f;

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        // TODOTEST
        //newPointWaterMomentumBufferData[pointIndex] *= 0.975f;
        mPoints.SetWaterVelocity(pointIndex, mPoints.GetWaterVelocity(pointIndex) * std::min(0.975f * simulationParameters.AntiMatterBombImplosionStrength, 1.0f) );

        // Update total air
        if (!mPoints.IsDamaged(pointIndex))
            todoTotalAir += mPoints.GetAirPressure(pointIndex);
    }

    mSimulationEventHandler.OnCustomProbe("TotalAir", todoTotalAir);



    //
    // TODOTEST: readings
    //

    std::vector<PressureReading> readings;

    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        ElementIndex prevPointIndex = PressureCrossCutReadingsStartPointIndex;
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                squeezeAir(mPoints.GetAirPressure(pointIndex), mPoints.GetWater(pointIndex)),
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y });

            if (pointIndex == mLastQueriedPointIndex)
            {
                LogMessage("READ: this : a=", mPoints.GetAirPressure(pointIndex), " sqzA=", squeezeAir(mPoints.GetAirPressure(pointIndex), mPoints.GetWater(pointIndex)), " w=", mPoints.GetWater(pointIndex));
                LogMessage("      other: a=", mPoints.GetAirPressure(prevPointIndex), " sqzA=", squeezeAir(mPoints.GetAirPressure(prevPointIndex), mPoints.GetWater(prevPointIndex)), " w=", mPoints.GetWater(prevPointIndex));
            }

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            prevPointIndex = pointIndex;
            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);

    // TODOTEST: moved into loop
    ////
    //// Transforming momenta into velocities
    ////
    //
    //mPoints.UpdateWaterVelocitiesFromMomenta();
}

void Ship::UpdateWaterAndAirPressure_NewtonRhapson_2(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move water and air along its connected springs,
    // based on pressure differentials and water momenta (https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/)
    //
    // Model is tanks, connected at bottom (for water moves) and at top (for air moves)
    //    - Hence, water moves are governed by pressures at bottom, which are air pressure + water pressure, and water momenta
    //    - Hence, air moves are governed by pressures at top, which are air pressure
    //      - But compressibility of air plays a role - i.e.water volumes plays a role
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // TODOTEST: moved into loop
    //// Calculate water momenta
    //mPoints.UpdateWaterMomentaFromVelocities();

    //// Source and result water buffers
    //auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    //float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
    //float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    //vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    //vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    //// Source and result air buffers
    //auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
    //float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
    //float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total water flow weight
    float totalOutboundWaterFlowWeight;
    float maxOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    // Weights of outbound air flows along each spring, only permeable ones;
    // set to zero for springs whose resultant scalar air flows are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundAirFlowWeights;

    // Total air flow weight
    float totalOutboundAirFlowWeight;
    float maxOutboundAirFlowWeight;
    float totalOutboundAirFlowWeightSquared = 0.0f;

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-mPoints.GetWater(pointIndex) * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif

    //
    // Visit all non-ephemeral points and:
    //  - Move water and its momenta according to momenta and pressure differentials
    //  - Move air (pressure) according to pressure differentials (and volumetric bias)
    //
    // No need to visit ephemeral points as they have no springs
    //

    // TODOTEST
    //int constexpr NumberOfIterations = 4;
    //int constexpr NumberOfIterations = 2;
    int constexpr NumberOfIterations = 1;
    //int constexpr NumberOfIterations = 32;
    for (int iter = 0; iter < NumberOfIterations; ++iter)
    {


        // TODOTEST: moved from outside into loop
        // Calculate water momenta
        mPoints.UpdateWaterMomentaFromVelocities();



        // Source and result water buffers
        auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
        float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
        float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
        vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
        vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

        // Source and result air buffers
        auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
        float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
        float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("Start W: ", oldPointWaterBufferData[mLastQueriedPointIndex], "  Start A: ", oldPointAirPressureBufferData[mLastQueriedPointIndex]);
        }
        float todoTotalWOut = 0.0f;
        float todoTotalAOut = 0.0f;


        for (auto pointIndex : mPoints.RawShipPoints())
        {
            //
            // 1a) Calculate water momenta along *all* springs connected to this point,
            //     including impermeable ones - as we'll eventually bounce back along those
            // 1b) Calculate air pressure transfers along travelable springs connected to this point
            //

            // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
            // and gravity were exaggerated
            //
            // WV[t] = WV[t-1] + alpha * Bernoulli
            //
            // WaterCrazyness=0   -> alpha=1
            // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
            // WaterCrazyness=1   -> alpha=Wh
            float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

#if !FS_IS_PLATFORM_MOBILE()
            pointSplashNeighbors = 0.0f;
            pointSplashFreeNeighbors = 0.0f;
#endif

            totalOutboundWaterFlowWeight = 0.0f;
            maxOutboundWaterFlowWeight = 0.0f;
            totalOutboundAirFlowWeight = 0.0f;
            maxOutboundAirFlowWeight = 0.0f;
            totalOutboundAirFlowWeightSquared = 0.0f;

            size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                // Upness: TODOHERE -- 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector

                // TODOTEST
                //float const springUpness = springNormalizedVector.y;

                // TODOTEST: step
                //float const springUpness = Step(0.0f, springNormalizedVector.y);
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: 0->1 smooth
                //float const springUpness = (1.0f + springNormalizedVector.y) / 2.0f;
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: -1->1 smooth
                //float const springUpness = springNormalizedVector.y;
                //float const springDownness = -springUpness;

                // TODOTEST: 0->1->1 smooth
                //float const springUpness = std::min(springNormalizedVector.y + 1.0f, 1.0f);
                //float const springDownness = std::min(1.0f - springNormalizedVector.y, 1.0f);

                // TODOTEST: 0->0->1 smooth
                float const springUpness = std::max(springNormalizedVector.y, 0.0f);
                float const springDownness = std::max(-springNormalizedVector.y, 0.0f);

                (void)springUpness;
                (void)springDownness;

                //
                // Water
                //
                // Moves according to water momentum + pressure differentials
                //    - Source pressure is water pressure + air pressure
                //    - Destination pressure:
                //      - When diffusing up: water pressure + air pressure
                //      - When diffusing down: water pressure - air pressure (Rayleigh–Taylor instability: water is not stopped by air below - actually drawn down)
                //

                // Component of the point's own water velocity along the spring
                float const pointWaterVelocityAlongSpring =
                    oldPointWaterVelocityBufferData[pointIndex]
                    .dot(springNormalizedVector);

                //
                // Calulate Bernoulli's velocity gained along this spring, from this point to
                // the other endpoint
                //

                // TODOTEST: ORIG (no air pressure)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex]);

                // TODOTEST: NEW-WRONG (with air pressure, but no laterals)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);

                // TODOTEST: NEW (with air pressure, and upness)
                float const dw =
                    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);

                // Gravity potential difference (positive implies point -> other endpoint flow)
                float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

                // Calculate gained water velocity along this spring, from point to other endpoint
                // (Bernoulli, 1738)
                //
                // We add pressure and heights as pressure is in "height equivalent units"
                float bernoulliVelocityAlongSpring;
                float const dwy = dw + dy;
                if (dwy >= 0.0f)
                {
                    // Gained velocity goes from point to other endpoint
                    bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
                }
                else
                {
                    // Gained velocity goes from other endpoint to point
                    bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
                }

                // Resultant scalar velocity along spring; outbound only, as
                // if this were inbound it wouldn't result in any movement of the point's
                // water between these two springs. Morevoer, Bernoulli's velocity injected
                // along this spring will be picked up later also by the other endpoint,
                // and at that time it would move water if it agrees with its velocity
                float const springOutboundScalarWaterVelocity = std::max(
                    pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                    0.0f);

                // Store weight along spring, as quantity of water (& pressure) moved by velocity;
                // scaling for the greater distance traveled along diagonal springs
                springOutboundWaterFlowWeights[s] =
                    // TODOTEST: orig
                    //springOutboundScalarWaterVelocity
                    // TODOTEST: new
                    springOutboundScalarWaterVelocity * SimulationParameters::SimulationStepTimeDuration<float> * oldPointWaterBufferData[pointIndex]
                    / mSprings.GetFactoryRestLength(cs.SpringIndex);

                // Resultant outbound velocity along spring
                springOutboundWaterVelocities[s] =
                    springNormalizedVector
                    * springOutboundScalarWaterVelocity;

                // Update total outbound flow weight
                totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];
                maxOutboundWaterFlowWeight = std::max(maxOutboundWaterFlowWeight, springOutboundWaterFlowWeights[s]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  W Out: springOutboundWaterFlowWeights=", springOutboundWaterFlowWeights[s], " dw=", dw, " springDir=", springNormalizedVector);
                    LogMessage("         pThis=", oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex],
                        " pOther=", oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex],
                        " bVel=", bernoulliVelocityAlongSpring, " wVel=", pointWaterVelocityAlongSpring);
                }

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update splash neighbors counts
                //

                pointSplashFreeNeighbors +=
                    mSprings.GetWaterPermeability(cs.SpringIndex)
                    * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

                pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif

                //
                // Air
                //
                // Moves according to pressure differentials:
                //    - Air-Air: moves to reach average air pressure
                //    - Water at this squeezes more air out
                //    - When traveling down, encounters resistance from water at other
                //    - When traveling up, TODOHERE
                //
                // Simple air pressure increase at a tank should force water to move out of it at water's turn
                //    - Hopefully downward because of Bernoulli/gravity and Rayleigh–Taylor
                //

                float const dAir =
                    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);

                float airMoved;
                if (dAir >= 0.0f)
                {
                    // Outbound
                    airMoved = dAir / 2.0f;
                }
                else
                {
                    // Not its turn
                    airMoved = 0.0f;
                }

                //
                // Add buoyancy: if layer above contains water, than this air moves up
                //

                // Indicator of "water above": 0 @ water[above] = 0.0, 1 @ water[above] >= 1.0
                float const omega = std::min(oldPointWaterBufferData[cs.OtherEndpointIndex], 1.0f);

                // Velocity along spring (only exists when going "up", and already projected onto vertical)
                float const upwardVelocity =
                    //0.3f // Magic: bubble goes up at 0.25/0.40 m/s
                    simulationParameters.ElectricalElementHeatProducedAdjustment
                    * omega
                    * springUpness;

                // TODOTEST: sum of air moved
                airMoved += upwardVelocity * SimulationParameters::SimulationStepTimeDuration<float> *oldPointAirPressureBufferData[pointIndex];

                // TODOTEST: replacement of air moved
                //airMoved =
                //    Mix(
                //        airMoved,
                //        simulationParameters.ElectricalElementHeatProducedAdjustment * SimulationParameters::SimulationStepTimeDuration<float> *oldPointAirPressureBufferData[pointIndex],
                //        omega * springUpness);

                // Store weight along spring, scaling for the greater distance traveled along
                // diagonal springs
                springOutboundAirFlowWeights[s] =
                    airMoved
                    / mSprings.GetFactoryRestLength(cs.SpringIndex)
                    * mSprings.GetWaterPermeability(cs.SpringIndex); // Only along permeable springs

                // Update total outbound flow weight
                totalOutboundAirFlowWeight += springOutboundAirFlowWeights[s];
                maxOutboundAirFlowWeight = std::max(maxOutboundAirFlowWeight, springOutboundAirFlowWeights[s]);
                totalOutboundAirFlowWeightSquared += springOutboundAirFlowWeights[s] * springOutboundAirFlowWeights[s];

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A Out: dAir=", dAir, " upwardVelocity=", upwardVelocity, " airMoved=", airMoved, " springDir=", springNormalizedVector, " springOutboundAirFlowWeights=", springOutboundAirFlowWeights[s]);
                }
            }

            //
            // 2a) Calculate normalization factors for water flows:
            //    the quantity of water along a spring is proportional to the weight of the spring
            //    (resultant velocity along that spring), and the sum of all outbound flows must
            //    not exceed the water currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundWaterFlowWeight >= 0.0f);
            assert(maxOutboundWaterFlowWeight >= 0.0f);

            float waterQuantityNormalizationFactor = 0.0f;
            if (totalOutboundWaterFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //waterQuantityNormalizationFactor = std::min(
                //    // TODOTEST
                //    (oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment),
                //    //(oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST: new norm factor
                //waterQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointWaterBufferData[pointIndex] * mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment)
                //    / totalOutboundWaterFlowWeight;

                // TODOTEST: max norm factor
                maxOutboundWaterFlowWeight = std::min(maxOutboundWaterFlowWeight, oldPointWaterBufferData[pointIndex]);
                waterQuantityNormalizationFactor = std::min(
                    (maxOutboundWaterFlowWeight / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("W: normFactor=", waterQuantityNormalizationFactor, " (oldWater=", oldPointWaterBufferData[pointIndex], " alpha=", (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment), " tot=", totalOutboundWaterFlowWeight, ")");
                }
            }

            // TODOTEST
            waterQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 2b) Calculate normalization factors for air flows:
            //    the quantity of air along a spring is proportional to the weight of the spring
            //    (pressure flow along that spring), and the sum of all outbound flows must not
            //    exceed the air pressure currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundAirFlowWeight >= 0.0f);
            assert(maxOutboundAirFlowWeight >= 0.0f);

            float airPressureQuantityNormalizationFactor = 0.0f;
            if (totalOutboundAirFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //airPressureQuantityNormalizationFactor = std::min(
                //    (oldPointAirPressureBufferData[pointIndex] / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST
                //airPressureQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment)
                //    / totalOutboundAirFlowWeight;

                // TODOTEST: quadratic norm factor
                //airPressureQuantityNormalizationFactor =
                //    std::min(
                //        1.0f / totalOutboundAirFlowWeight,
                //        oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment / totalOutboundAirFlowWeightSquared);

                // TODOTEST: max factor
                maxOutboundAirFlowWeight = std::min(maxOutboundAirFlowWeight, oldPointAirPressureBufferData[pointIndex]);
                airPressureQuantityNormalizationFactor = std::min(
                    (maxOutboundAirFlowWeight / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("A: normFactor=", airPressureQuantityNormalizationFactor, " (oldAir=", oldPointAirPressureBufferData[pointIndex], " alpha=", simulationParameters.AirDiffusionSpeedAdjustment, " tot=", totalOutboundAirFlowWeight, ")");
                }
            }

            // TODOTEST
            airPressureQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 3) Move water/air along all springs according to their flows,
            //    and update destination's momenta accordingly
            //

#if !FS_IS_PLATFORM_MOBILE()
        // Kinetic energy lost at this point
            pointKineticEnergyLoss = 0.0f;
#endif

            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];


                //
                // Water
                //

                // Calculate quantity of water directed outwards
                float const springOutboundQuantityOfWater =
                    springOutboundWaterFlowWeights[s]
                    * waterQuantityNormalizationFactor;

                assert(springOutboundQuantityOfWater >= 0.0f);

                if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
                {
                    //
                    // Water - and momentum - move from point to endpoint
                    //

                    // TODOTEST
                    if (pointIndex == mLastQueriedPointIndex)
                    {
                        LogMessage("  W: springOutboundQuantityOfWater=", springOutboundQuantityOfWater, " (w=", springOutboundWaterFlowWeights[s], " norm=", waterQuantityNormalizationFactor, ")");
                        todoTotalWOut += springOutboundQuantityOfWater;
                    }

                    // Move water quantity
                    newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                    newPointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                    // Remove "old momentum" (old velocity) from point
                    newPointWaterMomentumBufferData[pointIndex] -=
                        oldPointWaterVelocityBufferData[pointIndex]
                        * springOutboundQuantityOfWater;

                    // Add "new momentum" (old velocity + velocity gained) to other endpoint
                    newPointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                        springOutboundWaterVelocities[s]
                        * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // splintered water colliding with whole other endpoint
                    //

                    // Normalized spring vector, oriented point -> other endpoint
                    vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                        ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                        : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();
                    float mb = oldPointWaterBufferData[cs.OtherEndpointIndex];
                    float vb = oldPointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                    float vf = 0.0f;
                    if (ma + mb != 0.0f)
                        vf = (ma * va + mb * vb) / (ma + mb);

                    float deltaKa =
                        0.5f
                        * ma
                        * (va * va - vf * vf);

                    // Note: deltaKa might be negative, in which case deltaKb would have been
                    // more positive (perfectly inelastic -> deltaK == max); we will pickup
                    // deltaKb later
                    pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
                }
                else
                {
                    // Wall hit

                    // Deleted springs are removed from points' connected springs
                    assert(!mSprings.IsDeleted(cs.SpringIndex));

                    //
                    // New momentum (old velocity + velocity gained) bounces back
                    // (and zeroes outgoing), assuming perfectly inelastic collision
                    //
                    // No changes to other endpoint
                    //

                    newPointWaterMomentumBufferData[pointIndex] -=
                        springOutboundWaterVelocities[s]
                        * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // entire splintered water
                    //

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();

                    float deltaKa =
                        0.5f
                        * ma
                        * va * va;

                    assert(deltaKa >= 0.0f);
                    pointKineticEnergyLoss += deltaKa;
#endif
                }


                //
                // Air
                //

                // Calculate quantity of air pressure directed outwards,
                // being careful not to overdrain the point
                float const springOutboundQuantityOfAirPressure = std::min(
                    springOutboundAirFlowWeights[s] * airPressureQuantityNormalizationFactor,
                    newPointAirPressureBufferData[pointIndex]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A: springOutboundQuantityOfAirPressure=", springOutboundQuantityOfAirPressure, " (w=", springOutboundAirFlowWeights[s], " norm=", airPressureQuantityNormalizationFactor, ")");
                    todoTotalAOut += springOutboundQuantityOfAirPressure;
                }

                assert(springOutboundQuantityOfAirPressure >= 0.0f);
                assert(springOutboundQuantityOfAirPressure <= newPointAirPressureBufferData[pointIndex]);

                //
                // Air pressure moves from point to endpoint
                //

                newPointAirPressureBufferData[pointIndex] -= springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[pointIndex] >= 0.0f);
                newPointAirPressureBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[cs.OtherEndpointIndex] >= 0.0f);
            }

#if !FS_IS_PLATFORM_MOBILE()
            //
            // 4) Update water splash
            //

            if (pointSplashNeighbors != 0.0f)
            {
                // Water splashed is proportional to kinetic energy loss that took
                // place near free points (i.e. not drowned by water)
                waterSplashed +=
                    pointKineticEnergyLoss
                    * pointSplashFreeNeighbors
                    / pointSplashNeighbors;
            }
#endif
        }



        // TODOTEST: moved into this loop from outside
        //
        // Transforming momenta into velocities
        //

        mPoints.UpdateWaterVelocitiesFromMomenta();


        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("Total W Out: ", todoTotalWOut, "   Total A Out: ", todoTotalAOut);
        }

    } // Iter loop

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif

    // TODOTEST
    //
    // Damp water velocities
    //

    float todoTotalAir = 0.0f;

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        // TODOTEST
        //newPointWaterMomentumBufferData[pointIndex] *= 0.975f;
        mPoints.SetWaterVelocity(pointIndex, mPoints.GetWaterVelocity(pointIndex) * std::min(simulationParameters.AntiMatterBombImplosionStrength, 1.0f));

        // Update total air
        if (!mPoints.IsDamaged(pointIndex))
            todoTotalAir += mPoints.GetAirPressure(pointIndex);
    }

    mSimulationEventHandler.OnCustomProbe("TotalAir", todoTotalAir);



    //
    // TODOTEST: readings
    //

    std::vector<PressureReading> readings;

    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        ElementIndex prevPointIndex = PressureCrossCutReadingsStartPointIndex;
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                0.0f,
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y });

            if (pointIndex == mLastQueriedPointIndex)
            {
                LogMessage("READ: this : a=", mPoints.GetAirPressure(pointIndex), " w=", mPoints.GetWater(pointIndex));
                LogMessage("      other: a=", mPoints.GetAirPressure(prevPointIndex), " w=", mPoints.GetWater(prevPointIndex));
            }

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            prevPointIndex = pointIndex;
            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);

    // TODOTEST: moved into loop
    ////
    //// Transforming momenta into velocities
    ////
    //
    //mPoints.UpdateWaterVelocitiesFromMomenta();
}

void Ship::UpdateWaterAndAirPressure_NewtonRhapson_2_TwoStep(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move water and air along its connected springs,
    // based on pressure differentials and water momenta (https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/)
    //
    // Model is tanks, connected at bottom (for water moves) and at top (for air moves)
    //    - Hence, water moves are governed by pressures at bottom, which are air pressure + water pressure, and water momenta
    //    - Hence, air moves are governed by pressures at top, which are air pressure
    //      - But compressibility of air plays a role - i.e.water volumes plays a role
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // TODOTEST: moved into loop
    //// Calculate water momenta
    //mPoints.UpdateWaterMomentaFromVelocities();

    //// Source and result water buffers
    //auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    //float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
    //float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    //vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    //vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    //// Source and result air buffers
    //auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
    //float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
    //float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total water flow weight
    float totalOutboundWaterFlowWeight;
    float maxOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    // Weights of outbound air flows along each spring, only permeable ones;
    // set to zero for springs whose resultant scalar air flows are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundAirFlowWeights;

    // Total air flow weight
    float totalOutboundAirFlowWeight;
    float maxOutboundAirFlowWeight;
    float totalOutboundAirFlowWeightSquared = 0.0f;

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-mPoints.GetWater(pointIndex) * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif


    // TODOTEST
    // Calculate total water before
    float totalWaterPre = 0.0f;
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        totalWaterPre += mPoints.GetWater(pointIndex);
    }


    //
    // Visit all non-ephemeral points and:
    //  - Move water and its momenta according to momenta and pressure differentials
    //  - Move air (pressure) according to pressure differentials (and volumetric bias)
    //
    // No need to visit ephemeral points as they have no springs
    //

    // TODOTEST
    //int constexpr NumberOfIterations = 4;
    int constexpr NumberOfIterations = 2;
    //int constexpr NumberOfIterations = 1;
    //int constexpr NumberOfIterations = 32;


    //
    // Water step
    //

    for (int iter = 0; iter < NumberOfIterations; ++iter)
    {


        // TODOTEST: moved from outside into loop
        // Calculate water momenta
        mPoints.UpdateWaterMomentaFromVelocities();



        // Source and result water buffers
        auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
        float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
        float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
        vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
        vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

        // Source air buffers
        auto oldPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("Start W: ", oldPointWaterBufferData[mLastQueriedPointIndex], "  Start A: ", oldPointAirPressureBufferData[mLastQueriedPointIndex]);
        }
        float todoTotalWOutAtQueriedPoint = 0.0f;
        float todoTotalWInAtQueriedPoint = 0.0f;
        vec2f todoTotalWMomentumOutAtQueriedPoint = vec2f::zero();
        vec2f todoTotalWMomentumInAtQueriedPoint = vec2f::zero();

        for (auto pointIndex : mPoints.RawShipPoints())
        {
            //
            // 1a) Calculate water momenta along *all* springs connected to this point,
            //     including impermeable ones - as we'll eventually bounce back along those
            // 1b) Calculate air pressure transfers along travelable springs connected to this point
            //

            // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
            // and gravity were exaggerated
            //
            // WV[t] = WV[t-1] + alpha * Bernoulli
            //
            // WaterCrazyness=0   -> alpha=1
            // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
            // WaterCrazyness=1   -> alpha=Wh
            float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

#if !FS_IS_PLATFORM_MOBILE()
            pointSplashNeighbors = 0.0f;
            pointSplashFreeNeighbors = 0.0f;
#endif

            totalOutboundWaterFlowWeight = 0.0f;
            maxOutboundWaterFlowWeight = 0.0f;

            size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                // Upness: TODOHERE -- 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector

                // TODOTEST
                //float const springUpness = springNormalizedVector.y;

                // TODOTEST: step
                //float const springUpness = Step(0.0f, springNormalizedVector.y);
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: 0->1 smooth
                //float const springUpness = (1.0f + springNormalizedVector.y) / 2.0f;
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: -1->1 smooth
                //float const springUpness = springNormalizedVector.y;
                //float const springDownness = -springUpness;

                // TODOTEST: 0->1->1 smooth
                //float const springUpness = std::min(springNormalizedVector.y + 1.0f, 1.0f);
                //float const springDownness = std::min(1.0f - springNormalizedVector.y, 1.0f);

                //// TODOTEST: 0->0->1 smooth
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f);

                //// TODOTEST: 0->0->1 smooth, corrected with rest_length
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);

                // TODOTEST: 0->0->1 smooth, using delta_H
                // FUTUREWORK: need to divide by ship's square side size here, once we use scale; add ship member for that
                float const springUpness = std::max(mPoints.GetPosition(cs.OtherEndpointIndex).y - mPoints.GetPosition(pointIndex).y, 0.0f);
                float const springDownness = std::max(mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y, 0.0f);

                (void)springUpness;
                (void)springDownness;

                //
                // Water
                //
                // Moves according to water momentum + pressure differentials
                //    - Source pressure is water pressure + air pressure
                //    - Destination pressure:
                //      - When diffusing up: water pressure + air pressure
                //      - When diffusing down: water pressure - air pressure (Rayleigh–Taylor instability: water is not stopped by air below - actually drawn down)
                //

                // Component of the point's own water velocity along the spring
                float const pointWaterVelocityAlongSpring =
                    oldPointWaterVelocityBufferData[pointIndex]
                    .dot(springNormalizedVector);

                //
                // Calulate Bernoulli's velocity gained along this spring, from this point to
                // the other endpoint
                //

                // TODOTEST: ORIG (no air pressure)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex]);

                // TODOTEST: NEW-WRONG (with air pressure, but no laterals)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);

                // TODOTEST: NEW (with air pressure, and upness)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);

                //// TODOTEST: NEW (with air pressure, upness, and downness)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex] * springDownness)
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);

                // TODOTEST: NEW (with air pressure, upness, and downness), and no delta-pressure against wall
                float const dw = (
                    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex] * springDownness)
                    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness)
                    ) * mSprings.GetWaterPermeability(cs.SpringIndex); // Enforce no delta-pressure with (dry) wall

                // Gravity potential difference (positive implies point -> other endpoint flow)
                float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

                // Calculate gained water velocity along this spring, from point to other endpoint
                // (Bernoulli, 1738)
                //
                // We add pressure and heights as pressure is in "height equivalent units"
                float bernoulliVelocityAlongSpring;
                float const dwy = dw + dy;
                if (dwy >= 0.0f)
                {
                    // Gained velocity goes from point to other endpoint
                    bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
                }
                else
                {
                    // Gained velocity goes from other endpoint to point
                    bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
                }

                // Resultant scalar velocity along spring; outbound only, as
                // if this were inbound it wouldn't result in any movement of the point's
                // water between these two springs. Morevoer, Bernoulli's velocity injected
                // along this spring will be picked up later also by the other endpoint,
                // and at that time it would move water if it agrees with its velocity
                float const springOutboundScalarWaterVelocity = std::max(
                    pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                    0.0f);

                // Store weight along spring, as quantity of water (& pressure) moved by velocity;
                // scaling for the greater distance traveled along diagonal springs
                springOutboundWaterFlowWeights[s] =
                    // TODOTEST: orig
                    //springOutboundScalarWaterVelocity
                    // TODOTEST: new
                    springOutboundScalarWaterVelocity * SimulationParameters::SimulationStepTimeDuration<float> * oldPointWaterBufferData[pointIndex]
                    / mSprings.GetFactoryRestLength(cs.SpringIndex);

                // Resultant outbound velocity along spring
                springOutboundWaterVelocities[s] =
                    springNormalizedVector
                    * springOutboundScalarWaterVelocity;

                // Update total outbound flow weight
                totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];
                maxOutboundWaterFlowWeight = std::max(maxOutboundWaterFlowWeight, springOutboundWaterFlowWeights[s]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  W Out: springOutboundWaterFlowWeights=", springOutboundWaterFlowWeights[s], " dw=", dw, " upness=", springUpness, " downness=", springDownness);
                    LogMessage("         pThis=", oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex] * springDownness,
                        " pOther=", oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness,
                        " bVel=", bernoulliVelocityAlongSpring, " wVel=", pointWaterVelocityAlongSpring);
                }

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update splash neighbors counts
                //

                pointSplashFreeNeighbors +=
                    mSprings.GetWaterPermeability(cs.SpringIndex)
                    * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

                pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif

            }

            //
            // 2a) Calculate normalization factors for water flows:
            //    the quantity of water along a spring is proportional to the weight of the spring
            //    (resultant velocity along that spring), and the sum of all outbound flows must
            //    not exceed the water currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundWaterFlowWeight >= 0.0f);
            assert(maxOutboundWaterFlowWeight >= 0.0f);

            float waterQuantityNormalizationFactor = 0.0f;
            if (totalOutboundWaterFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //waterQuantityNormalizationFactor = std::min(
                //    // TODOTEST
                //    (oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment),
                //    //(oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST: new norm factor
                //waterQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointWaterBufferData[pointIndex] * mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment)
                //    / totalOutboundWaterFlowWeight;

                // TODOTEST: max norm factor
                maxOutboundWaterFlowWeight = std::min(maxOutboundWaterFlowWeight, oldPointWaterBufferData[pointIndex]);
                waterQuantityNormalizationFactor = std::min(
                    (maxOutboundWaterFlowWeight / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("W: normFactor=", waterQuantityNormalizationFactor, " (oldWater=", oldPointWaterBufferData[pointIndex], " alpha=", (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment), " tot=", totalOutboundWaterFlowWeight, ")");
                }
            }

            // TODOTEST
            waterQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 3) Move water/air along all springs according to their flows,
            //    and update destination's momenta accordingly
            //

#if !FS_IS_PLATFORM_MOBILE()
        // Kinetic energy lost at this point
            pointKineticEnergyLoss = 0.0f;
#endif

            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];


                //
                // Water
                //

                // Calculate quantity of water directed outwards
                float const springOutboundQuantityOfWater =
                    springOutboundWaterFlowWeights[s]
                    * waterQuantityNormalizationFactor;

                assert(springOutboundQuantityOfWater >= 0.0f);

                if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
                {
                    //
                    // Water - and momentum - move from point to endpoint
                    //

                    // TODOTEST
                    if (pointIndex == mLastQueriedPointIndex)
                    {
                        LogMessage("  W: springOutboundQuantityOfWater=", springOutboundQuantityOfWater, " (w=", springOutboundWaterFlowWeights[s], " norm=", waterQuantityNormalizationFactor, ")");
                        todoTotalWOutAtQueriedPoint += springOutboundQuantityOfWater;

                        todoTotalWMomentumOutAtQueriedPoint +=
                            oldPointWaterVelocityBufferData[pointIndex]
                            * springOutboundQuantityOfWater;
                    }
                    else if (cs.OtherEndpointIndex == mLastQueriedPointIndex)
                    {
                        todoTotalWInAtQueriedPoint += springOutboundQuantityOfWater;

                        todoTotalWMomentumInAtQueriedPoint +=
                            springOutboundWaterVelocities[s]
                            * springOutboundQuantityOfWater;
                    }

                    // Move water quantity
                    newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                    newPointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                    // Remove "old momentum" (old velocity) from point
                    newPointWaterMomentumBufferData[pointIndex] -=
                        oldPointWaterVelocityBufferData[pointIndex]
                        * springOutboundQuantityOfWater;

                    // Add "new momentum" (old velocity + velocity gained) to other endpoint
                    newPointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                        springOutboundWaterVelocities[s]
                        * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // splintered water colliding with whole other endpoint
                    //

                    // Normalized spring vector, oriented point -> other endpoint
                    vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                        ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                        : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();
                    float mb = oldPointWaterBufferData[cs.OtherEndpointIndex];
                    float vb = oldPointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                    float vf = 0.0f;
                    if (ma + mb != 0.0f)
                        vf = (ma * va + mb * vb) / (ma + mb);

                    float deltaKa =
                        0.5f
                        * ma
                        * (va * va - vf * vf);

                    // Note: deltaKa might be negative, in which case deltaKb would have been
                    // more positive (perfectly inelastic -> deltaK == max); we will pickup
                    // deltaKb later
                    pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
                }
                else
                {
                    // Wall hit

                    // Deleted springs are removed from points' connected springs
                    assert(!mSprings.IsDeleted(cs.SpringIndex));

                    //
                    // New momentum (old velocity + velocity gained) bounces back
                    // (and zeroes outgoing), assuming perfectly inelastic collision
                    //
                    // No changes to other endpoint
                    //

                    //// TODOTEST: orig
                    //newPointWaterMomentumBufferData[pointIndex] -=
                    //    springOutboundWaterVelocities[s]
                    //    * springOutboundQuantityOfWater;

                    // TODOTEST: new
                    // Remove "old momentum" (old velocity) from point, for the quantity of water we're willing to move
                    newPointWaterMomentumBufferData[pointIndex] -=
                        oldPointWaterVelocityBufferData[pointIndex]
                        * springOutboundQuantityOfWater;
                    // Add "new momentum" (new velocity gained), but after bounce
                    newPointWaterMomentumBufferData[pointIndex] +=
                        -springOutboundWaterVelocities[s] * (simulationParameters.BlastToolForceAdjustment / 10.0f)
                        * springOutboundQuantityOfWater;

                    //// TODOTEST: zero out all
                    //newPointWaterMomentumBufferData[pointIndex] = vec2f::zero();


#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // entire splintered water
                    //

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();

                    float deltaKa =
                        0.5f
                        * ma
                        * va * va;

                    assert(deltaKa >= 0.0f);
                    pointKineticEnergyLoss += deltaKa;
#endif
                }
            }

#if !FS_IS_PLATFORM_MOBILE()
            //
            // 4) Update water splash
            //

            if (pointSplashNeighbors != 0.0f)
            {
                // Water splashed is proportional to kinetic energy loss that took
                // place near free points (i.e. not drowned by water)
                waterSplashed +=
                    pointKineticEnergyLoss
                    * pointSplashFreeNeighbors
                    / pointSplashNeighbors;
            }
#endif
        }



        // TODOTEST: moved into this loop from outside
        //
        // Transforming momenta into velocities
        //

        mPoints.UpdateWaterVelocitiesFromMomenta();


        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("Total WOut=", todoTotalWOutAtQueriedPoint, " WIn=", todoTotalWInAtQueriedPoint, " WNetOut=", (todoTotalWOutAtQueriedPoint - todoTotalWInAtQueriedPoint));
            LogMessage("Total WMomOut=", todoTotalWMomentumOutAtQueriedPoint, " WMomIn=", todoTotalWMomentumInAtQueriedPoint, " WMomNetOut=", (todoTotalWMomentumOutAtQueriedPoint - todoTotalWMomentumInAtQueriedPoint));
        }


        // TODOTEST
        //
        // Damp water velocities
        //

        for (auto pointIndex : mPoints.RawShipPoints())
        {
            // TODOTEST
            //newPointWaterMomentumBufferData[pointIndex] *= 0.975f;
            mPoints.SetWaterVelocity(pointIndex, mPoints.GetWaterVelocity(pointIndex) * std::min(simulationParameters.AntiMatterBombImplosionStrength / 10.0f, 1.0f));
        }

    } // Iter loop

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif

    //
    // Read total air
    //

    float todoTotalAir = 0.0f;

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        if (!mPoints.IsDamaged(pointIndex))
            todoTotalAir += mPoints.GetAirPressure(pointIndex);
    }

    mSimulationEventHandler.OnCustomProbe("TotalAir", todoTotalAir);




    //
    // Air step
    //

    for (int iter = 0; iter < NumberOfIterations; ++iter)
    {
        // Source and result water buffers
        float const * restrict oldPointWaterBufferData = mPoints.GetWaterBufferAsFloat();

        // Source and result air buffers
        auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
        float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
        float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("Start W: ", oldPointWaterBufferData[mLastQueriedPointIndex], "  Start A: ", oldPointAirPressureBufferData[mLastQueriedPointIndex]);
        }
        float todoTotalAOut = 0.0f;


        for (auto pointIndex : mPoints.RawShipPoints())
        {
            totalOutboundAirFlowWeight = 0.0f;
            maxOutboundAirFlowWeight = 0.0f;
            totalOutboundAirFlowWeightSquared = 0.0f;

            size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                // Upness: TODOHERE -- 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector

                // TODOTEST
                //float const springUpness = springNormalizedVector.y;

                // TODOTEST: step
                //float const springUpness = Step(0.0f, springNormalizedVector.y);
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: 0->1 smooth
                //float const springUpness = (1.0f + springNormalizedVector.y) / 2.0f;
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: -1->1 smooth
                //float const springUpness = springNormalizedVector.y;
                //float const springDownness = -springUpness;

                // TODOTEST: 0->1->1 smooth
                //float const springUpness = std::min(springNormalizedVector.y + 1.0f, 1.0f);
                //float const springDownness = std::min(1.0f - springNormalizedVector.y, 1.0f);

                // TODOTEST: 0->0->1 smooth
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f);

                //// TODOTEST: 0->0->1 smooth, corrected with rest_length
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);

                // TODOTEST: 0->0->1 smooth, using delta_H
                // FUTUREWORK: need to divide by ship's square side size here, once we use scale; add ship member for that
                float const springUpness = std::max(mPoints.GetPosition(cs.OtherEndpointIndex).y - mPoints.GetPosition(pointIndex).y, 0.0f);
                float const springDownness = std::max(mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y, 0.0f);

                (void)springUpness;
                (void)springDownness;


                //
                // Air
                //
                // Moves according to pressure differentials:
                //    - Air-Air: moves to reach average air pressure
                //    - Water at this squeezes more air out
                //    - When traveling down, encounters resistance from water at other
                //    - When traveling up, TODOHERE
                //
                // Simple air pressure increase at a tank should force water to move out of it at water's turn
                //    - Hopefully downward because of Bernoulli/gravity and Rayleigh–Taylor
                //

                float const dAir =
                    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);

                float airMoved;
                if (dAir >= 0.0f)
                {
                    // Outbound
                    airMoved = dAir / 2.0f;
                }
                else
                {
                    // Not its turn
                    airMoved = 0.0f;
                }

                //
                // Add buoyancy: if layer above contains water, than this air moves up
                //

                // Indicator of "water above": 0 @ water[above] = 0.0, 1 @ water[above] >= 1.0
                float const omega = std::min(oldPointWaterBufferData[cs.OtherEndpointIndex], 1.0f);

                // Velocity along spring (only exists when going "up", and already projected onto vertical)
                float const upwardVelocity =
                    //0.3f // Magic: bubble goes up at 0.25/0.40 m/s
                    simulationParameters.ElectricalElementHeatProducedAdjustment
                    * omega
                    * springUpness;

                // TODOTEST: sum of air moved
                airMoved += upwardVelocity * SimulationParameters::SimulationStepTimeDuration<float> *oldPointAirPressureBufferData[pointIndex];

                // TODOTEST: replacement of air moved
                //airMoved =
                //    Mix(
                //        airMoved,
                //        simulationParameters.ElectricalElementHeatProducedAdjustment * SimulationParameters::SimulationStepTimeDuration<float> *oldPointAirPressureBufferData[pointIndex],
                //        omega * springUpness);

                // Store weight along spring, scaling for the greater distance traveled along
                // diagonal springs
                springOutboundAirFlowWeights[s] =
                    airMoved
                    / mSprings.GetFactoryRestLength(cs.SpringIndex)
                    * mSprings.GetWaterPermeability(cs.SpringIndex); // Only along permeable springs

                // Update total outbound flow weight
                totalOutboundAirFlowWeight += springOutboundAirFlowWeights[s];
                maxOutboundAirFlowWeight = std::max(maxOutboundAirFlowWeight, springOutboundAirFlowWeights[s]);
                totalOutboundAirFlowWeightSquared += springOutboundAirFlowWeights[s] * springOutboundAirFlowWeights[s];

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A Out: dAir=", dAir, " upwardVelocity=", upwardVelocity, " airMoved=", airMoved, " upness=", springUpness, " downness=", springDownness,
                               " springOutboundAirFlowWeights=", springOutboundAirFlowWeights[s]);
                }
            }

            //
            // 2b) Calculate normalization factors for air flows:
            //    the quantity of air along a spring is proportional to the weight of the spring
            //    (pressure flow along that spring), and the sum of all outbound flows must not
            //    exceed the air pressure currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundAirFlowWeight >= 0.0f);
            assert(maxOutboundAirFlowWeight >= 0.0f);

            float airPressureQuantityNormalizationFactor = 0.0f;
            if (totalOutboundAirFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //airPressureQuantityNormalizationFactor = std::min(
                //    (oldPointAirPressureBufferData[pointIndex] / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST
                //airPressureQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment)
                //    / totalOutboundAirFlowWeight;

                // TODOTEST: quadratic norm factor
                //airPressureQuantityNormalizationFactor =
                //    std::min(
                //        1.0f / totalOutboundAirFlowWeight,
                //        oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment / totalOutboundAirFlowWeightSquared);

                // TODOTEST: max factor
                maxOutboundAirFlowWeight = std::min(maxOutboundAirFlowWeight, oldPointAirPressureBufferData[pointIndex]);
                airPressureQuantityNormalizationFactor = std::min(
                    (maxOutboundAirFlowWeight / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("A: normFactor=", airPressureQuantityNormalizationFactor, " (oldAir=", oldPointAirPressureBufferData[pointIndex], " alpha=", simulationParameters.AirDiffusionSpeedAdjustment, " tot=", totalOutboundAirFlowWeight, ")");
                }
            }

            // TODOTEST
            airPressureQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 3) Move water/air along all springs according to their flows,
            //    and update destination's momenta accordingly
            //

            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                //
                // Air
                //

                // Calculate quantity of air pressure directed outwards,
                // being careful not to overdrain the point
                float const springOutboundQuantityOfAirPressure = std::min(
                    springOutboundAirFlowWeights[s] * airPressureQuantityNormalizationFactor,
                    newPointAirPressureBufferData[pointIndex]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A: springOutboundQuantityOfAirPressure=", springOutboundQuantityOfAirPressure, " (w=", springOutboundAirFlowWeights[s], " norm=", airPressureQuantityNormalizationFactor, ")");
                    todoTotalAOut += springOutboundQuantityOfAirPressure;
                }

                assert(springOutboundQuantityOfAirPressure >= 0.0f);
                assert(springOutboundQuantityOfAirPressure <= newPointAirPressureBufferData[pointIndex]);

                //
                // Air pressure moves from point to endpoint
                //

                newPointAirPressureBufferData[pointIndex] -= springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[pointIndex] >= 0.0f);
                newPointAirPressureBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[cs.OtherEndpointIndex] >= 0.0f);
            }
        }

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("Total A Out: ", todoTotalAOut);
        }

    } // Iter loop



    //
    // TODOTEST: readings
    //

    std::vector<PressureReading> readings;

    //ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8150;
    //ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 738;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        ElementIndex prevPointIndex = PressureCrossCutReadingsStartPointIndex;
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                0.0f,
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y});

            if (pointIndex == mLastQueriedPointIndex)
            {
                LogMessage("READ: this : a=", mPoints.GetAirPressure(pointIndex), " w=", mPoints.GetWater(pointIndex));
                LogMessage("      other: a=", mPoints.GetAirPressure(prevPointIndex), " w=", mPoints.GetWater(prevPointIndex));
            }

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            prevPointIndex = pointIndex;
            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);

    // TODOTEST
    // Calculate total water after
    float totalWaterPost = 0.0f;
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        totalWaterPost += mPoints.GetWater(pointIndex);
    }

    mSimulationEventHandler.OnCustomProbe("Total W In", totalWaterPost);
}

void Ship::UpdateWaterAndAirPressure_NewtonRhapson_2_TwoStep_NewMomenta(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move water and air along its connected springs,
    // based on pressure differentials and water momenta (https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/)
    //
    // Model is tanks, connected at bottom (for water moves) and at top (for air moves)
    //    - Hence, water moves are governed by pressures at bottom, which are air pressure + water pressure, and water momenta
    //    - Hence, air moves are governed by pressures at top, which are air pressure
    //      - But compressibility of air plays a role - i.e.water volumes plays a role
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // TODOTEST: moved into loop
    //// Calculate water momenta
    //mPoints.UpdateWaterMomentaFromVelocities();

    //// Source and result water buffers
    //auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    //float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
    //float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    //vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    //vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    //// Source and result air buffers
    //auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
    //float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
    //float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total water flow weight
    float totalOutboundWaterFlowWeight;
    float maxOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    // Weights of outbound air flows along each spring, only permeable ones;
    // set to zero for springs whose resultant scalar air flows are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundAirFlowWeights;

    // Total air flow weight
    float totalOutboundAirFlowWeight;
    float maxOutboundAirFlowWeight;
    float totalOutboundAirFlowWeightSquared = 0.0f;

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-mPoints.GetWater(pointIndex) * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif


    // TODOTEST
    // Calculate total water before
    float totalWaterPre = 0.0f;
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        totalWaterPre += mPoints.GetWater(pointIndex);
    }


    //
    // Visit all non-ephemeral points and:
    //  - Move water and its momenta according to momenta and pressure differentials
    //  - Move air (pressure) according to pressure differentials (and volumetric bias)
    //
    // No need to visit ephemeral points as they have no springs
    //

    // TODOTEST
    //int constexpr NumberOfIterations = 4;
    int constexpr NumberOfIterations = 2;
    //int constexpr NumberOfIterations = 1;
    //int constexpr NumberOfIterations = 32;


    //
    // Water step
    //

    for (int iter = 0; iter < NumberOfIterations; ++iter)
    {
        //
        // Damp velocities
        //

        float const dampingFactor = std::min(simulationParameters.AntiMatterBombImplosionStrength / 10.0f, 1.0f);
        for (auto pointIndex : mPoints.RawShipPoints())
        {
            mPoints.SetWaterVelocity(pointIndex, mPoints.GetWaterVelocity(pointIndex) * dampingFactor);
        }

        vec2f const * const restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("Start W=", mPoints.GetWater(mLastQueriedPointIndex), " WVel=", mPoints.GetWaterVelocity(mLastQueriedPointIndex),
                       " WMom=", mPoints.GetWaterMomentumBufferAsVec2f()[mLastQueriedPointIndex], "  Start A=", mPoints.GetAirPressure(mLastQueriedPointIndex));
        }

        //
        // Prepare momenta
        //

        for (auto pointIndex : mPoints.RawShipPoints())
        {
            mPoints.SetWaterMomentum(pointIndex, vec2f::zero());
        }

        vec2f * const restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

        //
        // Source and result water buffers
        //

        auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
        float const * const restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
        float * const restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();

        // Source air buffers
        float const * const oldPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();



        float todoTotalWOutAtQueriedPoint = 0.0f;
        float todoTotalWInAtQueriedPoint = 0.0f;
        vec2f todoTotalWMomentumOutAtQueriedPoint = vec2f::zero();
        vec2f todoTotalWMomentumInAtQueriedPoint = vec2f::zero();




        for (auto pointIndex : mPoints.RawShipPoints())
        {
            //
            // 1a) Calculate water momenta along *all* springs connected to this point,
            //     including impermeable ones - as we'll eventually bounce back along those
            // 1b) Calculate air pressure transfers along travelable springs connected to this point
            //

            // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
            // and gravity were exaggerated
            //
            // WV[t] = WV[t-1] + alpha * Bernoulli
            //
            // WaterCrazyness=0   -> alpha=1
            // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
            // WaterCrazyness=1   -> alpha=Wh
            float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

#if !FS_IS_PLATFORM_MOBILE()
            pointSplashNeighbors = 0.0f;
            pointSplashFreeNeighbors = 0.0f;
#endif

            totalOutboundWaterFlowWeight = 0.0f;
            maxOutboundWaterFlowWeight = 0.0f;

            size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                // Upness and downess

                // TODOTEST
                //float const springUpness = springNormalizedVector.y;

                // TODOTEST: step
                //float const springUpness = Step(0.0f, springNormalizedVector.y);
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: 0->1 smooth
                //float const springUpness = (1.0f + springNormalizedVector.y) / 2.0f;
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: -1->1 smooth
                //float const springUpness = springNormalizedVector.y;
                //float const springDownness = -springUpness;

                // TODOTEST: 0->1->1 smooth
                //float const springUpness = std::min(springNormalizedVector.y + 1.0f, 1.0f);
                //float const springDownness = std::min(1.0f - springNormalizedVector.y, 1.0f);

                //// TODOTEST: 0->0->1 smooth
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f);

                //// TODOTEST: 0->0->1 smooth, corrected with rest_length
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);

                // TODOTEST: 0->0->1 smooth, using delta_H
                // FUTUREWORK: need to divide by ship's square side size here, once we use scale; add ship member for that
                float const springUpness = std::max(mPoints.GetPosition(cs.OtherEndpointIndex).y - mPoints.GetPosition(pointIndex).y, 0.0f);
                float const springDownness = std::max(mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y, 0.0f);

                (void)springUpness;
                (void)springDownness;

                //
                // Water
                //
                // Moves according to water momentum + pressure differentials
                //    - Source pressure is water pressure + air pressure
                //    - Destination pressure:
                //      - When diffusing up: water pressure + air pressure
                //      - When diffusing down: water pressure - air pressure (Rayleigh–Taylor instability: water is not stopped by air below - actually drawn down)
                //

                // Component of the point's own water velocity along the spring
                float const pointWaterVelocityAlongSpring =
                    oldPointWaterVelocityBufferData[pointIndex]
                    .dot(springNormalizedVector);

                //
                // Calulate Bernoulli's velocity gained along this spring, from this point to
                // the other endpoint
                //

                // TODOTEST: ORIG (no air pressure)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex]);

                // TODOTEST: NEW-WRONG (with air pressure, but no laterals)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);

                // TODOTEST: NEW (with air pressure, and upness)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);

                //// TODOTEST: NEW (with air pressure, upness, and downness)
                //float const dw =
                //    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex] * springDownness)
                //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);

                // TODOTEST: NEW (with air pressure, upness, and downness), and no delta-pressure against wall
                float const dw = (
                    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex] * springDownness)
                    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness)
                    ) * mSprings.GetWaterPermeability(cs.SpringIndex); // Enforce no delta-pressure with (dry) wall

                // Gravity potential difference (positive implies point -> other endpoint flow)
                float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

                // Calculate gained water velocity along this spring, from point to other endpoint
                // (Bernoulli, 1738)
                //
                // We add pressure and heights as pressure is in "height equivalent units"
                float bernoulliVelocityAlongSpring;
                float const dwy = dw + dy;
                if (dwy >= 0.0f)
                {
                    // Gained velocity goes from point to other endpoint
                    bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
                }
                else
                {
                    // Gained velocity goes from other endpoint to point
                    bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
                }

                // Resultant scalar velocity along spring; outbound only, as
                // if this were inbound it wouldn't result in any movement of the point's
                // water between these two springs. Morevoer, Bernoulli's velocity injected
                // along this spring will be picked up later also by the other endpoint,
                // and at that time it would move water if it agrees with its velocity
                float const springOutboundScalarWaterVelocity = std::max(
                    pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                    0.0f);

                // Store weight along spring, as quantity of water (& pressure) moved by velocity;
                // scaling for the greater distance traveled along diagonal springs
                springOutboundWaterFlowWeights[s] =
                    // TODOTEST: orig
                    //springOutboundScalarWaterVelocity
                    // TODOTEST: new
                    springOutboundScalarWaterVelocity * SimulationParameters::SimulationStepTimeDuration<float> * oldPointWaterBufferData[pointIndex]
                    / mSprings.GetFactoryRestLength(cs.SpringIndex);

                // Resultant outbound velocity vector along spring
                springOutboundWaterVelocities[s] =
                    springNormalizedVector
                    * springOutboundScalarWaterVelocity;

                // Update total outbound flow weight
                totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];
                maxOutboundWaterFlowWeight = std::max(maxOutboundWaterFlowWeight, springOutboundWaterFlowWeights[s]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  W Out: springOutboundWaterFlowWeights=", springOutboundWaterFlowWeights[s], " dw=", dw, " springDir=", springNormalizedVector,
                               " upness=", springUpness, " downness=", springDownness);
                    LogMessage("         pThis=", oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex] * springDownness,
                        " pOther=", oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness,
                        " bVel=", bernoulliVelocityAlongSpring, " wVel=", pointWaterVelocityAlongSpring);
                }

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update splash neighbors counts
                //

                pointSplashFreeNeighbors +=
                    mSprings.GetWaterPermeability(cs.SpringIndex)
                    * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

                pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif

            }

            //
            // 2a) Calculate normalization factors for water flows:
            //    the quantity of water along a spring is proportional to the weight of the spring
            //    (resultant velocity along that spring), and the sum of all outbound flows must
            //    not exceed the water currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundWaterFlowWeight >= 0.0f);
            assert(maxOutboundWaterFlowWeight >= 0.0f);

            float waterQuantityNormalizationFactor = 0.0f;
            if (totalOutboundWaterFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //waterQuantityNormalizationFactor = std::min(
                //    // TODOTEST
                //    (oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment),
                //    //(oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST: new norm factor
                //waterQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointWaterBufferData[pointIndex] * mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment)
                //    / totalOutboundWaterFlowWeight;

                // TODOTEST: max norm factor
                maxOutboundWaterFlowWeight = std::min(maxOutboundWaterFlowWeight, oldPointWaterBufferData[pointIndex]);
                waterQuantityNormalizationFactor = std::min(
                    (maxOutboundWaterFlowWeight / totalOutboundWaterFlowWeight) * (simulationParameters.WaterDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("W: normFactor=", waterQuantityNormalizationFactor, " (oldWater=", oldPointWaterBufferData[pointIndex], " alpha=", (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment), " tot=", totalOutboundWaterFlowWeight, ")");
                }
            }

            // TODOTEST
            waterQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 3) Add to this point's water momentum the momentum that stays
            //

            float const pointTotalWaterOut = totalOutboundWaterFlowWeight * waterQuantityNormalizationFactor;
            float const pointRemainingWater = std::max(oldPointWaterBufferData[pointIndex] - pointTotalWaterOut, 0.0f);
            newPointWaterMomentumBufferData[pointIndex] += oldPointWaterVelocityBufferData[pointIndex] * pointRemainingWater;

            // TODOTEST
            if (pointIndex == mLastQueriedPointIndex)
            {
                LogMessage("  W Init: remaining=", pointRemainingWater, " new mom=", oldPointWaterVelocityBufferData[pointIndex] * pointRemainingWater, " final mom=", newPointWaterMomentumBufferData[pointIndex]);
            }

            //
            // 4) Move water/air along all springs according to their flows,
            //    and update destination's momenta accordingly
            //

#if !FS_IS_PLATFORM_MOBILE()
            // Kinetic energy lost at this point
            pointKineticEnergyLoss = 0.0f;
#endif

            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Deleted springs are removed from points' connected springs
                assert(!mSprings.IsDeleted(cs.SpringIndex));

                //
                // Water
                //

                // Calculate quantity of water directed outwards
                float const springOutboundQuantityOfWater =
                    springOutboundWaterFlowWeights[s]
                    * waterQuantityNormalizationFactor;

                assert(springOutboundQuantityOfWater >= 0.0f);

                if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
                {
                    //
                    // Water - and momentum - move from point to endpoint
                    //

                    // Move water quantity
                    newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                    newPointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                    // Add "new momentum" to target endpoint
                    newPointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                        springOutboundWaterVelocities[s]
                        * springOutboundQuantityOfWater;

                    // TODOTEST
                    if (pointIndex == mLastQueriedPointIndex)
                    {
                        LogMessage("  W Out: springOutboundQuantityOfWater=", springOutboundQuantityOfWater);

                        todoTotalWOutAtQueriedPoint += springOutboundQuantityOfWater;
                    }
                    else if (cs.OtherEndpointIndex == mLastQueriedPointIndex)
                    {
                        LogMessage("  W In: springOutboundQuantityOfWater=", springOutboundQuantityOfWater,
                            " mom in=", springOutboundWaterVelocities[s] * springOutboundQuantityOfWater, " final mom=", newPointWaterMomentumBufferData[cs.OtherEndpointIndex]);

                        todoTotalWInAtQueriedPoint += springOutboundQuantityOfWater;

                        todoTotalWMomentumInAtQueriedPoint +=
                            springOutboundWaterVelocities[s]
                            * springOutboundQuantityOfWater;
                    }

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // splintered water colliding with whole other endpoint
                    //

                    // Normalized spring vector, oriented point -> other endpoint
                    vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                        ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                        : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();
                    float mb = oldPointWaterBufferData[cs.OtherEndpointIndex];
                    float vb = oldPointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                    float vf = 0.0f;
                    if (ma + mb != 0.0f)
                        vf = (ma * va + mb * vb) / (ma + mb);

                    float deltaKa =
                        0.5f
                        * ma
                        * (va * va - vf * vf);

                    // Note: deltaKa might be negative, in which case deltaKb would have been
                    // more positive (perfectly inelastic -> deltaK == max); we will pickup
                    // deltaKb later
                    pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
                }
                else
                {
                    // Wall hit

                    //
                    // New momentum (old velocity + velocity gained) bounces back
                    // (and zeroes outgoing), assuming perfectly inelastic collision
                    //
                    // No changes to other endpoint
                    //

                    // Add "new momentum" (new velocity gained), but after bounce
                    newPointWaterMomentumBufferData[pointIndex] +=
                        -springOutboundWaterVelocities[s] * (simulationParameters.BlastToolForceAdjustment / 10.0f)
                        * springOutboundQuantityOfWater;

                    // TODOTEST
                    if (pointIndex == mLastQueriedPointIndex)
                    {
                        LogMessage("  W Bounce: springOutboundQuantityOfWater=", springOutboundQuantityOfWater,
                            " mom add=", -springOutboundWaterVelocities[s] * (simulationParameters.BlastToolForceAdjustment / 10.0f),
                            " final mom=", newPointWaterMomentumBufferData[pointIndex]);

                        todoTotalWMomentumInAtQueriedPoint +=
                            -springOutboundWaterVelocities[s] * (simulationParameters.BlastToolForceAdjustment / 10.0f)
                            * springOutboundQuantityOfWater;
                    }

#if !FS_IS_PLATFORM_MOBILE()
                    //
                    // Update point's kinetic energy loss:
                    // entire splintered water
                    //

                    float ma = springOutboundQuantityOfWater;
                    float va = springOutboundWaterVelocities[s].length();

                    float deltaKa =
                        0.5f
                        * ma
                        * va * va;

                    assert(deltaKa >= 0.0f);
                    pointKineticEnergyLoss += deltaKa;
#endif
                }
            }

#if !FS_IS_PLATFORM_MOBILE()
            //
            // 4) Update water splash
            //

            if (pointSplashNeighbors != 0.0f)
            {
                // Water splashed is proportional to kinetic energy loss that took
                // place near free points (i.e. not drowned by water)
                waterSplashed +=
                    pointKineticEnergyLoss
                    * pointSplashFreeNeighbors
                    / pointSplashNeighbors;
            }
#endif
        }



        // TODOTEST: moved into this loop from outside
        //
        // Transforming momenta into velocities
        //

        mPoints.UpdateWaterVelocitiesFromMomenta();


        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("Total WOut=", todoTotalWOutAtQueriedPoint, " WIn=", todoTotalWInAtQueriedPoint, " WNetOut=", (todoTotalWOutAtQueriedPoint - todoTotalWInAtQueriedPoint));
            LogMessage("Total WMomOut=", todoTotalWMomentumOutAtQueriedPoint, " WMomIn=", todoTotalWMomentumInAtQueriedPoint, " WMomNetOut=", (todoTotalWMomentumOutAtQueriedPoint - todoTotalWMomentumInAtQueriedPoint));
        }

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("End W=", mPoints.GetWater(mLastQueriedPointIndex), " WVel=", mPoints.GetWaterVelocity(mLastQueriedPointIndex),
                " WMom=", mPoints.GetWaterMomentumBufferAsVec2f()[mLastQueriedPointIndex], "  Start A=", mPoints.GetAirPressure(mLastQueriedPointIndex));
        }

    } // Iter loop

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif

    //
    // Read total air
    //

    float todoTotalAir = 0.0f;

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        if (!mPoints.IsDamaged(pointIndex))
            todoTotalAir += mPoints.GetAirPressure(pointIndex);
    }

    mSimulationEventHandler.OnCustomProbe("TotalAir", todoTotalAir);




    //
    // Air step
    //

    for (int iter = 0; iter < NumberOfIterations; ++iter)
    {
        // Source and result water buffers
        float const * restrict oldPointWaterBufferData = mPoints.GetWaterBufferAsFloat();

        // Source and result air buffers
        auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
        float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
        float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("================");
            LogMessage("Start W: ", oldPointWaterBufferData[mLastQueriedPointIndex], "  Start A: ", oldPointAirPressureBufferData[mLastQueriedPointIndex]);
        }
        float todoTotalAOut = 0.0f;


        for (auto pointIndex : mPoints.RawShipPoints())
        {
            totalOutboundAirFlowWeight = 0.0f;
            maxOutboundAirFlowWeight = 0.0f;
            totalOutboundAirFlowWeightSquared = 0.0f;

            size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                // Upness: TODOHERE -- 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector

                // TODOTEST
                //float const springUpness = springNormalizedVector.y;

                // TODOTEST: step
                //float const springUpness = Step(0.0f, springNormalizedVector.y);
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: 0->1 smooth
                //float const springUpness = (1.0f + springNormalizedVector.y) / 2.0f;
                //float const springDownness = 1.0f - springUpness;

                // TODOTEST: -1->1 smooth
                //float const springUpness = springNormalizedVector.y;
                //float const springDownness = -springUpness;

                // TODOTEST: 0->1->1 smooth
                //float const springUpness = std::min(springNormalizedVector.y + 1.0f, 1.0f);
                //float const springDownness = std::min(1.0f - springNormalizedVector.y, 1.0f);

                // TODOTEST: 0->0->1 smooth
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f);

                //// TODOTEST: 0->0->1 smooth, corrected with rest_length
                //float const springUpness = std::max(springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);
                //float const springDownness = std::max(-springNormalizedVector.y, 0.0f) * mSprings.GetFactoryRestLength(cs.SpringIndex);

                // TODOTEST: 0->0->1 smooth, using delta_H
                // FUTUREWORK: need to divide by ship's square side size here, once we use scale; add ship member for that
                float const springUpness = std::max(mPoints.GetPosition(cs.OtherEndpointIndex).y - mPoints.GetPosition(pointIndex).y, 0.0f);
                float const springDownness = std::max(mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y, 0.0f);

                (void)springUpness;
                (void)springDownness;


                //
                // Air
                //
                // Moves according to pressure differentials:
                //    - Air-Air: moves to reach average air pressure
                //    - Water at this squeezes more air out
                //    - When traveling down, encounters resistance from water at other
                //    - When traveling up, TODOHERE
                //
                // Simple air pressure increase at a tank should force water to move out of it at water's turn
                //    - Hopefully downward because of Bernoulli/gravity and Rayleigh–Taylor
                //

                float const dAir =
                    (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex])
                    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);

                float airMoved;
                if (dAir >= 0.0f)
                {
                    // Outbound
                    airMoved = dAir / 2.0f;
                }
                else
                {
                    // Not its turn
                    airMoved = 0.0f;
                }

                //
                // Add buoyancy: if layer above contains water, than this air moves up
                //

                // Indicator of "water above": 0 @ water[above] = 0.0, 1 @ water[above] >= 1.0
                float const omega = std::min(oldPointWaterBufferData[cs.OtherEndpointIndex], 1.0f);

                // Velocity along spring (only exists when going "up", and already projected onto vertical)
                float const upwardVelocity =
                    //0.3f // Magic: bubble goes up at 0.25/0.40 m/s
                    simulationParameters.ElectricalElementHeatProducedAdjustment
                    * omega
                    * springUpness;

                // TODOTEST: sum of air moved
                airMoved += upwardVelocity * SimulationParameters::SimulationStepTimeDuration<float> *oldPointAirPressureBufferData[pointIndex];

                // TODOTEST: replacement of air moved
                //airMoved =
                //    Mix(
                //        airMoved,
                //        simulationParameters.ElectricalElementHeatProducedAdjustment * SimulationParameters::SimulationStepTimeDuration<float> *oldPointAirPressureBufferData[pointIndex],
                //        omega * springUpness);

                // Store weight along spring, scaling for the greater distance traveled along
                // diagonal springs
                springOutboundAirFlowWeights[s] =
                    airMoved
                    / mSprings.GetFactoryRestLength(cs.SpringIndex)
                    * mSprings.GetWaterPermeability(cs.SpringIndex); // Only along permeable springs

                // Update total outbound flow weight
                totalOutboundAirFlowWeight += springOutboundAirFlowWeights[s];
                maxOutboundAirFlowWeight = std::max(maxOutboundAirFlowWeight, springOutboundAirFlowWeights[s]);
                totalOutboundAirFlowWeightSquared += springOutboundAirFlowWeights[s] * springOutboundAirFlowWeights[s];

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A Out: dAir=", dAir, " upwardVelocity=", upwardVelocity, " airMoved=", airMoved, " upness=", springUpness, " downness=", springDownness,
                        " springOutboundAirFlowWeights=", springOutboundAirFlowWeights[s]);
                }
            }

            //
            // 2b) Calculate normalization factors for air flows:
            //    the quantity of air along a spring is proportional to the weight of the spring
            //    (pressure flow along that spring), and the sum of all outbound flows must not
            //    exceed the air pressure currently at the point, accounting for diffusion speed
            //

            assert(totalOutboundAirFlowWeight >= 0.0f);
            assert(maxOutboundAirFlowWeight >= 0.0f);

            float airPressureQuantityNormalizationFactor = 0.0f;
            if (totalOutboundAirFlowWeight != 0.0f)
            {
                //// TODOTEST: orig norm factor
                //airPressureQuantityNormalizationFactor = std::min(
                //    (oldPointAirPressureBufferData[pointIndex] / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                //    1.0f);

                // TODOTEST
                //airPressureQuantityNormalizationFactor =
                //    std::min(1.0f, oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment)
                //    / totalOutboundAirFlowWeight;

                // TODOTEST: quadratic norm factor
                //airPressureQuantityNormalizationFactor =
                //    std::min(
                //        1.0f / totalOutboundAirFlowWeight,
                //        oldPointAirPressureBufferData[pointIndex] * simulationParameters.AirDiffusionSpeedAdjustment / totalOutboundAirFlowWeightSquared);

                // TODOTEST: max factor
                maxOutboundAirFlowWeight = std::min(maxOutboundAirFlowWeight, oldPointAirPressureBufferData[pointIndex]);
                airPressureQuantityNormalizationFactor = std::min(
                    (maxOutboundAirFlowWeight / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                    1.0f);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("A: normFactor=", airPressureQuantityNormalizationFactor, " (oldAir=", oldPointAirPressureBufferData[pointIndex], " alpha=", simulationParameters.AirDiffusionSpeedAdjustment, " tot=", totalOutboundAirFlowWeight, ")");
                }
            }

            // TODOTEST
            airPressureQuantityNormalizationFactor /= static_cast<float>(NumberOfIterations);

            //
            // 3) Move water/air along all springs according to their flows,
            //    and update destination's momenta accordingly
            //

            for (size_t s = 0; s < connectedSpringCount; ++s)
            {
                auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

                //
                // Air
                //

                // Calculate quantity of air pressure directed outwards,
                // being careful not to overdrain the point
                float const springOutboundQuantityOfAirPressure = std::min(
                    springOutboundAirFlowWeights[s] * airPressureQuantityNormalizationFactor,
                    newPointAirPressureBufferData[pointIndex]);

                // TODOTEST
                if (pointIndex == mLastQueriedPointIndex)
                {
                    LogMessage("  A: springOutboundQuantityOfAirPressure=", springOutboundQuantityOfAirPressure, " (w=", springOutboundAirFlowWeights[s], " norm=", airPressureQuantityNormalizationFactor, ")");
                    todoTotalAOut += springOutboundQuantityOfAirPressure;
                }

                assert(springOutboundQuantityOfAirPressure >= 0.0f);
                assert(springOutboundQuantityOfAirPressure <= newPointAirPressureBufferData[pointIndex]);

                //
                // Air pressure moves from point to endpoint
                //

                newPointAirPressureBufferData[pointIndex] -= springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[pointIndex] >= 0.0f);
                newPointAirPressureBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfAirPressure;
                assert(newPointAirPressureBufferData[cs.OtherEndpointIndex] >= 0.0f);
            }
        }

        // TODOTEST
        if (mLastQueriedPointIndex != NoneElementIndex)
        {
            LogMessage("Total A Out: ", todoTotalAOut);
        }

    } // Iter loop



    //
    // TODOTEST: readings
    //

    std::vector<PressureReading> readings;

    //ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8150;
    //ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 640;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        ElementIndex prevPointIndex = PressureCrossCutReadingsStartPointIndex;
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                0.0f,
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y });

            if (pointIndex == mLastQueriedPointIndex)
            {
                LogMessage("READ: this : a=", mPoints.GetAirPressure(pointIndex), " w=", mPoints.GetWater(pointIndex));
                LogMessage("      other: a=", mPoints.GetAirPressure(prevPointIndex), " w=", mPoints.GetWater(prevPointIndex));
            }

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            prevPointIndex = pointIndex;
            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);

    // TODOTEST
    // Calculate total water after
    float totalWaterPost = 0.0f;
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        totalWaterPost += mPoints.GetWater(pointIndex);
    }

    mSimulationEventHandler.OnCustomProbe("Total W In", totalWaterPost);
}

void Ship::UpdateWaterAndAirPressure_GaussSeidel_1(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move water and air along its connected springs,
    // based on pressure differentials and water momenta (https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/)
    //
    // Model is tanks, connected at bottom (for water moves) and at top (for air moves)
    //    - Hence, water moves are governed by pressures at bottom, which are air pressure + water pressure, and water momenta
    //    - Hence, air moves are governed by pressures at top, which are air pressure
    //      - But compressibility of air plays a role - i.e.water volumes plays a role
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // Calculate water momenta
    mPoints.UpdateWaterMomentaFromVelocities();

    // Water buffers
    float * restrict pointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    vec2f * restrict pointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    vec2f * restrict pointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    // Source and result air buffers
    float * restrict pointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total water flow weight
    float totalOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    // Weights of outbound air flows along each spring, only permeable ones;
    // set to zero for springs whose resultant scalar air flows are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundAirFlowWeights;

    // Total air flow weight
    float totalOutboundAirFlowWeight;

    auto const squeezeAir = [&](float air, float water)
        {
            // TODOTEST: original
            float const availableAirVolume = 1.0f / (1.0f + water);

            // TODOTEST: harder, multiplied
            //float const availableAirVolume = 1.0f / (1.0f + water * 0.1f);
            return air / availableAirVolume;
        };

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-pointWaterBufferData[pointIndex] * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif

    //
    // Visit all non-ephemeral points and:
    //  - Move water and its momenta according to momenta and pressure differentials
    //  - Move air (pressure) according to pressure differentials (and volumetric bias)
    //
    // No need to visit ephemeral points as they have no springs
    //

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        //
        // 1a) Calculate water momenta along *all* springs connected to this point,
        //     including impermeable ones - as we'll eventually bounce back along those
        // 1b) Calculate air pressure transfers along travelable springs connected to this point
        //

        // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
        // and gravity were exaggerated
        //
        // WV[t] = WV[t-1] + alpha * Bernoulli
        //
        // WaterCrazyness=0   -> alpha=1
        // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
        // WaterCrazyness=1   -> alpha=Wh
        float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (pointWaterBufferData[pointIndex] - 1.0f);

        // Total pressure at bottom of this point/tank
        // TODOTEST
        //float const oldThisPointTotalPressureAtBottom = oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex];

        // Volume at this tank that is available for air;
        // given that plain water would cause non-linearities, we make
        // air volume go to zero only asymptotically
        // TODOTEST
        //float const oldThisPointAvailableAirVolume = 1.0f / (1.0f + oldPointWaterBufferData[pointIndex]);

#if !FS_IS_PLATFORM_MOBILE()
        pointSplashNeighbors = 0.0f;
        pointSplashFreeNeighbors = 0.0f;
#endif

        totalOutboundWaterFlowWeight = 0.0f;
        totalOutboundAirFlowWeight = 0.0f;

        size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Normalized spring vector, oriented point -> other endpoint
            vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

            // Upness: 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector
            // TODOTEST
            //float const springUpness = springNormalizedVector.y;
            float const springUpness = Step(0.0f, springNormalizedVector.y);
            float const springDownness = 1.0f - springUpness;

            //
            // Water
            //
            // Moves according to water momentum + pressure differentials
            //    - Source pressure is water pressure + air pressure
            //    - Destination pressure:
            //      - When diffusing up: water pressure + air pressure
            //      - When diffusing down: water pressure - air pressure (Rayleigh–Taylor instability: water is not stopped by air below - actually drawn down)
            //

            // Component of the point's own water velocity along the spring
            float const pointWaterVelocityAlongSpring =
                pointWaterVelocityBufferData[pointIndex]
                .dot(springNormalizedVector);

            //
            // Calulate Bernoulli's velocity gained along this spring, from this point to
            // the other endpoint
            //

            // TODOOLD
            // Pressure difference (positive implies point -> other endpoint flow)
            // Bias with air below (Rayleigh–Taylor instability):
            //  - Going up: this total_pressure - other total_pressure
            //  - Going down: this total_pressure - (other water_pressure - other air_pressure)
            //float const dw =
            //    oldThisPointTotalPressureAtBottom
            //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);


            // TODOTEST
            //float const dwUp = oldPointWaterBufferData[pointIndex] - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);
            //float const dwDown = (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex]) - oldPointWaterBufferData[cs.OtherEndpointIndex];
            float const dwUp = pointWaterBufferData[pointIndex] - (pointWaterBufferData[cs.OtherEndpointIndex] + squeezeAir(pointAirPressureBufferData[cs.OtherEndpointIndex], pointWaterBufferData[cs.OtherEndpointIndex]));
            float const dwDown = (pointWaterBufferData[pointIndex] + squeezeAir(pointAirPressureBufferData[pointIndex], pointWaterBufferData[pointIndex])) - pointWaterBufferData[cs.OtherEndpointIndex];
            float const dw =
                (dwUp * springUpness + dwDown * springDownness)
                * mSprings.GetWaterPermeability(cs.SpringIndex); // Enforce no delta-pressure with (dry) wall



            // Gravity potential difference (positive implies point -> other endpoint flow)
            float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

            // Calculate gained water velocity along this spring, from point to other endpoint
            // (Bernoulli, 1738)
            float bernoulliVelocityAlongSpring;
            float const dwy = dw + dy;
            if (dwy >= 0.0f)
            {
                // Gained velocity goes from point to other endpoint
                bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
            }
            else
            {
                // Gained velocity goes from other endpoint to point
                bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
            }

            // Resultant scalar velocity along spring; outbound only, as
            // if this were inbound it wouldn't result in any movement of the point's
            // water between these two springs. Morevoer, Bernoulli's velocity injected
            // along this spring will be picked up later also by the other endpoint,
            // and at that time it would move water if it agrees with its velocity
            float const springOutboundScalarWaterVelocity = std::max(
                pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                0.0f);

            // Store weight along spring, scaling for the greater distance traveled along
            // diagonal springs
            springOutboundWaterFlowWeights[s] =
                // TODOTEST
                springOutboundScalarWaterVelocity
                //springOutboundScalarWaterVelocity * SimulationParameters::SimulationStepTimeDuration<float> * oldPointWaterBufferData[pointIndex]
                / mSprings.GetFactoryRestLength(cs.SpringIndex);

            // Resultant outbound velocity along spring
            springOutboundWaterVelocities[s] =
                springNormalizedVector
                * springOutboundScalarWaterVelocity;

            // Update total outbound flow weight
            totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];

#if !FS_IS_PLATFORM_MOBILE()
            //
            // Update splash neighbors counts
            //

            pointSplashFreeNeighbors +=
                mSprings.GetWaterPermeability(cs.SpringIndex)
                * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

            pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif

            //
            // Air
            //
            // Moves according to pressure differentials:
            //    - Air-Air: moves to reach average air pressure
            //    - Water at this squeezes more air out
            //    - When traveling down, encounters resistance from water at other
            //    - When traveling up, TODOHERE
            //
            // Simple air pressure increase at a tank should force water to move out of it at water's turn
            //    - Hopefully downward because of Bernoulli/gravity and Rayleigh–Taylor
            //

            // TODOTEST
            //float const equilibriumAirPressure = (oldPointAirPressureBufferData[pointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]) / 2.0f;

            //float constexpr SqueezeFactor = 0.05f;
            //float airPressureMoved =
            //    oldPointAirPressureBufferData[pointIndex]
            //    - equilibriumAirPressure * ((1.0f - SqueezeFactor) + SqueezeFactor * oldThisPointAvailableAirVolume); // Move it more if current tank has little volume left (squeeze effect)

            //// If going down, encounter resistance from water at destination (inverse squeezing)
            //// If going up, TODOHERE
            //float const oldOtherPointAvailableAirVolume = 1.0f / (1.0f + oldPointWaterBufferData[cs.OtherEndpointIndex]);
            //airPressureMoved = airPressureMoved * (1.0f - (1.0f - oldOtherPointAvailableAirVolume) * (1.0f - springUpness));




            // TODOHERE: come up with formula for delta_pressure => pressure move, and then calc delta_pressure based on squeezes (*)
            // - Orifice Flow Equation (v = C \sqrt{\frac{2 \Delta P}{\rho}}\)
            // - Make sure no air pressure is consumed/created; publish total

            float const thisAirPSqueezed = squeezeAir(pointAirPressureBufferData[pointIndex], pointWaterBufferData[pointIndex]);
            float const dAirUp = thisAirPSqueezed - pointAirPressureBufferData[cs.OtherEndpointIndex];

            float const otherAirPSqueezed = squeezeAir(pointAirPressureBufferData[cs.OtherEndpointIndex], pointWaterBufferData[cs.OtherEndpointIndex]);
            float const dAirDown = pointAirPressureBufferData[pointIndex] - otherAirPSqueezed;

            float const dAir = dAirUp * springUpness + dAirDown * springDownness;

            float airV;

            // TODOTEST
            ////if (dAir >= 0.0f)
            ////{
            ////    airV = sqrtf(2.0f * dAir / SimulationParameters::AirMass);
            ////}
            ////else
            ////{
            ////    // Not its turn
            ////    airV = 0.0f;
            ////}

            if (dAir >= 0.0f)
            {
                // Outbound
                airV = dAir / 2.0f;
            }
            else
            {
                // Not its turn
                airV = 0.0f;
            }

            // Store weight along spring, scaling for the greater distance traveled along
            // diagonal springs
            springOutboundAirFlowWeights[s] =
                // TODOTEST
                airV
                //airV * SimulationParameters::SimulationStepTimeDuration<float> * oldPointAirPressureBufferData[pointIndex]
                / mSprings.GetFactoryRestLength(cs.SpringIndex)
                * mSprings.GetWaterPermeability(cs.SpringIndex); // Only along permeable springs

            // Update total outbound flow weight
            totalOutboundAirFlowWeight += springOutboundAirFlowWeights[s];
        }

        //
        // 2a) Calculate normalization factors for water flows:
        //    the quantity of water along a spring is proportional to the weight of the spring
        //    (resultant velocity along that spring), and the sum of all outbound flows must
        //    not exceed the water currently at the point, accounting for diffusion speed
        //

        assert(totalOutboundWaterFlowWeight >= 0.0f);

        float waterQuantityNormalizationFactor = 0.0f;
        if (totalOutboundWaterFlowWeight != 0.0f)
        {
            waterQuantityNormalizationFactor = std::min(
                (pointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment),
                1.0f);
        }

        //
        // 2b) Calculate normalization factors for air flows:
        //    the quantity of air along a spring is proportional to the weight of the spring
        //    (pressure flow along that spring), and the sum of all outbound flows must not
        //    exceed the air pressure currently at the point, accounting for diffusion speed
        //

        assert(totalOutboundAirFlowWeight >= 0.0f);

        float airPressureQuantityNormalizationFactor = 0.0f;
        if (totalOutboundAirFlowWeight != 0.0f)
        {
            airPressureQuantityNormalizationFactor = std::min(
                (pointAirPressureBufferData[pointIndex] / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                1.0f);
        }

        //
        // 3) Move water/air along all springs according to their flows,
        //    and update destination's momenta accordingly
        //

#if !FS_IS_PLATFORM_MOBILE()
        // Kinetic energy lost at this point
        pointKineticEnergyLoss = 0.0f;
#endif

        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            //
            // Water
            //

            // Calculate quantity of water directed outwards
            float const springOutboundQuantityOfWater =
                springOutboundWaterFlowWeights[s]
                * waterQuantityNormalizationFactor;

            assert(springOutboundQuantityOfWater >= 0.0f);

            if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
            {
                //
                // Water - and momentum - move from point to endpoint
                //

                // Move water quantity
                pointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                pointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                // Remove "old momentum" (old velocity) from point
                pointWaterMomentumBufferData[pointIndex] -=
                    pointWaterVelocityBufferData[pointIndex]
                    * springOutboundQuantityOfWater;

                // Add "new momentum" (old velocity + velocity gained) to other endpoint
                pointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update point's kinetic energy loss:
                // splintered water colliding with whole other endpoint
                //

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();
                float mb = pointWaterBufferData[cs.OtherEndpointIndex];
                float vb = pointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                float vf = 0.0f;
                if (ma + mb != 0.0f)
                    vf = (ma * va + mb * vb) / (ma + mb);

                float deltaKa =
                    0.5f
                    * ma
                    * (va * va - vf * vf);

                // Note: deltaKa might be negative, in which case deltaKb would have been
                // more positive (perfectly inelastic -> deltaK == max); we will pickup
                // deltaKb later
                pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
            }
            else
            {
                // Wall hit

                // Deleted springs are removed from points' connected springs
                assert(!mSprings.IsDeleted(cs.SpringIndex));

                //
                // New momentum (old velocity + velocity gained) bounces back
                // (and zeroes outgoing), assuming perfectly inelastic collision
                //
                // No changes to other endpoint
                //

                pointWaterMomentumBufferData[pointIndex] -=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update point's kinetic energy loss:
                // entire splintered water
                //

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();

                float deltaKa =
                    0.5f
                    * ma
                    * va * va;

                assert(deltaKa >= 0.0f);
                pointKineticEnergyLoss += deltaKa;
#endif
            }

            //
            // Air
            //

            // Calculate quantity of air pressure directed outwards,
            // being careful not to overdrain the point
            float const springOutboundQuantityOfAirPressure = std::min(
                springOutboundAirFlowWeights[s] * airPressureQuantityNormalizationFactor,
                pointAirPressureBufferData[pointIndex]);

            assert(springOutboundQuantityOfAirPressure >= 0.0f);
            assert(springOutboundQuantityOfAirPressure <= pointAirPressureBufferData[pointIndex]);

            //
            // Air pressure moves from point to endpoint
            //

            pointAirPressureBufferData[pointIndex] -= springOutboundQuantityOfAirPressure;
            assert(pointAirPressureBufferData[pointIndex] >= 0.0f);
            pointAirPressureBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfAirPressure;
            assert(pointAirPressureBufferData[cs.OtherEndpointIndex] >= 0.0f);
        }

        // Update velocities
        if (pointWaterBufferData[pointIndex] != 0.0f)
        {
            pointWaterVelocityBufferData[pointIndex] =
                pointWaterMomentumBufferData[pointIndex]
                / pointWaterBufferData[pointIndex];
        }
        else
        {
            // No mass, no velocity
            pointWaterVelocityBufferData[pointIndex] = vec2f::zero();
        }

#if !FS_IS_PLATFORM_MOBILE()
        //
        // 4) Update water splash
        //

        if (pointSplashNeighbors != 0.0f)
        {
            // Water splashed is proportional to kinetic energy loss that took
            // place near free points (i.e. not drowned by water)
            waterSplashed +=
                pointKineticEnergyLoss
                * pointSplashFreeNeighbors
                / pointSplashNeighbors;
        }
#endif
    }

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif

    // TODOTEST
    //
    // Damp water velocities
    //

    float todoTotalAir = 0.0f;

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointWaterMomentumBufferData[pointIndex] *= 0.975f;
        if (!mPoints.IsDamaged(pointIndex))
            todoTotalAir += pointAirPressureBufferData[pointIndex];
    }

    mSimulationEventHandler.OnCustomProbe("TotalAir", todoTotalAir);



    //
    // TODOTEST: readings
    //

    std::vector<PressureReading> readings;

    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                squeezeAir(mPoints.GetAirPressure(pointIndex), mPoints.GetWater(pointIndex)),
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y });

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);

    //
    // Transforming momenta into velocities
    //

    mPoints.UpdateWaterVelocitiesFromMomenta();
}

void Ship::UpdateWaterAndAirPressure_GaussSeidel_2(
    SimulationParameters const & simulationParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move water and air along its connected springs,
    // based on pressure differentials and water momenta (https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/)
    //
    // Model is tanks, connected at bottom (for water moves) and at top (for air moves)
    //    - Hence, water moves are governed by pressures at bottom, which are air pressure + water pressure, and water momenta
    //    - Hence, air moves are governed by pressures at top, which are air pressure
    //      - But compressibility of air plays a role - i.e.water volumes plays a role
    //

#ifdef _DEBUG
    // We use cached springs vectors
    assert(!mPoints.Diagnostic_ArePositionsDirty());
#endif

    // Calculate water momenta
    mPoints.UpdateWaterMomentaFromVelocities();

    // Source and result water buffers
    auto oldPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    float const * restrict oldPointWaterBufferData = oldPointWaterBuffer->data();
    float * restrict newPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    vec2f * restrict oldPointWaterVelocityBufferData = mPoints.GetWaterVelocityBufferAsVec2();
    vec2f * restrict newPointWaterMomentumBufferData = mPoints.GetWaterMomentumBufferAsVec2f();

    // Source and result air buffers
    auto oldPointAirPressureBuffer = mPoints.MakeAirPressureBufferCopy();
    float const * restrict oldPointAirPressureBufferData = oldPointAirPressureBuffer->data();
    float * restrict newPointAirPressureBufferData = mPoints.GetAirPressureBufferAsFloat();

    // Weights of outbound water flows along each spring, including impermeable ones;
    // set to zero for springs whose resultant scalar water velocities are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total water flow weight
    float totalOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, SimulationParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    // Weights of outbound air flows along each spring, only permeable ones;
    // set to zero for springs whose resultant scalar air flows are
    // directed towards the point being visited
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundAirFlowWeights;

    // Total air flow weight
    float totalOutboundAirFlowWeight;

    auto const squeezeAir = [&](float air, float water)
        {
            // TODOTEST: original
            float const availableAirVolume = 1.0f / (1.0f + water);

            // TODOTEST: harder, multiplied
            //float const availableAirVolume = 1.0f / (1.0f + water * 0.1f);
            return air / availableAirVolume;
        };

    //
    // Quantities for water kinetic energy loss, used
    // only for sound
    //
    // Not on Mobile (as it's a small feature that costs a lot!)
    //

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses:
    //
    //  1.0f: point has no water
    //  0.0f: point has water
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-oldPointWaterBufferData[pointIndex] * 10.0f);
    }

    // Count of non-hull free and drowned neighbor points for a given point
    float pointSplashNeighbors;
    float pointSplashFreeNeighbors;

    // Kinetic energy lost for a given point
    float pointKineticEnergyLoss;
#endif

    //
    // WATER
    //

    //
    // Visit all non-ephemeral points and:
    //  - Move water and its momenta according to momenta and pressure differentials
    //  - Move air (pressure) according to pressure differentials (and volumetric bias)
    //
    // No need to visit ephemeral points as they have no springs
    //

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        //
        // 1a) Calculate water momenta along *all* springs connected to this point,
        //     including impermeable ones - as we'll eventually bounce back along those
        // 1b) Calculate air pressure transfers along travelable springs connected to this point
        //

        // A higher crazyness gives more emphasis to bernoulli's velocity, as if pressures
        // and gravity were exaggerated
        //
        // WV[t] = WV[t-1] + alpha * Bernoulli
        //
        // WaterCrazyness=0   -> alpha=1
        // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
        // WaterCrazyness=1   -> alpha=Wh
        float const alphaCrazyness = 1.0f + simulationParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

        // Total pressure at bottom of this point/tank
        // TODOTEST
        //float const oldThisPointTotalPressureAtBottom = oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex];

        // Volume at this tank that is available for air;
        // given that plain water would cause non-linearities, we make
        // air volume go to zero only asymptotically
        // TODOTEST
        //float const oldThisPointAvailableAirVolume = 1.0f / (1.0f + oldPointWaterBufferData[pointIndex]);

#if !FS_IS_PLATFORM_MOBILE()
        pointSplashNeighbors = 0.0f;
        pointSplashFreeNeighbors = 0.0f;
#endif

        totalOutboundWaterFlowWeight = 0.0f;
        totalOutboundAirFlowWeight = 0.0f;

        size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Normalized spring vector, oriented point -> other endpoint
            vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

            // Upness: 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector
            // TODOTEST
            //float const springUpness = springNormalizedVector.y;
            float const springUpness = Step(0.0f, springNormalizedVector.y);
            float const springDownness = 1.0f - springUpness;

            //
            // Water
            //
            // Moves according to water momentum + pressure differentials
            //    - Source pressure is water pressure + air pressure
            //    - Destination pressure:
            //      - When diffusing up: water pressure + air pressure
            //      - When diffusing down: water pressure - air pressure (Rayleigh–Taylor instability: water is not stopped by air below - actually drawn down)
            //

            // Component of the point's own water velocity along the spring
            float const pointWaterVelocityAlongSpring =
                oldPointWaterVelocityBufferData[pointIndex]
                .dot(springNormalizedVector);

            //
            // Calulate Bernoulli's velocity gained along this spring, from this point to
            // the other endpoint
            //

            // TODOOLD
            // Pressure difference (positive implies point -> other endpoint flow)
            // Bias with air below (Rayleigh–Taylor instability):
            //  - Going up: this total_pressure - other total_pressure
            //  - Going down: this total_pressure - (other water_pressure - other air_pressure)
            //float const dw =
            //    oldThisPointTotalPressureAtBottom
            //    - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex] * springUpness);


            // TODOTEST
            //float const dwUp = oldPointWaterBufferData[pointIndex] - (oldPointWaterBufferData[cs.OtherEndpointIndex] + oldPointAirPressureBufferData[cs.OtherEndpointIndex]);
            //float const dwDown = (oldPointWaterBufferData[pointIndex] + oldPointAirPressureBufferData[pointIndex]) - oldPointWaterBufferData[cs.OtherEndpointIndex];
            float const dwUp = oldPointWaterBufferData[pointIndex] - (oldPointWaterBufferData[cs.OtherEndpointIndex] + squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], oldPointWaterBufferData[cs.OtherEndpointIndex]));
            float const dwDown = (oldPointWaterBufferData[pointIndex] + squeezeAir(oldPointAirPressureBufferData[pointIndex], oldPointWaterBufferData[pointIndex])) - oldPointWaterBufferData[cs.OtherEndpointIndex];
            float const dw =
                (dwUp * springUpness + dwDown * springDownness)
                * mSprings.GetWaterPermeability(cs.SpringIndex); // Enforce no delta-pressure with (dry) wall



            // Gravity potential difference (positive implies point -> other endpoint flow)
            float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(cs.OtherEndpointIndex).y;

            // Calculate gained water velocity along this spring, from point to other endpoint
            // (Bernoulli, 1738)
            float bernoulliVelocityAlongSpring;
            float const dwy = dw + dy;
            if (dwy >= 0.0f)
            {
                // Gained velocity goes from point to other endpoint
                bernoulliVelocityAlongSpring = sqrtf(2.0f * SimulationParameters::GravityMagnitude * dwy);
            }
            else
            {
                // Gained velocity goes from other endpoint to point
                bernoulliVelocityAlongSpring = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * -dwy);
            }

            // Resultant scalar velocity along spring; outbound only, as
            // if this were inbound it wouldn't result in any movement of the point's
            // water between these two springs. Morevoer, Bernoulli's velocity injected
            // along this spring will be picked up later also by the other endpoint,
            // and at that time it would move water if it agrees with its velocity
            float const springOutboundScalarWaterVelocity = std::max(
                pointWaterVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
                0.0f);

            // Store weight along spring, scaling for the greater distance traveled along
            // diagonal springs
            springOutboundWaterFlowWeights[s] =
                // TODOTEST
                //springOutboundScalarWaterVelocity
                springOutboundScalarWaterVelocity * SimulationParameters::SimulationStepTimeDuration<float> *oldPointWaterBufferData[pointIndex]
                / mSprings.GetFactoryRestLength(cs.SpringIndex);

            // Resultant outbound velocity along spring
            springOutboundWaterVelocities[s] =
                springNormalizedVector
                * springOutboundScalarWaterVelocity;

            // Update total outbound flow weight
            totalOutboundWaterFlowWeight += springOutboundWaterFlowWeights[s];

#if !FS_IS_PLATFORM_MOBILE()
            //
            // Update splash neighbors counts
            //

            pointSplashFreeNeighbors +=
                mSprings.GetWaterPermeability(cs.SpringIndex)
                * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

            pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
#endif
        }

        //
        // 2a) Calculate normalization factors for water flows:
        //    the quantity of water along a spring is proportional to the weight of the spring
        //    (resultant velocity along that spring), and the sum of all outbound flows must
        //    not exceed the water currently at the point, accounting for diffusion speed
        //

        assert(totalOutboundWaterFlowWeight >= 0.0f);

        float waterQuantityNormalizationFactor = 0.0f;
        if (totalOutboundWaterFlowWeight != 0.0f)
        {
            waterQuantityNormalizationFactor = std::min(
                (oldPointWaterBufferData[pointIndex] / totalOutboundWaterFlowWeight) * (mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * simulationParameters.WaterDiffusionSpeedAdjustment),
                1.0f);
        }


        //
        // 3) Move water/air along all springs according to their flows,
        //    and update destination's momenta accordingly
        //

#if !FS_IS_PLATFORM_MOBILE()
        // Kinetic energy lost at this point
        pointKineticEnergyLoss = 0.0f;
#endif

        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            //
            // Water
            //

            // Calculate quantity of water directed outwards
            float const springOutboundQuantityOfWater =
                springOutboundWaterFlowWeights[s]
                * waterQuantityNormalizationFactor;

            assert(springOutboundQuantityOfWater >= 0.0f);

            if (mSprings.GetWaterPermeability(cs.SpringIndex) != 0.0f)
            {
                //
                // Water - and momentum - move from point to endpoint
                //

                // Move water quantity
                newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                newPointWaterBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfWater;

                // Remove "old momentum" (old velocity) from point
                newPointWaterMomentumBufferData[pointIndex] -=
                    oldPointWaterVelocityBufferData[pointIndex]
                    * springOutboundQuantityOfWater;

                // Add "new momentum" (old velocity + velocity gained) to other endpoint
                newPointWaterMomentumBufferData[cs.OtherEndpointIndex] +=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update point's kinetic energy loss:
                // splintered water colliding with whole other endpoint
                //

                // Normalized spring vector, oriented point -> other endpoint
                vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                    ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                    : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();
                float mb = oldPointWaterBufferData[cs.OtherEndpointIndex];
                float vb = oldPointWaterVelocityBufferData[cs.OtherEndpointIndex].dot(springNormalizedVector);

                float vf = 0.0f;
                if (ma + mb != 0.0f)
                    vf = (ma * va + mb * vb) / (ma + mb);

                float deltaKa =
                    0.5f
                    * ma
                    * (va * va - vf * vf);

                // Note: deltaKa might be negative, in which case deltaKb would have been
                // more positive (perfectly inelastic -> deltaK == max); we will pickup
                // deltaKb later
                pointKineticEnergyLoss += std::max(deltaKa, 0.0f);
#endif
            }
            else
            {
                // Wall hit

                // Deleted springs are removed from points' connected springs
                assert(!mSprings.IsDeleted(cs.SpringIndex));

                //
                // New momentum (old velocity + velocity gained) bounces back
                // (and zeroes outgoing), assuming perfectly inelastic collision
                //
                // No changes to other endpoint
                //

                newPointWaterMomentumBufferData[pointIndex] -=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;

#if !FS_IS_PLATFORM_MOBILE()
                //
                // Update point's kinetic energy loss:
                // entire splintered water
                //

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();

                float deltaKa =
                    0.5f
                    * ma
                    * va * va;

                assert(deltaKa >= 0.0f);
                pointKineticEnergyLoss += deltaKa;
#endif
            }
        }

#if !FS_IS_PLATFORM_MOBILE()
        //
        // 4) Update water splash
        //

        if (pointSplashNeighbors != 0.0f)
        {
            // Water splashed is proportional to kinetic energy loss that took
            // place near free points (i.e. not drowned by water)
            waterSplashed +=
                pointKineticEnergyLoss
                * pointSplashFreeNeighbors
                / pointSplashNeighbors;
        }
#endif
    }

#if !FS_IS_PLATFORM_MOBILE()
    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);
#endif

    // TODOTEST
    //
    // Damp water velocities
    //

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        newPointWaterMomentumBufferData[pointIndex] *= 0.975f;
    }

    //
    // Transforming momenta into velocities
    //

    mPoints.UpdateWaterVelocitiesFromMomenta();




    //
    // AIR
    //

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        totalOutboundAirFlowWeight = 0.0f;

        size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Normalized spring vector, oriented point -> other endpoint
            vec2f const springNormalizedVector = (pointIndex == mSprings.GetEndpointAIndex(cs.SpringIndex))
                ? mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex)
                : -mSprings.GetCachedVectorialNormalizedVector(cs.SpringIndex);

            // Upness: 1.0 when up, -1.0 when down - it's cos(alpha) with alpha being angle with upward vector
            // TODOTEST
            //float const springUpness = springNormalizedVector.y;
            float const springUpness = Step(0.0f, springNormalizedVector.y);
            float const springDownness = 1.0f - springUpness;

            float const thisAirPSqueezed = squeezeAir(oldPointAirPressureBufferData[pointIndex], newPointWaterBufferData[pointIndex]);
            float const dAirUp = thisAirPSqueezed - oldPointAirPressureBufferData[cs.OtherEndpointIndex];

            float const otherAirPSqueezed = squeezeAir(oldPointAirPressureBufferData[cs.OtherEndpointIndex], newPointWaterBufferData[cs.OtherEndpointIndex]);
            float const dAirDown = oldPointAirPressureBufferData[pointIndex] - otherAirPSqueezed;

            float const dAir = dAirUp * springUpness + dAirDown * springDownness;

            float airV;

            // TODOTEST
            ////if (dAir >= 0.0f)
            ////{
            ////    airV = sqrtf(2.0f * dAir / SimulationParameters::AirMass);
            ////}
            ////else
            ////{
            ////    // Not its turn
            ////    airV = 0.0f;
            ////}

            if (dAir >= 0.0f)
            {
                // Outbound
                airV = dAir / 2.0f;
            }
            else
            {
                // Not its turn
                airV = 0.0f;
            }

            // Store weight along spring, scaling for the greater distance traveled along
            // diagonal springs
            springOutboundAirFlowWeights[s] =
                // TODOTEST
                airV
                //airV * SimulationParameters::SimulationStepTimeDuration<float> * oldPointAirPressureBufferData[pointIndex]
                / mSprings.GetFactoryRestLength(cs.SpringIndex)
                * mSprings.GetWaterPermeability(cs.SpringIndex); // Only along permeable springs

            // Update total outbound flow weight
            totalOutboundAirFlowWeight += springOutboundAirFlowWeights[s];
        }

        //
        // 2b) Calculate normalization factors for air flows:
        //    the quantity of air along a spring is proportional to the weight of the spring
        //    (pressure flow along that spring), and the sum of all outbound flows must not
        //    exceed the air pressure currently at the point, accounting for diffusion speed
        //

        assert(totalOutboundAirFlowWeight >= 0.0f);

        float airPressureQuantityNormalizationFactor = 0.0f;
        if (totalOutboundAirFlowWeight != 0.0f)
        {
            airPressureQuantityNormalizationFactor = std::min(
                (oldPointAirPressureBufferData[pointIndex] / totalOutboundAirFlowWeight) * (simulationParameters.AirDiffusionSpeedAdjustment),
                1.0f);
        }

        //
        // 3) Move air along all springs according to their flows
        //

        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            //
            // Air
            //

            // Calculate quantity of air pressure directed outwards,
            // being careful not to overdrain the point
            float const springOutboundQuantityOfAirPressure = std::min(
                springOutboundAirFlowWeights[s] * airPressureQuantityNormalizationFactor,
                newPointAirPressureBufferData[pointIndex]);

            assert(springOutboundQuantityOfAirPressure >= 0.0f);
            assert(springOutboundQuantityOfAirPressure <= newPointAirPressureBufferData[pointIndex]);

            //
            // Air pressure moves from point to endpoint
            //

            newPointAirPressureBufferData[pointIndex] -= springOutboundQuantityOfAirPressure;
            assert(newPointAirPressureBufferData[pointIndex] >= 0.0f);
            newPointAirPressureBufferData[cs.OtherEndpointIndex] += springOutboundQuantityOfAirPressure;
            assert(newPointAirPressureBufferData[cs.OtherEndpointIndex] >= 0.0f);
        }
    }






    //
    // TODOTEST: readings
    //


    float todoTotalAir = 0.0f;
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        if (!mPoints.IsDamaged(pointIndex))
            todoTotalAir += newPointAirPressureBufferData[pointIndex];
    }
    mSimulationEventHandler.OnCustomProbe("TotalAir", todoTotalAir);



    std::vector<PressureReading> readings;

    ElementIndex constexpr PressureCrossCutReadingsStartPointIndex = 8283;
    ElementIndex constexpr PressureCrossCutReadingsEndPointIndex = 639;
    if (PressureCrossCutReadingsStartPointIndex < mPoints.GetRawShipPointCount())
    {
        for (ElementIndex pointIndex = PressureCrossCutReadingsStartPointIndex; pointIndex != NoneElementIndex && pointIndex != PressureCrossCutReadingsEndPointIndex; /* updated in loop */)
        {
            // Read
            readings.emplace_back(PressureReading{
                mPoints.GetAirPressure(pointIndex),
                squeezeAir(mPoints.GetAirPressure(pointIndex), mPoints.GetWater(pointIndex)),
                mPoints.GetWater(pointIndex),
                mPoints.GetPosition(pointIndex).y });

            // Advance
            ElementIndex nextPointIndex = NoneElementIndex;
            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const springOctant = mSprings.GetFactoryOtherEndpointOctant(cs.SpringIndex, pointIndex);
                if (springOctant == 6)
                {
                    nextPointIndex = cs.OtherEndpointIndex;
                    break;
                }
            }

            pointIndex = nextPointIndex;
        }
    }

    mSimulationEventHandler.OnPressureReadings(readings);
}

void Ship::UpdateSinking(float /*currentSimulationTime*/)
{
    //
    // Calculate total number of wet points
    //

    size_t wetPointCount = 0;

    for (auto p : mPoints.RawShipPoints())
    {
        if (mPoints.GetWater(p) >= 0.5f) // Magic number - we only count a point as wet if its water is above this threshold
            ++wetPointCount;
    }

    if (!mIsSinking)
    {
        if (wetPointCount > mPoints.GetRawShipPointCount() * 3 / 10 + mPoints.GetTotalFactoryWetPoints()) // High watermark
        {
            // Started sinking
            mParentWorld.GetNpcs().OnShipStartedSinking(mId); // Tell NPCs
            mSimulationEventHandler.OnSinkingBegin(mId);
            mIsSinking = true;
        }
    }
    else
    {
        if (wetPointCount < mPoints.GetRawShipPointCount() * 1 / 10 + mPoints.GetTotalFactoryWetPoints()) // Low watermark
        {
            // Stopped sinking
            mSimulationEventHandler.OnSinkingEnd(mId);
            mIsSinking = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Electrical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::RecalculateLightDiffusionParallelism(ThreadPool const & simulationThreadPool)
{
    auto const simulationParallelism = simulationThreadPool.GetParallelism();

    LogMessage("Ship::RecalculateLightDiffusionParallelism: simulationParallelism=", simulationParallelism);

    //
    // Prepare tasks
    //

    mLightDiffusionTasks.clear();

    ElementCount const numberOfPoints = mPoints.GetAlignedShipPointCount(); // No real reason to skip ephemerals, other than they're not expected to be lighted

    auto const pointShards = CalculatePointShards(
        numberOfPoints,
        simulationThreadPool);

    ElementIndex pointStart = 0;
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const pointEnd = pointStart + static_cast<ElementCount>(pointShards[t]);
        assert(pointEnd <= numberOfPoints);

        assert(((pointEnd - pointStart) % vectorization_float_count<ElementCount>) == 0);

        mLightDiffusionTasks.emplace_back(
            [this, pointStart, pointEnd]()
            {
                Algorithms::DiffuseLight(
                    pointStart,
                    pointEnd,
                    mPoints.GetPositionBufferAsVec2(),
                    mPoints.GetPlaneIdBufferAsPlaneId(),
                    mElectricalElements.GetLampPositionWorkBuffers()[0].data(),
                    mElectricalElements.GetLampPositionWorkBuffers()[1].data(),
                    mElectricalElements.GetLampPlaneIdWorkBuffer().data(),
                    mElectricalElements.GetLampDistanceCoefficientWorkBuffer().data(),
                    mElectricalElements.GetLampLightSpreadMaxDistanceBufferAsFloat(),
                    mElectricalElements.GetBufferLampCount(),
                    mPoints.GetLightBufferAsFloat());
            });

        pointStart = pointEnd;
    }
}

void Ship::DiffuseLight(
    SimulationParameters const & simulationParameters,
    ThreadManager & threadManager)
{
    //
    // Diffuse light from each lamp to all points on the same or lower plane ID,
    // inverse-proportionally to the lamp-point distance
    //

    // Shortcut
    if (mElectricalElements.Lamps().empty()
        || !simulationParameters.IsLightingEnabled
        || simulationParameters.LuminiscenceAdjustment == 0.0f)
    {
        // Zero out buffer if it's dirty
        if (mIsLightBufferPopulated)
        {
            mPoints.ZeroLightBuffer();
            mIsLightBufferPopulated = false;
        }

        return;
    }

    //
    // 1. Prepare lamp data
    //

    auto & lampPositionsX = mElectricalElements.GetLampPositionWorkBuffers()[0]; // Padded to vectorization float count
    auto & lampPositionsY = mElectricalElements.GetLampPositionWorkBuffers()[1]; // Padded to vectorization float count
    auto & lampPlaneIds = mElectricalElements.GetLampPlaneIdWorkBuffer(); // Padded to vectorization float count
    auto & lampDistanceCoeffs = mElectricalElements.GetLampDistanceCoefficientWorkBuffer(); // Padded to vectorization float count

    auto const lampCount = mElectricalElements.GetLampCount();
    for (ElementIndex l = 0; l < lampCount; ++l)
    {
        auto const lampElectricalElementIndex = mElectricalElements.Lamps()[l];
        auto const lampPointIndex = mElectricalElements.GetPointIndex(lampElectricalElementIndex);

        auto const & lampPosition = mPoints.GetPosition(lampPointIndex);
        lampPositionsX[l] = lampPosition.x;
        lampPositionsY[l] = lampPosition.y;
        lampPlaneIds[l] = mPoints.GetPlaneId(lampPointIndex);
        lampDistanceCoeffs[l] =
            mElectricalElements.GetLampRawDistanceCoefficient(l)
            * mElectricalElements.GetAvailableLight(lampElectricalElementIndex);
    }

    //
    // 2. Diffuse light
    //

    threadManager.GetSimulationThreadPool().Run(mLightDiffusionTasks);

    // Remember that we've diffused light, so we will zero out the buffer
    // when we stop running the algo
    mIsLightBufferPopulated = true;
}

///////////////////////////////////////////////////////////////////////////////////
// Heat
///////////////////////////////////////////////////////////////////////////////////

void Ship::PropagateHeat(
    float /*currentSimulationTime*/,
    float dt,
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters)
{
    //
    // Propagate temperature (via heat), and dissipate temperature
    //

    // Source and result temperature buffers
    auto oldPointTemperatureBuffer = mPoints.MakeTemperatureBufferCopy();
    float const * restrict const oldPointTemperatureBufferData = oldPointTemperatureBuffer->data();
    float * restrict const newPointTemperatureBufferData = mPoints.GetTemperatureBufferAsFloat();

    // Outbound heat flows along each spring
    std::array<float, SimulationParameters::MaxSpringsPerPoint> springOutboundHeatFlows;

    //
    // Visit all non-ephemeral points
    //
    // No particular reason to not do ephemeral points as well - it's just
    // that at the moment ephemeral particles are not connected to each other
    //

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        // Temperature of this point
        float const pointTemperature = oldPointTemperatureBufferData[pointIndex];

        //
        // 1) Calculate total outgoing heat
        //

        float totalOutgoingHeat = 0.0f;

        // Visit all springs
        size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Calculate outgoing heat flow per unit of time
            //
            // q = Ki * (Tp - Tpi) * dt / Li
            float const outgoingHeatFlow =
                mSprings.GetMaterialThermalConductivity(cs.SpringIndex) * simulationParameters.ThermalConductivityAdjustment
                * std::max(pointTemperature - oldPointTemperatureBufferData[cs.OtherEndpointIndex], 0.0f) // DeltaT, positive if going out
                * dt
                / mSprings.GetFactoryRestLength(cs.SpringIndex);

            // Store flow
            springOutboundHeatFlows[s] = outgoingHeatFlow;

            // Update total outgoing heat
            totalOutgoingHeat += outgoingHeatFlow;
        }


        //
        // 2) Calculate normalization factor - to ensure that point's temperature won't go below zero (Kelvin)
        //

        float normalizationFactor;
        if (totalOutgoingHeat > 0.0f)
        {
            // Q = Kp * Tp
            float const pointHeat =
                pointTemperature
                / mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);

            normalizationFactor = std::min(
                pointHeat / totalOutgoingHeat,
                1.0f);
        }
        else
        {
            normalizationFactor = 0.0f;
        }


        //
        // 3) Transfer outgoing heat, lowering temperature of point and increasing temperature of target points
        //

        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Raise target temperature due to this flow
            newPointTemperatureBufferData[cs.OtherEndpointIndex] +=
                springOutboundHeatFlows[s] * normalizationFactor
                * mPoints.GetMaterialHeatCapacityReciprocal(cs.OtherEndpointIndex);
        }

        // Update point's temperature due to total flow
        newPointTemperatureBufferData[pointIndex] -=
            totalOutgoingHeat * normalizationFactor
            * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);
    }

    //
    // Dissipate heat
    //

    float const effectiveWaterConvectiveHeatTransferCoefficient =
        SimulationParameters::WaterConvectiveHeatTransferCoefficient
        * dt
        * simulationParameters.HeatDissipationAdjustment
        * 2.0f; // We exaggerate a bit to take into account water wetting the material and thus making it more difficult for fire to re-kindle

    // Water temperature
    // We approximate the thermocline as a linear decrease of
    // temperature: 15 degrees in MaxSeaDepth meters
    float const surfaceWaterTemperature = simulationParameters.WaterTemperature;

    // We include rain in air
    float const effectiveAirConvectiveHeatTransferCoefficient =
        SimulationParameters::AirConvectiveHeatTransferCoefficient
        * dt
        * simulationParameters.HeatDissipationAdjustment
        + FastPow(stormParameters.RainDensity, 0.3f) * effectiveWaterConvectiveHeatTransferCoefficient;

    float const airTemperature =
        simulationParameters.AirTemperature
        + stormParameters.AirTemperatureDelta;

    // We also include ephemeral points, as they may be heated
    // and have a temperature
    for (auto pointIndex : mPoints)
    {
        float deltaT; // Temperature delta (particle - env)
        float heatLost; // Heat lost in this time quantum (positive when outgoing)

        if (mPoints.IsCachedUnderwater(pointIndex)
            || mPoints.GetWater(pointIndex) > SimulationParameters::SmotheringWaterHighWatermark)
        {
            // Dissipation in water
            float const waterTemperature = Formulae::CalculateWaterTemperature(mPoints.GetPosition(pointIndex).y, surfaceWaterTemperature);
            deltaT = newPointTemperatureBufferData[pointIndex] - waterTemperature;
            heatLost = effectiveWaterConvectiveHeatTransferCoefficient * deltaT;
        }
        else
        {
            // Dissipation in air
            deltaT = newPointTemperatureBufferData[pointIndex] - airTemperature;
            heatLost = effectiveAirConvectiveHeatTransferCoefficient * deltaT;
        }

        // Temperature delta due to heat removal
        float const dissipationDeltaT = heatLost * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);

        // Remove this heat from the point, making sure we don't overshoot
        if (deltaT >= 0)
        {
            newPointTemperatureBufferData[pointIndex] -=
                std::min(dissipationDeltaT, deltaT);
        }
        else
        {
            newPointTemperatureBufferData[pointIndex] -=
                std::max(dissipationDeltaT, deltaT);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////////////////

void Ship::DecayPoints(
    ElementIndex partition,
    ElementIndex partitionCount,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
{
    //
    // Decaying is done with a recursive equation:
    //  decay(0) = 1.0
    //  decay(n) = a * decay(n-1), with 0 < a < 1
    //
    // a (alpha): the smaller the alpha, the faster we decay.
    //
    // This converges to:
    //  decay(n) = a^n
    //
    // If at time N we want decay=Dn: a = Dn ^ (1/N)
    //
    // However, we use beta = 1 - alpha, so that our decay can take
    // the form: decay(n) = (1-beta*enabler) * decay(n-1), allowing
    // enabler to turn off decay when it's zero
    //

    //
    // Rot
    //

    if (simulationParameters.RotAcceler8r != mCurrentRotAcceler8r)
    {
        if (simulationParameters.RotAcceler8r >= 0.01f)
        {
            float constexpr NsExposed = 40.0f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;
            mDecayRotExposedDryAlpha = powf(0.85f, simulationParameters.RotAcceler8r / NsExposed);
            mDecayRotExposedWetAlpha = powf(0.25f, simulationParameters.RotAcceler8r / NsExposed);

            float constexpr NsDamage = 20.0f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;
            mDecayRotDamageDryAlpha = powf(0.85f, simulationParameters.RotAcceler8r / NsDamage);
            mDecayRotDamageWetAlpha = powf(0.25f, simulationParameters.RotAcceler8r / NsDamage);
        }
        else
        {
            mDecayRotExposedDryAlpha = 1.0f;
            mDecayRotExposedWetAlpha = 1.0f;
            mDecayRotDamageDryAlpha = 1.0f;
            mDecayRotDamageWetAlpha = 1.0f;
        }

        // Water solubility

        float constexpr NsWaterSolubility = 0.76782f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;

        mDecayWaterSolubilityAlpha = simulationParameters.RotAcceler8r != 0.0f
            ? powf(0.1f, simulationParameters.RotAcceler8r / NsWaterSolubility)
            : 1.0f;

        mCurrentRotAcceler8r = simulationParameters.RotAcceler8r;
    }

    float const a_rot_exposed_dry = mDecayRotExposedDryAlpha;
    float const a_rot_exposed_wet = mDecayRotExposedWetAlpha;
    float const a_rot_damage_dry = mDecayRotDamageDryAlpha;
    float const a_rot_damage_wet = mDecayRotDamageWetAlpha;

    //
    // Rust
    //

    if (simulationParameters.RustAcceler8r != mCurrentRustAcceler8r)
    {
        if (simulationParameters.RustAcceler8r != 0.0f)
        {
            float constexpr NsExposed = 30.0f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;
            mDecayRustExposedDryAlpha = std::max(powf(0.85f, simulationParameters.RustAcceler8r / NsExposed), 0.5f); // At least 0.5 to ensure sum of beta's < 1
            mDecayRustExposedWetAlpha = std::max(powf(0.75f, simulationParameters.RustAcceler8r / NsExposed), 0.5f); // At least 0.5 to ensure sum of beta's < 1

            float constexpr NsDamage = 4.5f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;
            mDecayRustDamageDryAlpha = std::max(powf(0.50f, simulationParameters.RustAcceler8r / NsDamage), 0.5f); // At least 0.5 to ensure sum of beta's < 1
            mDecayRustDamageWetAlpha = std::max(powf(0.0009765625f, simulationParameters.RustAcceler8r / NsDamage), 0.5f); // At least 0.5 to ensure sum of beta's < 1

            float constexpr NsNeighbors = 1.5f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;
            mDecayRustNeighborsDryAlpha = std::max(powf(0.25f, simulationParameters.RustAcceler8r / NsNeighbors), 0.5f); // At least 0.5 to ensure sum of beta's < 1
            mDecayRustNeighborsWetAlpha = std::max(powf(0.01f, simulationParameters.RustAcceler8r / NsNeighbors), 0.5f); // At least 0.5 to ensure sum of beta's < 1
        }
        else
        {
            mDecayRustExposedDryAlpha = 1.0f;
            mDecayRustExposedWetAlpha = 1.0f;
            mDecayRustDamageDryAlpha = 1.0f;
            mDecayRustDamageWetAlpha = 1.0f;
            mDecayRustNeighborsDryAlpha = 1.0f;
            mDecayRustNeighborsWetAlpha = 1.0f;
        }

        mCurrentRustAcceler8r = simulationParameters.RustAcceler8r;
    }

    float const a_rust_exposed_dry = mDecayRustExposedDryAlpha;
    float const a_rust_exposed_wet = mDecayRustExposedWetAlpha;
    float const a_rust_damage_dry = mDecayRustDamageDryAlpha;
    float const a_rust_damage_wet = mDecayRustDamageWetAlpha;
    float const a_rust_neighbors_dry = mDecayRustNeighborsDryAlpha;
    float const a_rust_neighbors_wet = mDecayRustNeighborsWetAlpha;

    // Adj = 0 => 0.0
    // Adj = 1 => Base
    // Adj = Max => 1.0
    static_assert(SimulationParameters::MaxRustWeaknessAdjustment > 1.0f);
    float constexpr BaseRustWeakness = 0.025f;
    float const rustWeaknessFactor = (simulationParameters.RustWeaknessAdjustment <= 1.0f)
        ? BaseRustWeakness * simulationParameters.RustWeaknessAdjustment
        : BaseRustWeakness + (1.0f - BaseRustWeakness) * (simulationParameters.RustWeaknessAdjustment - 1.0f) / (SimulationParameters::MaxRustWeaknessAdjustment - 1.0f);

    //
    // Algae growth
    //

    if (simulationParameters.AlgaeGrowthAcceler8r != mCurrentAlgaeGrowthAcceler8r)
    {
        float constexpr Ns = 30.0f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;

        mDecayAlgaeGrowthAlpha = simulationParameters.AlgaeGrowthAcceler8r != 0.0f
            ? powf(0.25f, simulationParameters.AlgaeGrowthAcceler8r / Ns)
            : 1.0f;

        mCurrentAlgaeGrowthAcceler8r = simulationParameters.AlgaeGrowthAcceler8r;
    }

    float const a_algeGrowth = mDecayAlgaeGrowthAlpha;

    //
    // Water solubility
    //

    float const b_waterSolubility = 1.0f - mDecayWaterSolubilityAlpha;

    //
    // Process all non-ephemeral points in this partition - no real reason
    // to exclude ephemerals, other than they're not expected to decay
    //

    ElementCount const partitionSize = (mPoints.GetRawShipPointCount() / partitionCount) + ((mPoints.GetRawShipPointCount() % partitionCount) ? 1 : 0);
    ElementCount const startPointIndex = partition * partitionSize;
    ElementCount const endPointIndex = std::min(startPointIndex + partitionSize, mPoints.GetRawShipPointCount());
    for (ElementIndex p = startPointIndex; p < endPointIndex; ++p)
    {
        float constexpr UnderwaterInterfaceWidth = 2.0f;
        float const isUnderwater = Clamp((mPoints.GetCachedDepth(p) + UnderwaterInterfaceWidth) / UnderwaterInterfaceWidth, 0.0f, 1.0f);
        float const water = std::min(mPoints.GetWater(p), 1.0f);
        float const isWet = std::min(isUnderwater + water, 1.0f);
        float const isDamaged = mPoints.GetIsDamaged(p);
        auto const & structuralMaterial = mPoints.GetStructuralMaterial(p);

        // The alpha we'll use for the weakness, resultant of all the weakening decay processes
        float alphaWeakness = 1.0f;

        //
        // Rot
        //

        //  - Not damaged, more so if wet
        //  - Damaged, more so if wet

        float const betaRot =
            (1.0f - Mix(a_rot_damage_dry, a_rot_damage_wet, isWet)) * isDamaged
            + (1.0f - Mix(a_rot_exposed_dry, a_rot_exposed_wet, isWet)) * (1.0f - isDamaged);

        float const alphaRot =
            1.0f
            - betaRot * structuralMaterial.RotReceptivity * (1.0f - 0.7f * mPoints.GetRandomNormalizedUniformPersonalitySeed(p)); // Allow zero's to rot

        // Rot
        mPoints.SetRot(p, mPoints.GetRot(p) * alphaRot);
        alphaWeakness = std::min(alphaWeakness, alphaRot);

        //
        // Rust
        //

        // 1) Base rust:
        //  - Not damaged, more so if wet
        //  - Damaged, more so if wet

        float const betaRustBase =
            (1.0f - Mix(a_rust_damage_dry, a_rust_damage_wet, isWet)) * isDamaged
            + (1.0f - Mix(a_rust_exposed_dry, a_rust_exposed_wet, isWet)) * (1.0f - isDamaged);

        // 2) Rust by neighbors, imprinting pattern via random personality seed

        float avgNeighborsRust = 0.0f;
        auto const nCs = mPoints.GetConnectedSprings(p).ConnectedSprings.size();
        if (nCs > 0)
        {
            for (auto const & cs : mPoints.GetConnectedSprings(p).ConnectedSprings)
            {
                avgNeighborsRust += 1.0f - mPoints.GetRust(cs.OtherEndpointIndex);
            }

            avgNeighborsRust /= static_cast<float>(nCs);
        }

        float const betaRustNeighbors =
            (1.0f - Mix(a_rust_neighbors_dry, a_rust_neighbors_wet, isWet)) // Rusts faster when wet
            * avgNeighborsRust
            * (1.0f - 0.982f * mPoints.GetRandomNormalizedUniformPersonalitySeed(p)); // Allow zero's to rust

        // Combine

        float const betaRust = (betaRustBase + betaRustNeighbors) * structuralMaterial.RustReceptivity;
        assert(betaRust >= 0.0f && betaRust <= 1.0f);

        // Rust
        mPoints.SetRust(p, mPoints.GetRust(p) * (1.0f - betaRust));
        alphaWeakness = std::min(alphaWeakness, 1.0f - betaRust * rustWeaknessFactor);

        //
        // Algae growth
        //

        // Growth if underwater

        float const betaAlgaeGrowthUnderwater = (1.0f - a_algeGrowth) * isUnderwater;

        // Grow towards the pattern
        float const currentAlgaeGrowth = mPoints.GetAlgaeGrowth(p);
        mPoints.SetAlgaeGrowth(p, currentAlgaeGrowth + (mPoints.GetAlgaeGrowthPattern(p) - currentAlgaeGrowth) * betaAlgaeGrowthUnderwater);

        //
        // Water solubility
        //

        float const betaSolubility =
            b_waterSolubility
            * isWet
            * structuralMaterial.WaterSolubility;

        alphaWeakness = std::min(alphaWeakness, 1.0f - betaSolubility);

        //
        // Weakness
        //

        mPoints.SetWeakness(p, mPoints.GetWeakness(p) * alphaWeakness);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateForSimulationParameters(
    ThreadPool const & simulationThreadPool,
    SimulationParameters const & simulationParameters)
{
    size_t const simulationParallelism = simulationThreadPool.GetParallelism();
    if (simulationParallelism != mCurrentSimulationParallelism
        || simulationParameters.SpringRelaxationParallelComputationMode != mCurrentSpringRelaxationParallelComputationMode)
    {
        // Re-calculate spring relaxation parallelism
        RecalculateSpringRelaxationParallelism(simulationThreadPool, simulationParameters);

        // Re-calculate light diffusion parallelism
        RecalculateLightDiffusionParallelism(simulationThreadPool);

        // Remember new values
        mCurrentSimulationParallelism = simulationParallelism;
        mCurrentSpringRelaxationParallelComputationMode = simulationParameters.SpringRelaxationParallelComputationMode;
    }
}

//#define RENDER_FLOOD_DISTANCE

void Ship::RunConnectivityVisit()
{
    //
    //
    // Here we visit the entire network of points (NOT including the ephemerals - they'll be assigned
    // their own plane ID's at creation time) and propagate connectivity information:
    //
    // - PlaneID: all points belonging to the same connected component, including "strings",
    //            are assigned the same plane ID
    //
    // - Connected Component ID: at this moment we assign the same value as the plane ID; in the future
    //                           we might want to only assign a connected component ID to "solids" by only
    //                           assigning it to points that are not string points
    //                           (this will then require a separate visit pass)
    //
    // At the end of a visit *ALL* (non-ephemeral) points will have a Plane ID.
    //
    // We also piggyback the visit to create the array containing the counts of triangles in each plane,
    // so that we can later upload triangles in {PlaneID, Tessellation Order} order.
    //

    // Generate a new visit sequence number
    auto const visitSequenceNumber = ++mCurrentConnectivityVisitSequenceNumber;

    // Initialize plane ID
    PlaneId currentPlaneId = 0; // Also serves as Connected Component ID
    float currentPlaneIdFloat = 0.0f;

    // Reset count of points per connected component
    mConnectedComponentSizes.clear();

#ifdef RENDER_FLOOD_DISTANCE
    std::optional<float> floodDistanceColor;
#endif

    // The set of (already) marked points, from which we still
    // have to propagate out
    std::queue<ElementIndex> pointsToPropagateFrom;

    // Reset per-plane triangle indices
    size_t totalPlaneTrianglesCount = 0;
    mPlaneTriangleIndicesToRender.clear();
    mPlaneTriangleIndicesToRender.push_back(totalPlaneTrianglesCount); // First plane starts at zero, and we have zero triangles

    // Initialize count of points in this connected component
    size_t currentConnectedComponentPointCount = 1;

    // Flag to remember whether we still have an un-finalized connected component, which would happen
    // when we are holding on to orphaned points waiting for a larger connected component
    bool hasUnfinalizedConnectedComponent = false;

    // Visit all non-ephemeral points
    for (auto pointIndex : mPoints.RawShipPointsReverse())
    {
        // Don't re-visit already-visited points
        if (mPoints.GetCurrentConnectivityVisitSequenceNumber(pointIndex) != visitSequenceNumber)
        {
            //
            // Flood a new plane from this point
            //

            // Visit this point first
            mPoints.SetPlaneId(pointIndex, currentPlaneId, currentPlaneIdFloat);
            mPoints.SetConnectedComponentId(pointIndex, static_cast<ConnectedComponentId>(currentPlaneId));
            mPoints.SetCurrentConnectivityVisitSequenceNumber(pointIndex, visitSequenceNumber);

            // Add point to queue
            assert(pointsToPropagateFrom.empty());
            pointsToPropagateFrom.push(pointIndex);

            // Visit all points reachable from this point via springs
            while (!pointsToPropagateFrom.empty())
            {
                // Pop point that we have to propagate from
                auto const currentPointIndex = pointsToPropagateFrom.front();
                pointsToPropagateFrom.pop();

                // This point has been visited already
                assert(visitSequenceNumber == mPoints.GetCurrentConnectivityVisitSequenceNumber(currentPointIndex));

#ifdef RENDER_FLOOD_DISTANCE
                if (!floodDistanceColor)
                {
                    mPoints.GetColor(currentPointIndex) = vec4f(0.0f, 0.0f, 0.75f, 1.0f);
                    floodDistanceColor = 0.0f;
                }
                else
                    mPoints.GetColor(currentPointIndex) = vec4f(*floodDistanceColor, 0.0f, 0.0f, 1.0f);
                floodDistanceColor = *floodDistanceColor + 1.0f / 128.0f;
                if (*floodDistanceColor > 1.0f)
                    floodDistanceColor = 0.0f;
#endif

                // Visit all its non-visited connected points
                for (auto const & cs : mPoints.GetConnectedSprings(currentPointIndex).ConnectedSprings)
                {
                    if (visitSequenceNumber != mPoints.GetCurrentConnectivityVisitSequenceNumber(cs.OtherEndpointIndex))
                    {
                        //
                        // Visit point
                        //

                        mPoints.SetPlaneId(cs.OtherEndpointIndex, currentPlaneId, currentPlaneIdFloat);
                        mPoints.SetConnectedComponentId(cs.OtherEndpointIndex, static_cast<ConnectedComponentId>(currentPlaneId));
                        mPoints.SetCurrentConnectivityVisitSequenceNumber(cs.OtherEndpointIndex, visitSequenceNumber);

                        // Add point to queue
                        pointsToPropagateFrom.push(cs.OtherEndpointIndex);

                        // Update count of points in this connected component
                        ++currentConnectedComponentPointCount;
                    }
                }

                // Update count of triangles with this points's triangles
                totalPlaneTrianglesCount += mPoints.GetConnectedOwnedTrianglesCount(currentPointIndex);
            }

            //
            // Now, if we have visited a ral connected component (i.e. > 1 particles, implying there's
            // at least one spring and thus a component), store its information and start a new connected
            // component; otherwise, hold on to this plane, eventually adding more to it
            //

            if (currentConnectedComponentPointCount > 1)
            {
                // Remember count of points in this connected component
                assert(mConnectedComponentSizes.size() == static_cast<size_t>(currentPlaneId));
                mConnectedComponentSizes.push_back(currentConnectedComponentPointCount);

                // Remember the starting index of the triangles in the next plane
                assert(mPlaneTriangleIndicesToRender.size() == static_cast<size_t>(currentPlaneId + 1));
                mPlaneTriangleIndicesToRender.push_back(totalPlaneTrianglesCount);

                //
                // Flood completed
                //

                // Remember max plane ID ever
                mMaxMaxPlaneId = std::max(mMaxMaxPlaneId, currentPlaneId);

                // Next we begin a new plane and connected component
                ++currentPlaneId;
                currentPlaneIdFloat = static_cast<float>(currentPlaneId);

                // Initialize count of points in the new connected component
                currentConnectedComponentPointCount = 1;

                // No more deferred points
                hasUnfinalizedConnectedComponent = false;
            }
            else
            {
                // Keep going, remembering that we are accumulating
                hasUnfinalizedConnectedComponent = true;
            }
        }
    }

    if (hasUnfinalizedConnectedComponent)
    {
        //
        // Finalize last connected component
        //

        // Remember count of points in this connected component
        assert(mConnectedComponentSizes.size() == static_cast<size_t>(currentPlaneId));
        mConnectedComponentSizes.push_back(currentConnectedComponentPointCount);

        // Remember the starting index of the triangles in the next plane
        assert(mPlaneTriangleIndicesToRender.size() == static_cast<size_t>(currentPlaneId + 1));
        mPlaneTriangleIndicesToRender.push_back(totalPlaneTrianglesCount);

        // Remember max plane ID ever
        mMaxMaxPlaneId = std::max(mMaxMaxPlaneId, currentPlaneId);
    }

#ifdef RENDER_FLOOD_DISTANCE
    // Remember colors are dirty
    mPoints.MarkColorBufferAsDirty();
#endif

    // Remember plane IDs are dirty
    mPoints.MarkPlaneIdBufferAsDirty();

    //
    // Re-order burning points, as their plane IDs might have changed
    //

    mPoints.ReorderBurningPointsForDepth();
}

void Ship::SetAndPropagateResultantPointHullness(
    ElementIndex pointElementIndex,
    bool isHull)
{
    // Set point's resultant hullness
    mPoints.SetIsHull(pointElementIndex, isHull);

    // Propagate springs' water permeability accordingly:
    // the spring is impermeable if at least one endpoint is hull
    // (we don't want to propagate water towards a hull point)
    for (auto const & cs : mPoints.GetConnectedSprings(pointElementIndex).ConnectedSprings)
    {
        mSprings.SetWaterPermeability(
            cs.SpringIndex,
            (isHull || mPoints.GetIsHull(cs.OtherEndpointIndex)) ? 0.0f : 1.0f);
    }
}

void Ship::DestroyConnectedTriangles(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    //
    // Destroy all triangles connected to the point
    //

    // Note: we can't simply iterate and destroy, as destroying a triangle causes
    // that triangle to be removed from the vector being iterated
    auto & connectedTriangles = mPoints.GetConnectedTriangles(pointElementIndex).ConnectedTriangles;
    while (!connectedTriangles.empty())
    {
        assert(!mTriangles.IsDeleted(connectedTriangles.back()));
        mTriangles.Destroy(connectedTriangles.back(), currentSimulationTime, simulationParameters);
    }

    assert(mPoints.GetConnectedTriangles(pointElementIndex).ConnectedTriangles.empty());
}

void Ship::DestroyConnectedTriangles(
    ElementIndex pointAElementIndex,
    ElementIndex pointBElementIndex,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    //
    // Destroy the triangles that have an edge among the two points
    //

    auto & connectedTriangles = mPoints.GetConnectedTriangles(pointAElementIndex).ConnectedTriangles;
    if (!connectedTriangles.empty())
    {
        for (size_t t = connectedTriangles.size() - 1; ;--t)
        {
            auto const triangleIndex = connectedTriangles[t];

            assert(!mTriangles.IsDeleted(triangleIndex));

            if (mTriangles.GetPointAIndex(triangleIndex) == pointBElementIndex
                || mTriangles.GetPointBIndex(triangleIndex) == pointBElementIndex
                || mTriangles.GetPointCIndex(triangleIndex) == pointBElementIndex)
            {
                // Erase it
                mTriangles.Destroy(triangleIndex, currentSimulationTime, simulationParameters);
            }

            if (t == 0)
                break;
        }
    }
}

void Ship::AttemptPointRestore(
    ElementIndex pointElementIndex,
    float currentSimulationTime)
{
    //
    // A point is eligible for restore if it's damaged and has all of its factory springs and all
    // of its factory triangles
    //

    if (mPoints.GetConnectedSprings(pointElementIndex).ConnectedSprings.size() == mPoints.GetFactoryConnectedSprings(pointElementIndex).ConnectedSprings.size()
        && mPoints.GetConnectedTriangles(pointElementIndex).ConnectedTriangles.size() == mPoints.GetFactoryConnectedTriangles(pointElementIndex).ConnectedTriangles.size()
        && mPoints.IsDamaged(pointElementIndex))
    {
        mPoints.Restore(pointElementIndex, currentSimulationTime);
    }
}

void Ship::InternalSpawnAirBubble(
    vec2f const & position,
    float depth,
    float finalScale, // Relative to texture's world dimensions
    float temperature,
    PlaneId planeId,
    float currentSimulationTime,
    SimulationParameters const & /*simulationParameters*/)
{
    std::uint64_t constexpr PhasePeriod = 10;
    float const phase = static_cast<float>((mAirBubblesCreatedCount++) % PhasePeriod) / static_cast<float>(PhasePeriod);

    float const endVortexAmplitude = 4.0f * finalScale / SimulationParameters::ShipAirBubbleFinalScale; // We want 4 for ship
    float const startVortexAmplitude = endVortexAmplitude / 40.0f;
    float const vortexAmplitude =
        (startVortexAmplitude + (endVortexAmplitude - startVortexAmplitude) * phase)
        * (GameRandomEngine::GetInstance().Choose(2) == 1 ? 1.0f : -1.0f);

    float const vortexPeriod = GameRandomEngine::GetInstance().GenerateUniformReal(
        1.5f,  // seconds
        4.5f); // seconds

    float constexpr StartBuoyancyVolumeFillAdjustment = 1.25f;
    float constexpr EndBuoyancyVolumeFillAdjustment = 0.75f;
    float const buoyancyVolumeFillAdjustment =
        StartBuoyancyVolumeFillAdjustment + (EndBuoyancyVolumeFillAdjustment - StartBuoyancyVolumeFillAdjustment) * phase;

    mPoints.CreateEphemeralParticleAirBubble(
        position,
        depth,
        finalScale,
        temperature,
        buoyancyVolumeFillAdjustment,
        vortexAmplitude,
        vortexPeriod,
        currentSimulationTime,
        planeId);
}

void Ship::InternalSpawnDebris(
    ElementIndex sourcePointElementIndex,
    StructuralMaterial const & debrisStructuralMaterial,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    if (simulationParameters.DoGenerateDebris)
    {
        unsigned int const debrisParticleCount = GameRandomEngine::GetInstance().GenerateUniformInteger(
            SimulationParameters::MinDebrisParticlesPerEvent, SimulationParameters::MaxDebrisParticlesPerEvent);

        auto const pointPosition = mPoints.GetPosition(sourcePointElementIndex);
        auto const pointDepth = mParentWorld.GetOceanSurface().GetDepth(pointPosition);

        for (unsigned int d = 0; d < debrisParticleCount; ++d)
        {
            // Choose velocity
            vec2f const velocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
                SimulationParameters::MinDebrisParticlesVelocity,
                SimulationParameters::MaxDebrisParticlesVelocity);

            // Choose a lifetime
            float const maxLifetime = GameRandomEngine::GetInstance().GenerateUniformReal(
                SimulationParameters::MinDebrisParticlesLifetime,
                SimulationParameters::MaxDebrisParticlesLifetime);

            mPoints.CreateEphemeralParticleDebris(
                pointPosition,
                velocity,
                pointDepth,
                debrisStructuralMaterial,
                sourcePointElementIndex,
                currentSimulationTime,
                maxLifetime);
        }
    }
}

void Ship::InternalSpawnSiltCloud(
    EnergeticSiltImpact const & siltImpact,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    assert(siltImpact.Position.x >= -SimulationParameters::HalfMaxWorldWidth
        && siltImpact.Position.x <= SimulationParameters::HalfMaxWorldWidth);

    float const siltImpactDepth = mParentWorld.GetOceanSurface().GetDepth(siltImpact.Position);

    float const kineticEnergyFactor = LinearStep(
        simulationParameters.SiltDustCloudEnergyThreshold,
        simulationParameters.SiltDustCloudEnergyThreshold * 4.0f,
        siltImpact.KineticEnergy);

    //
    // Calculate velocity: opposite particle's velocity, magnitude depending on kinetic energy
    //

    float constexpr MinVelocityMagnitude = 3.0f;
    float constexpr MaxVelocityMagnitude = 6.0f;
    float const velocityMagnitude =
        MinVelocityMagnitude
        + (MaxVelocityMagnitude - MinVelocityMagnitude) * kineticEnergyFactor;
    vec2f const velocity = -mPoints.GetVelocity(siltImpact.PointIndex).normalise_approx() * velocityMagnitude;

    //
    // Calculate scale: depends on kinetic energy
    //

    float const minMaxScale = (siltImpactDepth > 0.0f) ? 0.4f : 0.2f;
    float const maxScale =
        minMaxScale
        + (1.0f - minMaxScale) * kineticEnergyFactor;
    float const initialScale = maxScale / 4.0f;

    //
    // Calculate max lifetime
    //

    auto const & siltCloudMaterial = mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::SiltCloud);
    float maxLifetime;
    float buoyancyVolumeFill;
    if (siltImpactDepth > 0.0f)
    {
        // Underwater

        // Use desired lifetime
        maxLifetime = simulationParameters.SiltDustCloudUnderwaterLifetime;

        // We calculate the buoyancy volume fill required for the particle to
        // go up and down in the desired time, using:
        //    ay = g(bvf * 1000 / 10 - 1)
        //     t = 2 * vy / ay
        //
        // Note that we use velocity magnitude instead of actual vertical velocity
        // as we want to eschew very small vertical velocities

        buoyancyVolumeFill =
            (1.0f - (2.0f * velocityMagnitude) / (SimulationParameters::GravityMagnitude * maxLifetime))
            / (SimulationParameters::WaterMass / siltCloudMaterial.GetMass());
    }
    else
    {
        // Above-water: gravity is the only force acting on the silt cloud

        maxLifetime =
            2.0f
            * velocityMagnitude // We use velocity magnitude instead of actual vertical velocity as we want to eschew very small vertical velocities
            / SimulationParameters::GravityMagnitude
            * 1.25f; // For longer lifetime, empirical

        buoyancyVolumeFill = siltCloudMaterial.BuoyancyVolumeFill; // Almost irrelevant, unless it falls into water :-)
    }

    //
    // Create particle
    //

    //LogMessage("SiltCloudParticle: V=", velocity, " T=", maxLifetime, " minScale=", initialScale, " maxScale=", maxScale, " silImpactDepth=", siltImpactDepth);

    mPoints.CreateEphemeralParticleSiltCloud(
        siltImpact.Position,
        siltImpactDepth,
        velocity,
        initialScale,
        maxScale,
        currentSimulationTime,
        maxLifetime,
        buoyancyVolumeFill,
        mPoints.GetPlaneId(siltImpact.PointIndex),
        simulationParameters);
}

void Ship::InternalSpawnSparklesForCut(
    ElementIndex springElementIndex,
    vec2f const & cutDirectionStartPos,
    vec2f const & cutDirectionEndPos,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    if (simulationParameters.DoGenerateSparklesForCuts)
    {
        vec2f const sparklePosition = mSprings.GetMidpointPosition(springElementIndex, mPoints);

        float const sparkleDepth = mParentWorld.GetOceanSurface().GetDepth(sparklePosition);

        // Velocity magnitude
        float const velocityMagnitude = GameRandomEngine::GetInstance().GenerateUniformReal(
            SimulationParameters::MinSparkleParticlesForCutVelocity, SimulationParameters::MaxSparkleParticlesForCutVelocity);

        // Velocity angle: gaussian centered around direction opposite to cut direction
        float const centralAngleCW = (cutDirectionStartPos - cutDirectionEndPos).angleCw();
        float const velocityAngleCw = GameRandomEngine::GetInstance().GenerateNormalReal(centralAngleCW, Pi<float> / 100.0f);

        // Choose a lifetime
        float const maxLifetime = GameRandomEngine::GetInstance().GenerateUniformReal(
            SimulationParameters::MinSparkleParticlesForCutLifetime,
            SimulationParameters::MaxSparkleParticlesForCutLifetime);

        // Create sparkle
        mPoints.CreateEphemeralParticleSparkle(
            sparklePosition,
            vec2f::fromPolar(velocityMagnitude, velocityAngleCw),
            mSprings.GetBaseStructuralMaterial(springElementIndex),
            sparkleDepth,
            currentSimulationTime,
            maxLifetime,
            mSprings.GetPlaneId(springElementIndex, mPoints));
    }
}

void Ship::InternalSpawnSparklesForLightning(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    SimulationParameters const & /*simulationParameters*/)
{
    //
    // Choose number of particles
    //

    unsigned int const sparkleParticleCount = GameRandomEngine::GetInstance().GenerateUniformInteger(
        SimulationParameters::MinSparkleParticlesForLightningEvent, SimulationParameters::MaxSparkleParticlesForLightningEvent);

    //
    // Create particles
    //

    vec2f const sparklePosition = mPoints.GetPosition(pointElementIndex);

    float const sparkleDepth = mParentWorld.GetOceanSurface().GetDepth(sparklePosition);

    for (unsigned int d = 0; d < sparkleParticleCount; ++d)
    {
        // Velocity magnitude
        float const velocityMagnitude = GameRandomEngine::GetInstance().GenerateUniformReal(
            SimulationParameters::MinSparkleParticlesForLightningVelocity, SimulationParameters::MaxSparkleParticlesForLightningVelocity);

        // Velocity angle: uniform
        float const velocityAngleCw = GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>);

        // Choose a lifetime
        float const maxLifetime = GameRandomEngine::GetInstance().GenerateUniformReal(
                SimulationParameters::MinSparkleParticlesForLightningLifetime,
                SimulationParameters::MaxSparkleParticlesForLightningLifetime);

        // Create sparkle
        mPoints.CreateEphemeralParticleSparkle(
            sparklePosition,
            vec2f::fromPolar(velocityMagnitude, velocityAngleCw),
            mPoints.GetStructuralMaterial(pointElementIndex),
            sparkleDepth,
            currentSimulationTime,
            maxLifetime,
            mPoints.GetPlaneId(pointElementIndex));
    }
}

void Ship::InternalSpawnWaterFoam(
    vec2f const & position,
    float verticalDirection,
    float strength,
    PlaneId planeId,
    Buffer<float> & outputCachedPointDepths, // We may spawn these while calculating new cached depths
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    assert(position.x >= -SimulationParameters::HalfMaxWorldWidth
        && position.x <= SimulationParameters::HalfMaxWorldWidth);

    // Note: values of strength can get as high as 8.0, and possibly more
    float const normalizedStrength = LinearStep(0.0f, 1.0f, strength);

    //
    // Calculate y
    //
    // Spawn at a distance from starting y (~ocean surface), to simulate falling down/floating up
    //

    float constexpr DeltaY = 3.5f;
    float const foamPositionY = position.y + verticalDirection * DeltaY * normalizedStrength;

    //
    // Calculate base scale: depends on strength (only its 0.0...1.0 range)
    //

    float constexpr MinMaxScale = 0.5f;
    float constexpr MaxMaxScale = 1.2f;
    float const baseScale = MinMaxScale + (MaxMaxScale - MinMaxScale) * normalizedStrength;

    //
    // Calculate max lifetime: depends on strength
    //

    float constexpr MinMaxLifetime = 3.0f;
    float constexpr MaxMaxLifetime = 4.0f;
    float const maxLifetime =
        (MinMaxLifetime + (MaxMaxLifetime - MinMaxLifetime) * std::min(strength, 8.0f))
        * simulationParameters.WaterFoamLifetimeAdjustment;

    size_t const nParticles = GameRandomEngine::GetInstance().GenerateUniformInteger<size_t>(4, 5);
    for (size_t p = 0; p < nParticles; ++p)
    {
        //
        // Decide sign
        //

        float const sign = (p % 2) == 0 ? 1.0f : -1.0f;

        //
        // Decide x velocity
        //

        float constexpr MaxXVelocity = 4.0f;
        float const velocityX =
            sign
            * std::min(
                std::abs(GameRandomEngine::GetInstance().GenerateNormalReal(0.0f, MaxXVelocity / 2.0f)),
                MaxXVelocity)
            * (0.5f + 0.5f * normalizedStrength);

        //
        // Decide x position: spiraling around center, to spread particles horizontallydeltaDepth
        //

        float constexpr StepWorldWidth = 0.6f;
        float const numberOfSteps = static_cast<float>((p + 1) / 2);
        float const positionX = Clamp(
            position.x + numberOfSteps * StepWorldWidth * sign,
            -SimulationParameters::HalfMaxWorldWidth,
            SimulationParameters::HalfMaxWorldWidth);

        vec2f const foamPosition = vec2f(positionX, foamPositionY);

        float const foamDepth = mParentWorld.GetOceanSurface().GetDepth(foamPosition);

        //
        // Randomize scale
        //

        float const maxScale = baseScale * GameRandomEngine::GetInstance().GenerateUniformReal(0.7f, 1.1f);

        //
        // Create foam particle
        //

        auto const foamPointIndex = mPoints.CreateEphemeralParticleWaterFoam(
            foamPosition,
            foamDepth,
            velocityX,
            0.0f, // Initial scale
            maxScale,
            currentSimulationTime,
            maxLifetime,
            1.0f, // Alpha
            1.0f, // Neutral haste
            planeId,
            simulationParameters);

        if (foamPointIndex != NoneElementIndex)
        {
            // Store cached depth for it, as we might be calculating cached depths at this moment
            outputCachedPointDepths[foamPointIndex] = foamDepth;
        }
    }
}

void Ship::InternalSpawnWaterSplash(
    vec2f const & position,
    vec2f const & direction,
    float strength,
    PlaneId planeId,
    Buffer<float> & outputCachedPointDepths, // We may spawn these while calculating new cached depths
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    assert(position.x >= -SimulationParameters::HalfMaxWorldWidth
        && position.x <= SimulationParameters::HalfMaxWorldWidth);

    float const impactDepth = mParentWorld.GetOceanSurface().GetDepth(position);

    //
    // Calculate velocity: magnitude depending on strength
    //

    float constexpr MinVelocityMagnitude = 5.2f;
    float constexpr MaxVelocityMagnitude = 6.0f;
    float const velocityMagnitude =
        MinVelocityMagnitude
        + (MaxVelocityMagnitude - MinVelocityMagnitude) * std::min(strength, 9.6f);

    //
    // Calculate scale: depends on strength
    //

    float constexpr MinMaxScale = 0.45f;
    float constexpr MaxMaxScale = 2.0f;
    float const maxScale =
        MinMaxScale
        + (MaxMaxScale - MinMaxScale) * std::min(strength, 4.8f);
    float const initialScale = maxScale / 4.0f; // Start already visible

    //
    // Calculate max lifetime: gravity is the only force acting on the splash
    //

    float const maxLifetime =
        2.0f
        * Clamp(velocityMagnitude, 10.0f, 15.0f) // Ensure long persistence with weak splashes, but never too long
        / SimulationParameters::GravityMagnitude;

    //
    // Spawn all particles
    //

    size_t const nParticles = GameRandomEngine::GetInstance().GenerateUniformInteger<size_t>(2, 3);
    for (size_t p = 0; p < nParticles; ++p)
    {
        //
        // Decide direction
        //

        vec2f particleDirection;
        if (p == 0)
        {
            particleDirection = direction;
        }
        else
        {
            float const directionOffsetAngleCcw = Clamp(
                GameRandomEngine::GetInstance().GenerateNormalReal(0.0f, Pi<float> / 3.0f),
                -Pi<float> / 3.0f,
                Pi<float> / 3.0f);
            particleDirection = direction.rotate(directionOffsetAngleCcw);
        }

        //
        // Create particle
        //

        auto const splashPointIndex = mPoints.CreateEphemeralParticleWaterSplash(
            position,
            impactDepth,
            particleDirection * velocityMagnitude,
            initialScale,
            maxScale,
            currentSimulationTime,
            maxLifetime,
            1.0f, // Alpha
            planeId,
            simulationParameters);

        // Store cached depth for it, as we might be calculating cached depths at this moment
        assert(splashPointIndex != NoneElementIndex);
        outputCachedPointDepths[splashPointIndex] = impactDepth;
    }
}

/////////////////////////////////////////////////////////////////////////
// IShipPhysicsHandler
/////////////////////////////////////////////////////////////////////////

void Ship::HandlePointDetach(
    ElementIndex pointElementIndex,
    bool generateDebris,
    bool fireDestroyEvent,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    bool hasAnythingBeenDestroyed = false;

    //
    // Destroy all springs attached to this point
    //

    // Note: we can't simply iterate and destroy, as destroying a spring causes
    // that spring to be removed from the vector being iterated
    auto & connectedSprings = mPoints.GetConnectedSprings(pointElementIndex).ConnectedSprings;
    while (!connectedSprings.empty())
    {
        assert(!mSprings.IsDeleted(connectedSprings.back().SpringIndex));

        mSprings.Destroy(
            connectedSprings.back().SpringIndex,
            Springs::DestroyOptions::DoNotFireBreakEvent // We're already firing the Destroy event for the point
            | Springs::DestroyOptions::DestroyAllTriangles, // Destroy all triangles connected to each endpoint
            currentSimulationTime,
            simulationParameters,
            mPoints);

        hasAnythingBeenDestroyed = true;
    }

    assert(mPoints.GetConnectedSprings(pointElementIndex).ConnectedSprings.empty());

    // At this moment, we've deleted all springs connected to this point, and we
    // asked those strings to destroy all triangles connected to each endpoint
    // (thus including this one).
    // Given that a point is connected to a triangle iff the point is an endpoint
    // of a spring-edge of that triangle, then we shouldn't have any triangles now
    assert(mPoints.GetConnectedTriangles(pointElementIndex).ConnectedTriangles.empty());

    //
    // Destroy the connected electrical element, if any
    //
    // Note: we rely on the fact that this happens after connected springs have been destroyed, which
    // ensures that the electrical element's set of connected electrical elements is now empty
    //

    auto const electricalElementIndex = mPoints.GetElectricalElement(pointElementIndex);
    if (NoneElementIndex != electricalElementIndex)
    {
        if (!mElectricalElements.IsDeleted(electricalElementIndex))
        {
            assert(mElectricalElements.GetConnectedElectricalElements(electricalElementIndex).empty());
            assert(mElectricalElements.GetConductingConnectedElectricalElements(electricalElementIndex).empty());

            mElectricalElements.Destroy(
                electricalElementIndex,
                fireDestroyEvent ? ElectricalElements::DestroyReason::Other : ElectricalElements::DestroyReason::SilentRemoval,
                currentSimulationTime,
                simulationParameters);

            hasAnythingBeenDestroyed = true;
        }
    }

    if (hasAnythingBeenDestroyed)
    {
        // Notify gadgets
        mGadgets.OnPointDetached(
            pointElementIndex,
            currentSimulationTime,
            simulationParameters);

        if (generateDebris)
        {
            // Emit debris
            InternalSpawnDebris(
                pointElementIndex,
                mPoints.GetStructuralMaterial(pointElementIndex),
                currentSimulationTime,
                simulationParameters);
        }

        if (fireDestroyEvent)
        {
            // Notify destroy
            mSimulationEventHandler.OnDestroy(
                mPoints.GetStructuralMaterial(pointElementIndex),
                mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex)),
                1);
        }

        // Remember the structure is now dirty
        mIsStructureDirty = true;
    }
}

void Ship::HandlePointDamaged(ElementIndex /*pointElementIndex*/)
{
    // Update count of damaged points
    ++mDamagedPointsCount;
}

void Ship::HandleEphemeralParticleDestroy(ElementIndex pointElementIndex)
{
    // Notify pins
    mPinnedPoints.OnEphemeralParticleDestroyed(pointElementIndex);
}

void Ship::HandlePointRestore(
    ElementIndex pointElementIndex,
    float currentSimulationTime)
{
    //
    // Restore the connected electrical element, if any and if it's deleted
    //
    // Note: this happens after connected springs have been restored
    //

    auto const electricalElementIndex = mPoints.GetElectricalElement(pointElementIndex);
    if (NoneElementIndex != electricalElementIndex
        && mElectricalElements.IsDeleted(electricalElementIndex))
    {
        mElectricalElements.Restore(electricalElementIndex);
    }

    // Update count of damaged points
    assert(mDamagedPointsCount > 0);
    --mDamagedPointsCount;

    // Notify if we've just completely restored the ship
    if (mDamagedPointsCount == 0 && mBrokenSpringsCount == 0 && mBrokenTrianglesCount == 0)
    {
        mParentWorld.GetNpcs().OnShipRepaired(mId, currentSimulationTime); // Tell NPCs
        mSimulationEventHandler.OnShipRepaired(mId);
    }
}

void Ship::HandleSpringDestroy(
    ElementIndex springElementIndex,
    bool destroyAllTriangles,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    auto const pointAIndex = mSprings.GetEndpointAIndex(springElementIndex);
    auto const pointBIndex = mSprings.GetEndpointBIndex(springElementIndex);

    //
    // Remove spring from other elements
    //

    // Remove the spring from its endpoints
    mPoints.DisconnectSpring(pointAIndex, springElementIndex, pointBIndex);
    mPoints.DisconnectSpring(pointBIndex, springElementIndex, pointAIndex);

    // Notify endpoints that have become orphaned
    if (mPoints.GetConnectedSprings(pointAIndex).ConnectedSprings.empty())
        mPoints.OnOrphaned(pointAIndex);
    if (mPoints.GetConnectedSprings(pointBIndex).ConnectedSprings.empty())
        mPoints.OnOrphaned(pointBIndex);

    /////////////////////////////////////////////////

    //
    // Destroy connected triangles
    //
    // These are not only the triangles that have this spring as an edge;
    // they also include triangles that have this spring as traverse (i.e.
    // the non-edge diagonal of a two-triangle square)
    //

    if (destroyAllTriangles)
    {
        // We destroy all triangles connected to each endpoint
        DestroyConnectedTriangles(pointAIndex, currentSimulationTime, simulationParameters);
        DestroyConnectedTriangles(pointBIndex, currentSimulationTime, simulationParameters);
    }
    else
    {
        // We destroy only triangles connected to both endpoints
        DestroyConnectedTriangles(pointAIndex, pointBIndex, currentSimulationTime, simulationParameters);
    }


    //
    // Damage both endpoints
    //  - They'll start leaking if they're not hull, among other things
    //

    mPoints.Damage(pointAIndex, currentSimulationTime, simulationParameters);
    mPoints.Damage(pointBIndex, currentSimulationTime, simulationParameters);


    //
    // If endpoints are electrical elements connected to each other, then
    // disconnect them from each other - i.e. remove them from each other's
    // set of connected electrical elements
    //

    if (auto const electricalElementAIndex = mPoints.GetElectricalElement(pointAIndex);
        NoneElementIndex != electricalElementAIndex)
    {
        if (auto electricalElementBIndex = mPoints.GetElectricalElement(pointBIndex);
            NoneElementIndex != electricalElementBIndex)
        {
            if (mElectricalElements.AreConnected(electricalElementAIndex, electricalElementBIndex))
            {
                mElectricalElements.RemoveConnectedElectricalElement(
                    electricalElementAIndex,
                    electricalElementBIndex,
                    true /*severed*/);

                mElectricalElements.RemoveConnectedElectricalElement(
                    electricalElementBIndex,
                    electricalElementAIndex,
                    true /*severed*/);
            }
        }
    }

    //
    // Misc
    //

    // Notify gadgets
    mGadgets.OnSpringDestroyed(
        springElementIndex,
        currentSimulationTime,
        simulationParameters);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken springs
    ++mBrokenSpringsCount;
}

void Ship::HandleSpringRestore(
    ElementIndex springElementIndex,
    SimulationParameters const & /*simulationParameters*/)
{
    auto const pointAIndex = mSprings.GetEndpointAIndex(springElementIndex);
    auto const pointBIndex = mSprings.GetEndpointBIndex(springElementIndex);

    //
    // Add others to self
    //

    // Restore factory supertriangles
    mSprings.RestoreFactorySuperTriangles(springElementIndex);

    //
    // Add self to others
    //

    // Connect self to endpoints
    mPoints.ConnectSpring(pointAIndex, springElementIndex, pointBIndex);
    mPoints.ConnectSpring(pointBIndex, springElementIndex, pointAIndex);

    //
    // If both endpoints are electrical elements, and neither is deleted,
    // then connect them - i.e. add them to each other's set of connected electrical elements
    //

    auto electricalElementAIndex = mPoints.GetElectricalElement(pointAIndex);
    if (NoneElementIndex != electricalElementAIndex
        && !mElectricalElements.IsDeleted(electricalElementAIndex))
    {
        auto electricalElementBIndex = mPoints.GetElectricalElement(pointBIndex);
        if (NoneElementIndex != electricalElementBIndex
            && !mElectricalElements.IsDeleted(electricalElementBIndex))
        {
            mElectricalElements.AddConnectedElectricalElement(
                electricalElementAIndex,
                electricalElementBIndex);

            mElectricalElements.AddConnectedElectricalElement(
                electricalElementBIndex,
                electricalElementAIndex);
        }
    }

    //
    // Misc
    //

    // Fire event - using point A's properties (quite arbitrarily)
    auto const endpointAIndex = mSprings.GetEndpointAIndex(springElementIndex);
    mSimulationEventHandler.OnSpringRepaired(
        mPoints.GetStructuralMaterial(endpointAIndex),
        mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(endpointAIndex)),
        1);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken springs
    assert(mBrokenSpringsCount > 0);
    --mBrokenSpringsCount;

    // Notify if we've just completely restored the ship
    if (mDamagedPointsCount == 0 && mBrokenSpringsCount == 0 && mBrokenTrianglesCount == 0)
    {
        mSimulationEventHandler.OnShipRepaired(mId);
    }
}

void Ship::HandleTriangleDestroy(
    ElementIndex triangleElementIndex,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    //
    // Remove triangle from other elements
    //

    // Remove triangle from sets of super triangles of its sub springs
    for (ElementIndex const subSpringIndex : mTriangles.GetSubSprings(triangleElementIndex).SpringIndices)
    {
        mSprings.RemoveSuperTriangle(subSpringIndex, triangleElementIndex);
    }

    // Decrement count of covering triangles of each covered spring
    for (ElementIndex const coveredSpringIndex : mTriangles.GetCoveredSprings(triangleElementIndex))
    {
        mSprings.RemoveCoveringTriangle(coveredSpringIndex);
    }

    // Disconnect triangle from its endpoints, and marking the points as damaged
    mPoints.DisconnectTriangle(mTriangles.GetPointAIndex(triangleElementIndex), triangleElementIndex, true); // Owner
    mPoints.Damage(mTriangles.GetPointAIndex(triangleElementIndex), currentSimulationTime, simulationParameters);
    mPoints.DisconnectTriangle(mTriangles.GetPointBIndex(triangleElementIndex), triangleElementIndex, false); // Not owner
    mPoints.Damage(mTriangles.GetPointBIndex(triangleElementIndex), currentSimulationTime, simulationParameters);
    mPoints.DisconnectTriangle(mTriangles.GetPointCIndex(triangleElementIndex), triangleElementIndex, false); // Not owner
    mPoints.Damage(mTriangles.GetPointCIndex(triangleElementIndex), currentSimulationTime, simulationParameters);

    //
    // Maintain frontier
    //
    // Must be invoked here, and not earlier, as the springs are expected to be
    // already consistent with the removal of the triangle.
    //

    mFrontiers.HandleTriangleDestroy(
        triangleElementIndex,
        mPoints,
        mSprings,
        mTriangles);

    /////////////////////////////////////////////////////////

    // Notify NPCs
    mParentWorld.GetNpcs().OnShipTriangleDestroyed(
        mId,
        triangleElementIndex);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken triangles
    ++mBrokenTrianglesCount;
}

void Ship::HandleTriangleRestore(ElementIndex triangleElementIndex)
{
    //
    // Maintain frontier
    //

    mFrontiers.HandleTriangleRestore(
        triangleElementIndex,
        mPoints,
        mSprings,
        mTriangles);

    //
    // Add self to others
    //

    // Connect triangle to its endpoints
    mPoints.ConnectTriangle(mTriangles.GetPointAIndex(triangleElementIndex), triangleElementIndex, true); // Owner
    mPoints.ConnectTriangle(mTriangles.GetPointBIndex(triangleElementIndex), triangleElementIndex, false); // Not owner
    mPoints.ConnectTriangle(mTriangles.GetPointCIndex(triangleElementIndex), triangleElementIndex, false); // Not owner

    // Increment count of covering triangles for each of the covered springs
    for (ElementIndex const coveredSpringIndex : mTriangles.GetCoveredSprings(triangleElementIndex))
    {
        mSprings.AddCoveringTriangle(coveredSpringIndex);
    }

    // Add triangle to set of super triangles of each of its sub springs
    assert(mTriangles.GetSubSprings(triangleElementIndex).SpringIndices.size() == 3);
    for (ElementIndex subSpringIndex : mTriangles.GetSubSprings(triangleElementIndex).SpringIndices)
    {
        mSprings.AddSuperTriangle(subSpringIndex, triangleElementIndex);
    }

    /////////////////////////////////////////////////////////

    // Fire event - using point A's properties (quite arbitrarily)
    auto const endpointAIndex = mTriangles.GetPointAIndex(triangleElementIndex);
    mSimulationEventHandler.OnTriangleRepaired(
        mPoints.GetStructuralMaterial(endpointAIndex),
        mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(endpointAIndex)),
        1);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken triangles
    assert(mBrokenTrianglesCount > 0);
    --mBrokenTrianglesCount;

    // Notify if we've just completely restored the ship
    if (mDamagedPointsCount == 0 && mBrokenSpringsCount == 0 && mBrokenTrianglesCount == 0)
    {
        mSimulationEventHandler.OnShipRepaired(mId);
    }
}

void Ship::HandleElectricalElementDestroy(
    ElementIndex electricalElementIndex,
    ElementIndex pointElementIndex,
    ElectricalElementDestroySpecializationType specialization,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    //
    // For all of the connected electrical elements: remove electrical connections
    // (when should have one)
    //

    while (!mElectricalElements.GetConnectedElectricalElements(electricalElementIndex).empty())
    {
        auto const connectedElectricalElementIndex =
            *(mElectricalElements.GetConnectedElectricalElements(electricalElementIndex).begin());

        mElectricalElements.RemoveConnectedElectricalElement(
            electricalElementIndex,
            connectedElectricalElementIndex,
            true /*severed*/);

        mElectricalElements.RemoveConnectedElectricalElement(
            connectedElectricalElementIndex,
            electricalElementIndex,
            true /*severed*/);
    }

    //
    // Address specialization
    //

    switch (specialization)
    {
        case ElectricalElementDestroySpecializationType::Lamp:
        {
            mSimulationEventHandler.OnLampBroken(
                mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex)),
                1);

            break;
        }

        case ElectricalElementDestroySpecializationType::LampExplosion:
        {
            InternalSpawnDebris(
                pointElementIndex,
                mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Glass),
                currentSimulationTime,
                simulationParameters);

            mSimulationEventHandler.OnLampExploded(
                mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex)),
                1);

            break;
        }

        case ElectricalElementDestroySpecializationType::LampImplosion:
        {
            mSimulationEventHandler.OnLampImploded(
                mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex)),
                1);

            break;
        }

        case ElectricalElementDestroySpecializationType::SilentRemoval:
        case ElectricalElementDestroySpecializationType::None:
        {
            // Nothing else
            break;
        }
    }
}

void Ship::HandleElectricalElementRestore(ElementIndex electricalElementIndex)
{
    //
    // For all of the connected springs: restore electrical connections if eligible
    //

    assert(!mElectricalElements.IsDeleted(electricalElementIndex));

    auto const pointIndex = mElectricalElements.GetPointIndex(electricalElementIndex);
    for (auto const & connected : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
    {
        auto otherElectricalElementIndex = mPoints.GetElectricalElement(connected.OtherEndpointIndex);
        if (NoneElementIndex != otherElectricalElementIndex
            && !mElectricalElements.IsDeleted(otherElectricalElementIndex))
        {
            mElectricalElements.AddConnectedElectricalElement(
                electricalElementIndex,
                otherElectricalElementIndex);

            mElectricalElements.AddConnectedElectricalElement(
                otherElectricalElementIndex,
                electricalElementIndex);
        }
    }
}

void Ship::StartExplosion(
    float currentSimulationTime,
    PlaneId planeId,
    vec2f const & centerPosition,
    float blastForce,
    float blastForceRadius,
    float blastHeat,
    float blastHeatRadius,
    float renderRadiusOffset,
    ExplosionType explosionType,
    SimulationParameters const & /*simulationParameters*/)
{
    // Queue state machine
    mStateMachines.push_back(
        std::make_unique<ExplosionStateMachine>(
            currentSimulationTime,
            planeId,
            centerPosition,
            blastForce,
            blastForceRadius,
            blastHeat,
            blastHeatRadius,
            renderRadiusOffset,
            explosionType));
}

void Ship::DoAntiMatterBombPreimplosion(
    vec2f const & centerPosition,
    float /*sequenceProgress*/,
    float radius,
    SimulationParameters const & simulationParameters)
{
    float constexpr RadiusThickness = 10.0f; // Thickness of radius, magic number

    // Apply the force field
    {
        float const strength =
            130000.0f // Magic number
            * (simulationParameters.IsUltraViolentMode ? 5.0f : 1.0f);

        for (auto pointIndex : mPoints)
        {
            vec2f const pointRadius = mPoints.GetPosition(pointIndex) - centerPosition;
            float const pointDistanceFromRadius = pointRadius.length() - radius;
            float const absolutePointDistanceFromRadius = std::abs(pointDistanceFromRadius);
            if (absolutePointDistanceFromRadius <= RadiusThickness)
            {
                float const forceDirection = pointDistanceFromRadius >= 0.0f ? 1.0f : -1.0f;

                float const forceStrength = strength * (1.0f - absolutePointDistanceFromRadius / RadiusThickness);

                mPoints.AddStaticForce(
                    pointIndex,
                    pointRadius.normalise() * forceStrength * forceDirection);
            }
        }
    }

    // Also apply to NPCs
    mParentWorld.GetNpcs().ApplyAntiMatterBombPreimplosion(
        mId,
        centerPosition,
        radius,
        RadiusThickness,
        simulationParameters);

    // Scare fishes
    mParentWorld.DisturbOceanAt(
        centerPosition,
        radius,
        std::chrono::milliseconds(0));
}

void Ship::DoAntiMatterBombImplosion(
    vec2f const & centerPosition,
    float sequenceProgress,
    SimulationParameters const & simulationParameters)
{
    // Apply the force field
    {
        float const strength =
            (sequenceProgress * sequenceProgress)
            * simulationParameters.AntiMatterBombImplosionStrength
            * 10000.0f // Magic number
            * (simulationParameters.IsUltraViolentMode ? 50.0f : 1.0f);

        for (auto pointIndex : mPoints)
        {
            vec2f displacement = centerPosition - mPoints.GetPosition(pointIndex);
            float const displacementLength = displacement.length();
            vec2f normalizedDisplacement = displacement.normalise(displacementLength);

            // Make final acceleration somewhat independent from mass
            float const massNormalization = mPoints.GetMass(pointIndex) / 50.0f;

            // Angular (constant)
            mPoints.AddStaticForce(
                pointIndex,
                vec2f(-normalizedDisplacement.y, normalizedDisplacement.x)
                * strength
                * massNormalization
                / 10.0f); // Magic number

            // Radial (stronger when closer)
            mPoints.AddStaticForce(
                pointIndex,
                normalizedDisplacement
                * strength
                / (0.2f + 0.5f * sqrt(displacementLength))
                * massNormalization
                * 10.0f); // Magic number
        }
    }

    // Also apply to NPCs
    mParentWorld.GetNpcs().ApplyAntiMatterBombImplosion(
        mId,
        centerPosition,
        sequenceProgress,
        simulationParameters);
}

void Ship::DoAntiMatterBombExplosion(
    vec2f const & centerPosition,
    float sequenceProgress,
    SimulationParameters const & simulationParameters)
{
    //
    // Single explosion peak at progress=0.0
    //

    if (0.0f == sequenceProgress)
    {
        // Apply the force field
        {
            //
            // F = ForceStrength/sqrt(distance), along radius
            //

            float const strength =
                30000.0f // Magic number
                * (simulationParameters.IsUltraViolentMode ? 50.0f : 1.0f);

            for (auto pointIndex : mPoints)
            {
                vec2f displacement = mPoints.GetPosition(pointIndex) - centerPosition;
                float forceMagnitude = strength / sqrtf(0.1f + displacement.length());

                mPoints.AddStaticForce(
                    pointIndex,
                    displacement.normalise() * forceMagnitude);
            }
        }

        // Also apply to NPCs
        mParentWorld.GetNpcs().ApplyAntiMatterBombExplosion(
            mId,
            centerPosition,
            simulationParameters);

        // Scare fishes
        mParentWorld.DisturbOceanAt(
            centerPosition,
            300.0f, // Magic radius
            std::chrono::milliseconds(0));
    }
}

void Ship::HandleWatertightDoorUpdated(
    ElementIndex pointElementIndex,
    bool isOpen)
{
    // Update point and springs
    bool const isHull = !isOpen;
    SetAndPropagateResultantPointHullness(pointElementIndex, isHull);

    if (!isOpen)
    {
        //
        // Open->Close transition
        //

        // Dry up point
        mPoints.SetWater(pointElementIndex, 0.0f);

        // Fire event
        mSimulationEventHandler.OnWatertightDoorClosed(
            mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex)),
            1);
    }
    else
    {
        //
        // Close->Open transition
        //

        // Fire event
        mSimulationEventHandler.OnWatertightDoorOpened(
            mParentWorld.GetOceanSurface().IsUnderwater(mPoints.GetPosition(pointElementIndex)),
            1);
    }
}

void Ship::HandleElectricSpark(
    ElementIndex pointElementIndex,
    float strength,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    assert(!mPoints.IsEphemeral(pointElementIndex));

    //
    // Electrification
    //

    mPoints.SetIsElectrified(pointElementIndex, (strength > 0.0f));

    //
    // Heat
    //

    float const heat =
        10.0f * 1000.0f // KJoule->Joule
        * strength
        * (simulationParameters.IsUltraViolentMode ? 15.0f : 1.0f);

    // Calc temperature delta
    // T = Q/HeatCapacity
    float const deltaT =
        heat
        * mPoints.GetMaterialHeatCapacityReciprocal(pointElementIndex);

    // Increase/lower temperature
    mPoints.SetTemperature(
        pointElementIndex,
        std::max(mPoints.GetTemperature(pointElementIndex) + deltaT, 0.1f)); // 3rd principle of thermodynamics

    //
    // Rotting
    //

    float const rotCoefficient =
        (simulationParameters.IsUltraViolentMode ? 0.99f : 0.9995f)
        + (1.0f - strength) * 0.0003f;

    mPoints.SetRot(
        pointElementIndex,
        mPoints.GetRot(pointElementIndex) * rotCoefficient);

    //
    // Electrical elements
    //

    auto const electricalElementIndex = mPoints.GetElectricalElement(pointElementIndex);
    if (NoneElementIndex != electricalElementIndex)
    {
        mElectricalElements.OnElectricSpark(
            electricalElementIndex,
            currentSimulationTime,
            simulationParameters);
    }

    //
    // Gadgets
    //

    mGadgets.OnElectricSpark(
        pointElementIndex,
        currentSimulationTime,
        simulationParameters);
}

#ifdef _DEBUG
void Ship::VerifyInvariants()
{
    //
    // Points
    //

    for (auto p : mPoints)
    {
        auto const & pos = mPoints.GetPosition(p);
        Verify(pos.x >= -SimulationParameters::HalfMaxWorldWidth && pos.x <= SimulationParameters::HalfMaxWorldWidth);
        Verify(pos.y >= -SimulationParameters::HalfMaxWorldHeight && pos.y <= SimulationParameters::HalfMaxWorldHeight);
    }

    //
    // Triangles and points
    //

    for (auto t : mTriangles)
    {
        if (!mTriangles.IsDeleted(t))
        {
            Verify(mPoints.GetConnectedTriangles(mTriangles.GetPointAIndex(t)).ConnectedTriangles.contains([t](auto const & c) { return c == t; }));
            Verify(mPoints.GetConnectedTriangles(mTriangles.GetPointBIndex(t)).ConnectedTriangles.contains([t](auto const & c) { return c == t; }));
            Verify(mPoints.GetConnectedTriangles(mTriangles.GetPointCIndex(t)).ConnectedTriangles.contains([t](auto const & c) { return c == t; }));
        }
        else
        {
            Verify(!mPoints.GetConnectedTriangles(mTriangles.GetPointAIndex(t)).ConnectedTriangles.contains([t](auto const & c) { return c == t; }));
            Verify(!mPoints.GetConnectedTriangles(mTriangles.GetPointBIndex(t)).ConnectedTriangles.contains([t](auto const & c) { return c == t; }));
            Verify(!mPoints.GetConnectedTriangles(mTriangles.GetPointCIndex(t)).ConnectedTriangles.contains([t](auto const & c) { return c == t; }));
        }
    }


    //
    // Springs and points
    //

    for (auto s : mSprings)
    {
        if (!mSprings.IsDeleted(s))
        {
            Verify(mPoints.GetConnectedSprings(mSprings.GetEndpointAIndex(s)).ConnectedSprings.contains([s](auto const & c) { return c.SpringIndex == s; }));
            Verify(mPoints.GetConnectedSprings(mSprings.GetEndpointBIndex(s)).ConnectedSprings.contains([s](auto const & c) { return c.SpringIndex == s; }));
        }
        else
        {
            Verify(!mPoints.GetConnectedSprings(mSprings.GetEndpointAIndex(s)).ConnectedSprings.contains([s](auto const & c) { return c.SpringIndex == s; }));
            Verify(!mPoints.GetConnectedSprings(mSprings.GetEndpointBIndex(s)).ConnectedSprings.contains([s](auto const & c) { return c.SpringIndex == s; }));
        }
    }


    //
    // SuperTriangles and SubSprings
    //

    for (auto s : mSprings)
    {
        if (!mSprings.IsDeleted(s))
        {
            Verify(mSprings.GetSuperTriangles(s).size() <= 2);

            for (auto superTriangle : mSprings.GetSuperTriangles(s))
            {
                Verify(
                    mTriangles.GetSubSprings(superTriangle).SpringIndices[0] == s
                    || mTriangles.GetSubSprings(superTriangle).SpringIndices[1] == s
                    || mTriangles.GetSubSprings(superTriangle).SpringIndices[2] == s);
            }
        }
        else
        {
            Verify(mSprings.GetSuperTriangles(s).empty());
        }
    }

    for (auto t : mTriangles)
    {
        Verify(mTriangles.GetSubSprings(t).SpringIndices.size() == 3);

        if (!mTriangles.IsDeleted(t))
        {
            for (auto subSpring : mTriangles.GetSubSprings(t).SpringIndices)
            {
                Verify(mSprings.GetSuperTriangles(subSpring).contains(t));
            }
        }
        else
        {
            for (auto subSpring : mTriangles.GetSubSprings(t).SpringIndices)
            {
                Verify(!mSprings.GetSuperTriangles(subSpring).contains(t));
            }
        }
    }


    //
    // Frontiers
    //

    mFrontiers.VerifyInvariants(
        mPoints,
        mSprings,
        mTriangles);
}
#endif
}