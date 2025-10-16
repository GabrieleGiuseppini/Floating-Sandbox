﻿/***************************************************************************************
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
#include <array>
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
// RotPoints x 4
// SpringDecayAndTemperature x 4
// UpdateSinking x 1

static int constexpr CombustionStateMachineSlowStep1 = 2;
static int constexpr SpringDecayAndTemperatureStep1 = 5;
static int constexpr RotPointsStep1 = 8;
static int constexpr CombustionStateMachineSlowStep2 = 11;
static int constexpr SpringDecayAndTemperatureStep2 = 14;
static int constexpr RotPointsStep2 = 17;
static int constexpr UpdateSinkingStep = 18;
static int constexpr CombustionStateMachineSlowStep3 = 20;
static int constexpr SpringDecayAndTemperatureStep3 = 23;
static int constexpr RotPointsStep3 = 26;
static int constexpr CombustionStateMachineSlowStep4 = 29;
static int constexpr SpringDecayAndTemperatureStep4 = 32;
static int constexpr RotPointsStep4 = 35;

static_assert(RotPointsStep4 < SimulationParameters::ParticleUpdateLowFrequencyPeriod);

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
    , mLastLuminiscenceAdjustmentDiffused(-1.0f)
    , mRepairGracePeriodMultiplier(1.0f)
    , mLastQueriedPointIndex(NoneElementIndex)
    , mAirBubblesCreatedCount(0)
    , mCurrentSimulationParallelism(0) // We'll detect a difference on first run
    , mCurrentSpringRelaxationParallelComputationMode() // We'll detect a difference on first run
    // Static pressure
    , mStaticPressureBuffer(mPoints.GetAlignedShipPointCount())
    , mStaticPressureNetForceMagnitudeSum(0.0f)
    , mStaticPressureNetForceMagnitudeCount(0.0f)
    , mStaticPressureIterationsPercentagesSum(0.0f)
    , mStaticPressureIterationsCount(0.0f)
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
        simulationParameters,
        threadManager);

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
    // Reset static forces, now that we have integrated them
    ///////////////////////////////////////////////////////////////////

    mPoints.ResetStaticForces();

    ///////////////////////////////////////////////////////////////////
    // Apply interaction forces that have been queued before this
    // step
    ///////////////////////////////////////////////////////////////////

    ApplyQueuedInteractionForces(simulationParameters);

    ///////////////////////////////////////////////////////////////////
    // Apply world forces
    //
    // Also calculates cached depths, and updates frontiers' AABBs and
    // geometric centers - hence needs to come _after _ UpdateForStrains()
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    ApplyWorldForces(
        effectiveAirDensity,
        effectiveWaterDensity,
        simulationParameters,
        externalAabbSet);

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedWorldForces = GameChronometer::Now() - startTimestamp1;
#endif

    // Cached depths are valid from now on --------------------------->

    ///////////////////////////////////////////////////////////////////
    // Rot points
    ///////////////////////////////////////////////////////////////////

#ifdef FS_PROFILE_SHIP_UPDATE
    startTimestamp1 = GameChronometer::Now();
#endif

    // - Inputs: Position, Water, IsLeaking
    // - Output: Decay

    if (mCurrentSimulationSequenceNumber.IsStepOf(RotPointsStep1, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        RotPoints(
            0, 4,
            currentSimulationTime,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(RotPointsStep2, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        RotPoints(
            1, 4,
            currentSimulationTime,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(RotPointsStep3, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        RotPoints(
            2, 4,
            currentSimulationTime,
            simulationParameters);
    }
    else if (mCurrentSimulationSequenceNumber.IsStepOf(RotPointsStep4, SimulationParameters::ParticleUpdateLowFrequencyPeriod))
    {
        RotPoints(
            3, 4,
            currentSimulationTime,
            simulationParameters);
    }

#ifdef FS_PROFILE_SHIP_UPDATE
    auto const elapsedRotPoints = GameChronometer::Now() - startTimestamp1;
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
    // Update state machines
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
            UpdateWaterVelocities(simulationParameters, waterSplashedInStep);

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
    static std::chrono::microseconds rotPointsTotal{0};
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
    rotPointsTotal += std::chrono::duration_cast<std::chrono::microseconds>(elapsedRotPoints);
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
                   " rotPoints=", rotPointsTotal.count() / profilingFrameCounter / 1000.0f,
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
        rotPointsTotal = std::chrono::microseconds(0);
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
        }
    }

    mQueuedInteractions.clear();
}

void Ship::ApplyWorldForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
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
        ApplyWorldSurfaceForces<true>(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, simulationParameters, externalAabbSet);
    else
        ApplyWorldSurfaceForces<false>(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, simulationParameters, externalAabbSet);

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

    for (auto pointIndex : mPoints.BufferElements())
    {
        vec2f staticForce = vec2f::zero();

        //
        // Calculate and store depth
        //

        newCachedPointDepthsBuffer[pointIndex] = oceanSurface.GetDepth(mPoints.GetPosition(pointIndex));

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

        staticForcesBuffer[pointIndex] += staticForce;
    }

    //
    // 2. Radial wind field, if any
    //

    auto const & radialWindField = mParentWorld.GetCurrentRadialWindField();
    if (radialWindField.has_value())
    {
        for (auto pointIndex : mPoints.BufferElements())
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
    // Water displacement constants
    //

    float constexpr wdmX0 = 2.0f; // Vertical velocity at which displacement transitions from quadratic to linear
    float constexpr wdmY0 = 0.16f; // Displacement magnitude at x0

    // Linear portion
    float const wdmLinearSlope =
        SimulationParameters::SimulationStepTimeDuration<float> * 6.0f // Magic number
        * simulationParameters.WaterDisplacementWaveHeightAdjustment;

    // Quadratic portion: y = ax^2 + bx, with constraints:
    //  y(0) = 0
    //  y'(x0) = slope
    //  y(x0) = y0
    float const wdmQuadraticA = (wdmLinearSlope * wdmX0 - wdmY0) / (wdmX0 * wdmX0);
    float const wdmQuadraticB = 2.0f * wdmY0 / wdmX0 - wdmLinearSlope;

    //
    // Visit all frontiers
    //

    for (FrontierId frontierId : mFrontiers.GetFrontierIds())
    {
        // Initialize AABB and geometric center
        Geometry::ShipAABB aabb;
        vec2f geometricCenter = vec2f::zero();

        auto & frontier = mFrontiers.GetFrontier(frontierId);

        // We only apply velocity drag and displace water for *external* frontiers,
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

            ElementIndex const visitStartEdgeIndex = thisFrontierEdge.NextEdgeIndex;

            for (ElementIndex nextEdgeIndex = visitStartEdgeIndex; /*checked in loop*/; /*advanced in loop*/)
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

                // Normal to surface - calculated between p1 and p3; points outside
                vec2f const surfaceNormal = (nextPointPosition - previousPointPosition).normalise().to_perpendicular();

                // Velocity along normal - capped to the same direction as velocity, to avoid suction force
                // (i.e. drag force attracting surface facing opposite of velocity)
                float const velocityMagnitudeAlongNormal = std::max(
                    mPoints.GetVelocity(thisPointIndex).dot(surfaceNormal),
                    0.0f);

                // Max drag force magnitude: m * (V dot Nn) / dt
                float const maxDragForceMagnitude =
                    mPoints.GetMass(thisPointIndex) * velocityMagnitudeAlongNormal
                    / SimulationParameters::SimulationStepTimeDuration<float>;

                // Calculate drag coefficient: air or water, with soft transition
                // to avoid discontinuities in drag force close to the air-water interface
                float const dragCoefficient = Mix(
                    airPressureDragCoefficient,
                    waterPressureDragCoefficient,
                    Clamp(thisPointDepth, 0.0f, 1.0f));

                // Calculate magnitude of drag force (opposite sign), capped by max drag force
                //  - C * |V| * cos(a) == - C * |V| * (Vn dot Nn) == -C * (V dot Nn)
                float const dragForceMagnitude = std::min(
                    dragCoefficient * velocityMagnitudeAlongNormal,
                    maxDragForceMagnitude);

                //
                // Impact force
                //
                // Impact force is proportional to kinetic energy, and we only apply it
                // when there's a discontinuity in the "underwaterness" of a frontier
                // particle, i.e. when this is the first frame in which the particle
                // gets underwater.
                //

                float const kineticEnergy =
                    velocityMagnitudeAlongNormal * velocityMagnitudeAlongNormal
                    * mPoints.GetMass(thisPointIndex);

                float const waterImpactForceMagnitude =
                    kineticEnergy
                    * waterImpactForceCoefficient
                    * Step(mPoints.GetCachedDepth(thisPointIndex), 0.0f) * Step(0.0f, newCachedPointDepths[thisPointIndex]);

                //
                // Apply drag and impact forces
                //

                mPoints.AddStaticForce(
                    thisPointIndex,
                    -surfaceNormal * (dragForceMagnitude + waterImpactForceMagnitude));

                //
                // Water displacement
                //
                // * The magnitude of water displacement is proportional to the square root of
                //   the kinetic energy of the particle, thus it is proportional to the square
                //   root of the particle mass, and linearly to the particle's velocity
                //      * However, in order to generate visible waves also for very small velocities,
                //        we want the contribution of small velocities to be more than linear wrt
                //        the contribution of higher velocities, and so we'll be using a piecewise
                //        function: quadratic for small velocities, and linear for higher
                // * The deeper the particle is, the less it contributes to displacement
                //

                if constexpr (DoDisplaceWater)
                {
                    // Calculate vertical velocity, clamping it to a maximum to prevent
                    // ocean surface instabilities with extremely high velocities
                    float const verticalVelocity = mPoints.GetVelocity(thisPointIndex).y;
                    float const absVerticalVelocity = std::min(
                        std::abs(verticalVelocity),
                        10000.0f); // Magic number

                    //
                    // Displacement magnitude calculation
                    //

                    float const linearDisplacementMagnitude = wdmY0 + wdmLinearSlope * (absVerticalVelocity - wdmX0);
                    float const quadraticDisplacementMagnitude = wdmQuadraticA * absVerticalVelocity * absVerticalVelocity + wdmQuadraticB * absVerticalVelocity;

                    //
                    // Depth attenuation: tapers down displacement the deeper the point is
                    //

                    // Depth at which the point stops contributing: rises quadratically, asymptotically, and asymmetric wrt sinking or rising
                    float constexpr MaxVel = 35.0f;
                    float constexpr a2 = -0.5f / (MaxVel * MaxVel);
                    float constexpr b2 = 1.0f / MaxVel;
                    float const clampedAbsVerticalVelocity = std::min(absVerticalVelocity, MaxVel);
                    float const maxDepth =
                        (a2 * clampedAbsVerticalVelocity * clampedAbsVerticalVelocity + b2 * clampedAbsVerticalVelocity + 0.5f)
                        * (verticalVelocity <= 0.0f ? 12.0f : 4.0f); // Keep up-push low or else bodies keep jumping up and down forever

                    // Linear attenuation up to maxDepth
                    float const depthAttenuation = 1.0f - LinearStep(0.0f, maxDepth, thisPointDepth); // Tapers down contribution the deeper the point is

                    //
                    // Displacement
                    //

                    float const displacement =
                        (absVerticalVelocity < wdmX0 ? quadraticDisplacementMagnitude : linearDisplacementMagnitude)
                        * depthAttenuation
                        * SignStep(0.0f, verticalVelocity) // Displacement has same sign as vertical velocity
                        * Step(0.0f, thisPointDepth) // No displacement for above-water points
                        * 0.4f; // Magic number

                    mParentWorld.DisplaceOceanSurfaceAt(thisPointPosition.x, displacement);

                    totalWaterDisplacementMagnitude += std::abs(displacement);
                }

                //
                // Advance edge in the frontier visit
                //

                nextEdgeIndex = nextFrontierEdge.NextEdgeIndex;
                if (nextEdgeIndex == visitStartEdgeIndex)
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
        mPoints.GetDynamicForceBufferAsVec2(),
        mPoints.GetDynamicForceBufferAsVec2() + mPoints.GetElementCount(),
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
    // so to not produce buoyancy forces), Mw * G is the weight of water, and
    // |Ei| accounts for wider edges being subject to more pressure.
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
    //
    // Notes:
    //  - We use the frontiers' gemetric centers as the place that depth is calculated at;
    //    as a consequence, if the ship is interactively moved or rotated, the centers
    //    that we use here are stale. Not a big deal...
    //    Outside of these "moving" interactions, the centers we use here are also
    //    inconsistent with the current positions because of integration during dynamic
    //    iterations, unless hydrostatic pressures are calculated on the *first* dynamic
    //    iteration.
    //

    vec2f const & geometricCenterPosition = frontier.GeometricCenterPosition;
    float const oceanSurfaceY = mParentWorld.GetOceanSurface().GetHeightAt(geometricCenterPosition.x);
    float const depth = oceanSurfaceY - geometricCenterPosition.y;

    float const totalExternalPressure = Formulae::CalculateTotalPressureAt(
        geometricCenterPosition.y,
        oceanSurfaceY,
        effectiveAirDensity,
        effectiveWaterDensity,
        simulationParameters);

    assert(totalExternalPressure != 0.0f); // Air pressure is never zero

    // Counterbalance adjustment: a "trick" to reduce the effect of inner pressure on the external pressure
    // applied to the hull, so to generate higher hydrostatic forces.
    // Factor for counterbalance adjustment:
    //  - At adj=0.0, we want the internal pressure to NEVER counterbalance the external pressure as-is
    //  - At adj=0.5, we want the internal pressure to start counterbalancing the external pressure somewhere mid-way along the depth
    //  - At adj=1.0, we want the internal pressure to ALWAYS counterbalance the external pressure
    float const hydrostaticPressureCounterbalanceAdjustmentFactor =
        1.0f / totalExternalPressure
        * (1.0f - SmoothStep(
            SimulationParameters::HalfMaxWorldHeight,
            SimulationParameters::HalfMaxWorldHeight * 2.0f,
            depth + (1.0f - simulationParameters.HydrostaticPressureCounterbalanceAdjustment) * SimulationParameters::HalfMaxWorldHeight * 2.0f) * Step(0.0f, depth));

    //
    // 1. Calculate geometry of forces and populate interim buffer
    //
    // Here we calculate the *perpendicular* to each edge, rather than the normal, in order
    // to take into account the length of the edge, as the pressure force on an edge is
    // proportional to its length
    //

    mStaticPressureBuffer.clear();

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

    ElementIndex edge2Index = mFrontiers.GetFrontierEdge(edge1Index).NextEdgeIndex;
    ElementIndex thisPointIndex = mFrontiers.GetFrontierEdge(edge2Index).PointAIndex;

    vec2f edge1PerpVector =
        -(mPoints.GetPosition(thisPointIndex) - mPoints.GetPosition(prevPointIndex)).to_perpendicular();

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

        vec2f edge2PerpVector =
            -(mPoints.GetPosition(nextPointIndex) - mPoints.GetPosition(thisPointIndex)).to_perpendicular();

        neighboringHullPointsCount += (mPoints.GetIsHull(nextPointIndex) ? 1 : 0);
        if (neighboringHullPointsCount == 3) // Avoid applying force to one or two isolated hull particles, allows for more stability of wretched wrecks
        {
            // Calculate internal pressure counterbalance: we want the force vector
            // to be zero when internal pressure == external pressure, at 1.0 counterbalance
            float const internalPressureCounterbalanceFactor = 1.0f - mPoints.GetInternalPressure(thisPointIndex) * hydrostaticPressureCounterbalanceAdjustmentFactor;

            vec2f const forceVector = (edge1PerpVector + edge2PerpVector) / 2.0f * internalPressureCounterbalanceFactor;
            vec2f const torqueArm = mPoints.GetPosition(thisPointIndex) - geometricCenterPosition;

            mStaticPressureBuffer.emplace_back(
                thisPointIndex,
                forceVector,
                torqueArm);

            // Update resultant force and torque
            netForce += forceVector;
            netTorque += torqueArm.cross(forceVector);
        }

        // Advance
        nextEdgeIndex = nextEdge.NextEdgeIndex;
        if (nextEdgeIndex == startEdgeIndex)
            break;

        neighboringHullPointsCount -= (mPoints.GetIsHull(prevPointIndex) ? 1 : 0);

        prevPointIndex = thisPointIndex;
        thisPointIndex = nextPointIndex;
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
    // and/or the net torque by getting its force reduced (via "lambda")
    //

    ElementCount iter;
    for (iter = 0; iter < frontier.Size; ++iter)
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

        if (!bestHPIndex.has_value())
        {
            // Couldn't find a minimizer, stop
            break;
        }

        vec2f const thisForce = mStaticPressureBuffer[*bestHPIndex].ForceVector;
        float const thisTorque = mStaticPressureBuffer[*bestHPIndex].TorqueArm.cross(thisForce);

        // Adjust force vector of optimal particle
        mStaticPressureBuffer[*bestHPIndex].ForceVector *= bestLambda;

        // Update net force and torque
        netForce -= thisForce * (1.0f - bestLambda);
        netTorque -= thisTorque * (1.0f - bestLambda);
    }

    // Update stats
    mStaticPressureNetForceMagnitudeSum += netForce.length();
    mStaticPressureNetForceMagnitudeCount += 1.0f;
    mStaticPressureIterationsPercentagesSum += static_cast<float>(iter + 1) / static_cast<float>(frontier.Size);
    mStaticPressureIterationsCount += 1.0f;

    //
    // 3. Apply forces as dynamic forces - so they only apply to current positions,
    //    as these forces are very sensitive to their position, and would generate
    //    phantom forces and torques otherwise
    //

    float const forceMultiplier =
        totalExternalPressure
        * simulationParameters.StaticPressureForceAdjustment
        * mRepairGracePeriodMultiplier; // Static pressure hinders the repair process

    size_t const particleCount = mStaticPressureBuffer.GetCurrentPopulatedSize();
    for (size_t hpi = 0; hpi < particleCount; ++hpi)
    {
        mPoints.AddDynamicForce(
            mStaticPressureBuffer[hpi].PointIndex,
            mStaticPressureBuffer[hpi].ForceVector * forceMultiplier);
    }
}

