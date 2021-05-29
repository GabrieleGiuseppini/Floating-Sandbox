﻿/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "Ship_StateMachines.h"

#include <GameCore/AABB.h>
#include <GameCore/Algorithms.h>
#include <GameCore/GameDebug.h>
#include <GameCore/GameMath.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/Log.h>

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

static constexpr int UpdateSinkingPeriodStep = 6;
static constexpr int CombustionStateMachineSlowPeriodStep1 = 13;
static constexpr int RotPointsPeriodStep = 20;
static constexpr int CombustionStateMachineSlowPeriodStep2 = 27;
static constexpr int SpringDecayAndTemperaturePeriodStep = 34;
static constexpr int CombustionStateMachineSlowPeriodStep3 = 41;
static constexpr int CombustionStateMachineSlowPeriodStep4 = 48;

static_assert(CombustionStateMachineSlowPeriodStep4 < GameParameters::ParticleUpdateLowFrequencyPeriod);

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
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::shared_ptr<TaskThreadPool> taskThreadPool,
    Points && points,
    Springs && springs,
    Triangles && triangles,
    ElectricalElements && electricalElements,
    Frontiers && frontiers)
    : mId(id)
    , mParentWorld(parentWorld)
    , mMaterialDatabase(materialDatabase)
    , mGameEventHandler(std::move(gameEventDispatcher))
    , mTaskThreadPool(std::move(taskThreadPool))
    , mEventRecorder(nullptr)
    , mSize(points.GetAABB().GetSize())
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mFrontiers(std::move(frontiers))
    , mPinnedPoints(
        mParentWorld,
        mGameEventHandler,
        mPoints)
    , mGadgets(
        mParentWorld,
        mId,
        mGameEventHandler,
        *this,
        mPoints,
        mSprings)
    , mElectricSparks(mSprings)
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

void Ship::SetEventRecorder(EventRecorder * eventRecorder)
{
    mEventRecorder = eventRecorder;
}

bool Ship::ReplayRecordedEvent(
    RecordedEvent const & event,
    GameParameters const & gameParameters)
{
    if (event.GetType() == RecordedEvent::RecordedEventType::PointDetachForDestroy)
    {
        RecordedPointDetachForDestroyEvent const & detachEvent = static_cast<RecordedPointDetachForDestroyEvent const &>(event);

        DetachPointForDestroy(
            detachEvent.GetPointIndex(),
            detachEvent.GetDetachVelocity(),
            detachEvent.GetSimulationTime(),
            gameParameters);
    }

    return false;
}

