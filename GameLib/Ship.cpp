/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "GameDebug.h"
#include "GameMath.h"
#include "GameRandomEngine.h"
#include "Log.h"
#include "Materials.h"
#include "Segment.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <queue>
#include <set>

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
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    MaterialDatabase const & materialDatabase,
    Points && points,
    Springs && springs,
    Triangles && triangles,
    ElectricalElements && electricalElements,
    VisitSequenceNumber currentVisitSequenceNumber)
    : mId(id)
    , mParentWorld(parentWorld)
    , mGameEventHandler(std::move(gameEventHandler))
    , mMaterialDatabase(materialDatabase)
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mConnectedComponentSizes()
    , mAreElementsDirty(true)
    , mLastShipRenderMode()
    , mIsSinking(false)
    , mTotalWater(0.0)
    , mWaterSplashedRunningAverage()
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
    , mCurrentForceFields()
{
    // Set destroy handlers
    mPoints.RegisterDestroyHandler(std::bind(&Ship::PointDestroyHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    mSprings.RegisterDestroyHandler(std::bind(&Ship::SpringDestroyHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    mTriangles.RegisterDestroyHandler(std::bind(&Ship::TriangleDestroyHandler, this, std::placeholders::_1));
    mElectricalElements.RegisterDestroyHandler(std::bind(&Ship::ElectricalElementDestroyHandler, this, std::placeholders::_1));

    // Do a first connected component detection pass
    DetectConnectedComponents(currentVisitSequenceNumber);
}

Ship::~Ship()
{
}

void Ship::MoveBy(
    vec2f const & offset,
    GameParameters const & gameParameters)
{
    vec2f const velocity =
        offset
        * gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f * restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    size_t const count = mPoints.GetBufferElementCount();
    for (size_t p = 0; p < count; ++p)
    {
        positionBuffer[p] += offset;
        velocityBuffer[p] = velocity;
    }
}

void Ship::RotateBy(
    float angle,
    vec2f const & center,
    GameParameters const & gameParameters)
{
    float const inertia =
        gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f const rotX(cos(angle), sin(angle));
    vec2f const rotY(-sin(angle), cos(angle));

    vec2f * restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    size_t const count = mPoints.GetBufferElementCount();
    for (size_t p = 0; p < count; ++p)
    {
        vec2f pos = positionBuffer[p] - center;
        pos = vec2f(pos.dot(rotX), pos.dot(rotY)) + center;

        velocityBuffer[p] = (pos - positionBuffer[p]) * inertia;
        positionBuffer[p] = pos;
    }
}

void Ship::DestroyAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    float const radius =
        gameParameters.DestroyRadius
        * radiusMultiplier
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    float const squareRadius = radius * radius;

    // Destroy all points within the radius
    for (auto pointIndex : mPoints)
    {
        // The only ephemeral points we allow to delete are air bubbles
        if (!mPoints.IsDeleted(pointIndex)
            && (Points::EphemeralType::None == mPoints.GetEphemeralType(pointIndex)
                   || Points::EphemeralType::AirBubble == mPoints.GetEphemeralType(pointIndex)))
        {
            if ((mPoints.GetPosition(pointIndex) - targetPos).squareLength() < squareRadius)
            {
                // Destroy point
                mPoints.Destroy(
                    pointIndex,
                    currentSimulationTime,
                    gameParameters);
            }
        }
    }
}

void Ship::SawThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Find all springs that intersect the saw segment
    //

    unsigned int metalsSawed = 0;
    unsigned int nonMetalsSawed = 0;

    for (auto springIndex : mSprings)
    {
        if (!mSprings.IsDeleted(springIndex))
        {
            if (Geometry::Segment::ProperIntersectionTest(
                startPos,
                endPos,
                mSprings.GetPointAPosition(springIndex, mPoints),
                mSprings.GetPointBPosition(springIndex, mPoints)))
            {
                // Destroy spring
                mSprings.Destroy(
                    springIndex,
                    Springs::DestroyOptions::FireBreakEvent
                    | Springs::DestroyOptions::DestroyOnlyConnectedTriangle,
                    currentSimulationTime,
                    gameParameters,
                    mPoints);

                bool const isMetal =
                    mSprings.GetBaseStructuralMaterial(springIndex).MaterialSound == StructuralMaterial::MaterialSoundType::Metal;

                if (isMetal)
                {
                    // Emit sparkles
                    GenerateSparkles(
                        springIndex,
                        startPos,
                        endPos,
                        currentSimulationTime,
                        gameParameters);
                }

                // Remember we have sawed this material
                if (isMetal)
                    metalsSawed++;
                else
                    nonMetalsSawed++;
            }
        }
    }

    // Notify (including zero)
    mGameEventHandler->OnSawed(true, metalsSawed);
    mGameEventHandler->OnSawed(false, nonMetalsSawed);
}

void Ship::DrawTo(
    vec2f const & targetPos,
    float strength,
    GameParameters const & gameParameters)
{
    // Store the force field
    mCurrentForceFields.emplace_back(
        new DrawForceField(
            targetPos,
            strength * (gameParameters.IsUltraViolentMode ? 20.0f : 1.0f)));
}

void Ship::SwirlAt(
    vec2f const & targetPos,
    float strength,
    GameParameters const & gameParameters)
{
    // Store the force field
    mCurrentForceFields.emplace_back(
        new SwirlForceField(
            targetPos,
            strength * (gameParameters.IsUltraViolentMode ? 40.0f : 1.0f)));
}

bool Ship::TogglePinAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mPinnedPoints.ToggleAt(
        targetPos,
        gameParameters);
}

bool Ship::InjectBubblesAt(
    vec2f const & targetPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (targetPos.y < mParentWorld.GetWaterHeightAt(targetPos.x))
    {
        GenerateAirBubbles(
            targetPos,
            currentSimulationTime,
            NoneConnectedComponentId, // FUTURE: use mMaxConnectedComponentId/ZPlane
            gameParameters);

        return true;
    }
    else
    {
        return false;
    }
}

bool Ship::FloodAt(
    vec2f const & targetPos,
    float waterQuantityMultiplier,
    float searchRadius,
    GameParameters const & gameParameters)
{
    float const quantityOfWater =
        gameParameters.FloodQuantityOfWater
        * waterQuantityMultiplier
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    // Find the closest point
    float const searchSquareRadius = searchRadius * searchRadius;
    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        if (!mPoints.IsDeleted(pointIndex)
            && !mPoints.IsHull(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < searchSquareRadius && squareDistance < bestSquareDistance)
            {
                bestPointIndex = pointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    if (NoneElementIndex != bestPointIndex)
    {
        if (quantityOfWater >= 0.0f)
            mPoints.GetWater(bestPointIndex) += quantityOfWater;
        else
            mPoints.GetWater(bestPointIndex) -= std::min(-quantityOfWater, mPoints.GetWater(bestPointIndex));

        return true;
    }
    else
    {
        // No luck
        return false;
    }
}

bool Ship::ToggleAntiMatterBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleAntiMatterBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleImpactBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleImpactBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleRCBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleRCBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleTimerBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleTimerBombAt(
        targetPos,
        gameParameters);
}

void Ship::DetonateRCBombs()
{
    mBombs.DetonateRCBombs();
}

void Ship::DetonateAntiMatterBombs()
{
    mBombs.DetonateAntiMatterBombs();
}

ElementIndex Ship::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    float const squareRadius = radius * radius;

    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < squareRadius && squareDistance < bestSquareDistance)
            {
                bestPointIndex = pointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    return bestPointIndex;
}

bool Ship::QueryNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    float const squareRadius = radius * radius;

    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < squareRadius && squareDistance < bestSquareDistance)
            {
                bestPointIndex = pointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    if (NoneElementIndex != bestPointIndex)
    {
        mPoints.Query(bestPointIndex);
        return true;
    }

    return false;
}

