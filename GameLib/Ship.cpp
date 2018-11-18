/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"
#include "Log.h"
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
    int id,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    Points && points,
    Springs && springs,
    Triangles && triangles,
    ElectricalElements && electricalElements,
    std::shared_ptr<MaterialDatabase> materialDatabase,
    VisitSequenceNumber currentVisitSequenceNumber)
    : mId(id)
    , mParentWorld(parentWorld)    
    , mGameEventHandler(std::move(gameEventHandler))
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mMaterialDatabase(std::move(materialDatabase))
    , mConnectedComponentSizes()
    , mAreElementsDirty(true)
    , mIsSinking(false)
    , mTotalWater(0.0)
    , mWaterSplashedRunningAverage()
    , mPinnedPoints(
        mParentWorld,
        mGameEventHandler,
        mPoints,
        mSprings)
    , mBombs(
        mParentWorld,
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

void Ship::DestroyAt(
    vec2f const & targetPos, 
    float radiusMultiplier,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    float const radius = gameParameters.DestroyRadius * radiusMultiplier;
    float const squareRadius = radius * radius;

    // Destroy all (non-ephemeral) points within the radius
    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        if (!mPoints.IsDeleted(pointIndex))
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

                if (!mSprings.IsRope(springIndex))
                {
                    // Emit sparkles
                    GenerateSparkles(
                        springIndex,
                        startPos,
                        endPos,
                        currentSimulationTime,
                        gameParameters);
                }
            }
        }
    }
}

void Ship::DrawTo(
    vec2f const & targetPos,
    float strength)
{
    // Store the force field
    mCurrentForceFields.emplace_back(
        new DrawForceField(
            targetPos,
            strength));
}

void Ship::SwirlAt(
    vec2f const & targetPos,
    float strength)
{
    // Store the force field
    mCurrentForceFields.emplace_back(
        new SwirlForceField(
            targetPos,
            strength));
}