void Ship::Update(
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters,
    Geometry::AABBSet & aabbSet)
{
    /////////////////////////////////////////////////////////////////
    //         This is where most of the magic happens             //
    /////////////////////////////////////////////////////////////////

    std::vector<TaskThreadPool::Task> parallelTasks;

    /////////////////////////////////////////////////////////////////
    // At this moment:
    //  - Particle positions are within world boundaries
    //  - Particle non-spring forces contain interaction-provided forces
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

    mPoints.UpdateForGameParameters(
        gameParameters);

    if (mCurrentSimulationSequenceNumber.IsStepOf(SpringDecayAndTemperaturePeriodStep, GameParameters::ParticleUpdateLowFrequencyPeriod))
    {
        mSprings.UpdateForDecayAndTemperatureAndGameParameters(
            gameParameters,
            mPoints);
    }
    else
    {
        // Just plain parameter check
        mSprings.UpdateForGameParameters(
            gameParameters,
            mPoints);
    }

    mElectricalElements.UpdateForGameParameters(
        gameParameters);

    ///////////////////////////////////////////////////////////////////
    // Calculate some widely-used physical constants
    ///////////////////////////////////////////////////////////////////

    float const effectiveAirTemperature =
        gameParameters.AirTemperature
        + stormParameters.AirTemperatureDelta;

    // Density of air, adjusted for temperature
    float const effectiveAirDensity =
        GameParameters::AirMass
        / (1.0f + GameParameters::AirThermalExpansionCoefficient * (effectiveAirTemperature - GameParameters::Temperature0));

    // Density of water, adjusted for temperature and manual adjustment
    float const effectiveWaterDensity =
        GameParameters::WaterMass
        / (1.0f + GameParameters::WaterThermalExpansionCoefficient * (gameParameters.WaterTemperature - GameParameters::Temperature0))
        * gameParameters.WaterDensityAdjustment;

    ///////////////////////////////////////////////////////////////////
    // Recalculate current masses and everything else that derives from them
    ///////////////////////////////////////////////////////////////////

    // - Inputs: Water, AugmentedMaterialMass
    // - Outputs: Mass
    mPoints.UpdateMasses(gameParameters);

    ///////////////////////////////////////////////////////////////////
    // Run spring relaxation iterations and hydrostatic pressure,
    // together with integration and ocean floor collision handling
    ///////////////////////////////////////////////////////////////////

    int const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<int>();

    // We run ocean floor collision handling every so often
    int constexpr SeaFloorCollisionPeriod = 2;
    float const seaFloorCollisionDt = gameParameters.MechanicalSimulationStepTimeDuration<float>() * static_cast<float>(SeaFloorCollisionPeriod);

    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - DynamicForces = 0

        // Apply spring forces
        ApplySpringsForces_BySprings(gameParameters);

        // - DynamicForces = fs

        if (iter == numMechanicalDynamicsIterations - 1)
        {
            // Last step: apply hydrostatic pressure forces
            ApplyHydrostaticPressureForces(
                effectiveAirDensity,
                effectiveWaterDensity,
                gameParameters);

            // - DynamicForces = fs + hp
        }

        // Integrate dynamic and static forces,
        // and reset dynamic forces
        IntegrateAndResetDynamicForces(gameParameters);

        // - DynamicForces = 0

        if ((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1)
        {
            // Handle collisions with sea floor
            //  - Changes position and velocity
            HandleCollisionsWithSeaFloor(
                seaFloorCollisionDt,
                gameParameters);
        }
    }

    ///////////////////////////////////////////////////////////////////
    // Trim for world bounds
    ///////////////////////////////////////////////////////////////////

    // - Inputs: Position
    // - Outputs: Position, Velocity
    TrimForWorldBounds(gameParameters);

    // We're done with changing positions for the rest of the Update() loop
#ifdef _DEBUG
    mPoints.Diagnostic_ClearDirtyPositions();
#endif

    ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////
    // From now on, we only work with (static) forces and never update positions
    ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // Reset static forces, now that we have integrated them
    ///////////////////////////////////////////////////////////////////

    mPoints.ResetStaticForces();

    ///////////////////////////////////////////////////////////////////
    // Apply interaction forces that have been queued before this
    // step
    ///////////////////////////////////////////////////////////////////

    ApplyQueuedInteractionForces();

    ///////////////////////////////////////////////////////////////////
    // Apply world forces
    //
    // Also calculates cached depths, and updates AABBs
    ///////////////////////////////////////////////////////////////////

    ApplyWorldForces(
        effectiveAirDensity,
        effectiveWaterDensity,
        gameParameters,
        aabbSet);

    // Cached depths are valid from now on --------------------------->

    ///////////////////////////////////////////////////////////////////
    // Rot points
    ///////////////////////////////////////////////////////////////////

    if (mCurrentSimulationSequenceNumber.IsStepOf(RotPointsPeriodStep, GameParameters::ParticleUpdateLowFrequencyPeriod))
    {
        // - Inputs: Position, Water, IsLeaking
        // - Output: Decay
        RotPoints(
            currentSimulationTime,
            gameParameters);
    }

    /////////////////////////////////////////////////////////////////
    // Update gadgets
    /////////////////////////////////////////////////////////////////

    // Might cause explosions; might cause elements to be detached/destroyed
    // (which would flag our structure as dirty)
    mGadgets.Update(
        currentWallClockTime,
        currentSimulationTime,
        stormParameters,
        gameParameters);

    ///////////////////////////////////////////////////////////////////
    // Update strain for all springs; might cause springs to break
    // (which would flag our structure as dirty)
    ///////////////////////////////////////////////////////////////////

    // - Inputs: P.Position, S.SpringDeletion, S.ResetLength, S.BreakingElongation
    // - Outputs: S.Destroy()
    // - FiresEvents
    mSprings.UpdateForStrains(
        gameParameters,
        mPoints);

    ///////////////////////////////////////////////////////////////////
    // Update state machines
    ///////////////////////////////////////////////////////////////////

    // - Outputs:   Non-spring forces, temperature
    //              Point Detach, Debris generation
    UpdateStateMachines(currentSimulationTime, gameParameters);

    /////////////////////////////////////////////////////////////////
    // Update water dynamics - may generate ephemeral particles
    /////////////////////////////////////////////////////////////////

    //
    // Update intake of water
    //

    float waterTakenInStep = 0.f;

    // - Inputs: P.Position, P.Water, P.IsLeaking, P.Temperature, P.PlaneId
    // - Outputs: P.Water, P.CumulatedIntakenWater
    // - Creates ephemeral particles
    UpdateWaterInflow(
        currentSimulationTime,
        stormParameters,
        gameParameters,
        waterTakenInStep);

    // Notify intaken water
    mGameEventHandler->OnWaterTaken(waterTakenInStep);

    //
    // Diffuse water
    //

    float waterSplashedInStep = 0.f;

    // - Inputs: Position, Water, WaterVelocity, WaterMomentum, ConnectedSprings
    // - Outpus: Water, WaterVelocity, WaterMomentum
    UpdateWaterVelocities(gameParameters, waterSplashedInStep);

    // Notify
    mGameEventHandler->OnWaterSplashed(waterSplashedInStep);

    //
    // Run sinking/unsinking detection
    //

    if (mCurrentSimulationSequenceNumber.IsStepOf(UpdateSinkingPeriodStep, GameParameters::ParticleUpdateLowFrequencyPeriod))
    {
        UpdateSinking();
    }

    ///////////////////////////////////////////////////////////////////
    // Update electrical dynamics
    ///////////////////////////////////////////////////////////////////

    // Generate a new visit sequence number
    ++mCurrentElectricalVisitSequenceNumber;

    //
    // 1. Update automatic conductivity toggles (e.g. water-sensing switches)
    //

    mElectricalElements.UpdateAutomaticConductivityToggles(
        mPoints,
        gameParameters);

    //
    // 2. Update sources and connectivity
    //
    // We do this regardless of dirty elements, as elements might have changed their state
    // (e.g. generators might have become wet, switches might have been toggled, etc.)
    //

    mElectricalElements.UpdateSourcesAndPropagation(
        mCurrentElectricalVisitSequenceNumber,
        mPoints,
        gameParameters);

    //
    // 3. Update sinks
    //
    // - Applies static forces, will be integrated at next loop
    //

    mElectricalElements.UpdateSinks(
        currentWallClockTime,
        currentSimulationTime,
        mCurrentElectricalVisitSequenceNumber,
        mPoints,
        stormParameters,
        gameParameters);

    ///////////////////////////////////////////////////////////////////
    // Update heat dynamics
    ///////////////////////////////////////////////////////////////////

    assert(parallelTasks.empty()); // We want this to run on the main thread

    parallelTasks.emplace_back(
        [&]()
        {
            //
            // Propagate heat
            //

            // - Inputs: P.Position, P.Temperature, P.ConnectedSprings, P.Water
            // - Outputs: P.Temperature
            PropagateHeat(
                currentSimulationTime,
                GameParameters::SimulationStepTimeDuration<float>,
                stormParameters,
                gameParameters);

            //
            // Update slow combustion state machine
            //

            if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep1, GameParameters::ParticleUpdateLowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    0,
                    4,
                    currentSimulationTime,
                    stormParameters,
                    gameParameters);
            }
            else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep2, GameParameters::ParticleUpdateLowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    1,
                    4,
                    currentSimulationTime,
                    stormParameters,
                    gameParameters);
            }
            else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep3, GameParameters::ParticleUpdateLowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    2,
                    4,
                    currentSimulationTime,
                    stormParameters,
                    gameParameters);
            }
            else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep4, GameParameters::ParticleUpdateLowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    3,
                    4,
                    currentSimulationTime,
                    stormParameters,
                    gameParameters);
            }

            //
            // Update fast combustion state machine
            //

            mPoints.UpdateCombustionHighFrequency(
                currentSimulationTime,
                GameParameters::SimulationStepTimeDuration<float>,
                gameParameters);
        });

    ///////////////////////////////////////////////////////////////////
    // Diffuse light from lamps
    ///////////////////////////////////////////////////////////////////

    parallelTasks.emplace_back(
        [&]()
        {
            // - Inputs: P.Position, P.PlaneId, EL.AvailableLight
            //      - EL.AvailableLight depends on electricals which depend on water
            // - Outputs: P.Light
            DiffuseLight(gameParameters);
        });

    mTaskThreadPool->RunAndClear(parallelTasks);

    ///////////////////////////////////////////////////////////////////
    // Update ephemeral particles
    ///////////////////////////////////////////////////////////////////

    mPoints.UpdateEphemeralParticles(
        currentSimulationTime,
        gameParameters);

    ///////////////////////////////////////////////////////////////////
    // Update highlights
    ///////////////////////////////////////////////////////////////////

    mPoints.UpdateHighlights(currentWallClockTimeFloat);

    ///////////////////////////////////////////////////////////////////
    // Electric sparks
    ///////////////////////////////////////////////////////////////////

    mElectricSparks.Update();

    ///////////////////////////////////////////////////////////////////
    // Diagnostics
    ///////////////////////////////////////////////////////////////////

#ifdef _DEBUG

    Verify(!mPoints.Diagnostic_ArePositionsDirty());

    VerifyInvariants();

#endif
}

