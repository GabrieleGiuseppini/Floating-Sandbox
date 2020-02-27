/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "Ship_StateMachines.h"

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
// we distribute all of these low-frequency updates in an interval of S steps (frames).
//

static constexpr int LowFrequencyPeriod = 7 * 7; // Number of simulation steps

static constexpr int UpdateSinkingPeriodStep = 6;
static constexpr int CombustionStateMachineSlowPeriodStep1 = 13;
static constexpr int RotPointsPeriodStep = 20;
static constexpr int CombustionStateMachineSlowPeriodStep2 = 27;
static constexpr int SpringDecayAndTemperaturePeriodStep = 34;
static constexpr int CombustionStateMachineSlowPeriodStep3 = 41;
static constexpr int CombustionStateMachineSlowPeriodStep4 = 48;

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
    ElectricalElements && electricalElements)
    : mId(id)
    , mParentWorld(parentWorld)
    , mMaterialDatabase(materialDatabase)
    , mGameEventHandler(std::move(gameEventDispatcher))
    , mTaskThreadPool(std::move(taskThreadPool))
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mPinnedPoints(
        mParentWorld,
        mId,
        mGameEventHandler,
        mPoints,
        mSprings)
    , mBombs(
        mParentWorld,
        mId,
        mGameEventHandler,
        *this,
        mPoints,
        mSprings)
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
    , mLastDebugShipRenderMode()
    , mPlaneTriangleIndicesToRender()
    , mWindSpeedMagnitudeToRender(0.0)
{
    mPlaneTriangleIndicesToRender.reserve(mTriangles.GetElementCount());

    // Set handlers
    mPoints.RegisterShipPhysicsHandler(this);
    mSprings.RegisterShipPhysicsHandler(this);
    mTriangles.RegisterShipPhysicsHandler(this);
    mElectricalElements.RegisterShipPhysicsHandler(this);

    // Do a first connectivity pass (for the first Update)
    RunConnectivityVisit();
}

void Ship::Announce()
{
    // Announce instanced electrical elements
    mElectricalElements.AnnounceInstancedElements();
}