void Ship::HandleCollisionsWithSeaFloor(
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    SimulationParameters const & simulationParameters)
{
    //
    // Note: this implementation of friction imparts directly displacement and velocity,
    // rather than imparting forces, and is an approximation of real friction in that it's
    // independent from the force against the surface
    //

    float const dt = simulationParameters.MechanicalSimulationStepTimeDuration<float>();

    OceanFloor const & oceanFloor = mParentWorld.GetOceanFloor();

    float const siltingFactor1 = simulationParameters.OceanFloorSiltHardness;
    float const siltingFactor2 = 1.0f - simulationParameters.OceanFloorSiltHardness;

    for (ElementIndex pointIndex = startPointIndex; pointIndex < endPointIndex; ++pointIndex)
    {
        auto const & position = mPoints.GetPosition(pointIndex);

        // Check if point is below the sea floor
        //
        // At this moment the point might be outside of world boundaries,
        // so better clamp its x before sampling ocean floor height
        float const clampedX = Clamp(position.x, -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth);
        auto const [isUnderneathFloor, oceanFloorHeight, integralIndex] = oceanFloor.GetHeightIfUnderneathAt(clampedX, position.y);
        if (isUnderneathFloor)
        {
            // Collision!

            //
            // Calculate post-bounce velocity
            //

            vec2f const pointVelocity = mPoints.GetVelocity(pointIndex);

            // Calculate sea floor anti-normal
            // (positive points down)
            vec2f const seaFloorAntiNormal = -oceanFloor.GetNormalAt(integralIndex);

            // Calculate the component of the point's velocity along the anti-normal,
            // i.e. towards the interior of the floor...
            float const pointVelocityAlongAntiNormal = pointVelocity.dot(seaFloorAntiNormal);

            // ...if negative, it's already pointing outside the floor, hence we leave it as-is
            if (pointVelocityAlongAntiNormal > 0.0f)
            {
                // Decompose point velocity into normal and tangential
                vec2f const normalVelocity = seaFloorAntiNormal * pointVelocityAlongAntiNormal;
                vec2f const tangentialVelocity = pointVelocity - normalVelocity;

                // Calculate normal reponse: Vn' = -e*Vn (e = elasticity, [0.0 - 1.0])
                float const elasticityFactor = mPoints.GetOceanFloorCollisionFactors(pointIndex).ElasticityFactor;
                vec2f const normalResponse =
                    normalVelocity
                    * elasticityFactor; // Already negative

                // Calculate tangential response: Vt' = a*Vt (a = (1.0-friction), [0.0 - 1.0])
                float constexpr KineticThreshold = 2.0f;
                float const frictionFactor = (std::abs(tangentialVelocity.x) > KineticThreshold || std::abs(tangentialVelocity.y) > KineticThreshold)
                    ? mPoints.GetOceanFloorCollisionFactors(pointIndex).KineticFrictionFactor
                    : mPoints.GetOceanFloorCollisionFactors(pointIndex).StaticFrictionFactor;
                vec2f const tangentialResponse =
                    tangentialVelocity
                    * frictionFactor;

                // Calculate floor hardness:
                //  0.0: full silting - i.e. burrowing into floor; also zero accumulation of velocity
                //  1.0: full restore of before-impact position; also full impact response velocity
                // As follows:
                //  Changes from current param (e.g. 0.5) to 1.0 linearly with magnitude of velocity, up to a maximum velocity at which
                //  moment hardness is max/1.0 (simulating mud where you borrow when still and stay still if move)
                float const velocitySquared = pointVelocity.squareLength();
                float constexpr MaxVelocityForSilting = 2.0f; // Empirical - was 10.0 < 1.19
                float const floorHardness = (oceanFloorHeight - position.y < 40.f) // Just make sure won't ever get buried too deep
                    ? siltingFactor1 + siltingFactor2 * LinearStep(0.0f, MaxVelocityForSilting, velocitySquared) // The faster, the less silting
                    : 1.0f;

                assert(floorHardness <= 1.0f);

                //
                // Impart final position and velocity
                //

                // Move point back along its velocity direction (i.e. towards where it was in the previous step,
                // which is guaranteed to be more towards the outside), but not too much - or else springs
                // might start oscillating between the point burrowing down and then bouncing up
                vec2f deltaPosition = pointVelocity * dt * floorHardness;
                float const deltaPositionLength = deltaPosition.length();
                deltaPosition = deltaPosition.normalise_approx(deltaPositionLength) * std::min(deltaPositionLength, 0.01f); // Magic number, empirical
                mPoints.SetPosition(
                    pointIndex,
                    position - deltaPosition);

                // Set velocity to resultant collision velocity
                mPoints.SetVelocity(
                    pointIndex,
                    (normalResponse + tangentialResponse) * floorHardness);
            }
        }
    }
}