void Ship::RenderUpload(Render::RenderContext & renderContext)
{
    //
    // Run connectivity visit, if there have been any deletions
    //
    // Note: we have to do this here, at render time rathern than
    // at update time, because the structure might have been dirtied
    // by an interactive tool while the game is paused
    //

    if (mIsStructureDirty)
    {
        RunConnectivityVisit();
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

    mPoints.UploadFlames(
        mId,
        renderContext);

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

void Ship::ApplyQueuedInteractionForces()
{
    for (auto const & interaction : mQueuedInteractions)
    {
        switch (interaction.Type)
        {
            case Interaction::InteractionType::Blast:
            {
                ApplyBlastAt(interaction.Arguments.Blast);

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
    GameParameters const & gameParameters,
    Geometry::AABBSet & aabbSet)
{
    // New buffer to which new cached depths will be written to
    std::shared_ptr<Buffer<float>> newCachedPointDepths = mPoints.AllocateWorkBufferFloat();

    //
    // Particle forces
    //

    ApplyWorldParticleForces(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, gameParameters);

    //
    // Surface forces
    //

    if (gameParameters.DoDisplaceWater)
        ApplyWorldSurfaceForces<true>(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, gameParameters, aabbSet);
    else
        ApplyWorldSurfaceForces<false>(effectiveAirDensity, effectiveWaterDensity, *newCachedPointDepths, gameParameters, aabbSet);

    // Commit new particle depth buffer
    mPoints.SwapCachedDepthBuffer(*newCachedPointDepths);
}

void Ship::ApplyWorldParticleForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    Buffer<float> & newCachedPointDepths,
    GameParameters const & gameParameters)
{
    // Wind force:
    //  Km/h -> Newton: F = 1/2 rho v**2 A
    float constexpr WindVelocityConversionFactor = 1000.0f / 3600.0f;
    vec2f const windForce =
        mParentWorld.GetCurrentWindSpeed().square()
        * (WindVelocityConversionFactor * WindVelocityConversionFactor)
        * 0.5f
        * effectiveAirDensity;

    // Abovewater points feel this amount of air drag, due to friction
    float const airFrictionDragCoefficient =
        GameParameters::AirFrictionDragCoefficient
        * gameParameters.AirFrictionDragAdjustment;

    // Underwater points feel this amount of water drag, due to friction
    float const waterFrictionDragCoefficient =
        GameParameters::WaterFrictionDragCoefficient
        * gameParameters.WaterFrictionDragAdjustment;

    float * const restrict newCachedPointDepthsBuffer = newCachedPointDepths.data();
    vec2f * const restrict staticForcesBuffer = mPoints.GetStaticForceBufferAsVec2();

    for (auto pointIndex : mPoints.BufferElements())
    {
        vec2f staticForce = vec2f::zero();

        //
        // Calculate and store depth
        //

        newCachedPointDepthsBuffer[pointIndex] = mParentWorld.GetDepth(mPoints.GetPosition(pointIndex));

        //
        // Calculate above/under-water coefficient
        //
        // 0.0: above water
        // 1.0: under water
        // in-between: smooth air-water interface (nature abhors discontinuities)
        //

        float const uwCoefficient = Clamp(newCachedPointDepthsBuffer[pointIndex], 0.0f, 1.0f);

        //
        // Apply gravity
        //

        staticForce +=
            gameParameters.Gravity
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
        // Wind force
        //

        // Note: should be based on relative velocity, but we simplify here for performance reasons
        staticForce +=
            windForce
            * mPoints.GetMaterialWindReceptivity(pointIndex)
            * (1.0f - uwCoefficient); // Only above-water

        staticForcesBuffer[pointIndex] += staticForce;
    }
}

template<bool DoDisplaceWater>
void Ship::ApplyWorldSurfaceForces(
    float effectiveAirDensity,
    float effectiveWaterDensity,
    Buffer<float> & newCachedPointDepths,
    GameParameters const & gameParameters,
    Geometry::AABBSet & aabbSet)
{
    //
    // Drag constants
    //

    // Abovewater points feel this amount of air drag, due to pressure
    float const airPressureDragCoefficient =
        GameParameters::AirPressureDragCoefficient
        * gameParameters.AirPressureDragAdjustment
        * (effectiveAirDensity / GameParameters::AirMass);

    // Underwater points feel this amount of water drag, due to pressure
    float const waterPressureDragCoefficient =
        GameParameters::WaterPressureDragCoefficient
        * gameParameters.WaterPressureDragAdjustment
        * (effectiveWaterDensity / GameParameters::WaterMass);

    //
    // Water displacement constants
    //

    float constexpr wdmX0 = 3.75f; // Velocity at which displacement transitions from quadratic to linear
    float constexpr wdmY0 = 0.3f; // Displacement magnitude at x0

    // Linear portion
    float const wdmLinearSlope =
        GameParameters::SimulationStepTimeDuration<float> * 4.0f // Magic number
        * gameParameters.WaterDisplacementWaveHeightAdjustment;

    // Quadratic portion: y = ax^2 + bx, with constraints:
    //  y(0) = 0
    //  y'(x0) = slope
    //  y(x0) = y0
    float const wdmQuadraticA = (wdmLinearSlope * wdmX0 - wdmY0) / (wdmX0 * wdmX0);
    float const wdmQuadraticB = 2.0f * wdmY0 / wdmX0 - wdmLinearSlope;

    //
    // Visit all frontiers and apply surface forces
    //

    for (FrontierId frontierId : mFrontiers.GetFrontierIds())
    {
        auto & frontier = mFrontiers.GetFrontier(frontierId);

        // We only apply velocity drag and displace water for *external* frontiers,
        // not for internal ones
        if (frontier.Type == FrontierType::External)
        {
            // Initialize AABB
            Geometry::AABB aabb;

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

                // Update AABB with this point
                aabb.ExtendTo(thisPointPosition);

                // Get next edge and point
                auto const & nextFrontierEdge = mFrontiers.GetFrontierEdge(nextEdgeIndex);
                ElementIndex const nextPointIndex = nextFrontierEdge.PointAIndex;
                vec2f const nextPointPosition = mPoints.GetPosition(nextPointIndex);

                //
                // Drag force
                //
                // We would like to use a square law (i.e. drag force proportional to square
                // of velocity), but then particles at high velocities become subject to
                // enormous forces, which, for small masses - such as cloth - means astronomical
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
                    / GameParameters::SimulationStepTimeDuration<float>;

                // Get point depth (positive at greater depths, negative over-water)
                float const thisPointDepth = newCachedPointDepths[thisPointIndex];

                // Calculate drag coefficient: air or water, with soft transition
                // to avoid discontinuities in drag force close to the air-water interface
                float const dragCoefficient = Mix(
                    airPressureDragCoefficient,
                    waterPressureDragCoefficient,
                    Clamp(thisPointDepth, 0.0f, 1.0f));

                // Calculate magnitude of drag force (opposite sign)
                //  - C * |V| * cos(a) == - C * |V| * (Vn dot Nn) == -C * (V dot Nn)
                float const dragForceMagnitude =
                    dragCoefficient
                    * velocityMagnitudeAlongNormal;

                // Final drag force - at this moment in the direction of the normal (i.e. outside)
                vec2f const dragForce = surfaceNormal * std::min(dragForceMagnitude, maxDragForceMagnitude);

                // Apply drag force
                mPoints.AddStaticForce(
                    thisPointIndex,
                    -dragForce);

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
                    float const verticalVelocity = mPoints.GetVelocity(thisPointIndex).y;
                    float const absVerticalVelocity = std::abs(verticalVelocity);

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
                    // Mass impact
                    // - The impact of mass should follow a square root law, but for performance we approximate it
                    //   with a linear law based on some points taken on a sqrt curve
                    //

                    float constexpr Mass1 = 18.0f;
                    float constexpr Impact1 = 11.0f;
                    float constexpr Mass2 = 1000.0f;
                    float constexpr Impact2 = 25.0f;

                    float const massImpact = Impact1 + (mPoints.GetMass(thisPointIndex) - Mass1) * (Impact2 - Impact1) / (Mass2 - Mass1);

                    //
                    // Displacement
                    //

                    float const displacement =
                        (absVerticalVelocity < wdmX0 ? quadraticDisplacementMagnitude : linearDisplacementMagnitude)
                        * massImpact
                        * depthAttenuation
                        * SignStep(0.0f, verticalVelocity) // Displacement has same sign as vertical velocity
                        * Step(0.0f, thisPointDepth) // No displacement for above-water points
                        * 0.02f; // Magic number

                    mParentWorld.DisplaceOceanSurfaceAt(thisPointPosition.x, displacement);
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
            //
            // Finalize AABB update
            //

            // Store AABB in frontier
            frontier.ExternalFrontierAABB = aabb;

            // Store AABB in AABB set
            aabbSet.Add(aabb);
        }
    }
}

void Ship::ApplyHydrostaticPressureForces(
    float /*effectiveAirDensity*/, // CODEWORK: future, if we want to also apply *air* pressure
    float effectiveWaterDensity,
    GameParameters const & gameParameters)
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
    //  - We use the AABB's center as the point where the depth is calculated at;
    //    as a consequence, if the ship is interactively moved or rotated, the AABB's
    //    that we use here are stale. Not a big deal...
    //    Outside of these "moving" interactions, the AABBs we use here are also
    //    inconsistent with the current positions because of integration during dynamic
    //    iterations, unless hydrostatic pressures are calculated on the *first* dynamic
    //    iteration.
    //

    for (FrontierId const frontierId : mFrontiers.GetFrontierIds())
    {
        auto const & frontier = mFrontiers.GetFrontier(frontierId);

        // Only consider external frontiers for which we already have an AABB
        if (frontier.Type == FrontierType::External
            && frontier.ExternalFrontierAABB.has_value())
        {
            //
            // Calculate force stem
            //

            float const pressureForceStem =
                std::max(mParentWorld.GetDepth(frontier.ExternalFrontierAABB->CalculateCenter()), 0.0f)
                * effectiveWaterDensity
                * GameParameters::GravityMagnitude
                * gameParameters.HydrostaticPressureAdjustment
                / 2.0f; // We include the division that we'll need for the particle force

            //
            // Visit all edges
            //
            //               thisPoint
            //                   V
            // ...---*---edge1---*---edge2---*---nextEdge---....
            //

            ElementIndex edge1Index = frontier.StartingEdgeIndex;

            ElementIndex edge2Index = mFrontiers.GetFrontierEdge(edge1Index).NextEdgeIndex;

            ElementIndex thisPointIndex = mFrontiers.GetFrontierEdge(edge2Index).PointAIndex;

            vec2f edge1PerpVector =
                -(mPoints.GetPosition(thisPointIndex) - mPoints.GetPosition(mFrontiers.GetFrontierEdge(edge1Index).PointAIndex)).to_perpendicular();

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

                // Apply force - we apply it as a *dynamic* force, otherwise if it were static it would
                // generate phantom torques due to geometries changing for every dynamic step

                vec2f const pressureForce =
                    (edge1PerpVector + edge2PerpVector) * pressureForceStem;

                mPoints.AddDynamicForce(
                    thisPointIndex,
                    pressureForce);

                // Advance
                nextEdgeIndex = nextEdge.NextEdgeIndex;
                if (nextEdgeIndex == startEdgeIndex)
                    break;

                thisPointIndex = nextPointIndex;
                edge1PerpVector = edge2PerpVector;
            }

#ifdef _DEBUG
            assert(visitedPoints == frontier.Size);
#endif
        }
    }
}

void Ship::ApplySpringsForces_BySprings(GameParameters const & /*gameParameters*/)
{
    vec2f const * restrict const pointPositionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f const * restrict const pointVelocityBuffer = mPoints.GetVelocityBufferAsVec2();
    vec2f * restrict const pointDynamicForceBuffer = mPoints.GetDynamicForceBufferAsVec2();

    Springs::Endpoints const * restrict const endpointsBuffer = mSprings.GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = mSprings.GetRestLengthBuffer();
    Springs::Coefficients const * restrict const coefficientsBuffer = mSprings.GetCoefficientsBuffer();

    ElementCount const springCount = mSprings.GetElementCount();
    for (ElementIndex springIndex = 0; springIndex < springCount; ++springIndex)
    {
        auto const pointAIndex = endpointsBuffer[springIndex].PointAIndex;
        auto const pointBIndex = endpointsBuffer[springIndex].PointBIndex;

        // No need to check whether the spring is deleted, as a deleted spring
        // has zero coefficients

        vec2f const displacement = pointPositionBuffer[pointBIndex] - pointPositionBuffer[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        float const fSpring =
            (displacementLength - restLengthBuffer[springIndex])
            * coefficientsBuffer[springIndex].StiffnessCoefficient;

        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = pointVelocityBuffer[pointBIndex] - pointVelocityBuffer[pointAIndex];
        float const fDamp =
            relVelocity.dot(springDir)
            * coefficientsBuffer[springIndex].DampingCoefficient;

        //
        // Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        pointDynamicForceBuffer[pointAIndex] += forceA;
        pointDynamicForceBuffer[pointBIndex] -= forceA;
    }
}

void Ship::IntegrateAndResetDynamicForces(GameParameters const & gameParameters)
{
    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();

    // Global damp - lowers velocity uniformly, damping oscillations originating between gravity and buoyancy
    //
    // Considering that:
    //
    //  v1 = d*v0
    //  v2 = d*v1 =(d^2)*v0
    //  ...
    //  vN = (d^N)*v0
    //
    // ...the more the number of iterations, the more damped the initial velocity would be.
    // We want damping to be independent from the number of iterations though, so we need to find the value
    // d such that after N iterations the damping is the same as our reference value, which is based on
    // 12 (basis) iterations. For example, double the number of iterations requires square root (1/2) of
    // this value.
    //

    float const globalDamping = 1.0f -
        pow((1.0f - GameParameters::GlobalDamping),
            12.0f / gameParameters.NumMechanicalDynamicsIterations<float>());

    // Incorporate adjustment
    float const globalDampingCoefficient = 1.0f -
        (
            gameParameters.GlobalDampingAdjustment <= 1.0f
            ? globalDamping * (1.0f - (gameParameters.GlobalDampingAdjustment - 1.0f) * (gameParameters.GlobalDampingAdjustment - 1.0f))
            : globalDamping +
                (gameParameters.GlobalDampingAdjustment - 1.0f) * (gameParameters.GlobalDampingAdjustment - 1.0f)
                / ((gameParameters.MaxGlobalDampingAdjustment - 1.0f) * (gameParameters.MaxGlobalDampingAdjustment - 1.0f))
                * (1.0f - globalDamping)
        );

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = globalDampingCoefficient / dt;

    //
    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible
    //
    // This loop is compiled with single-precision packet SSE instructions on MSVC 17,
    // integrating two points at each iteration
    //

    float * const restrict positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * const restrict velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float * const restrict dynamicForceBuffer = mPoints.GetDynamicForceBufferAsFloat();
    float const * const restrict staticForceBuffer = mPoints.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (dynamicForceBuffer[i] + staticForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out dynamic force now that we've integrated it
        dynamicForceBuffer[i] = 0.0f;
    }

#ifdef _DEBUG
    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::HandleCollisionsWithSeaFloor(
    float dt,
    GameParameters const & gameParameters)
{
    float const elasticityFactor = -gameParameters.OceanFloorElasticity;
    float const inverseFriction = 1.0f - gameParameters.OceanFloorFriction;

    for (auto pointIndex : mPoints)
    {
        auto const & position = mPoints.GetPosition(pointIndex);

        // Check if point is below the sea floor
        //
        // At this moment the point might be outside of world boundaries,
        // so better clamp its x before sampling ocean floor height
        float const clampedX = Clamp(position.x, -GameParameters::HalfMaxWorldWidth, GameParameters::HalfMaxWorldWidth);
        if (mParentWorld.IsUnderOceanFloor(clampedX, position.y))
        {
            // Collision!

            //
            // Calculate post-bounce velocity
            //

            vec2f const pointVelocity = mPoints.GetVelocity(pointIndex);

            // Calculate sea floor anti-normal
            // (positive points down)
            vec2f const seaFloorAntiNormal = -mParentWorld.GetOceanFloorNormalAt(clampedX);

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
                vec2f const normalResponse =
                    normalVelocity
                    * elasticityFactor; // Already negative

                // Calculate tangential response: Vt' = a*Vt (a = (1.0-friction), [0.0 - 1.0])
                vec2f const tangentialResponse =
                    tangentialVelocity
                    * inverseFriction;

                //
                // Impart final position and velocity
                //

                // Move point back to where it was in the previous step,
                // which is guaranteed to be more towards the outside
                mPoints.SetPosition(
                    pointIndex,
                    mPoints.GetPosition(pointIndex) - pointVelocity * dt);

                // Set velocity to resultant collision velocity
                mPoints.SetVelocity(
                    pointIndex,
                    normalResponse + tangentialResponse);
            }
        }
    }
}

void Ship::TrimForWorldBounds(GameParameters const & gameParameters)
{
    float constexpr MaxWorldLeft = -GameParameters::HalfMaxWorldWidth;
    float constexpr MaxWorldRight = GameParameters::HalfMaxWorldWidth;

    float constexpr MaxWorldTop = GameParameters::HalfMaxWorldHeight;
    float constexpr MaxWorldBottom = -GameParameters::HalfMaxWorldHeight;

    // Elasticity of the bounce against world boundaries
    //  - We use the ocean floor's elasticity for convenience
    float const elasticity = gameParameters.OceanFloorElasticity;

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
    }

#ifdef _DEBUG
    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

///////////////////////////////////////////////////////////////////////////////////
// Water Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateWaterInflow(
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters,
    float & waterTakenInStep)
{
    //
    // Intake/outtake water into/from all the leaking nodes (structural or forced)
    // that are either underwater or are overwater and taking rain.
    //
    // Ephemeral points are never leaking, hence we ignore them
    //

    float const rainEquivalentWaterHeight =
        stormParameters.RainQuantity // m/h
        / 3600.0f // -> m/s
        * GameParameters::SimulationStepTimeDuration<float> // -> m/step
        * gameParameters.RainFloodAdjustment;

    float const waterPumpPowerMultiplier =
        gameParameters.WaterPumpPowerAdjustment
        * (gameParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    bool const doGenerateAirBubbles = (gameParameters.AirBubblesDensity != 0.0f);

    float const cumulatedIntakenWaterThresholdForAirBubbles =
        GameParameters::AirBubblesDensityToCumulatedIntakenWater(gameParameters.AirBubblesDensity);

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        // This is one of the few cases in which we prefer branching over calculating
        // for all points, mostly because we expect a tiny fraction of all points to
        // be leaking at any moment
        auto const & pointCompositeLeaking = mPoints.GetLeakingComposite(pointIndex);
        if (pointCompositeLeaking.IsCumulativelyLeaking)
        {
            float const pointDepth = mPoints.GetCachedDepth(pointIndex);

            // External water height (~=external pressure)
            //
            // We also incorporate rain in the sources of external water height:
            // - If point is below water surface: external water height is due to depth
            // - If point is above water surface: external water height is due to rain
            float const externalWaterHeight = std::max(
                pointDepth + 0.1f, // Magic number to force flotsam to take some water in and eventually sink
                rainEquivalentWaterHeight); // At most is one meter, so does not interfere with underwater pressure

            // Internal water height (~=internal pressure)
            float const internalWaterHeight = mPoints.GetWater(pointIndex);

            //
            // 1) Calculate new water due to structural leaks (holes)
            //

            float newWater_Structural;
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
                    incomingWaterVelocity_Structural = sqrtf(2.0f * GameParameters::GravityMagnitude * (externalWaterHeight - internalWaterHeight));
                }
                else
                {
                    // Outgoing water
                    incomingWaterVelocity_Structural = -sqrtf(2.0f * GameParameters::GravityMagnitude * (internalWaterHeight - externalWaterHeight));
                }

                //
                // 1.2) In/Outtake water according to velocity:
                // - During dt, we move a volume of water Vw equal to A*v*dt; the equivalent change in water
                //   height is thus Vw/A, i.e. v*dt
                //

                newWater_Structural =
                    pointCompositeLeaking.LeakingSources.StructuralLeak // Dichotomical switch
                    * incomingWaterVelocity_Structural
                    * GameParameters::SimulationStepTimeDuration<float>
                    * mPoints.GetMaterialWaterIntake(pointIndex)
                    * gameParameters.WaterIntakeAdjustment;
            }

            //
            // 2) Calculate new water due to forced leaks (pumps)
            //

            float newWater_Forced = 0.0f;
            float const waterPumpForce = pointCompositeLeaking.LeakingSources.WaterPumpForce;
            if (waterPumpForce > 0.0f)
            {
                // Inward pump: only works if underwater
                newWater_Forced = (externalWaterHeight > 0.0f)
                    ? waterPumpForce * waterPumpPowerMultiplier // No need to cap as sea is infinite
                    : 0.0f;
            }
            else if (waterPumpForce < 0.0f)
            {
                // Outward pump: only works if water inside
                newWater_Forced = (internalWaterHeight > 0.0f)
                    ? waterPumpForce * waterPumpPowerMultiplier // We'll cap it
                    : 0.0f;
            }

            //
            // 3) Apply resultant water changes
            //

            float newWater = newWater_Structural + newWater_Forced;

            if (newWater < 0.0f)
            {
                // Outgoing water

                // Make sure we don't over-drain the point
                newWater = -std::min(-newWater, mPoints.GetWater(pointIndex));

                // Honor the water retention of this material
                newWater *= mPoints.GetMaterialWaterRestitution(pointIndex);
            }

            // Adjust water
            mPoints.SetWater(
                pointIndex,
                mPoints.GetWater(pointIndex) + newWater);

            // Check if it's time to produce air bubbles
            mPoints.GetCumulatedIntakenWater(pointIndex) += newWater;
            if (mPoints.GetCumulatedIntakenWater(pointIndex) > cumulatedIntakenWaterThresholdForAirBubbles)
            {
                // Generate air bubbles - but not on ropes as that looks awful
                if (doGenerateAirBubbles
                    && !mPoints.IsRope(pointIndex))
                {
                    GenerateAirBubble(
                        mPoints.GetPosition(pointIndex),
                        pointDepth,
                        mPoints.GetTemperature(pointIndex),
                        currentSimulationTime,
                        mPoints.GetPlaneId(pointIndex),
                        gameParameters);
                }

                // Consume all cumulated water
                mPoints.GetCumulatedIntakenWater(pointIndex) = 0.0f;
            }

            // Adjust total water taken during this step
            waterTakenInStep += newWater;
        }
    }
}