void Ship::Update(
    float currentSimulationTime,
    VisitSequenceNumber currentVisitSequenceNumber,
    GameParameters const & gameParameters,
    Render::RenderContext const & renderContext)
{
    auto const currentWallClockTime = GameWallClock::GetInstance().Now();

#ifdef _DEBUG
    VerifyInvariants();
#endif

    //
    // Process eventual parameter changes
    //

    mPoints.UpdateGameParameters(
        gameParameters);

    mSprings.UpdateGameParameters(
        gameParameters,
        mPoints);


    //
    // Update mechanical dynamics
    //

    UpdateMechanicalDynamics(
        currentSimulationTime,
        gameParameters,
        renderContext);


    //
    // Update bombs
    //
    // Might cause explosions; might cause points to be destroyed
    // (which would flag our elements as dirty)
    //

    mBombs.Update(
        currentWallClockTime,
        gameParameters);


    //
    // Update strain for all springs; might cause springs to break
    // (which would flag our elements as dirty)
    //

    mSprings.UpdateStrains(
        currentSimulationTime,
        gameParameters,
        mPoints);


    //
    // Detect connected components, if there have been any deletions
    //

    if (mAreElementsDirty)
    {
        DetectConnectedComponents(currentVisitSequenceNumber);
    }


    //
    // Update water dynamics
    //

    UpdateWaterDynamics(
        currentSimulationTime,
        gameParameters);


    //
    // Update electrical dynamics
    //

    UpdateElectricalDynamics(
        currentWallClockTime,
        currentVisitSequenceNumber,
        gameParameters);


    //
    // Update ephemeral particles
    //

    UpdateEphemeralParticles(
        currentSimulationTime,
        gameParameters);

#ifdef _DEBUG
    VerifyInvariants();
#endif
}

void Ship::Render(
    GameParameters const & /*gameParameters*/,
    Render::RenderContext & renderContext)
{
    //
    // Initialize render
    //

    renderContext.RenderShipStart(
        mId,
        mConnectedComponentSizes);


    //
    // Upload points's mutable attributes
    //

    mPoints.Upload(
        mId,
        renderContext);


    //
    // Upload elements
    //

    if (!mConnectedComponentSizes.empty())
    {
        //
        // Upload elements (point (elements), springs, ropes, triangles), iff dirty
        // or the ship render mode has changed
        //

        if (mAreElementsDirty
            || !mLastShipRenderMode
            || *mLastShipRenderMode != renderContext.GetShipRenderMode())
        {
            renderContext.UploadShipElementsStart(mId);

            //
            // Upload all the point elements
            //

            mPoints.UploadElements(
                mId,
                renderContext);

            //
            // Upload all the spring elements (including ropes)
            //

            mSprings.UploadElements(
                mId,
                renderContext,
                mPoints);

            //
            // Upload all the triangle elements
            //

            mTriangles.UploadElements(
                mId,
                renderContext,
                mPoints);

            renderContext.UploadShipElementsEnd(mId);
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
                renderContext,
                mPoints);
        }

        renderContext.UploadShipElementStressedSpringsEnd(mId);

        // Reset state
        mAreElementsDirty = false;
        mLastShipRenderMode = renderContext.GetShipRenderMode();
    }


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
    // Upload ephemeral points
    //

    mPoints.UploadEphemeralParticles(
        mId,
        renderContext);

    //
    // Upload point vectors
    //

    mPoints.UploadVectors(
        mId,
        renderContext);

    //
    // Finalize render
    //

    renderContext.RenderShipEnd(mId);
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// Mechanical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateMechanicalDynamics(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    Render::RenderContext const & renderContext)
{
    //
    // 1. Recalculate total masses and everything else that derives from them, once and for all
    //

    mPoints.UpdateTotalMasses(gameParameters);

    //
    // 2. Run iterations
    //

    int const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<int>();

    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // Apply force fields - if we have any
        for (auto const & forceField : mCurrentForceFields)
        {
            forceField->Apply(
                mPoints,
                currentSimulationTime,
                gameParameters);
        }

        // Update point forces
        UpdatePointForces(gameParameters);

        // Update springs forces
        UpdateSpringForces(gameParameters);

        // Check whether we need to save the last force buffer before we zero it out
        if (iter == numMechanicalDynamicsIterations - 1
            && VectorFieldRenderMode::PointForce == renderContext.GetVectorFieldRenderMode())
        {
            mPoints.CopyForceBufferToForceRenderBuffer();
        }

        // Integrate and reset forces to zero
        IntegrateAndResetPointForces(gameParameters);

        // Handle collisions with sea floor
        HandleCollisionsWithSeaFloor(gameParameters);
    }

    // Consume force fields
    mCurrentForceFields.clear();
}