bool Ship::TogglePinAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mPinnedPoints.ToggleAt(
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

bool Ship::ToggleRCBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleRCBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleAntiMatterBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleAntiMatterBombAt(
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

ElementIndex Ship::GetNearestPointIndexAt(
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

void Ship::Update(
    float currentSimulationTime,
    VisitSequenceNumber currentVisitSequenceNumber,
    GameParameters const & gameParameters)
{
    auto const currentWallClockTime = GameWallClock::GetInstance().Now();


    //
    // Process eventual parameter changes
    //

    mSprings.UpdateGameParameters(
        gameParameters, 
        mPoints);


    //
    // Update mechanical dynamics
    //

    UpdateMechanicalDynamics(
        currentSimulationTime,
        gameParameters);


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

    UpdateWaterDynamics(gameParameters);


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
}

void Ship::Render(
    GameParameters const & /*gameParameters*/,
    Render::RenderContext & renderContext) const
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
        //

        if (mAreElementsDirty)
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

        renderContext.UploadShipElementStressedSpringsStart(mId);

        if (renderContext.GetShowStressedSprings())
        {        
            mSprings.UploadStressedSpringElements(
                mId,
                renderContext,
                mPoints);
        }

        renderContext.UploadShipElementStressedSpringsEnd(mId);

        mAreElementsDirty = false;
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
    GameParameters const & gameParameters)
{
    for (int iter = 0; iter < GameParameters::NumMechanicalDynamicsIterations<int>; ++iter)
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

        // Integrate and reset forces to zero
        IntegrateAndResetPointForces();

        // Handle collisions with sea floor
        HandleCollisionsWithSeaFloor();
    }

    // Consume force fields
    mCurrentForceFields.clear();
}

void Ship::UpdatePointForces(GameParameters const & gameParameters)
{
    // Water mass = 1000Kg
    static constexpr float WaterMass = 1000.0f;

    // Effective water mass that we want to use for buoyancy calculation
    float const BuoyancyAdjustedWaterMass = WaterMass * gameParameters.BuoyancyAdjustment;

    // Underwater points feel this amount of water drag
    //
    // The higher the value, the more viscous the water looks when a body moves through it
    constexpr float WaterDragCoefficient = 0.020f; // ~= 1.0f - powf(0.6f, 0.02f)
    
    for (auto pointIndex : mPoints)
    {
        // Get height of water at this point
        float const waterHeightAtThisPoint = mParentWorld.GetWaterHeightAt(mPoints.GetPosition(pointIndex).x);

        //
        // 1. Add gravity and buoyancy
        //        

        // Mass = own + contained water (clamped to 1)
        float const totalPointMass =
            mPoints.GetMass(pointIndex)
            // FUTURE: revert to the line below (physically correct) once we also have a mass adjustment;
            // without a mass adjustment, it'd be hard to sink ships when using a higher buoyancy adjustment
            //+ std::min(mPoints.GetWater(pointIndex), 1.0f) * WaterMass;
			+ std::min(mPoints.GetWater(pointIndex), 1.0f) * BuoyancyAdjustedWaterMass;

        mPoints.GetForce(pointIndex) += 
            gameParameters.Gravity
            * totalPointMass;

        if (mPoints.GetPosition(pointIndex).y < waterHeightAtThisPoint)
        {
            //
            // Apply upward push of water mass (i.e. buoyancy!)
            //
            // We don't want hull points to feel buoyancy, otherwise hull points lighter than water (e.g. wood hull)
            // would never sink as they don't get any water
            //

            mPoints.GetForce(pointIndex) -=
                gameParameters.Gravity
                * BuoyancyAdjustedWaterMass
                * mPoints.GetBuoyancy(pointIndex);
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
            mPoints.GetForce(pointIndex) += mPoints.GetVelocity(pointIndex) * (-WaterDragCoefficient);
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

void Ship::IntegrateAndResetPointForces()
{
    static constexpr float dt = GameParameters::MechanicalDynamicsSimulationStepTimeDuration<float>;

    // Global damp - lowers velocity uniformly, damping oscillations originating between gravity and buoyancy
    // Note: it's extremely sensitive, big difference between 0.9995 and 0.9998
    // Note: technically it's not a drag force, it's just a dimensionless deceleration
    float constexpr GlobalDampCoefficient = 0.9996f;

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

    size_t const numIterations = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < numIterations; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos = velocityBuffer[i] * dt + forceBuffer[i] * integrationFactorBuffer[i];
        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * GlobalDampCoefficient / dt;

        // Zero out force now that we've integrated it
        forceBuffer[i] = 0.0f;
    }
}

void Ship::HandleCollisionsWithSeaFloor()
{
    for (auto pointIndex : mPoints)
    {
        // Check if point is now below the sea floor
        float const floorheight = mParentWorld.GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x);
        if (mPoints.GetPosition(pointIndex).y < floorheight)
        {
            // Calculate normal to sea floor
            static constexpr float Dx = 0.01f;
            vec2f seaFloorNormal = vec2f(
                floorheight - mParentWorld.GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x + Dx),
                Dx).normalise();

            // Calculate displacement to move point back to sea floor, along the normal to the floor 
            // (which is oriented upwards)
            vec2f bounceDisplacement = seaFloorNormal * (floorheight - mPoints.GetPosition(pointIndex).y);

            // Move point back along normal to ~basically floor level
            mPoints.GetPosition(pointIndex) += bounceDisplacement;

            // Simulate a perfectly elastic impact, bouncing along specular to sea floor normal:
            // R = 2*n*dot_product(n,-V) + V
            mPoints.GetVelocity(pointIndex) += 
                seaFloorNormal
                * -2.0f
                * seaFloorNormal.dot(mPoints.GetVelocity(pointIndex));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Water Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateWaterDynamics(GameParameters const & gameParameters)
{
    float waterTakenInStep = 0.f;
    float waterSplashedInStep = 0.f;

    for (int i = 0; i < GameParameters::NumWaterDynamicsIterations<int>; ++i)
    {
        UpdateWaterInflow(gameParameters, waterTakenInStep);

        UpdateWaterVelocities(gameParameters, waterSplashedInStep);
    }

    // Notify waters
    mGameEventHandler->OnWaterTaken(waterTakenInStep);
    mGameEventHandler->OnWaterSplashed(waterSplashedInStep);

    // Update total water taken and check whether we've started sinking
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
                    * GameParameters::WaterDynamicsSimulationStepTimeDuration<float>
                    * gameParameters.WaterIntakeAdjustment;

                if (newWater < 0.0f)
                {
                    // Outgoing water

                    // Make sure we don't over-drain the point
                    newWater = -std::min(-newWater, mPoints.GetWater(pointIndex));

                    // Simulate water "sticking" into material - after all, water that once entered
                    // won't leave completely afterwards, something will stay behind
                    newWater *= 0.95f;
                }

                // Adjust water
                mPoints.AddWater(pointIndex, newWater);

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
            exp(-oldPointWaterBufferData[pointIndex] * 10.0f);
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

            // Component of the point's own velocity along the spring
            float const pointVelocityAlongSpring =
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
            // if this were inbound it wouldn't result in any water movement
            // between these two springs. Morevoer, Bernoulli's velocity injected
            // along this spring will be picked up later also by the other endpoint,
            // and at that time it would move water if it agrees with its velocity
            float const springOutboundScalarWaterVelocity = std::max(
                pointVelocityAlongSpring + bernoulliVelocityAlongSpring * alphaCrazyness,
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
        //    match the water currently at the point times the quickness fractio
        //

        assert(totalOutboundWaterFlowWeight >= 0.0f);

        float waterQuantityNormalizationFactor = 0.0f;
        if (totalOutboundWaterFlowWeight != 0.0f)
        { 
            waterQuantityNormalizationFactor =
                oldPointWaterBufferData[pointIndex]
                * gameParameters.WaterQuickness
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

                // TODO: get rid of this re-calculation once we pre-calculate all spring normalized vectors
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

    for (auto generatorIndex : mElectricalElements.GetGenerators())
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
                if (mPoints.GetWater(mElectricalElements.GetPointIndex(generatorIndex)) < 0.3f)
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
    // Diffuse light from each lamp to the closest adjacent (i.e. spring-connected) points,
    // inversely-proportional to the square of the distance
    //

    // Greater adjustment => underrated distance => wider diffusion
    float const adjustmentCoefficient = 10.0f * powf(1.0f - gameParameters.LightDiffusionAdjustment, 2.0f);

    // Zero-out light at all points first
    for (auto pointIndex : mPoints)
    {
        // Zero its light
        mPoints.GetLight(pointIndex) = 0.0f;
    }

    // Go through all lamps;
    // can safely visit deleted lamps as their current will always be zero
    for (auto lampIndex : mElectricalElements.GetLamps())
    {
        float const lampLight = mElectricalElements.GetAvailableCurrent(lampIndex);
        auto const lampPointIndex = mElectricalElements.GetPointIndex(lampIndex);
        vec2f const & lampPosition = mPoints.GetPosition(lampPointIndex);
        ConnectedComponentId const lampConnectedComponentId = mPoints.GetConnectedComponentId(lampPointIndex);        

        // Visit all points (including deleted ones) in the same connected component
        for (auto pointIndex : mPoints)
        {
            if (mPoints.GetConnectedComponentId(pointIndex) == lampConnectedComponentId)
            {
                float squareDistance = std::max(
                    1.0f,
                    (mPoints.GetPosition(pointIndex) - lampPosition).squareLength() * adjustmentCoefficient);

                assert(squareDistance >= 1.0f);

                float newLight = lampLight / squareDistance;
                if (newLight > mPoints.GetLight(pointIndex))
                    mPoints.GetLight(pointIndex) = newLight;
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
    // Destroy connected triangles
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
                mPoints.GetMaterial(pointElementIndex),
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
        // Choose start and end colors
        //

        vec4f startColor;
        vec4f endColor;
        Material const * material = mSprings.GetBaseMaterial(springElementIndex);
        if (!!(material->Sound)
            && Material::SoundProperties::SoundElementType::Metal == material->Sound->ElementType)
        {
            startColor = vec4f(1.0f, 0.95f, 0.09f, 1.0f); // Opaque
            endColor = vec4f(0.55f, 0.1f, 0.1f, 0.0f); // Transparent
        }
        else
        {
            startColor = endColor = material->RenderColour;
        }


        //
        // Choose velocity angle distribution: butterfly perpendicular to cut direction
        //

        vec2f const perpendicularCutVector = (cutDirectionEndPos - cutDirectionStartPos).normalise().to_perpendicular();
        float const axisAngle = perpendicularCutVector.angle(vec2f(1.0f, 0.0f));
        float constexpr AxisAngleWidth = Pi<float> / 4.0f;
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

            mPoints.CreateEphemeralParticleSparkle(
                mSprings.GetMidpointPosition(springElementIndex, mPoints),
                vec2f::fromPolar(velocityMagnitude, velocityAngle),
                mSprings.GetBaseMaterial(springElementIndex),
                currentSimulationTime,
                maxLifetime,
                startColor,
                endColor,
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

    // Store the force field
    mCurrentForceFields.emplace_back(
        new BlastForceField(
            blastPosition,
            blastRadius,
            1000.0f * (gameParameters.IsUltraViolentMode ? 100.0f : 1.0f),
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

}