void Ship::UpdateWaterVelocities(
    GameParameters const & gameParameters,
    float & waterSplashed)
{
    //
    // For each (non-ephemeral) point, move each spring's outgoing water momentum to
    // its destination point
    //
    // Implementation of https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/
    //

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
    std::array<float, GameParameters::MaxSpringsPerPoint> springOutboundWaterFlowWeights;

    // Total weight
    float totalOutboundWaterFlowWeight;

    // Resultant water velocities along each spring
    std::array<vec2f, GameParameters::MaxSpringsPerPoint> springOutboundWaterVelocities;

    //
    // Precalculate point "freeness factors", i.e. how much each point's
    // quantity of water "suppresses" splashes from adjacent kinetic energy losses
    //

    auto pointFreenessFactorBuffer = mPoints.AllocateWorkBufferFloat();
    float * restrict pointFreenessFactorBufferData = pointFreenessFactorBuffer->data();
    for (auto pointIndex : mPoints.RawShipPoints())
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-oldPointWaterBufferData[pointIndex] * 10.0f);
    }


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

        // A higher crazyness gives more emphasys to bernoulli's velocity, as if pressures
        // and gravity were exaggerated
        //
        // WV[t] = WV[t-1] + alpha * Bernoulli
        //
        // WaterCrazyness=0   -> alpha=1
        // WaterCrazyness=0.5 -> alpha=0.5 + 0.5*Wh
        // WaterCrazyness=1   -> alpha=Wh
        float const alphaCrazyness = 1.0f + gameParameters.WaterCrazyness * (oldPointWaterBufferData[pointIndex] - 1.0f);

        // Kinetic energy lost at this point
        float pointKineticEnergyLoss = 0.0f;

        // Count of non-hull free and drowned neighbor points
        float pointSplashNeighbors = 0.0f;
        float pointSplashFreeNeighbors = 0.0f;

        totalOutboundWaterFlowWeight = 0.0f;

        size_t const connectedSpringCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
        for (size_t s = 0; s < connectedSpringCount; ++s)
        {
            auto const & cs = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings[s];

            // Normalized spring vector, oriented point -> other endpoint
            vec2f const springNormalizedVector = (mPoints.GetPosition(cs.OtherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise_approx();

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
                bernoulliVelocityAlongSpring = sqrtf(2.0f * GameParameters::GravityMagnitude * dwy);
            }
            else
            {
                // Gained velocity goes from other endpoint to point
                bernoulliVelocityAlongSpring = -sqrtf(2.0f * GameParameters::GravityMagnitude * -dwy);
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


            //
            // Update splash neighbors counts
            //

            pointSplashFreeNeighbors +=
                mSprings.GetWaterPermeability(cs.SpringIndex)
                * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

            pointSplashNeighbors += mSprings.GetWaterPermeability(cs.SpringIndex);
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
                * mPoints.GetMaterialWaterDiffusionSpeed(pointIndex) * gameParameters.WaterDiffusionSpeedAdjustment
                / totalOutboundWaterFlowWeight;
        }


        //
        // 3) Move water along all springs according to their flows,
        //    and update destination's momenta accordingly
        //

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


                //
                // Update point's kinetic energy loss:
                // splintered water colliding with whole other endpoint
                //

                // FUTURE: get rid of this re-calculation once we pre-calculate all spring normalized vectors
                vec2f const springNormalizedVector = (mPoints.GetPosition(cs.OtherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise_approx();

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
            }
            else
            {
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
            }
        }

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
    }


    //
    // Average kinetic energy loss
    //

    waterSplashed = mWaterSplashedRunningAverage.Update(waterSplashed);


    //
    // Transforming momenta into velocities
    //

    mPoints.UpdateWaterVelocitiesFromMomenta();
}