void Ship::Update(
    float currentSimulationTime,
	Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters,
    Render::RenderContext const & renderContext)
{
    std::vector<TaskThreadPool::Task> parallelTasks;

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

    if (mCurrentSimulationSequenceNumber.IsStepOf(SpringDecayAndTemperaturePeriodStep, LowFrequencyPeriod))
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

    mWindSpeedMagnitudeToRender = mParentWorld.GetCurrentWindSpeed().x;


    ///////////////////////////////////////////////////////////////////
    // Update state machines
    ///////////////////////////////////////////////////////////////////

    UpdateStateMachines(currentSimulationTime, gameParameters);

    /////////////////////////////////////////////////////////////////
    // Update mechanical dynamics
    /////////////////////////////////////////////////////////////////

    //
    // Rot points
    //

    if (mCurrentSimulationSequenceNumber.IsStepOf(RotPointsPeriodStep, LowFrequencyPeriod))
    {
        // - Inputs: Position, Water, IsLeaking
        // - Output: Decay
        RotPoints(
            currentSimulationTime,
            gameParameters);
    }


    //
    // Recalculate current masses and everything else that derives from them, once and for all
    //

    // - Inputs: Water, AugmentedMaterialMass
    // - Outputs: Mass
    mPoints.UpdateMasses(gameParameters);


    //
    // Update non-spring forces
    //

    // Apply world forces
    ApplyWorldForces(gameParameters);

    //
    // Run spring relaxation iterations
    //

    int const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<int>();
    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - SpringForces = 0

        // Apply spring forces
        ApplySpringsForces_BySprings(gameParameters);

        // - SpringForces = fs

        // Integrate spring and non-spring forces,
        // and reset spring forces
        IntegrateAndResetSpringForces(gameParameters);

        // - SpringForces = 0

        // Handle collisions with sea floor
        //  - Changes position and velocity
        HandleCollisionsWithSeaFloor(gameParameters);
    }

    //
    // Reset non-spring forces, now that we have integrated them
    //

    // Check whether we need to save the non-spring force buffer before we zero it out
    if (VectorFieldRenderMode::PointForce == renderContext.GetVectorFieldRenderMode())
    {
        mPoints.CopyNonSpringForceBufferToForceRenderBuffer();
    }

    // Zero-out non-spring forces
    // - Outputs: NonSpringForce
    mPoints.ResetNonSpringForces();


    //
    // Trim for world bounds
    //

    // - Inputs: Position
    // - Outputs: Position, Velocity
    TrimForWorldBounds(gameParameters);

    /////////////////////////////////////////////////////////////////
    // Update bombs
    /////////////////////////////////////////////////////////////////

    //
    // Might cause explosions; might cause elements to be detached/destroyed
    // (which would flag our structure as dirty)
    //

    mBombs.Update(
        currentWallClockTime,
        currentSimulationTime,
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
    // Run sink/unsink detection
    //

    if (mCurrentSimulationSequenceNumber.IsStepOf(UpdateSinkingPeriodStep, LowFrequencyPeriod))
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
    // - Applies NonSpring force, will be integrated at next loop
    //

    mElectricalElements.UpdateSinks(
        currentWallClockTime,
        currentSimulationTime,
        mCurrentElectricalVisitSequenceNumber,
        mPoints,
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

            if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep1, LowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    0,
                    4,
                    currentSimulationTime,
                    GameParameters::SimulationStepTimeDuration<float> * static_cast<float>(LowFrequencyPeriod),
                    stormParameters,
                    gameParameters);
            }
            else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep2, LowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    1,
                    4,
                    currentSimulationTime,
                    GameParameters::SimulationStepTimeDuration<float> * static_cast<float>(LowFrequencyPeriod),
                    stormParameters,
                    gameParameters);
            }
            else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep3, LowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    2,
                    4,
                    currentSimulationTime,
                    GameParameters::SimulationStepTimeDuration<float> * static_cast<float>(LowFrequencyPeriod),
                    stormParameters,
                    gameParameters);
            }
            else if (mCurrentSimulationSequenceNumber.IsStepOf(CombustionStateMachineSlowPeriodStep4, LowFrequencyPeriod))
            {
                mPoints.UpdateCombustionLowFrequency(
                    3,
                    4,
                    currentSimulationTime,
                    GameParameters::SimulationStepTimeDuration<float> * static_cast<float>(LowFrequencyPeriod),
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

#ifdef _DEBUG
    VerifyInvariants();
#endif
}

void Ship::Render(
    GameParameters const & /*gameParameters*/,
    Render::RenderContext & renderContext)
{
    //
    // Run connectivity visit, if there have been any deletions
    //

    if (mIsStructureDirty)
    {
        RunConnectivityVisit();
    }


    //
    // Initialize render
    //

    renderContext.RenderShipStart(
        mId,
        mMaxMaxPlaneId);


    //
    // Upload points's attributes
    //

    mPoints.UploadAttributes(
        mId,
        renderContext);


    //
    // Upload elements, if needed
    //

    if (mIsStructureDirty
        || !mLastDebugShipRenderMode
        || *mLastDebugShipRenderMode != renderContext.GetDebugShipRenderMode())
    {
        renderContext.UploadShipElementsStart(mId);

        //
        // Upload point elements (either orphaned only or all, depending
        // on the debug render mode)
        //

        mPoints.UploadNonEphemeralPointElements(
            mId,
            renderContext);

        //
        // Upload all the spring elements (including ropes)
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

            renderContext.UploadShipElementTrianglesStart(
                mId,
                mPlaneTriangleIndicesToRender.back());

            mTriangles.UploadElements(
                mPlaneTriangleIndicesToRender,
                mId,
                mPoints,
                renderContext);

            renderContext.UploadShipElementTrianglesEnd(mId);
        }

        renderContext.UploadShipElementsEnd(
            mId,
            !mPoints.AreEphemeralPointsDirtyForRendering()); // Finalize ephemeral points only if there are no subsequent ephemeral point uploads
    }


    //
    // Upload stressed springs
    //
    // We do this regardless of whether or not elements are dirty,
    // as the set of stressed springs is bound to change from frame to frame
    //

    renderContext.UploadShipElementStressedSpringsStart(mId);

    if (renderContext.GetShowStressedSprings())
    {
        mSprings.UploadStressedSpringElements(
            mId,
            renderContext);
    }

    renderContext.UploadShipElementStressedSpringsEnd(mId);


    //
    // Upload flames
    //

    mPoints.UploadFlames(
        mId,
        mWindSpeedMagnitudeToRender,
        renderContext);


    //
    // Upload bombs
    //

    mBombs.Upload(
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
    // Finalize render
    //

    renderContext.RenderShipEnd(mId);


    //
    // Reset render state
    //

    mIsStructureDirty = false;
    mLastDebugShipRenderMode = renderContext.GetDebugShipRenderMode();
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// Mechanical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::ApplyWorldForces(GameParameters const & gameParameters)
{
    // Density of air, adjusted for temperature
    float const effectiveAirDensity =
        GameParameters::AirMass
        / (1.0f + GameParameters::AirThermalExpansionCoefficient * (gameParameters.AirTemperature - GameParameters::Temperature0));

    // Density of water, adjusted for temperature and manual adjustment
    float const effectiveWaterDensity  =
        GameParameters::WaterMass
        / (1.0f + GameParameters::WaterThermalExpansionCoefficient * (gameParameters.WaterTemperature - GameParameters::Temperature0) )
        * gameParameters.WaterDensityAdjustment;

    // Calculate wind force:
    //  Km/h -> Newton: F = 1/2 rho v**2 A
    float constexpr WindVelocityConversionFactor = 1000.0f / 3600.0f;
    vec2f const windForce =
        mParentWorld.GetCurrentWindSpeed().square()
        * (WindVelocityConversionFactor * WindVelocityConversionFactor)
        * 0.5f
        * GameParameters::AirMass;

    // Underwater points feel this amount of water drag
    //
    // The higher the value, the more viscous the water looks when a body moves through it
    float const waterDragCoefficient =
        GameParameters::WaterDragLinearCoefficient
        * gameParameters.WaterDragAdjustment;

    for (auto pointIndex : mPoints)
    {
        //
        // Add gravity
        //

        mPoints.GetNonSpringForce(pointIndex) +=
            gameParameters.Gravity
            * mPoints.GetMass(pointIndex); // Material + Augmentation + Water

        //
        // Add buoyancy
        //

        // Calculate upward push of water/air mass
        auto const & buoyancyCoefficients = mPoints.GetBuoyancyCoefficients(pointIndex);
        float const buoyancyPush =
            buoyancyCoefficients.Coefficient1
            + buoyancyCoefficients.Coefficient2 * mPoints.GetTemperature(pointIndex);

        // Get height of water at this point
        float const waterHeightAtThisPoint = mParentWorld.GetOceanSurfaceHeightAt(mPoints.GetPosition(pointIndex).x);

        // Check whether we are above or below water
        if (mPoints.GetPosition(pointIndex).y <= waterHeightAtThisPoint)
        {
            // Water
            mPoints.GetNonSpringForce(pointIndex).y +=
                buoyancyPush
                * effectiveWaterDensity;
        }
        else
        {
            // Air
            mPoints.GetNonSpringForce(pointIndex).y +=
                buoyancyPush
                * effectiveAirDensity;
        }


        //
        // Apply water drag - if under water - or wind force - if above water
        //
        // FUTURE: should replace with directional water drag, which acts on frontier points only,
        // proportional to angle between velocity and normal to surface at this point;
        // this would ensure that masses would also have a horizontal velocity component when sinking,
        // providing a "gliding" effect
        //

        if (mPoints.GetPosition(pointIndex).y <= waterHeightAtThisPoint)
        {
            //
            // Note: we would have liked to use the square law:
            //
            //  Drag force = -C * (|V|^2*Vn)
            //
            // But when V >= m / (C * dt) (and also when V = 0), the drag force overcomes
            // the current velocity and thus it accelerates it, resulting in an unstable system.
            // The maximum force is thus -m^2/(C*dt^2).
            //
            // With a linear law, we know that the force will never accelerate the current velocity
            // as long as m > (C * dt) / 2 (~=0.0002), which is a mass we won't have in our system (air is 1.2754).
            //

            // Square law:
            ////mPoints.GetForce(pointIndex) +=
            ////    mPoints.GetVelocity(pointIndex).square()
            ////    * (-waterDragCoefficient);

            // Linear law:
            mPoints.GetNonSpringForce(pointIndex) +=
                mPoints.GetVelocity(pointIndex)
                * (-waterDragCoefficient);
        }
        else
        {
            // Wind force
            //
            // Note: should be based on relative velocity, but we simplify here for performance reasons
            mPoints.GetNonSpringForce(pointIndex) +=
                windForce
                * mPoints.GetMaterialWindReceptivity(pointIndex);
        }
    }
}

void Ship::ApplySpringsForces_BySprings(GameParameters const & /*gameParameters*/)
{
    vec2f const * restrict const pointPositionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f const * restrict const pointVelocityBuffer = mPoints.GetVelocityBufferAsVec2();
    vec2f * restrict const pointSpringForceBuffer = mPoints.GetSpringForceBufferAsVec2();

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
        pointSpringForceBuffer[pointAIndex] += forceA;
        pointSpringForceBuffer[pointBIndex] -= forceA;
    }
}

void Ship::ApplySpringsForces_ByPoints(GameParameters const & gameParameters)
{
    // At the end of this loop:
    //  - We will have changed points' positions to their final values
    //  - We will have changed points' velocities to their final values
    //  - Spring forces will be zero

    vec2f * restrict const pointPositionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict const pointVelocityBuffer = mPoints.GetVelocityBufferAsVec2();
    vec2f * restrict const pointSpringForceBuffer = mPoints.GetSpringForceBufferAsVec2();
    vec2f const * restrict const pointNonSpringForceBuffer = mPoints.GetNonSpringForceBufferAsVec2();
    float const * const restrict pointIntegrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();
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
    float const velocityFactor = globalDampingCoefficient / dt;

    float const * restrict const restLengthBuffer = mSprings.GetRestLengthBuffer();
    Springs::Coefficients const * restrict const coefficientsBuffer = mSprings.GetCoefficientsBuffer();

    ElementCount const pointCount = mPoints.GetElementCount();
    for (ElementIndex pointAIndex = 0; pointAIndex < pointCount; ++pointAIndex)
    {
        //
        // Apply spring forces
        //

        vec2f & pointAPosition = pointPositionBuffer[pointAIndex];
        vec2f & pointAVelocity = pointVelocityBuffer[pointAIndex];
        vec2f & pointASpringForce = pointSpringForceBuffer[pointAIndex];

        auto const & connectedSprings = mPoints.GetConnectedSprings(pointAIndex);

        // Loop for owned springs
        //  - This ensures that point Pi only updates forces of Pj with j > i
        auto const springsCount = connectedSprings.OwnedConnectedSpringsCount;
        for (ElementIndex s = 0; s < springsCount; ++s)
        {
            auto const & connectedSpring = connectedSprings.ConnectedSprings[s];

            auto const pointBIndex = connectedSpring.OtherEndpointIndex;
            assert(pointBIndex > pointAIndex);

            auto const springIndex = connectedSpring.SpringIndex;

            // No need to check whether the spring is deleted, as a deleted spring
            // wouldn't be here
            assert(!mSprings.IsDeleted(springIndex));

            vec2f const displacement = pointPositionBuffer[pointBIndex] - pointAPosition;
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
            vec2f const relVelocity = pointVelocityBuffer[pointBIndex] - pointAVelocity;
            float const fDamp =
                relVelocity.dot(springDir)
                * coefficientsBuffer[springIndex].DampingCoefficient;


            //
            // Apply forces
            //

            vec2f const springForce = springDir * (fSpring + fDamp);
            pointASpringForce += springForce;
            pointSpringForceBuffer[pointBIndex] -= springForce;
        }

        //
        // Integrate total forces at this point
        //

        vec2f const deltaPos =
            pointAVelocity * dt
            + (pointASpringForce + pointNonSpringForceBuffer[pointAIndex]) * pointIntegrationFactorBuffer[pointAIndex * 2];

        pointAPosition += deltaPos;
        pointAVelocity = deltaPos * velocityFactor;

        // Zero out spring force now that we've integrated it
        pointASpringForce = vec2f::zero();
    }
}

void Ship::IntegrateAndResetSpringForces(GameParameters const & gameParameters)
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
    float * const restrict springForceBuffer = mPoints.GetSpringForceBufferAsFloat();
    float const * const restrict nonSpringForceBuffer = mPoints.GetNonSpringForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (springForceBuffer[i] + nonSpringForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring force now that we've integrated it
        springForceBuffer[i] = 0.0f;
    }
}