void Ship::TrimForWorldBounds(SimulationParameters const & simulationParameters)
{
    float constexpr MaxWorldLeft = -SimulationParameters::HalfMaxWorldWidth;
    float constexpr MaxWorldRight = SimulationParameters::HalfMaxWorldWidth;

    float constexpr MaxWorldTop = SimulationParameters::HalfMaxWorldHeight;
    float constexpr MaxWorldBottom = -SimulationParameters::HalfMaxWorldHeight;

    // Elasticity of the bounce against world boundaries
    //  - We use the ocean floor's elasticity for convenience
    float const elasticity = simulationParameters.OceanFloorElasticityCoefficient * simulationParameters.ElasticityAdjustment;

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
    // Intake/outtake pressure and water into/from all the leaking nodes (structural or forced)
    // that are either underwater or are overwater and taking rain.
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

    float const cumulatedIntakenWaterThresholdForAirBubbles =
        SimulationParameters::AirBubblesDensityToCumulatedIntakenWater(simulationParameters.AirBubblesDensity);

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        // This is one of the few cases in which we prefer branching over calculating
        // for all points, mostly because we expect a tiny fraction of all points to
        // be leaking at any moment
        auto const & pointCompositeLeaking = mPoints.GetLeakingComposite(pointIndex);
        if (pointCompositeLeaking.IsCumulativelyLeaking)
        {
            assert(!mPoints.GetIsHull(pointIndex)); // Hull points are never leaking

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

            float totalDeltaWater = 0.0f;

            if (pointCompositeLeaking.LeakingSources.StructuralLeak != 0.0f)
            {
                //
                // 1. Update water due to structural leaks (holes)
                //

                {
                    //
                    // 1.1) Calculate velocity of incoming water, based off Bernoulli's equation applied to point:
                    //  v**2/2 + p/density = c (assuming y of incoming water does not change along the intake)
                    //      With: p = pressure of water at point = d*wh*g (d = water density, wh = water height in point)
                    //
                    // Considering that at equilibrium we have v=0 and p=external_pressure,
                    // then c=external_pressure/density;
                    // external_pressure is height_of_water_at_y*g*density, then c=height_of_water_at_y*g;
                    // hence, the velocity of water incoming at point p, when the "water height" in the point is already
                    // wh and the external water pressure is d*height_of_water_at_y*g, is:
                    //  v = +/- sqrt(2*g*|height_of_water_at_y-wh|)
                    //

                    float incomingWaterVelocity_Structural;
                    if (externalWaterHeight >= internalWaterHeight)
                    {
                        // Incoming water
                        incomingWaterVelocity_Structural = sqrtf(2.0f * SimulationParameters::GravityMagnitude * (externalWaterHeight - internalWaterHeight));
                    }
                    else
                    {
                        // Outgoing water
                        incomingWaterVelocity_Structural = -sqrtf(2.0f * SimulationParameters::GravityMagnitude * (internalWaterHeight - externalWaterHeight));
                    }

                    //
                    // 1.2) In/Outtake water according to velocity:
                    // - During dt, we move a volume of water Vw equal to A*v*dt; the equivalent change in water
                    //   height is thus Vw/A, i.e. v*dt
                    //

                    float deltaWater_Structural =
                        incomingWaterVelocity_Structural
                        * SimulationParameters::SimulationStepTimeDuration<float>
                        * mPoints.GetMaterialWaterIntake(pointIndex)
                        * simulationParameters.WaterIntakeAdjustment;

                    //
                    // 1.3) Update water
                    //

                    if (deltaWater_Structural < 0.0f)
                    {
                        // Outgoing water

                        // Make sure we don't over-drain the point
                        deltaWater_Structural = std::max(-mPoints.GetWater(pointIndex), deltaWater_Structural);

                        // Honor the water retention of this material
                        deltaWater_Structural *= mPoints.GetMaterialWaterRestitution(pointIndex);
                    }

                    // Adjust water
                    mPoints.SetWater(
                        pointIndex,
                        mPoints.GetWater(pointIndex) + deltaWater_Structural);

                    totalDeltaWater += deltaWater_Structural;
                }

                //
                // 2. Update internal pressure due to structural leaks (holes)
                //    (positive is incoming)
                //
                //    Structural delta pressure is independent from structural delta water
                //

                {
                    float const externalPressure = Formulae::CalculateTotalPressureAt(
                        mPoints.GetPosition(pointIndex).y,
                        mPoints.GetPosition(pointIndex).y + pointDepth, // oceanSurfaceY
                        effectiveAirDensity,
                        effectiveWaterDensity,
                        simulationParameters);

                    mPoints.SetInternalPressure(
                        pointIndex,
                        externalPressure);
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

                totalDeltaWater += deltaWater_Forced;

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

            mPoints.GetCumulatedIntakenWater(pointIndex) += totalDeltaWater;
            if (mPoints.GetCumulatedIntakenWater(pointIndex) > cumulatedIntakenWaterThresholdForAirBubbles)
            {
                // Generate air bubbles - but not on ropes as that looks awful
                if (doGenerateAirBubbles
                    && !mPoints.IsRope(pointIndex))
                {
                    InternalSpawnAirBubble(
                        mPoints.GetPosition(pointIndex),
                        pointDepth,
                        SimulationParameters::ShipAirBubbleFinalScale,
                        mPoints.GetTemperature(pointIndex),
                        currentSimulationTime,
                        mPoints.GetPlaneId(pointIndex),
                        simulationParameters);
                }

                // Consume all cumulated water
                mPoints.GetCumulatedIntakenWater(pointIndex) = 0.0f;
            }

            // Adjust total water taken during this step, but not counting
            // ropes, to prevent "rushing water" sound from playing for
            // ropes, and also to prevent rope-only ships from playing
            // "farewell"
            if (!mPoints.IsRope(pointIndex))
            {
                waterTakenInStep += totalDeltaWater;
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
}

void Ship::UpdateSinking(float currentSimulationTime)
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
            mParentWorld.GetNpcs().OnShipStartedSinking(mId, currentSimulationTime); // Tell NPCs
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

void Ship::RecalculateLightDiffusionParallelism(size_t simulationParallelism)
{
    // Clear threading state
    mLightDiffusionTasks.clear();

    //
    // Given the available simulation parallelism as a constraint (max), calculate
    // the best parallelism for the light diffusion algorithm
    //

    ElementCount const numberOfPoints = mPoints.GetAlignedShipPointCount(); // No real reason to skip ephemerals, other than they're not expected to have light

    ElementCount constexpr PointsPerThread = 1000; // Was 2000 at 19.1

    size_t const lightDiffusionParallelism = std::max(
        std::min(static_cast<size_t>(numberOfPoints) / PointsPerThread, simulationParallelism),
        size_t(1));

    LogMessage("Ship::RecalculateLightDiffusionParallelism: points=", numberOfPoints, " simulationParallelism=", simulationParallelism,
        " lightDiffusionParallelism=", lightDiffusionParallelism);

    //
    // Prepare tasks
    //
    // We want each thread to work on a multiple of our vectorization word size
    //

    assert(numberOfPoints >= static_cast<ElementCount>(lightDiffusionParallelism) * vectorization_float_count<ElementCount>);
    ElementCount const numberOfVecPointsPerThread = numberOfPoints / (static_cast<ElementCount>(lightDiffusionParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex pointStart = 0;
    for (size_t t = 0; t < lightDiffusionParallelism; ++t)
    {
        ElementIndex const pointEnd = (t < lightDiffusionParallelism - 1)
            ? pointStart + numberOfVecPointsPerThread * vectorization_float_count<ElementCount>
            : numberOfPoints;

        assert(((pointEnd - pointStart) % vectorization_float_count<ElementCount>) == 0);

        mLightDiffusionTasks.emplace_back(
            [this, pointStart, pointEnd]()
            {
                Algorithms::DiffuseLight(
                    pointStart,
                    pointEnd,
                    mPoints.GetPositionBufferAsVec2(),
                    mPoints.GetPlaneIdBufferAsPlaneId(),
                    mElectricalElements.GetLampPositionWorkBuffer().data(),
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
        || (simulationParameters.LuminiscenceAdjustment == 0.0f && mLastLuminiscenceAdjustmentDiffused == 0.0f))
    {
        return;
    }

    //
    // 1. Prepare lamp data
    //

    auto & lampPositions = mElectricalElements.GetLampPositionWorkBuffer(); // Padded to vectorization float count
    auto & lampPlaneIds = mElectricalElements.GetLampPlaneIdWorkBuffer(); // Padded to vectorization float count
    auto & lampDistanceCoeffs = mElectricalElements.GetLampDistanceCoefficientWorkBuffer(); // Padded to vectorization float count

    auto const lampCount = mElectricalElements.GetLampCount();
    for (ElementIndex l = 0; l < lampCount; ++l)
    {
        auto const lampElectricalElementIndex = mElectricalElements.Lamps()[l];
        auto const lampPointIndex = mElectricalElements.GetPointIndex(lampElectricalElementIndex);

        lampPositions[l] = mPoints.GetPosition(lampPointIndex);
        lampPlaneIds[l] = mPoints.GetPlaneId(lampPointIndex);
        lampDistanceCoeffs[l] =
            mElectricalElements.GetLampRawDistanceCoefficient(l)
            * mElectricalElements.GetAvailableLight(lampElectricalElementIndex);
    }

    //
    // 2. Diffuse light
    //

    threadManager.GetSimulationThreadPool().Run(mLightDiffusionTasks);

    // Remember that we've diffused light with this luminiscence adjustment
    mLastLuminiscenceAdjustmentDiffused = simulationParameters.LuminiscenceAdjustment;
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
    float constexpr ThermoclineSlope = -15.0f / SimulationParameters::MaxSeaDepth;

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
            float const waterTemperature = surfaceWaterTemperature - Clamp(mPoints.GetPosition(pointIndex).y * ThermoclineSlope, 0.0f, surfaceWaterTemperature);
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

void Ship::RotPoints(
    ElementIndex partition,
    ElementIndex partitionCount,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
{
    if (simulationParameters.RotAcceler8r == 0.0f)
    {
        // Disable rotting altogether
        return;
    }

    //
    // Rotting is done with a recursive equation:
    //  decay(0) = 1.0
    //  decay(n) = A * decay(n-1), with 0 < A < 1
    //
    // A (alpha): the smaller the alpha, the faster we rot.
    //
    // This converges to:
    //  decay(n) = A^n
    //
    // We want full decay (decay=1e-10) after Nf steps:
    //  ZeroDecay = Af ^ Nf
    //

    //
    // We want to calculate alpha(x) as 1 - beta*x, with x depending on the particle's state:
    //      underwater not flooded: x_uw
    //      not underwater flooded: x_fl == 1.0 (so that we can use particle's water, clamped)
    //      underwater and flooded: x_uw_fl
    //
    // Constraints: after 20 minutes (Ns rot steps) we want the following decays:
    //      underwater not flooded: a_uw ^ Ns = 0.75 (little rusting)
    //      underwater and flooded: a_uw_fl ^ Ns = 0.25 (severe rusting)
    //
    // Which leads to the following formulation for the constraints:
    //      alpha(x_uw) = a_uw (~= 0.99981643)
    //      alpha(x_uw_fl) = a_uw_fl (~- 0.999115711)
    //      alpha(0) = 1.0
    //
    // After some kung-fu we obtain:
    //      beta = (1-alpha(x_uw)) / x_uw
    //      x_uw = (1-a_uw)/(a_uw - a_uw_fl)
    //

    float constexpr Ns = 20.0f * 60.0f / SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;

    float const a_uw = simulationParameters.RotAcceler8r != 0.0f
        ? powf(0.75f, simulationParameters.RotAcceler8r / Ns) // a_uw = 0.75 ^ (1/Ns)
        : 1.0f;

    float const a_uw_fl = simulationParameters.RotAcceler8r != 0.0f
        ? powf(0.25f, simulationParameters.RotAcceler8r / Ns) // a_uw = 0.25 ^ (1/Ns)
        : 1.0f;

    float const x_uw = (1.0f - a_uw) / (a_uw - a_uw_fl);
    float const beta = (1.0f - a_uw) / x_uw;

    // Process all non-ephemeral points in this partition - no real reason
    // to exclude ephemerals, other than they're not expected to rot
    ElementCount const partitionSize = (mPoints.GetRawShipPointCount() / partitionCount) + ((mPoints.GetRawShipPointCount() % partitionCount) ? 1 : 0);
    ElementCount const startPointIndex = partition * partitionSize;
    ElementCount const endPointIndex = std::min(startPointIndex + partitionSize, mPoints.GetRawShipPointCount());
    for (ElementIndex p = startPointIndex; p < endPointIndex; ++p)
    {
        float x =
            (mPoints.IsCachedUnderwater(p) ? x_uw : 0.0f) // x_uw
            + std::min(mPoints.GetWater(p), 1.0f); // x_fl

        // Adjust with leaking: if leaking and subject to rusting, then rusts faster
        x += mPoints.GetLeakingComposite(p).LeakingSources.StructuralLeak * x * x_uw;

        // Adjust with material's rust receptivity
        x *= mPoints.GetMaterialRustReceptivity(p);

        // Calculate alpha
        float const alpha = std::max(1.0f - beta * x, 0.0f);

        // Decay
        mPoints.SetDecay(p, mPoints.GetDecay(p) * alpha);
    }

    // Remember that the decay buffer is dirty
    mPoints.MarkDecayBufferAsDirty();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateForSimulationParameters(
    SimulationParameters const & simulationParameters,
    ThreadManager & threadManager)
{
    size_t const simulationParallelism = threadManager.GetSimulationParallelism();
    if (simulationParallelism != mCurrentSimulationParallelism
        || simulationParameters.SpringRelaxationParallelComputationMode != mCurrentSpringRelaxationParallelComputationMode)
    {
        // Re-calculate spring relaxation parallelism
        RecalculateSpringRelaxationParallelism(simulationParallelism, simulationParameters);

        // Re-calculate light diffusion parallelism
        RecalculateLightDiffusionParallelism(simulationParallelism);

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

    // Remember non-ephemeral portion of plane IDs is dirty
    mPoints.MarkPlaneIdBufferNonEphemeralAsDirty();

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

void Ship::DestroyConnectedTriangles(ElementIndex pointElementIndex)
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
        mTriangles.Destroy(connectedTriangles.back());
    }

    assert(mPoints.GetConnectedTriangles(pointElementIndex).ConnectedTriangles.empty());
}

void Ship::DestroyConnectedTriangles(
    ElementIndex pointAElementIndex,
    ElementIndex pointBElementIndex)
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
                mTriangles.Destroy(triangleIndex);
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
    float currentSimulationTime,
    PlaneId planeId,
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
        auto const pointWater = mPoints.GetWater(sourcePointElementIndex);
        auto const pointPlaneId = mPoints.GetPlaneId(sourcePointElementIndex);

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
                pointWater,
                debrisStructuralMaterial,
                currentSimulationTime,
                maxLifetime,
                pointPlaneId);
        }
    }
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
        DestroyConnectedTriangles(pointAIndex);
        DestroyConnectedTriangles(pointBIndex);
    }
    else
    {
        // We destroy only triangles connected to both endpoints
        DestroyConnectedTriangles(pointAIndex, pointBIndex);
    }


    //
    // Damage both endpoints
    //  - They'll start leaking if they're not hull, among other things
    //

    mPoints.Damage(pointAIndex);
    mPoints.Damage(pointBIndex);


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

void Ship::HandleTriangleDestroy(ElementIndex triangleElementIndex)
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

    // Disconnect triangle from its endpoints
    mPoints.DisconnectTriangle(mTriangles.GetPointAIndex(triangleElementIndex), triangleElementIndex, true); // Owner
    mPoints.DisconnectTriangle(mTriangles.GetPointBIndex(triangleElementIndex), triangleElementIndex, false); // Not owner
    mPoints.DisconnectTriangle(mTriangles.GetPointCIndex(triangleElementIndex), triangleElementIndex, false); // Not owner

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

    mPoints.SetDecay(
        pointElementIndex,
        mPoints.GetDecay(pointElementIndex) * rotCoefficient);

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