void Ship::UpdateSinking()
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
            mGameEventHandler->OnSinkingBegin(mId);
            mIsSinking = true;
        }
    }
    else
    {
        if (wetPointCount < mPoints.GetRawShipPointCount() * 1 / 10 + mPoints.GetTotalFactoryWetPoints()) // Low watermark
        {
            // Stopped sinking
            mGameEventHandler->OnSinkingEnd(mId);
            mIsSinking = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Electrical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::DiffuseLight(GameParameters const & gameParameters)
{
    //
    // Diffuse light from each lamp to all points on the same or lower plane ID,
    // inverse-proportionally to the lamp-point distance
    //

    // Shortcut
    if (mElectricalElements.Lamps().empty()
        || (gameParameters.LuminiscenceAdjustment == 0.0f && mLastLuminiscenceAdjustmentDiffused == 0.0f))
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

    Algorithms::DiffuseLight(
        mPoints.GetPositionBufferAsVec2(),
        mPoints.GetPlaneIdBufferAsPlaneId(),
        mPoints.GetAlignedShipPointCount(), // No real reason to skip ephemerals, other than they're not expected to have light
        lampPositions.data(),
        lampPlaneIds.data(),
        lampDistanceCoeffs.data(),
        mElectricalElements.GetLampLightSpreadMaxDistanceBufferAsFloat(),
        mElectricalElements.GetBufferLampCount(),
        mPoints.GetLightBufferAsFloat());

    // Remember that we've diffused light with this luminiscence adjustment
    mLastLuminiscenceAdjustmentDiffused = gameParameters.LuminiscenceAdjustment;
}

///////////////////////////////////////////////////////////////////////////////////
// Heat
///////////////////////////////////////////////////////////////////////////////////

void Ship::PropagateHeat(
    float /*currentSimulationTime*/,
    float dt,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    //
    // Propagate temperature (via heat), and dissipate temperature
    //

    // Source and result temperature buffers
    auto oldPointTemperatureBuffer = mPoints.MakeTemperatureBufferCopy();
    float const * restrict const oldPointTemperatureBufferData = oldPointTemperatureBuffer->data();
    float * restrict const newPointTemperatureBufferData = mPoints.GetTemperatureBufferAsFloat();

    // Outbound heat flows along each spring
    std::array<float, GameParameters::MaxSpringsPerPoint> springOutboundHeatFlows;

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
                mSprings.GetMaterialThermalConductivity(cs.SpringIndex) * gameParameters.ThermalConductivityAdjustment
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
        GameParameters::WaterConvectiveHeatTransferCoefficient
        * dt
        * gameParameters.HeatDissipationAdjustment
        * 2.0f; // We exaggerate a bit to take into account water wetting the material and thus making it more difficult for fire to re-kindle

    // Water temperature
    // We approximate the thermocline as a linear decrease of
    // temperature: 15 degrees in MaxSeaDepth meters
    float const surfaceWaterTemperature = gameParameters.WaterTemperature;
    float constexpr ThermoclineSlope = -15.0f / GameParameters::MaxSeaDepth;

    // We include rain in air
    float const effectiveAirConvectiveHeatTransferCoefficient =
        GameParameters::AirConvectiveHeatTransferCoefficient
        * dt
        * gameParameters.HeatDissipationAdjustment
        + FastPow(stormParameters.RainDensity, 0.3f) * effectiveWaterConvectiveHeatTransferCoefficient;

    float const airTemperature =
        gameParameters.AirTemperature
        + stormParameters.AirTemperatureDelta;

    // We also include ephemeral points, as they may be heated
    // and have a temperature
    for (auto pointIndex : mPoints)
    {
        float deltaT; // Temperature delta (particle - env)
        float heatLost; // Heat lost in this time quantum (positive when outgoing)

        if (mPoints.IsCachedUnderwater(pointIndex)
            || mPoints.GetWater(pointIndex) > GameParameters::SmotheringWaterHighWatermark)
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
    float /*currentSimulationTime*/,
    GameParameters const & gameParameters)
{
    if (gameParameters.RotAcceler8r == 0.0f)
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

    float constexpr Ns = 20.0f * 60.0f / GameParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>;

    float const a_uw = gameParameters.RotAcceler8r != 0.0f
        ? powf(0.75f, gameParameters.RotAcceler8r / Ns) // a_uw = 0.75 ^ (1/Ns)
        : 1.0f;

    float const a_uw_fl = gameParameters.RotAcceler8r != 0.0f
        ? powf(0.25f, gameParameters.RotAcceler8r / Ns) // a_uw = 0.25 ^ (1/Ns)
        : 1.0f;

    float const x_uw = (1.0f - a_uw) / (a_uw - a_uw_fl);
    float const beta = (1.0f - a_uw) / x_uw;

    // Process all non-ephemeral points - no real reason to exclude ephemerals, other
    // than they're not expected to rot
    for (auto p : mPoints.RawShipPoints())
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

            // Initialize count of points in this connected component
            size_t currentConnectedComponentPointCount = 1;

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
        }
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

void Ship::AttemptPointRestore(ElementIndex pointElementIndex)
{
    //
    // A point is eligible for restore if it's damaged and has all of its factory springs and all
    // of its factory triangles
    //

    if (mPoints.GetConnectedSprings(pointElementIndex).ConnectedSprings.size() == mPoints.GetFactoryConnectedSprings(pointElementIndex).ConnectedSprings.size()
        && mPoints.GetConnectedTriangles(pointElementIndex).ConnectedTriangles.size() == mPoints.GetFactoryConnectedTriangles(pointElementIndex).ConnectedTriangles.size()
        && mPoints.IsDamaged(pointElementIndex))
    {
        mPoints.Restore(pointElementIndex);
    }
}

void Ship::GenerateAirBubble(
    vec2f const & position,
    float depth,
    float temperature,
    float currentSimulationTime,
    PlaneId planeId,
    GameParameters const & /*gameParameters*/)
{
    float const vortexAmplitude =
        GameRandomEngine::GetInstance().GenerateUniformReal(
            0.1f,
            4.0f)
        * (GameRandomEngine::GetInstance().Choose(2) == 1 ? 1.0f : -1.0f);

    float const vortexPeriod = GameRandomEngine::GetInstance().GenerateUniformReal(
        1.5f,  // seconds
        4.5f); // seconds

    mPoints.CreateEphemeralParticleAirBubble(
        position,
        depth,
        temperature,
        vortexAmplitude,
        vortexPeriod,
        currentSimulationTime,
        planeId);
}

void Ship::GenerateDebris(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (gameParameters.DoGenerateDebris)
    {
        unsigned int const debrisParticleCount = GameRandomEngine::GetInstance().GenerateUniformInteger(
            GameParameters::MinDebrisParticlesPerEvent, GameParameters::MaxDebrisParticlesPerEvent);

        vec2f const pointPosition = mPoints.GetPosition(pointElementIndex);

        float const pointDepth = mParentWorld.GetDepth(pointPosition);

        for (unsigned int d = 0; d < debrisParticleCount; ++d)
        {
            // Choose velocity
            vec2f const velocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
                GameParameters::MinDebrisParticlesVelocity,
                GameParameters::MaxDebrisParticlesVelocity);

            // Choose a lifetime
            float const maxLifetime = GameRandomEngine::GetInstance().GenerateUniformReal(
                GameParameters::MinDebrisParticlesLifetime,
                GameParameters::MaxDebrisParticlesLifetime);

            mPoints.CreateEphemeralParticleDebris(
                pointPosition,
                velocity,
                pointDepth,
                mPoints.GetWater(pointElementIndex),
                mPoints.GetStructuralMaterial(pointElementIndex),
                currentSimulationTime,
                maxLifetime,
                mPoints.GetPlaneId(pointElementIndex));
        }
    }
}

void Ship::GenerateSparklesForCut(
    ElementIndex springElementIndex,
    vec2f const & cutDirectionStartPos,
    vec2f const & cutDirectionEndPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (gameParameters.DoGenerateSparklesForCuts)
    {
        //
        // Choose number of particles
        //

        unsigned int const sparkleParticleCount = GameRandomEngine::GetInstance().GenerateUniformInteger(
            GameParameters::MinSparkleParticlesForCutEvent, GameParameters::MaxSparkleParticlesForCutEvent);


        //
        // Calculate velocity angle: we want a gaussian centered around direction opposite to cut direction
        //

        float const centralAngleCW = (cutDirectionStartPos - cutDirectionEndPos).angleCw();
        float constexpr AngleWidth = Pi<float> / 20.0f;


        //
        // Create particles
        //

        vec2f const sparklePosition = mSprings.GetMidpointPosition(springElementIndex, mPoints);

        float const sparkleDepth = mParentWorld.GetDepth(sparklePosition);

        for (unsigned int d = 0; d < sparkleParticleCount; ++d)
        {
            // Velocity magnitude
            float const velocityMagnitude = GameRandomEngine::GetInstance().GenerateUniformReal(
                GameParameters::MinSparkleParticlesForCutVelocity, GameParameters::MaxSparkleParticlesForCutVelocity);

            // Velocity angle: gaussian centered around central angle
            float const velocityAngleCw =
                centralAngleCW
                + AngleWidth * GameRandomEngine::GetInstance().GenerateNormalizedNormalReal();

            // Choose a lifetime
            float const maxLifetime = GameRandomEngine::GetInstance().GenerateUniformReal(
                    GameParameters::MinSparkleParticlesForCutLifetime,
                    GameParameters::MaxSparkleParticlesForCutLifetime);

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
}

void Ship::GenerateSparklesForLightning(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    GameParameters const & /*gameParameters*/)
{
    //
    // Choose number of particles
    //

    unsigned int const sparkleParticleCount = GameRandomEngine::GetInstance().GenerateUniformInteger(
        GameParameters::MinSparkleParticlesForLightningEvent, GameParameters::MaxSparkleParticlesForLightningEvent);

    //
    // Create particles
    //

    vec2f const sparklePosition = mPoints.GetPosition(pointElementIndex);

    float const sparkleDepth = mParentWorld.GetDepth(sparklePosition);

    for (unsigned int d = 0; d < sparkleParticleCount; ++d)
    {
        // Velocity magnitude
        float const velocityMagnitude = GameRandomEngine::GetInstance().GenerateUniformReal(
            GameParameters::MinSparkleParticlesForLightningVelocity, GameParameters::MaxSparkleParticlesForLightningVelocity);

        // Velocity angle: uniform
        float const velocityAngleCw = GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>);

        // Choose a lifetime
        float const maxLifetime = GameRandomEngine::GetInstance().GenerateUniformReal(
                GameParameters::MinSparkleParticlesForLightningLifetime,
                GameParameters::MaxSparkleParticlesForLightningLifetime);

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
    GameParameters const & gameParameters)
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
            gameParameters,
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

            mElectricalElements.Destroy(electricalElementIndex);

            hasAnythingBeenDestroyed = true;
        }
    }

    if (hasAnythingBeenDestroyed)
    {
        // Notify gadgets
        mGadgets.OnPointDetached(pointElementIndex);

        if (generateDebris)
        {
            // Emit debris
            GenerateDebris(
                pointElementIndex,
                currentSimulationTime,
                gameParameters);
        }

        if (fireDestroyEvent)
        {
            // Notify destroy
            mGameEventHandler->OnDestroy(
                mPoints.GetStructuralMaterial(pointElementIndex),
                mParentWorld.IsUnderwater(mPoints.GetPosition(pointElementIndex)),
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

void Ship::HandlePointRestore(ElementIndex pointElementIndex)
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
        mGameEventHandler->OnShipRepaired(mId);
    }
}

void Ship::HandleSpringDestroy(
    ElementIndex springElementIndex,
    bool destroyAllTriangles,
    GameParameters const & /*gameParameters*/)
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
    // If both endpoints are electrical elements, then disconnect them - i.e. remove
    // them from each other's set of connected electrical elements
    //

    auto electricalElementAIndex = mPoints.GetElectricalElement(pointAIndex);
    if (NoneElementIndex != electricalElementAIndex)
    {
        auto electricalElementBIndex = mPoints.GetElectricalElement(pointBIndex);
        if (NoneElementIndex != electricalElementBIndex)
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

    //
    // Misc
    //

    // Notify gadgets
    mGadgets.OnSpringDestroyed(springElementIndex);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken springs
    ++mBrokenSpringsCount;
}

void Ship::HandleSpringRestore(
    ElementIndex springElementIndex,
    GameParameters const & /*gameParameters*/)
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
    mGameEventHandler->OnSpringRepaired(
        mPoints.GetStructuralMaterial(endpointAIndex),
        mParentWorld.IsUnderwater(mPoints.GetPosition(endpointAIndex)),
        1);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken springs
    assert(mBrokenSpringsCount > 0);
    --mBrokenSpringsCount;

    // Notify if we've just completely restored the ship
    if (mDamagedPointsCount == 0 && mBrokenSpringsCount == 0 && mBrokenTrianglesCount == 0)
    {
        mGameEventHandler->OnShipRepaired(mId);
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
    mGameEventHandler->OnTriangleRepaired(
        mPoints.GetStructuralMaterial(endpointAIndex),
        mParentWorld.IsUnderwater(mPoints.GetPosition(endpointAIndex)),
        1);

    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken triangles
    assert(mBrokenTrianglesCount > 0);
    --mBrokenTrianglesCount;

    // Notify if we've just completely restored the ship
    if (mDamagedPointsCount == 0 && mBrokenSpringsCount == 0 && mBrokenTrianglesCount == 0)
    {
        mGameEventHandler->OnShipRepaired(mId);
    }
}

void Ship::HandleElectricalElementDestroy(ElementIndex electricalElementIndex)
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
    float blastRadius,
    float blastForce,
    float blastHeat,
    ExplosionType explosionType,
    GameParameters const & /*gameParameters*/)
{
    // Queue state machine
    mStateMachines.push_back(
        std::make_unique<ExplosionStateMachine>(
            currentSimulationTime,
            planeId,
            centerPosition,
            blastRadius,
            blastForce,
            blastHeat,
            explosionType));
}

void Ship::DoAntiMatterBombPreimplosion(
    vec2f const & centerPosition,
    float /*sequenceProgress*/,
    float radius,
    GameParameters const & gameParameters)
{
    float const strength =
        130000.0f // Magic number
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    // Apply the force field
    ApplyRadialSpaceWarpForceField(
        centerPosition,
        radius,
        10.0f, // Thickness of radius, magic number
        strength);

    // Scare fishes
    mParentWorld.DisturbOceanAt(
        centerPosition,
        radius,
        std::chrono::milliseconds(0));
}

void Ship::DoAntiMatterBombImplosion(
    vec2f const & centerPosition,
    float sequenceProgress,
    GameParameters const & gameParameters)
{
    float const strength =
        (sequenceProgress * sequenceProgress * sequenceProgress)
        * gameParameters.AntiMatterBombImplosionStrength
        * 10000.0f
        * (gameParameters.IsUltraViolentMode ? 50.0f : 1.0f);

    // Apply the force field
    ApplyImplosionForceField(
        centerPosition,
        strength);
}

void Ship::DoAntiMatterBombExplosion(
    vec2f const & centerPosition,
    float sequenceProgress,
    GameParameters const & gameParameters)
{
    //
    // Single explosion peak at progress=0.0
    //

    if (0.0f == sequenceProgress)
    {
        float const strength =
            30000.0f
            * (gameParameters.IsUltraViolentMode ? 50.0f : 1.0f);

        // Apply the force field
        ApplyRadialExplosionForceField(
            centerPosition,
            strength);

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
        mGameEventHandler->OnWatertightDoorClosed(
            mParentWorld.IsUnderwater(mPoints.GetPosition(pointElementIndex)),
            1);
    }
    else
    {
        //
        // Close->Open transition
        //

        // Fire event
        mGameEventHandler->OnWatertightDoorOpened(
            mParentWorld.IsUnderwater(mPoints.GetPosition(pointElementIndex)),
            1);
    }
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
        Verify(pos.x >= -GameParameters::HalfMaxWorldWidth && pos.x <= GameParameters::HalfMaxWorldWidth);
        Verify(pos.y >= -GameParameters::HalfMaxWorldHeight && pos.y <= GameParameters::HalfMaxWorldHeight);
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