void Ship::HandleCollisionsWithSeaFloor(GameParameters const & gameParameters)
{
    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();

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
        float const floorHeight = mParentWorld.GetOceanFloorHeightAt(clampedX);
        if (position.y <= floorHeight)
        {
            // Collision!

            //
            // Calculate post-bounce velocity
            //

            vec2f const pointVelocity = mPoints.GetVelocity(pointIndex);

            // Calculate sea floor normal
            // (optimized)
            ////////vec2f const seaFloorAntiNormal = -vec2f(
            ////////    floorHeight - mParentWorld.GetOceanFloorHeightAt(clampedX + 0.01f),
            ////////    0.01f).normalise(); // Points below
            vec2f const seaFloorAntiNormal = vec2f(
                mParentWorld.GetOceanFloorHeightAt(clampedX + 0.01f) - floorHeight,
                -0.01f).normalise(); // Points below

            // Decompose point velocity into normal and tangential
            vec2f const normalVelocity = seaFloorAntiNormal * pointVelocity.dot(seaFloorAntiNormal);
            vec2f const tangentialVelocity = pointVelocity - normalVelocity;

            // Calculate normal reponse: Vn' = -e*Vn (e = elasticity, [0.0 - 1.0])
            vec2f const normalResponse =
                normalVelocity
                * elasticityFactor; // Already negative

            // Calculate tangential response: Vt' = a*Vt (a = (1.0-friction), [0.0 - 1.0])
            vec2f const tangentialResponse =
                tangentialVelocity
                * inverseFriction;

            // Impart final position and velocity
            mPoints.GetPosition(pointIndex) -= pointVelocity * dt; // Move point back to where it was in the previous step
            mPoints.GetVelocity(pointIndex) = normalResponse + tangentialResponse;
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
    for (auto pointIndex : mPoints)
    {
        auto & pos = mPoints.GetPosition(pointIndex);

        if (pos.x < MaxWorldLeft)
        {
            // Simulate bounce, bounded
            pos.x = std::min(MaxWorldLeft + elasticity * (MaxWorldLeft - pos.x), 0.0f);

            // Bounce bounded
            mPoints.GetVelocity(pointIndex).x = std::min(-mPoints.GetVelocity(pointIndex).x, MaxBounceVelocity);
        }
        else if (pos.x > MaxWorldRight)
        {
            // Simulate bounce, bounded
            pos.x = std::max(MaxWorldRight - elasticity * (pos.x - MaxWorldRight), 0.0f);

            // Bounce bounded
            mPoints.GetVelocity(pointIndex).x = std::max(-mPoints.GetVelocity(pointIndex).x, -MaxBounceVelocity);
        }

        if (pos.y > MaxWorldTop)
        {
            // Simulate bounce, bounded
            pos.y = std::max(MaxWorldTop - elasticity * (pos.y - MaxWorldTop), 0.0f);

            // Bounce bounded
            mPoints.GetVelocity(pointIndex).y = std::max(-mPoints.GetVelocity(pointIndex).y, -MaxBounceVelocity);
        }
        else if (pos.y < MaxWorldBottom)
        {
            // Simulate bounce, bounded
            pos.y = std::min(MaxWorldBottom + elasticity * (MaxWorldBottom - pos.y), 0.0f);

            // Bounce bounded
            mPoints.GetVelocity(pointIndex).y = std::min(-mPoints.GetVelocity(pointIndex).y, MaxBounceVelocity);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Water Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateWaterInflow(
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters,
    float & waterTaken)
{
    //
    // Intake/outtake water into/from all the leaking nodes that are either underwater
    // or are overwater and taking rain.
    //
    // Ephemeral points are never leaking, hence we ignore them
    //

    float const rainEquivalentWaterHeight =
        stormParameters.RainQuantity // m/h
        / 3600.0f // -> m/s
        * GameParameters::SimulationStepTimeDuration<float> // -> m/step
        * gameParameters.RainFloodAdjustment;

    for (auto pointIndex : mPoints.RawShipPoints())
    {
        if (mPoints.IsLeaking(pointIndex))
        {
            //
            // 1) Calculate velocity of incoming water, based off Bernoulli's equation applied to point:
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

            // We also incorporate rain in the sources of external water height:
            // - If point is below water surface: external water height is due to depth
            // - If point is above water surface: external water height is due to rain
            float const externalWaterHeight = std::max(
                mParentWorld.GetOceanSurfaceHeightAt(mPoints.GetPosition(pointIndex).x)
                    + 0.1f // Magic number to force flotsam to take some water in and eventually sink
                    - mPoints.GetPosition(pointIndex).y,
                rainEquivalentWaterHeight); // At most is one meter, so does not interfere with underwater pressure

            float const internalWaterHeight = mPoints.GetWater(pointIndex);

            float incomingWaterVelocity;
            if (externalWaterHeight >= internalWaterHeight)
            {
                // Incoming water
                incomingWaterVelocity = sqrtf(2.0f * GameParameters::GravityMagnitude * (externalWaterHeight - internalWaterHeight));
            }
            else
            {
                // Outgoing water
                incomingWaterVelocity = - sqrtf(2.0f * GameParameters::GravityMagnitude * (internalWaterHeight - externalWaterHeight));
            }

            //
            // 2) In/Outtake water according to velocity:
            // - During dt, we move a volume of water Vw equal to A*v*dt; the equivalent change in water
            //   height is thus Vw/A, i.e. v*dt
            //

            float newWater =
                incomingWaterVelocity
                * GameParameters::SimulationStepTimeDuration<float>
                * mPoints.GetMaterialWaterIntake(pointIndex)
                * gameParameters.WaterIntakeAdjustment;

            if (newWater < 0.0f)
            {
                // Outgoing water

                // Make sure we don't over-drain the point
                newWater = -std::min(-newWater, mPoints.GetWater(pointIndex));

                // Honor the water retention of this material
                newWater *= mPoints.GetMaterialWaterRestitution(pointIndex);
            }

            // Adjust water
            mPoints.GetWater(pointIndex) += newWater;

            // Adjust total cumulated intaken water at this point
            mPoints.GetCumulatedIntakenWater(pointIndex) += newWater;

            // Check if it's time to produce air bubbles
            if (mPoints.GetCumulatedIntakenWater(pointIndex) > gameParameters.CumulatedIntakenWaterThresholdForAirBubbles)
            {
                // Generate air bubbles - but not on ropes as that looks awful
                if (gameParameters.DoGenerateAirBubbles
                    && !mPoints.IsRope(pointIndex))
                {
                    GenerateAirBubbles(
                        mPoints.GetPosition(pointIndex),
                        mPoints.GetTemperature(pointIndex),
                        currentSimulationTime,
                        mPoints.GetPlaneId(pointIndex),
                        gameParameters);
                }

                // Consume all cumulated water
                mPoints.GetCumulatedIntakenWater(pointIndex) = 0.0f;
            }

            // Adjust total water taken during step
            waterTaken += newWater;
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
        // 1) Calculate water momenta along all springs connected to this point
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
            vec2f const springNormalizedVector = (mPoints.GetPosition(cs.OtherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise();

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
                mSprings.GetMaterialWaterPermeability(cs.SpringIndex)
                * pointFreenessFactorBufferData[cs.OtherEndpointIndex];

            pointSplashNeighbors += mSprings.GetMaterialWaterPermeability(cs.SpringIndex);
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

            if (mSprings.GetMaterialWaterPermeability(cs.SpringIndex) != 0.0f)
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
                vec2f const springNormalizedVector = (mPoints.GetPosition(cs.OtherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise();

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
        if (wetPointCount > mPoints.GetRawShipPointCount() * 3 / 10) // High watermark
        {
            // Started sinking
            mGameEventHandler->OnSinkingBegin(mId);
            mIsSinking = true;
        }
    }
    else
    {
        if (wetPointCount < mPoints.GetRawShipPointCount() * 1 / 10) // Low watermark
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

    auto & lampPositions = mElectricalElements.GetLampPositionWorkBuffer();
    auto & lampPlaneIds = mElectricalElements.GetLampPlaneIdWorkBuffer();
    auto & lampDistanceCoeffs = mElectricalElements.GetLampDistanceCoefficientWorkBuffer();

    for (ElementIndex l = 0; l < mElectricalElements.GetLampCount(); ++l)
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

    Algorithms::DiffuseLight_Vectorized(
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

    float const waterTemperature = gameParameters.WaterTemperature;

	// We include rain in air
	float const effectiveAirConvectiveHeatTransferCoefficient =
		GameParameters::AirConvectiveHeatTransferCoefficient
		* dt
		* gameParameters.HeatDissipationAdjustment
		+ FastPow(stormParameters.RainDensity, 0.3f) * effectiveWaterConvectiveHeatTransferCoefficient;

    float const airTemperature = gameParameters.AirTemperature;

    // We also include ephemeral points, as they may be heated
    // and have a temperature
    for (auto pointIndex : mPoints)
    {
        // Heat lost in this time quantum (positive when outgoing)
        float heatLost;
        if (mParentWorld.IsUnderwater(mPoints.GetPosition(pointIndex))
            || mPoints.GetWater(pointIndex) > GameParameters::SmotheringWaterHighWatermark)
        {
            // Dissipation in water
            heatLost =
                effectiveWaterConvectiveHeatTransferCoefficient
                * (newPointTemperatureBufferData[pointIndex] - waterTemperature);
        }
        else
        {
            // Dissipation in air
            heatLost =
                effectiveAirConvectiveHeatTransferCoefficient
                * (newPointTemperatureBufferData[pointIndex] - airTemperature);
        }

        // Remove this heat from the point
        newPointTemperatureBufferData[pointIndex] -=
            heatLost
            * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////////////////

void Ship::RotPoints(
    float /*currentSimulationTime*/,
    GameParameters const & gameParameters)
{
    //
    // Rotting is done with a recursive equation:
    //  decay(0) = 1.0
    //  decay(n) = A * decay(n-1), with 0 < A < 1
    //
    // This converges to:
    //  decay(n) = A^n
    //
    // We want full decay (decay=1e-10) after Nf steps when flooded (Nf => 15 minutes @ 50fps):
    //
    //  ZeroDecay = Af ^ Nf
    //

    // After 15 mins: on the surface=>0.75, flooded=>0.25
    float constexpr Nf =
        15.0f * 60.0f * 50.0f / static_cast<float>(LowFrequencyPeriod)
        * 10.0f; // Upping up a bit to fight against initial steep curve

    // Alpha: the smaller, the faster we rot
    float const alphaMax =
        gameParameters.RotAcceler8r != 0.0f
        ? powf(1e-10f, gameParameters.RotAcceler8r / Nf)
        : 1.0f;

    // Leaking points rot faster - they are directly in contact with water after all!
    float const leakingAlphaMax =
        gameParameters.RotAcceler8r != 0.0f
        ? alphaMax * 0.995f
        : 1.0f;

    // Process all non-ephemeral points - no real reason to exclude ephemerals, other
    // than they're not expected to rot
    for (auto p : mPoints.RawShipPoints())
    {
        float waterEquivalent =
            mPoints.GetWater(p)
            + (mParentWorld.IsUnderwater(mPoints.GetPosition(p)) ? 0.175f : 0.0f); // Also rust a bit underwater points, even hull ones

        // Adjust with material's rust receptivity
        waterEquivalent *= mPoints.GetMaterialRustReceptivity(p);

        // Clamp
        waterEquivalent = std::min(waterEquivalent, 1.0f);

        // Interpolate alpha
        float const alpha = Mix(
            1.0f,
            (mPoints.IsLeaking(p) ? leakingAlphaMax : alphaMax),
            waterEquivalent);

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

void Ship::GenerateAirBubbles(
    vec2f const & position,
    float temperature,
    float currentSimulationTime,
    PlaneId planeId,
    GameParameters const & /*gameParameters*/)
{
    float vortexAmplitude = GameRandomEngine::GetInstance().GenerateUniformReal(
        GameParameters::MinAirBubblesVortexAmplitude, GameParameters::MaxAirBubblesVortexAmplitude);
    float vortexPeriod = GameRandomEngine::GetInstance().GenerateUniformReal(
        GameParameters::MinAirBubblesVortexPeriod, GameParameters::MaxAirBubblesVortexPeriod);

    mPoints.CreateEphemeralParticleAirBubble(
        position,
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
                mPoints.GetPosition(pointElementIndex),
                velocity,
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
                mSprings.GetMidpointPosition(springElementIndex, mPoints),
                vec2f::fromPolar(velocityMagnitude, velocityAngleCw),
                mSprings.GetBaseStructuralMaterial(springElementIndex),
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
			mPoints.GetPosition(pointElementIndex),
			vec2f::fromPolar(velocityMagnitude, velocityAngleCw),
			mPoints.GetStructuralMaterial(pointElementIndex),
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
        assert(!mElectricalElements.IsDeleted(electricalElementIndex));
        assert(mElectricalElements.GetConnectedElectricalElements(electricalElementIndex).empty());
        assert(mElectricalElements.GetConductingConnectedElectricalElements(electricalElementIndex).empty());

        mElectricalElements.Destroy(electricalElementIndex);

        hasAnythingBeenDestroyed = true;
    }

    if (hasAnythingBeenDestroyed)
    {
        // Notify bombs
        mBombs.OnPointDetached(pointElementIndex);

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

    // Remove spring from set of sub springs at each super-triangle
    for (auto superTriangleIndex : mSprings.GetSuperTriangles(springElementIndex))
    {
        mTriangles.RemoveSubSpring(superTriangleIndex, springElementIndex);
    }

    // Remove the spring from its endpoints
    mPoints.DisconnectSpring(pointAIndex, springElementIndex, pointBIndex);
    mPoints.DisconnectSpring(pointBIndex, springElementIndex, pointAIndex);

    // Notify endpoints that have become orphaned
    if (mPoints.GetConnectedSprings(pointAIndex).ConnectedSprings.empty())
        mPoints.OnOrphaned(pointAIndex);
    if (mPoints.GetConnectedSprings(pointBIndex).ConnectedSprings.empty())
        mPoints.OnOrphaned(pointBIndex);


    //
    // Remove other elements from self
    //

    mSprings.ClearSuperTriangles(springElementIndex);


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
                electricalElementBIndex);

            mElectricalElements.RemoveConnectedElectricalElement(
                electricalElementBIndex,
                electricalElementAIndex);
        }
    }

    //
    // Misc
    //

    // Notify bombs
    mBombs.OnSpringDestroyed(springElementIndex);

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

    // Add spring to set of sub springs at each super-triangle
    for (auto superTriangleIndex : mSprings.GetSuperTriangles(springElementIndex))
    {
        mTriangles.AddSubSpring(superTriangleIndex, springElementIndex);
    }

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
    mGameEventHandler->OnSpringRepaired(
        mPoints.GetStructuralMaterial(mSprings.GetEndpointAIndex(springElementIndex)),
        mParentWorld.IsUnderwater(mPoints.GetPosition(mSprings.GetEndpointAIndex(springElementIndex))),
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

    // Remove triangle from set of super triangles of its sub springs
    for (ElementIndex subSpringIndex : mTriangles.GetSubSprings(triangleElementIndex))
    {
        mSprings.RemoveSuperTriangle(subSpringIndex, triangleElementIndex);
    }

    // Disconnect triangle from its endpoints
    mPoints.DisconnectTriangle(mTriangles.GetPointAIndex(triangleElementIndex), triangleElementIndex, true); // Owner
    mPoints.DisconnectTriangle(mTriangles.GetPointBIndex(triangleElementIndex), triangleElementIndex, false); // Not owner
    mPoints.DisconnectTriangle(mTriangles.GetPointCIndex(triangleElementIndex), triangleElementIndex, false); // Not owner

    //
    // Remove other elements from self
    //

    mTriangles.ClearSubSprings(triangleElementIndex);


    // Remember our structure is now dirty
    mIsStructureDirty = true;

    // Update count of broken triangles
    ++mBrokenTrianglesCount;
}

void Ship::HandleTriangleRestore(ElementIndex triangleElementIndex)
{
    //
    // Add others to self
    //

    // Restore factory subsprings
    mTriangles.RestoreFactorySubSprings(triangleElementIndex);


    //
    // Add self to others
    //

    // Connect triangle to its endpoints
    mPoints.ConnectTriangle(mTriangles.GetPointAIndex(triangleElementIndex), triangleElementIndex, true); // Owner
    mPoints.ConnectTriangle(mTriangles.GetPointBIndex(triangleElementIndex), triangleElementIndex, false); // Not owner
    mPoints.ConnectTriangle(mTriangles.GetPointCIndex(triangleElementIndex), triangleElementIndex, false); // Not owner

    // Add triangle to set of super triangles of its sub springs
    assert(!mTriangles.GetSubSprings(triangleElementIndex).empty());
    for (ElementIndex subSpringIndex : mTriangles.GetSubSprings(triangleElementIndex))
    {
        mSprings.AddSuperTriangle(subSpringIndex, triangleElementIndex);
    }

    // Fire event - using point A's properties (quite arbitrarily)
    mGameEventHandler->OnTriangleRepaired(
        mPoints.GetStructuralMaterial(mTriangles.GetPointAIndex(triangleElementIndex)),
        mParentWorld.IsUnderwater(mPoints.GetPosition(mTriangles.GetPointAIndex(triangleElementIndex))),
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

        mElectricalElements.RemoveConnectedElectricalElement(electricalElementIndex, connectedElectricalElementIndex);
        mElectricalElements.RemoveConnectedElectricalElement(connectedElectricalElementIndex, electricalElementIndex);
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
    float blastStrength,
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
            blastStrength,
            blastHeat,
            explosionType));
}

void Ship::DoAntiMatterBombPreimplosion(
    vec2f const & centerPosition,
    float sequenceProgress,
    GameParameters const & gameParameters)
{
    float const strength =
        100000.0f
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    // Apply the force field
    ApplyRadialSpaceWarpForceField(
        centerPosition,
        7.0f + sequenceProgress * 100.0f,
        10.0f,
        strength);
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
    }
}

#ifdef _DEBUG
void Ship::VerifyInvariants()
{
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
                Verify(mTriangles.GetSubSprings(superTriangle).contains(s));
            }
        }
        else
        {
            Verify(mSprings.GetSuperTriangles(s).empty());
        }
    }

    for (auto t : mTriangles)
    {
        Verify(mTriangles.GetSubSprings(t).size() <= 4);

        for (auto subSpring : mTriangles.GetSubSprings(t))
        {
            Verify(mSprings.GetSuperTriangles(subSpring).contains(t));
        }
    }
}
#endif
}