void Ship::UpdatePointForces(GameParameters const & gameParameters)
{
    float const densityAdjustedWaterMass = GameParameters::WaterMass * gameParameters.WaterDensityAdjustment;

    // Underwater points feel this amount of water drag
    //
    // The higher the value, the more viscous the water looks when a body moves through it
    float const waterDragCoefficient =
        0.020f // ~= 1.0f - powf(0.6f, 0.02f)
        * gameParameters.WaterDragAdjustment;

    for (auto pointIndex : mPoints)
    {
        // Get height of water at this point
        float const waterHeightAtThisPoint = mParentWorld.GetWaterHeightAt(mPoints.GetPosition(pointIndex).x);

        //
        // 1. Add gravity and buoyancy
        //

        mPoints.GetForce(pointIndex) +=
            gameParameters.Gravity
            * mPoints.GetTotalMass(pointIndex);

        if (mPoints.GetPosition(pointIndex).y < waterHeightAtThisPoint)
        {
            //
            // Apply upward push of water mass (i.e. buoyancy!)
            //

            mPoints.GetForce(pointIndex) -=
                gameParameters.Gravity
                * mPoints.GetWaterVolumeFill(pointIndex)
                * densityAdjustedWaterMass;
        }


        //
        // 2. Apply water drag
        //
        // FUTURE: should replace with directional water drag, which acts on frontier points only,
        // proportional to angle between velocity and normal to surface at this point;
        // this would ensure that masses would also have a horizontal velocity component when sinking,
        // providing a "gliding" effect
        //

        if (mPoints.GetPosition(pointIndex).y < waterHeightAtThisPoint)
        {
            // Drag force = -C*V^2*Vn
            mPoints.GetForce(pointIndex) +=
                mPoints.GetVelocity(pointIndex).square()
                * (-waterDragCoefficient);
        }
    }
}

void Ship::UpdateSpringForces(GameParameters const & /*gameParameters*/)
{
    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

        // No need to check whether the spring is deleted, as a deleted spring
        // has zero coefficients

        vec2f const displacement = mPoints.GetPosition(pointBIndex) - mPoints.GetPosition(pointAIndex);
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        vec2f const fSpringA =
            springDir
            * (displacementLength - mSprings.GetRestLength(springIndex))
            * mSprings.GetStiffnessCoefficient(springIndex);


        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = mPoints.GetVelocity(pointBIndex) - mPoints.GetVelocity(pointAIndex);
        vec2f const fDampA =
            springDir
            * relVelocity.dot(springDir)
            * mSprings.GetDampingCoefficient(springIndex);


        //
        // Apply forces
        //

        mPoints.GetForce(pointAIndex) += fSpringA + fDampA;
        mPoints.GetForce(pointBIndex) -= fSpringA + fDampA;
    }
}

void Ship::IntegrateAndResetPointForces(GameParameters const & gameParameters)
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

    float const globalDampCoefficient = pow(
        GameParameters::GlobalDamp,
        12.0f / gameParameters.NumMechanicalDynamicsIterations<float>());

    //
    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible
    //
    // This loop is compiled with single-precision packet SSE instructions on MSVC 17,
    // integrating two points at each iteration
    //

    float * restrict positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float * restrict forceBuffer = mPoints.GetForceBufferAsFloat();
    float * restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos = velocityBuffer[i] * dt + forceBuffer[i] * integrationFactorBuffer[i];
        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * globalDampCoefficient / dt;

        // Zero out force now that we've integrated it
        forceBuffer[i] = 0.0f;
    }
}

void Ship::HandleCollisionsWithSeaFloor(GameParameters const & gameParameters)
{
    //
    // We handle collisions really simplistically: we move back points to where they were
    // at the last update, when they were NOT under the ocean floor, and fully bounce velocity back.
    //
    // Regarding calculating the post-collision position: ideally we would have to find the
    // mid-point - between the position at t-1 and t - at which we really entered the sea floor,
    // and then move the point there. We could find the midpoint with successive approximations,
    // but this might not work when the floor is really rugged.
    //
    // Regarding calculating the post-collision velocity: ideally we would mirror velocity around
    // the sea floor normal, but if we did this together with moving the point at the previous position,
    // that point would start oscillating up and down, as the new position would allow it to gather
    // momentum and come crashing down again.
    //
    // Hence we're gonna stick with this simple algorithm.
    //

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();

    for (auto pointIndex : mPoints)
    {
        // Check if point is now below the sea floor
        float const floorheight = mParentWorld.GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x);
        if (mPoints.GetPosition(pointIndex).y < floorheight)
        {
            // Move point back to where it was
            mPoints.GetPosition(pointIndex) -= mPoints.GetVelocity(pointIndex) * dt;

            // Bounce velocity (naively)
            mPoints.GetVelocity(pointIndex) = -mPoints.GetVelocity(pointIndex);

            // Add a small normal component, so to have some non-infinite friction
            vec2f seaFloorNormal = vec2f(
                floorheight - mParentWorld.GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x + 0.01f),
                0.01f).normalise();
            mPoints.GetVelocity(pointIndex) += seaFloorNormal * 0.5f;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Water Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateWaterDynamics(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Update intake of water
    //

    float waterTakenInStep = 0.f;

    UpdateWaterInflow(
        currentSimulationTime,
        gameParameters,
        waterTakenInStep);

    // Notify
    mGameEventHandler->OnWaterTaken(waterTakenInStep);


    //
    // Diffuse water
    //

    float waterSplashedInStep = 0.f;
    UpdateWaterVelocities(gameParameters, waterSplashedInStep);

    // Notify
    mGameEventHandler->OnWaterSplashed(waterSplashedInStep);


    //
    // Update total water taken and check whether we've started sinking
    //

    mTotalWater += waterTakenInStep;
    if (!mIsSinking
        && mTotalWater > static_cast<float>(mPoints.GetElementCount()) / 1.5f)
    {
        // Started sinking
        mGameEventHandler->OnSinkingBegin(mId);
        mIsSinking = true;
    }
}

void Ship::UpdateWaterInflow(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    float & waterTaken)
{
    //
    // Intake/outtake water into/from all the leaking nodes that are underwater
    //

    for (auto pointIndex : mPoints)
    {
        // Avoid taking water into points that are destroyed, as that would change total water taken
        if (!mPoints.IsDeleted(pointIndex))
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

                float const externalWaterHeight = std::max(
                    mParentWorld.GetWaterHeightAt(mPoints.GetPosition(pointIndex).x) - mPoints.GetPosition(pointIndex).y,
                    0.0f);

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
                    * gameParameters.WaterIntakeAdjustment;

                if (newWater < 0.0f)
                {
                    // Outgoing water

                    // Make sure we don't over-drain the point
                    newWater = -std::min(-newWater, mPoints.GetWater(pointIndex));

                    // Honor the water retention of this material
                    newWater *= mPoints.GetWaterRestitution(pointIndex);
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
                            currentSimulationTime,
                            mPoints.GetConnectedComponentId(pointIndex),
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
}

void Ship::UpdateWaterVelocities(
    GameParameters const & gameParameters,
    float & waterSplashed)
{
    //
    // For each point, move each spring's outgoing water momentum to
    // its destination point
    //
    // Implementation of https://gabrielegiuseppini.wordpress.com/2018/09/08/momentum-based-simulation-of-water-flooding-2d-spaces/
    //

    // Calculate water momenta
    mPoints.UpdateWaterMomentaFromVelocities();

    // Source and result water buffers
    float * restrict oldPointWaterBufferData = mPoints.GetWaterBufferAsFloat();
    auto newPointWaterBuffer = mPoints.MakeWaterBufferCopy();
    float * restrict newPointWaterBufferData = newPointWaterBuffer->data();
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
    for (auto pointIndex : mPoints)
    {
        pointFreenessFactorBufferData[pointIndex] =
            FastExp(-oldPointWaterBufferData[pointIndex] * 10.0f);
    }


    //
    // Visit all points and move water and its momenta
    //

    for (auto pointIndex : mPoints)
    {
        //
        // 1) Calculate water momenta along all springs
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

        for (size_t s = 0; s < mPoints.GetConnectedSprings(pointIndex).size(); ++s)
        {
            auto const springIndex = mPoints.GetConnectedSprings(pointIndex)[s];

            auto const otherEndpointIndex = mSprings.GetOtherEndpointIndex(springIndex, pointIndex);

            // Normalized spring vector, oriented point -> other endpoint
            vec2f const springNormalizedVector = (mPoints.GetPosition(otherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise();

            // Component of the point's own water velocity along the spring
            float const pointWaterVelocityAlongSpring =
                oldPointWaterVelocityBufferData[pointIndex]
                .dot(springNormalizedVector);

            //
            // Calulate Bernoulli's velocity gained along this spring, from this point to
            // the other endpoint
            //

            // Pressure difference (positive implies point -> other endpoint flow)
            float const dw = oldPointWaterBufferData[pointIndex] - oldPointWaterBufferData[otherEndpointIndex];

            // Gravity potential difference (positive implies point -> other endpoint flow)
            float const dy = mPoints.GetPosition(pointIndex).y - mPoints.GetPosition(otherEndpointIndex).y;

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
            // diagonalsprings
            springOutboundWaterFlowWeights[s] =
                springOutboundScalarWaterVelocity
                / mSprings.GetRestLength(springIndex);

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
                mSprings.GetWaterPermeability(springIndex)
                * pointFreenessFactorBufferData[otherEndpointIndex];

            pointSplashNeighbors += mSprings.GetWaterPermeability(springIndex);
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
                * mPoints.GetWaterDiffusionSpeed(pointIndex)
                * gameParameters.WaterDiffusionSpeedAdjustment
                / totalOutboundWaterFlowWeight;
        }


        //
        // 3) Move water along all springs according to their flows,
        //    and update destination's momenta accordingly
        //

        for (size_t s = 0; s < mPoints.GetConnectedSprings(pointIndex).size(); ++s)
        {
            auto const springIndex = mPoints.GetConnectedSprings(pointIndex)[s];

            auto const otherEndpointIndex = mSprings.GetOtherEndpointIndex(
                springIndex,
                pointIndex);

            // Calculate quantity of water directed outwards
            float const springOutboundQuantityOfWater =
                springOutboundWaterFlowWeights[s]
                * waterQuantityNormalizationFactor;

            assert(springOutboundQuantityOfWater >= 0.0f);

            if (mSprings.GetWaterPermeability(springIndex) != 0.0f)
            {
                //
                // Water - and momentum - move from point to endpoint
                //

                // Move water quantity
                newPointWaterBufferData[pointIndex] -= springOutboundQuantityOfWater;
                newPointWaterBufferData[otherEndpointIndex] += springOutboundQuantityOfWater;

                // Remove "old momentum" (old velocity) from point
                newPointWaterMomentumBufferData[pointIndex] -=
                    oldPointWaterVelocityBufferData[pointIndex]
                    * springOutboundQuantityOfWater;

                // Add "new momentum" (old velocity + velocity gained) to other endpoint
                newPointWaterMomentumBufferData[otherEndpointIndex] +=
                    springOutboundWaterVelocities[s]
                    * springOutboundQuantityOfWater;


                //
                // Update point's kinetic energy loss:
                // splintered water colliding with whole other endpoint
                //

                // FUTURE: get rid of this re-calculation once we pre-calculate all spring normalized vectors
                vec2f const springNormalizedVector = (mPoints.GetPosition(otherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise();

                float ma = springOutboundQuantityOfWater;
                float va = springOutboundWaterVelocities[s].length();
                float mb = oldPointWaterBufferData[otherEndpointIndex];
                float vb = oldPointWaterVelocityBufferData[otherEndpointIndex].dot(springNormalizedVector);

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
                assert(!mSprings.IsDeleted(springIndex));

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
    // Move result values back to point, transforming momenta into velocities
    //

    mPoints.UpdateWaterBuffer(std::move(newPointWaterBuffer));
    mPoints.UpdateWaterVelocitiesFromMomenta();
}

///////////////////////////////////////////////////////////////////////////////////
// Electrical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateElectricalDynamics(
    GameWallClock::time_point currentWallclockTime,
    VisitSequenceNumber currentVisitSequenceNumber,
    GameParameters const & gameParameters)
{
    // Invoked regardless of dirty elements, as generators might become wet
    UpdateElectricalConnectivity(currentVisitSequenceNumber);

    mElectricalElements.Update(
        currentWallclockTime,
        currentVisitSequenceNumber,
        mPoints,
        gameParameters);

    DiffuseLight(gameParameters);
}

void Ship::UpdateElectricalConnectivity(VisitSequenceNumber currentVisitSequenceNumber)
{
    //
    // Visit electrical graph starting from (non-wet) generators, and propagate
    // visit sequence number
    //

    std::queue<ElementIndex> electricalElementsToVisit;

    for (auto generatorIndex : mElectricalElements.Generators())
    {
        // Do not visit deleted generators
        if (!mElectricalElements.IsDeleted(generatorIndex))
        {
            // Make sure we haven't visited it already
            if (currentVisitSequenceNumber != mElectricalElements.GetCurrentConnectivityVisitSequenceNumber(generatorIndex))
            {
                // Mark it as visited
                mElectricalElements.SetConnectivityVisitSequenceNumber(
                    generatorIndex,
                    currentVisitSequenceNumber);

                // Check if dry enough
                if (!mPoints.IsWet(mElectricalElements.GetPointIndex(generatorIndex), 0.3f))
                {
                    // Add generator to queue
                    assert(electricalElementsToVisit.empty());
                    electricalElementsToVisit.push(generatorIndex);

                    // Visit all electrical elements reachable from this generator
                    while (!electricalElementsToVisit.empty())
                    {
                        auto e = electricalElementsToVisit.front();
                        electricalElementsToVisit.pop();

                        assert(currentVisitSequenceNumber == mElectricalElements.GetCurrentConnectivityVisitSequenceNumber(e));

                        for (auto reachableElectricalElementIndex : mElectricalElements.GetConnectedElectricalElements(e))
                        {
                            assert(!mElectricalElements.IsDeleted(reachableElectricalElementIndex));

                            // Make sure not visited already
                            if (currentVisitSequenceNumber != mElectricalElements.GetCurrentConnectivityVisitSequenceNumber(reachableElectricalElementIndex))
                            {
                                // Add to queue
                                electricalElementsToVisit.push(reachableElectricalElementIndex);

                                // Mark it as visited
                                mElectricalElements.SetConnectivityVisitSequenceNumber(
                                    reachableElectricalElementIndex,
                                    currentVisitSequenceNumber);
                            }
                        }
                    }
                }
            }
        }
    }
}

void Ship::DiffuseLight(GameParameters const & gameParameters)
{
    //
    // Diffuse light from each lamp to all connected (i.e. spring-connected) points,
    // inverse-proportionally to the nth power of the distance, where n is the spread
    //

    // Zero-out light at all points first
    for (auto pointIndex : mPoints)
    {
        // Zero its light
        mPoints.GetLight(pointIndex) = 0.0f;
    }

    // Go through all lamps;
    // can safely visit deleted lamps as their current will always be zero
    for (auto lampIndex : mElectricalElements.Lamps())
    {
        auto const lampPointIndex = mElectricalElements.GetPointIndex(lampIndex);

        float const effectiveLampLight =
            mElectricalElements.GetAvailableCurrent(lampIndex)
            * mElectricalElements.GetLuminiscence(lampIndex)
            * gameParameters.LuminiscenceAdjustment;

        float const lampLightSpread = mElectricalElements.GetLightSpread(lampIndex);
        if (lampLightSpread == 0.0f)
        {
            // No spread, just the lamp point itself
            mPoints.GetLight(lampPointIndex) = effectiveLampLight;
        }
        else
        {
            // Spread light to all the points in the same connected component

            float const effectiveExponent =
                (1.0f / lampLightSpread)
                * gameParameters.LightSpreadAdjustment
                / 2.0f; // We piggyback on the power to avoid taking a sqrt for distance

            vec2f const & lampPosition = mPoints.GetPosition(lampPointIndex);
            ConnectedComponentId const lampConnectedComponentId = mPoints.GetConnectedComponentId(lampPointIndex);

            for (auto pointIndex : mPoints)
            {
                if (mPoints.GetConnectedComponentId(pointIndex) == lampConnectedComponentId)
                {
                    float const squareDistance = (mPoints.GetPosition(pointIndex) - lampPosition).squareLength();

                    float const newLight =
                        effectiveLampLight
                        / (1.0f + FastPow(squareDistance, effectiveExponent));

                    if (newLight > mPoints.GetLight(pointIndex))
                        mPoints.GetLight(pointIndex) = newLight;
                }
            }
        }
    }
}

void Ship::UpdateEphemeralParticles(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // 1. Update existing particles
    //

    mPoints.UpdateEphemeralParticles(
        currentSimulationTime,
        gameParameters);


    //
    // 2. Emit new particles
    //

    // FUTURE: when we have emitters
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////////////////////////////////////////

void Ship::DetectConnectedComponents(VisitSequenceNumber currentVisitSequenceNumber)
{
    mConnectedComponentSizes.clear();

    ConnectedComponentId currentConnectedComponentId = 0;
    std::queue<ElementIndex> pointsToVisitForConnectedComponents;

    // Visit all non-ephemeral points, or we run the risk of creating a zillion connected components
    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        // Don't visit destroyed points, or we run the risk of creating a zillion connected components
        if (!mPoints.IsDeleted(pointIndex))
        {
            // Check if visited
            if (mPoints.GetCurrentConnectedComponentDetectionVisitSequenceNumber(pointIndex) != currentVisitSequenceNumber)
            {
                // This node has not been visited, hence it's the beginning of a new connected component
                ++currentConnectedComponentId;
                size_t pointsInCurrentConnectedComponent = 0;

                //
                // Propagate the connected component ID to all points reachable from this point
                //

                // Add point to queue
                assert(pointsToVisitForConnectedComponents.empty());
                pointsToVisitForConnectedComponents.push(pointIndex);

                // Mark as visited
                mPoints.SetCurrentConnectedComponentDetectionVisitSequenceNumber(
                    pointIndex,
                    currentVisitSequenceNumber);

                // Visit all points reachable from this point via springs
                while (!pointsToVisitForConnectedComponents.empty())
                {
                    auto currentPointIndex = pointsToVisitForConnectedComponents.front();
                    pointsToVisitForConnectedComponents.pop();

                    assert(currentVisitSequenceNumber == mPoints.GetCurrentConnectedComponentDetectionVisitSequenceNumber(currentPointIndex));

                    // Assign the connected component ID
                    mPoints.SetConnectedComponentId(currentPointIndex, currentConnectedComponentId);
                    ++pointsInCurrentConnectedComponent;

                    // Go through this point's adjacents
                    for (auto adjacentSpringElementIndex : mPoints.GetConnectedSprings(currentPointIndex))
                    {
                        assert(!mSprings.IsDeleted(adjacentSpringElementIndex));

                        auto pointAIndex = mSprings.GetPointAIndex(adjacentSpringElementIndex);
                        assert(!mPoints.IsDeleted(pointAIndex));
                        if (currentVisitSequenceNumber != mPoints.GetCurrentConnectedComponentDetectionVisitSequenceNumber(pointAIndex))
                        {
                            mPoints.SetCurrentConnectedComponentDetectionVisitSequenceNumber(pointAIndex, currentVisitSequenceNumber);
                            pointsToVisitForConnectedComponents.push(pointAIndex);
                        }

                        auto pointBIndex = mSprings.GetPointBIndex(adjacentSpringElementIndex);
                        assert(!mPoints.IsDeleted(pointBIndex));
                        if (currentVisitSequenceNumber != mPoints.GetCurrentConnectedComponentDetectionVisitSequenceNumber(pointBIndex))
                        {
                            mPoints.SetCurrentConnectedComponentDetectionVisitSequenceNumber(pointBIndex, currentVisitSequenceNumber);
                            pointsToVisitForConnectedComponents.push(pointBIndex);
                        }
                    }
                }

                // Store number of connected components
                mConnectedComponentSizes.push_back(pointsInCurrentConnectedComponent);
            }
        }
    }
}

void Ship::DestroyConnectedTriangles(ElementIndex pointElementIndex)
{
    //
    // Destroy all triangles connected to the point
    //

    // Note: we can't simply iterate and destroy, as destroying a triangle causes
    // that triangle to be removed from the vector being iterated
    auto & connectedTriangles = mPoints.GetConnectedTriangles(pointElementIndex);
    while (!connectedTriangles.empty())
    {
        assert(!mTriangles.IsDeleted(connectedTriangles.back()));
        mTriangles.Destroy(connectedTriangles.back());
    }

    assert(mPoints.GetConnectedTriangles(pointElementIndex).empty());
}

void Ship::DestroyConnectedTriangles(
    ElementIndex pointAElementIndex,
    ElementIndex pointBElementIndex)
{
    //
    // Destroy the triangles that have an edge among the two points
    //

    auto & connectedTriangles = mPoints.GetConnectedTriangles(pointAElementIndex);
    if (!connectedTriangles.empty())
    {
        for (size_t t = connectedTriangles.size() - 1; ;--t)
        {
            assert(!mTriangles.IsDeleted(connectedTriangles[t]));

            if (mTriangles.GetPointAIndex(connectedTriangles[t]) == pointBElementIndex
                || mTriangles.GetPointBIndex(connectedTriangles[t]) == pointBElementIndex
                || mTriangles.GetPointCIndex(connectedTriangles[t]) == pointBElementIndex)
            {
                // Erase it
                mTriangles.Destroy(connectedTriangles[t]);
            }

            if (t == 0)
                break;
        }
    }
}

void Ship::PointDestroyHandler(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Destroy all springs attached to this point
    //

    // Note: we can't simply iterate and destroy, as destroying a spring causes
    // that spring to be removed from the vector being iterated
    auto & connectedSprings = mPoints.GetConnectedSprings(pointElementIndex);
    while (!connectedSprings.empty())
    {
        assert(!mSprings.IsDeleted(connectedSprings.back()));

        mSprings.Destroy(
            connectedSprings.back(),
            Springs::DestroyOptions::DoNotFireBreakEvent // We're already firing the Destroy event for the point
            | Springs::DestroyOptions::DestroyAllTriangles,
            currentSimulationTime,
            gameParameters,
            mPoints);
    }

    assert(mPoints.GetConnectedSprings(pointElementIndex).empty());


    //
    // Destroy all triangles connected to this point
    //

    // Note: we can't simply iterate and destroy, as destroying a triangle causes
    // that triangle to be removed from the vector being iterated
    auto & connectedTriangles = mPoints.GetConnectedTriangles(pointElementIndex);
    while(!connectedTriangles.empty())
    {
        assert(!mTriangles.IsDeleted(connectedTriangles.back()));

        mTriangles.Destroy(connectedTriangles.back());
    }

    assert(mPoints.GetConnectedTriangles(pointElementIndex).empty());


    //
    // Destroy the connected electrical element, if any
    //
    // Note: we rely on the fact that this happens after connected springs have been destroyed, which
    // ensures that the electrical element's set of connected electrical elements is empty
    //

    if (NoneElementIndex != mPoints.GetElectricalElement(pointElementIndex))
    {
        assert(!mElectricalElements.IsDeleted(mPoints.GetElectricalElement(pointElementIndex)));

        mElectricalElements.Destroy(mPoints.GetElectricalElement(pointElementIndex));
    }

    // Notify bombs
    mBombs.OnPointDestroyed(pointElementIndex);

    // Notify pinned points
    mPinnedPoints.OnPointDestroyed(pointElementIndex);

    // Emit debris
    GenerateDebris(
        pointElementIndex,
        currentSimulationTime,
        gameParameters);

    // Remember our elements are now dirty
    mAreElementsDirty = true;
}

void Ship::SpringDestroyHandler(
    ElementIndex springElementIndex,
    bool destroyAllTriangles,
    float /*currentSimulationTime*/,
    GameParameters const & /*gameParameters*/)
{
    auto const pointAIndex = mSprings.GetPointAIndex(springElementIndex);
    auto const pointBIndex = mSprings.GetPointBIndex(springElementIndex);

    //
    // Remove spring from set of sub springs at each super-triangle
    //

    for (auto superTriangleIndex : mSprings.GetSuperTriangles(springElementIndex))
    {
        mTriangles.RemoveSubSpring(superTriangleIndex, springElementIndex);
    }

    // Let's be neat
    mSprings.ClearSuperTriangles(springElementIndex);


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
    // Remove the spring from its endpoints
    //

    mPoints.RemoveConnectedSpring(pointAIndex, springElementIndex);
    mPoints.RemoveConnectedSpring(pointBIndex, springElementIndex);


    //
    // Make non-hull endpoints leak
    //

    if (!mPoints.IsHull(pointAIndex))
        mPoints.SetLeaking(pointAIndex);

    if (!mPoints.IsHull(pointBIndex))
        mPoints.SetLeaking(pointBIndex);


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

    // Notify bombs
    mBombs.OnSpringDestroyed(springElementIndex);

    // Notify pinned points
    mPinnedPoints.OnSpringDestroyed(springElementIndex);

    // Remember our elements are now dirty
    mAreElementsDirty = true;
}

void Ship::TriangleDestroyHandler(ElementIndex triangleElementIndex)
{
    // Remove triangle from set of super triangles of its sub springs
    for (ElementIndex subSpringIndex : mTriangles.GetSubSprings(triangleElementIndex))
    {
        mSprings.RemoveSuperTriangle(subSpringIndex, triangleElementIndex);
    }

    // Let's be neat
    mTriangles.ClearSubSprings(triangleElementIndex);

    // Remove triangle from its endpoints
    mPoints.RemoveConnectedTriangle(mTriangles.GetPointAIndex(triangleElementIndex), triangleElementIndex);
    mPoints.RemoveConnectedTriangle(mTriangles.GetPointBIndex(triangleElementIndex), triangleElementIndex);
    mPoints.RemoveConnectedTriangle(mTriangles.GetPointCIndex(triangleElementIndex), triangleElementIndex);

    // Remember our elements are now dirty
    mAreElementsDirty = true;
}

void Ship::ElectricalElementDestroyHandler(ElementIndex /*electricalElementIndex*/)
{
    // Remember our elements are now dirty
    mAreElementsDirty = true;
}

void Ship::GenerateAirBubbles(
    vec2f const & position,
    float currentSimulationTime,
    ConnectedComponentId connectedComponentId,
    GameParameters const & /*gameParameters*/)
{
    float vortexAmplitude = GameRandomEngine::GetInstance().GenerateRandomReal(
        GameParameters::MinAirBubblesVortexAmplitude, GameParameters::MaxAirBubblesVortexAmplitude);
    float vortexFrequency = 1.0f / GameRandomEngine::GetInstance().GenerateRandomReal(
        GameParameters::MinAirBubblesVortexFrequency, GameParameters::MaxAirBubblesVortexFrequency);

    mPoints.CreateEphemeralParticleAirBubble(
        position,
        0.3f,
        vortexAmplitude,
        vortexFrequency,
        mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Air),
        currentSimulationTime,
        connectedComponentId);
}

void Ship::GenerateDebris(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (gameParameters.DoGenerateDebris)
    {
        auto const debrisParticleCount = GameRandomEngine::GetInstance().GenerateRandomInteger(
            GameParameters::MinDebrisParticlesPerEvent, GameParameters::MaxDebrisParticlesPerEvent);

        for (size_t d = 0; d < debrisParticleCount; ++d)
        {
            // Choose a velocity vector: point on a circle with random radius and random angle
            float const velocityMagnitude = GameRandomEngine::GetInstance().GenerateRandomReal(
                GameParameters::MinDebrisParticlesVelocity, GameParameters::MaxDebrisParticlesVelocity);
            float const velocityAngle = GameRandomEngine::GetInstance().GenerateRandomReal(0.0f, 2.0f * Pi<float>);

            // Choose a lifetime
            std::chrono::milliseconds const maxLifetime = std::chrono::milliseconds(
                GameRandomEngine::GetInstance().GenerateRandomInteger(
                    GameParameters::MinDebrisParticlesLifetime.count(),
                    GameParameters::MaxDebrisParticlesLifetime.count()));

            mPoints.CreateEphemeralParticleDebris(
                mPoints.GetPosition(pointElementIndex),
                vec2f::fromPolar(velocityMagnitude, velocityAngle),
                mPoints.GetStructuralMaterial(pointElementIndex),
                currentSimulationTime,
                maxLifetime,
                mPoints.GetConnectedComponentId(pointElementIndex));
        }
    }
}

void Ship::GenerateSparkles(
    ElementIndex springElementIndex,
    vec2f const & cutDirectionStartPos,
    vec2f const & cutDirectionEndPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (gameParameters.DoGenerateSparkles)
    {
        //
        // Choose number of particles
        //

        auto const sparkleParticleCount = GameRandomEngine::GetInstance().GenerateRandomInteger<size_t>(
            GameParameters::MinSparkleParticlesPerEvent, GameParameters::MaxSparkleParticlesPerEvent);


        //
        // Choose velocity angle distribution: butterfly perpendicular to cut direction
        //

        vec2f const perpendicularCutVector = (cutDirectionEndPos - cutDirectionStartPos).normalise().to_perpendicular();
        float const axisAngle = perpendicularCutVector.angle(vec2f(1.0f, 0.0f));
        float constexpr AxisAngleWidth = Pi<float> / 7.0f;
        float const startAngle = axisAngle - AxisAngleWidth;
        float const endAngle = axisAngle + AxisAngleWidth;


        //
        // Create particles
        //

        for (size_t d = 0; d < sparkleParticleCount; ++d)
        {
            // Velocity magnitude
            float const velocityMagnitude = GameRandomEngine::GetInstance().GenerateRandomReal(
                GameParameters::MinSparkleParticlesVelocity, GameParameters::MaxSparkleParticlesVelocity);

            // Velocity angle: butterfly perpendicular to *direction of sawing*, not spring
            float const velocityAngle =
                GameRandomEngine::GetInstance().GenerateRandomReal(startAngle, endAngle)
                + (GameRandomEngine::GetInstance().Choose(2) == 0 ? Pi<float> : 0.0f);

            // Choose a lifetime
            std::chrono::milliseconds const maxLifetime = std::chrono::milliseconds(
                GameRandomEngine::GetInstance().GenerateRandomInteger(
                    GameParameters::MinSparkleParticlesLifetime.count(),
                    GameParameters::MaxSparkleParticlesLifetime.count()));

            // Create sparkle
            mPoints.CreateEphemeralParticleSparkle(
                mSprings.GetMidpointPosition(springElementIndex, mPoints),
                vec2f::fromPolar(velocityMagnitude, velocityAngle),
                mSprings.GetBaseStructuralMaterial(springElementIndex),
                currentSimulationTime,
                maxLifetime,
                mSprings.GetConnectedComponentId(springElementIndex, mPoints));
        }
    }
}

/////////////////////////////////////////////////////////////////////////
// Bomb::IPhysicsHandler
/////////////////////////////////////////////////////////////////////////

void Ship::DoBombExplosion(
    vec2f const & blastPosition,
    float sequenceProgress,
    ConnectedComponentId connectedComponentId,
    GameParameters const & gameParameters)
{
    // Blast radius: from 0.6 to BombBlastRadius
    float const blastRadius = 0.6f + (std::max(gameParameters.BombBlastRadius - 0.6f, 0.0f)) * sequenceProgress;

    float const strength =
        750.0f
        * (gameParameters.IsUltraViolentMode ? 100.0f : 1.0f);

    // Store the force field
    mCurrentForceFields.emplace_back(
        new BlastForceField(
            blastPosition,
            blastRadius,
            strength,
            connectedComponentId,
            sequenceProgress == 0.0f));
}

void Ship::DoAntiMatterBombPreimplosion(
    vec2f const & centerPosition,
    float sequenceProgress,
    GameParameters const & gameParameters)
{
    float const strength =
        100000.0f
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    // Store the force field
    mCurrentForceFields.emplace_back(
        new RadialSpaceWarpForceField(
            centerPosition,
            7.0f + sequenceProgress * 100.0f,
            10.0f,
            strength));
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

    // Store the force field
    mCurrentForceFields.emplace_back(
        new ImplosionForceField(
            centerPosition,
            strength));
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

        // Store the force field
        mCurrentForceFields.emplace_back(
            new RadialExplosionForceField(
                centerPosition,
                strength));
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
            Verify(mPoints.GetConnectedTriangles(mTriangles.GetPointAIndex(t)).contains(t));
            Verify(mPoints.GetConnectedTriangles(mTriangles.GetPointBIndex(t)).contains(t));
            Verify(mPoints.GetConnectedTriangles(mTriangles.GetPointCIndex(t)).contains(t));
        }
    }


    //
    // SuperTriangles and SubSprings
    //

    for (auto s : mSprings)
    {
        Verify(mSprings.GetSuperTriangles(s).size() <= 2);

        for (auto superTriangle : mSprings.GetSuperTriangles(s))
        {
            Verify(mTriangles.GetSubSprings(superTriangle).contains(s));
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