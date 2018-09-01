/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "Log.h"
#include "Segment.h"

#include <algorithm>
#include <array>
#include <cassert>
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
    VisitSequenceNumber currentVisitSequenceNumber)
    : mId(id)
    , mParentWorld(parentWorld)    
    , mGameEventHandler(std::move(gameEventHandler))
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mConnectedComponentSizes()
    , mAreElementsDirty(true)
    , mIsSinking(false)
    , mTotalWater(0.0)
    , mPinnedPoints(
        mParentWorld,
        mGameEventHandler,
        mPoints,
        mSprings)
    , mBombs(
        mParentWorld,
        mGameEventHandler,
        [this](
            vec2f const & position, 
            ConnectedComponentId connectedComponentId, 
            int blastSequenceNumber,
            int blastSequenceCount,
            GameParameters const & gameParameters)
            {
                this->BombBlastHandler(
                    position, 
                    connectedComponentId,
                    blastSequenceNumber,
                    blastSequenceCount,
                    gameParameters);
            },        
        mPoints,
        mSprings)
    , mCurrentToolForce(std::nullopt)
{
    // Set destroy handlers
    mPoints.RegisterDestroyHandler(std::bind(&Ship::PointDestroyHandler, this, std::placeholders::_1));
    mSprings.RegisterDestroyHandler(std::bind(&Ship::SpringDestroyHandler, this, std::placeholders::_1, std::placeholders::_2));
    mTriangles.RegisterDestroyHandler(std::bind(&Ship::TriangleDestroyHandler, this, std::placeholders::_1));
    mElectricalElements.RegisterDestroyHandler(std::bind(&Ship::ElectricalElementDestroyHandler, this, std::placeholders::_1));

    // Do a first connected component detection pass 
    DetectConnectedComponents(currentVisitSequenceNumber);
}

Ship::~Ship()
{
}

void Ship::DestroyAt(
    vec2 const & targetPos, 
    float radius)
{
    float const squareRadius = radius * radius;

    // Destroy all points within the radius
    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex))
        {
            if ((mPoints.GetPosition(pointIndex) - targetPos).squareLength() < squareRadius)
            {
                // Destroy point
                mPoints.Destroy(pointIndex);
            }
        }
    }
}

void Ship::SawThrough(
    vec2 const & startPos,
    vec2 const & endPos)
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
                    mPoints);
            }
        }
    }
}

void Ship::DrawTo(
    vec2f const & targetPos,
    float strength)
{
    // Store the force
    assert(!mCurrentToolForce);
    mCurrentToolForce.emplace(targetPos, strength, false);
}

void Ship::SwirlAt(
    vec2f const & targetPos,
    float strength)
{
    // Store the force
    assert(!mCurrentToolForce);
    mCurrentToolForce.emplace(targetPos, strength, true);
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
    vec2 const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleTimerBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleRCBombAt(
    vec2 const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleRCBombAt(
        targetPos,
        gameParameters);
}

void Ship::DetonateRCBombs()
{
    mBombs.DetonateRCBombs();
}

ElementIndex Ship::GetNearestPointIndexAt(
    vec2 const & targetPos, 
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
    VisitSequenceNumber currentVisitSequenceNumber,
    GameParameters const & gameParameters)
{
    //
    // Process eventual parameter changes
    //

    mSprings.UpdateGameParameters(
        gameParameters, 
        mPoints);


    //
    // Update mechanical dynamics
    //

    UpdateMechanicalDynamics(gameParameters);


    //
    // Update bombs
    //
    // Might cause explosions; might cause points to be destroyed
    // (which would flag our elements as dirty)
    //

    mBombs.Update(gameParameters);


    //
    // Update strain for all springs; might cause springs to break
    // (which would flag our elements as dirty)
    //

    mSprings.UpdateStrains(
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
        currentVisitSequenceNumber, 
        gameParameters);
}

void Ship::Render(
    GameParameters const & /*gameParameters*/,
    RenderContext & renderContext) const
{
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
            renderContext.UploadShipElementsStart(
                mId,
                mConnectedComponentSizes);

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
    // Render ship
    //

    renderContext.RenderShip(mId);
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// Mechanical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateMechanicalDynamics(GameParameters const & gameParameters)
{
    for (int iter = 0; iter < GameParameters::NumMechanicalDynamicsIterations<int>; ++iter)
    {
        // Update tool forces, if we have any
        if (!!mCurrentToolForce)
        {
            if (mCurrentToolForce->IsRadial)
                UpdateSwirlForces(
                    mCurrentToolForce->Position,
                    mCurrentToolForce->Strength);
            else
                UpdateDrawForces(
                    mCurrentToolForce->Position,
                    mCurrentToolForce->Strength);
        }

        // Update point forces
        UpdatePointForces(gameParameters);

        // Update springs forces
        UpdateSpringForces(gameParameters);

        // Integrate
        IntegratePointForces();

        // Handle collisions with sea floor
        HandleCollisionsWithSeaFloor();
    }

    //
    // Reset tool force
    //

    mCurrentToolForce.reset();
}

void Ship::UpdateDrawForces(
    vec2f const & position,
    float forceStrength)
{
    for (auto pointIndex : mPoints)
    {
        // F = ForceStrength/sqrt(distance), along radius
        vec2f displacement = (position - mPoints.GetPosition(pointIndex));
        float forceMagnitude = forceStrength / sqrtf(0.1f + displacement.length());
        mPoints.GetForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

void Ship::UpdateSwirlForces(
    vec2f const & position,
    float forceStrength)
{
    for (auto pointIndex : mPoints)
    {
        // F = ForceStrength/sqrt(distance), perpendicular to radius
        vec2f displacement = (position - mPoints.GetPosition(pointIndex));
        float const displacementLength = displacement.length();
        float forceMagnitude = forceStrength / sqrtf(0.1f + displacementLength);
        displacement.normalise(displacementLength);
        mPoints.GetForce(pointIndex) += vec2f(-displacement.y, displacement.x) * forceMagnitude;
    }
}

void Ship::UpdatePointForces(GameParameters const & gameParameters)
{
    // Water mass = 1000kg * adjustment
    float const waterMass = 1000.0f * gameParameters.BuoyancyAdjustment;

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

        // Hull points have buoyancy=0 so that there's no buoyancy (and they're always dry)
        float const effectiveWaterMass = waterMass * mPoints.GetBuoyancy(pointIndex);

        // Mass = own + contained water (clamped to 1)
        mPoints.GetForce(pointIndex) += gameParameters.Gravity * (mPoints.GetMass(pointIndex) + std::min(mPoints.GetWater(pointIndex), 1.0f) * effectiveWaterMass);
        
        if (mPoints.GetPosition(pointIndex).y < waterHeightAtThisPoint)
        {
            // Apply upward push of water mass (i.e. buoyancy!)
            mPoints.GetForce(pointIndex) -= gameParameters.Gravity * effectiveWaterMass;
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
        vec2f const fSpringA = springDir * (displacementLength - mSprings.GetRestLength(springIndex)) * mSprings.GetStiffnessCoefficient(springIndex);


        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = mPoints.GetVelocity(pointBIndex) - mPoints.GetVelocity(pointAIndex);
        vec2f const fDampA = springDir * relVelocity.dot(springDir) * mSprings.GetDampingCoefficient(springIndex);


        //
        // Apply forces
        //

        mPoints.GetForce(pointAIndex) += fSpringA + fDampA;
        mPoints.GetForce(pointBIndex) -= fSpringA + fDampA;
    }
}

void Ship::IntegratePointForces()
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

    size_t const numIterations = mPoints.GetElementCount() * 2;
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
            vec2f seaFloorNormal = vec2f(
                floorheight - mParentWorld.GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x + 0.01f),
                0.01f).normalise();

            // Calculate displacement to move point back to sea floor, along the normal to the floor
            vec2f bounceDisplacement = seaFloorNormal * (floorheight - mPoints.GetPosition(pointIndex).y);

            // Move point back along normal
            mPoints.GetPosition(pointIndex) += bounceDisplacement;
            mPoints.GetVelocity(pointIndex) = bounceDisplacement / GameParameters::MechanicalDynamicsSimulationStepTimeDuration<float>;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Water Dynamics
///////////////////////////////////////////////////////////////////////////////////
void Ship::UpdateWaterDynamics(GameParameters const & gameParameters)
{
    float waterTakenInStep = 0.f;

    for (int i = 0; i < GameParameters::NumWaterDynamicsIterations<int>; ++i)
    {
        UpdateWaterVelocities(gameParameters);

        IntegrateWaterVelocities(gameParameters);

        UpdateWaterInflow(gameParameters, waterTakenInStep);
    }

    // Notify water taken
    mGameEventHandler->OnWaterTaken(waterTakenInStep);

    // Update total water taken
    mTotalWater += waterTakenInStep;

    // Check whether we've started sinking
    if (!mIsSinking
        && mTotalWater > static_cast<float>(mPoints.GetElementCount()) / 1.5f)
    {
        // Started sinking
        mGameEventHandler->OnSinkingBegin(mId);
        mIsSinking = true;
    }

    /* TODOOLD
    // Take water
    float waterTaken = 0.f;
    LeakWater(gameParameters, waterTaken);

    // Notify
    mGameEventHandler->OnWaterTaken(waterTaken);

    // Update total water taken
    mTotalWater += waterTaken;

    // Check whether we've started sinking
    if (!mIsSinking
    && mTotalWater > static_cast<float>(mPoints.GetElementCount()) / 1.5f)
    {
    // Started sinking
    mGameEventHandler->OnSinkingBegin(mId);
    mIsSinking = true;
    }

    // Balance pressure
    for (int i = 0; i < 4; i++)
    BalancePressure(gameParameters);

    // Balance pressure and gravitate water
    for (int i = 0; i < 4; i++)
    {
    BalancePressure(gameParameters);
    GravitateWater(gameParameters);
    }
    */
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
        // Avoid taking water into points that are destroyed, changes total water taken
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

                float const externalWaterHeight = fmaxf(
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
                // 2) Move water according to velocity:
                // - During dt, we move a volume of water Vw equal to A*v*dt; the equivalent change in water
                //   height is thus Vw/A, i.e. v*dt
                //

                float newWater = GameParameters::WaterDynamicsSimulationStepTimeDuration<float> * incomingWaterVelocity;

                if (newWater >= 0.0f)
                {
                    mPoints.GetWater(pointIndex) += newWater;
                }
                else
                {
                    // Make sure we don't over-drain the point in case of an outgoing flow
                    mPoints.GetWater(pointIndex) -= fminf(-newWater, mPoints.GetWater(pointIndex));
                }

                // Update water taken during step
                waterTaken += newWater;
            }

            // TODO: apply water drag (this is the only handy place to do so at the moment)
            mPoints.GetWaterMomentum(pointIndex) *= 1.0f - gameParameters.WaterVelocityDrag;
        }
    }
}

void Ship::UpdateWaterMomenta(GameParameters const & gameParameters)
{
    //
    // For each spring, calculate the velocity gained by water in the source endpoint due to 
    // pressure difference and gravity difference along the spring, and update the source endpoint's
    // water momentum with it. 
    //
    // At integration time, we will move the dt portion of this source endpoint momentum to the 
    // destination endpoint.
    //
    // The velocity of the outgoing water is given by Bernoulli's equation:
    //
    //      Pa/density + g*ya + 1/2va**2 = Pb/density + g*yb + 1/2vb**2
    //
    // In which:
    //   Pa, Pb = pressure at point A, B
    //   ya, yb = height of point A, B
    //   va, vb = velocity of water entering point A, B
    // 
    // Considering that:
    //  - P is Wh*density*g
    //  - vb is zero (we calculate the new velocity only, hence we assume everything's static at this moment)
    //
    // Then we can rewrite Bernoulli's equation as:
    //  
    //      Wha*g + g*ya + 1/2va**2 = Whb*g + g*yb
    //
    // Hence:
    //   va = sqrt( 2*g*( (Whb-Wha) + (yb-ya) )  )
    //

    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

        // Normalized spring vector, oriented A -> B
        vec2f const s = (mPoints.GetPosition(pointBIndex) - mPoints.GetPosition(pointAIndex)).normalise();

        float const dw = mPoints.GetWater(pointAIndex) - mPoints.GetWater(pointBIndex);
        float const dy = mPoints.GetPosition(pointAIndex).y - mPoints.GetPosition(pointBIndex).y;
        float const dwy = dw + dy;

        // Update momentum of source endpoint with this velocity delta
        if (dwy >= 0.0f)
        {
            // Water goes from A to B, update A's momentum
            mPoints.GetWaterMomentum(pointAIndex) +=
                s
                * mPoints.GetWater(pointAIndex)
                * sqrtf(2.0f * GameParameters::GravityMagnitude * dwy);
        }
        else
        {
            // Water goes from B to A, update B's momentum
            mPoints.GetWaterMomentum(pointBIndex) +=
                -s
                * mPoints.GetWater(pointBIndex)
                * sqrtf(2.0f * GameParameters::GravityMagnitude * -dwy);
        }
    }
}

void Ship::IntegrateWaterMomenta(GameParameters const & gameParameters)
{
    //
    // For each point, move a dt portion of its water momentum along all springs to the destination
    // endpoint.
    //
    // In order to avoid the first springs draining out all the water and not leaving any for
    // subsequent springs, we normalize the outgoing flows to ensure we can distribute water among
    // all springs.
    //

    // Theoretical outgoing water along each spring; the actual water we move will be a fraction of 
    // these so that we never exceed the quantity of water available at this point
    std::array<float, GameParameters::MaxSpringsPerPoint> theoreticalSpringOutgoingWater;

    // Velocity vectors along all outgoing springs
    std::array<vec2f, GameParameters::MaxSpringsPerPoint> springOutgoingVelocity;

    for (auto pointIndex : mPoints)
    {
        // Water velocity - independent from mass
        vec2f pointWaterVelocity = vec2f::zero();
        if (mPoints.GetWater(pointIndex) > 0.0f)
            pointWaterVelocity  = mPoints.GetWaterMomentum(pointIndex) / mPoints.GetWater(pointIndex);

        // Adjust velocity based off user's preferences
        pointWaterVelocity *= gameParameters.WaterVelocityAdjustment;

        // Water velocity norm and normalized vector
        float const pointWaterVelocityNorm = pointWaterVelocity.length();
        vec2f const normalizedPointWaterVelocity = pointWaterVelocity.normalise(pointWaterVelocityNorm);

        // Visit all springs and calculate theoretical outgoing flows
        for (size_t s = 0; s < mPoints.GetConnectedSprings(pointIndex).size(); ++s)
        {
            auto const springIndex = mPoints.GetConnectedSprings(pointIndex)[s];

            auto otherEndpointIndex = mSprings.GetPointBIndex(springIndex);
            if (otherEndpointIndex == pointIndex)
            {
                // This point is B, other point is then A
                otherEndpointIndex = mSprings.GetPointAIndex(springIndex);
            }            

            vec2f normalizedSpring = (mPoints.GetPosition(otherEndpointIndex) - mPoints.GetPosition(pointIndex)).normalise();

            // Calculate angle between our water's velocity and this spring in the outgoing direction
            float cosTheta = normalizedPointWaterVelocity.dot(normalizedSpring);
            if (cosTheta > 0.0f)
            {
                // The water momentum at this point has a positive component along the spring 
                // in the outgoing direction

                // Scalar velocity along the spring
                float const vs = pointWaterVelocityNorm * cosTheta;

                // Theoretical quantity of water leaving the point, adjusted for
                // the springg length
                float const theoreticalOutgoingWater =
                    vs * GameParameters::WaterDynamicsSimulationStepTimeDuration<float>
                    / mSprings.GetRestLength(springIndex)
                    * mSprings.GetWaterPermeability(springIndex);

                // TODOHERE: normalization

                // Actual quantity of water leaving the point
                float const actualOutgoingWater = fminf(
                    theoreticalOutgoingWater,
                    mPoints.GetWater(pointIndex));

                // Actual momentum leaving the point
                vec2f const actualOutgoingWaterMomentum =
                    normalizedSpring
                    * actualOutgoingWater
                    * vs;

                // Update quantities of water
                mPoints.GetWater(pointIndex) -= actualOutgoingWater;
                mPoints.GetWater(otherEndpointIndex) += actualOutgoingWater;

                assert(mPoints.GetWater(pointIndex) >= 0.0f);
                assert(mPoints.GetWater(otherEndpointIndex) >= 0.0f);

                // Update momenta
                mPoints.GetWaterMomentum(pointIndex) -= actualOutgoingWaterMomentum;
                mPoints.GetWaterMomentum(otherEndpointIndex) += actualOutgoingWaterMomentum;
            }
            else
            {
                // Nothing along this spring
                theoreticalSpringOutgoingWater[s] = 0.0f;
                springOutgoingVelocity[s] = vec2f::zero(); // TODO: see if needed
            }
        }

        // Calculate normalization factor
        // TODOHERE

        // Move momenta now
        // TODOHERE
    }
}

// TODOOLD
void Ship::UpdateWaterVelocities(GameParameters const & gameParameters)
{
    //
    // For each spring, update the velocity of water along the spring.
    //
    // At spring s:
    //    v(t+1) = v(t) + Dv
    //
    // Dv is the additional velocity gained during this dt, which is the sum of 2 components: 
    //  - pressure gradient - i.e. difference in "water height" among the two endpoints, assuming no y difference
    //  - gravity gradient - i.e. difference in y among the two endpoints
    //
    // The two components are taken care of automagically by Bernoulli's equation:
    //
    //      Pa/density + g*ya + 1/2va**2 = Pb/density + g*yb + 1/2vb**2
    //
    // In which:
    //   Pa, Pb = pressure at point A, B
    //   ya, yb = height of point A, B
    //   va, vb = velocity of water entering point A, B
    // 
    // Considering that:
    //  - P is Wh*density*g
    //  - vb is zero
    //
    // Then we can rewrite Bernoulli's equation as:
    //  
    //      Wha*g + g*ya + 1/2va**2 = Whb*g + g*yb
    //
    // Hence:
    //   va = sqrt( 2*g*( (Whb-Wha) + (yb-ya) )  )
    //

    float const waterVelocityDragAdjustment = 1.0f - gameParameters.WaterVelocityDrag;

    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

        mSprings.GetWaterVelocity(springIndex) *= waterVelocityDragAdjustment;

        float const dw = mPoints.GetWater(pointAIndex) - mPoints.GetWater(pointBIndex);
        float const dy = mPoints.GetPosition(pointAIndex).y - mPoints.GetPosition(pointBIndex).y;
        float const dwy = dw + dy;

        // Update valocities with this velocity delta
        if (dwy >= 0.0f)
        {
            // From A to B
            mSprings.GetWaterVelocity(springIndex) +=
                sqrtf(2.0f * GameParameters::GravityMagnitude * dwy)
                * 1.0f;
        }
        else
        {
            // From B to A
            mSprings.GetWaterVelocity(springIndex) -= 
                sqrtf(2.0f * GameParameters::GravityMagnitude * -dwy)
                * 1.0f;
        }
    }
}

// TODOOLD
void Ship::IntegrateWaterVelocities(GameParameters const & gameParameters)
{
    //
    // For each spring, move water from one endpoint to the other, based on the current water velocity
    // in the spring.
    //
    // Given a water velocity v, the distance travelled by water from the source endpoint towards the destination
    // endpoint during a simulation step is D=v*dt; given a spring section A, this travelled distance corresponds 
    // to a moved volume of water V=D*A. This volume of water V is responsible for a change in water height
    // at its source and destination points of +/- V/A. 
    //
    // We immediately move this volume of water to its destination point along a spring of length 1.0, 
    // but then we have to adjust for springs with lengths different than 1.0: V=v*dt*A/L.
    //
    // Putting it all together, the change in water height at either endpoint is dW = +/- v*dt/L.
    //
    // We then have to take into account boundary conditions (due to our Eulearian explicit integration scheme),
    // which might cause more water to leave a point than it is actually available. 
    // Furthermore, the first spring connected to a point might drain completely that point,
    // leaving no water available for the other springs connected to the same point. This causes unpleasant
    // artifacts in the water distribution.
    //
    // To overcome this, we use a normalization factor for all outgoing flows to ensure we don't drain
    // more water that it is available. 
    //
    // We also actually only consider half of the velocity of each spring, as the other endpoint will/has 
    // in turn move/moved its other "half" (even though technically it's not guaranteed that it will mirror 
    // our very same calculations).
    //

    // Theoretical delta-water of all outgoing flows; 
    // in reality we only move a fraction of these so that we never exceed the quantity of water
    // available at this point
    std::array<float, GameParameters::MaxSpringsPerPoint> theoreticalSpringOutgoingDw;

    for (auto pointIndex : mPoints)
    {
        //
        // Process incoming flows first, and keep track of total outgoing flow
        //

        float totalTheoreticalOutgoingFlow = 0.0f;

        for (size_t s = 0; s < mPoints.GetConnectedSprings(pointIndex).size(); ++s)
        {
            auto const springIndex = mPoints.GetConnectedSprings(pointIndex)[s];
            
            auto otherEndpointIndex = mSprings.GetPointBIndex(springIndex);
            float springDirection = 1.0f;
            if (otherEndpointIndex == pointIndex)
            {
                // This point is B, other point is then A
                otherEndpointIndex = mSprings.GetPointAIndex(springIndex);
                springDirection = -1.0f;
            }
            
            float const theoreticalOutgoingWaterDelta =
                mSprings.GetWaterVelocity(springIndex) * GameParameters::WaterDynamicsSimulationStepTimeDuration<float>
                / mSprings.GetRestLength(springIndex)
                * mSprings.GetWaterPermeability(springIndex)
                * gameParameters.WaterVelocityAdjustment
                * springDirection
                / 2.0f; // Leave room for the endpoint's other spring

            if (theoreticalOutgoingWaterDelta >= 0.0f)
            {
                // Outgoing flow, from us to other endpoint

                // Store it for later
                theoreticalSpringOutgoingDw[s] = theoreticalOutgoingWaterDelta;

                // Update total outgoing flow
                totalTheoreticalOutgoingFlow += theoreticalOutgoingWaterDelta;
            }
            else
            {
                // Ingoing flow, from other endpoint to us

                theoreticalSpringOutgoingDw[s] = 0.0f;

                // Cap it to make sure we don't drain the other point completely
                float const actualIncomingWaterDelta =
                    fminf(mPoints.GetWater(otherEndpointIndex), -theoreticalOutgoingWaterDelta);

                assert(actualIncomingWaterDelta >= 0.0f);

                // Adjust waters
                mPoints.GetWater(pointIndex) += actualIncomingWaterDelta;
                mPoints.GetWater(otherEndpointIndex) -= actualIncomingWaterDelta;
            }
        }

        float normalizationFactor = 1.0f;
        if (totalTheoreticalOutgoingFlow > mPoints.GetWater(pointIndex))
        {
            // We need to scale down outgoing flows
            assert(totalTheoreticalOutgoingFlow > 0.0f);
            normalizationFactor = mPoints.GetWater(pointIndex) / totalTheoreticalOutgoingFlow;
        }


        //
        // Now process outgoing flows
        //

        for (size_t s = 0; s < mPoints.GetConnectedSprings(pointIndex).size(); ++s)
        {
            auto const springIndex = mPoints.GetConnectedSprings(pointIndex)[s];

            auto otherEndpointIndex = mSprings.GetPointBIndex(springIndex);
            if (otherEndpointIndex == pointIndex)
                otherEndpointIndex = mSprings.GetPointAIndex(springIndex);

            mPoints.GetWater(otherEndpointIndex) +=
                theoreticalSpringOutgoingDw[s]
                * normalizationFactor;
        }

        // Update point's water
        mPoints.GetWater(pointIndex) -= fminf(totalTheoreticalOutgoingFlow, mPoints.GetWater(pointIndex));
    }

    // TODOOLD
    ////for (auto springIndex : mSprings)
    ////{
    ////    auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
    ////    auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

    ////    // TODOTEST
    ////    static constexpr float waterVelocityAdjustment = 1.0f;

    ////    float const theoreticalWaterDelta =
    ////        mSprings.GetWaterVelocity(springIndex) * GameParameters::WaterDynamicsSimulationStepTimeDuration<float>
    ////        / mSprings.GetRestLength(springIndex)
    ////        * mSprings.GetWaterPermeability(springIndex)
    ////        * waterVelocityAdjustment;

    ////    float actualWaterDelta;
    ////    if (theoreticalWaterDelta >= 0.f)
    ////    { 
    ////        // From A to B
    ////        actualWaterDelta = fminf(
    ////            mPoints.GetWater(pointAIndex),
    ////            theoreticalWaterDelta);
    ////    }
    ////    else
    ////    {
    ////        // From B to A
    ////        actualWaterDelta = fmaxf(
    ////            -mPoints.GetWater(pointBIndex),
    ////            theoreticalWaterDelta);
    ////    }

    ////    mPoints.GetWater(pointAIndex) -= actualWaterDelta;
    ////    mPoints.GetWater(pointBIndex) += actualWaterDelta;
    ////}
}

// TODOOLD
void Ship::LeakWater(
    GameParameters const & /*gameParameters*/,
    float & waterTaken)
{
    // Magic number
    //float const effectivePressureAdjustment = 0.1f * gameParameters.WaterPressureAdjustment;
    float const effectivePressureAdjustment = 0.1f;

    //
    // Inject water into all the leaking nodes that are underwater, 
    // provided the external pressure is larger than the internal water pressure
    // of the node
    //

    for (auto pointIndex : mPoints)
    {
        // Avoid taking water into points that are destroyed, changes total water taken
        if (!mPoints.IsDeleted(pointIndex))
        {
            if (mPoints.IsLeaking(pointIndex))
            {
                float waterLevel = mParentWorld.GetWaterHeightAt(mPoints.GetPosition(pointIndex).x);

                float const externalWaterPressure = mPoints.GetExternalWaterPressure(
                    pointIndex,
                    waterLevel) * effectivePressureAdjustment;

                if (externalWaterPressure > mPoints.GetWater(pointIndex))
                {
                    float newWater = GameParameters::SimulationStepTimeDuration<float> * (externalWaterPressure - mPoints.GetWater(pointIndex));
                    mPoints.GetWater(pointIndex) += newWater;
                    waterTaken += newWater;
                }
            }
        }
    }
}

// TODOOLD
void Ship::GravitateWater(GameParameters const & gameParameters)
{
    //
    // Water flows into adjacent nodes in a quantity proportional to the cos of angle the spring makes
    // against gravity (parallel with gravity => 1 (full flow), perpendicular = 0, parallel-opposite => -1 (goes back))
    //
    // Note: we don't take any shortcuts when a point has no water, as that would cause the speed of the 
    // simulation to change over time.
    //

    // Visit all connected non-hull points - i.e. non-hull springs
    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

        // cos_theta > 0 => pointA above pointB
        float cos_theta = (mPoints.GetPosition(pointBIndex) - mPoints.GetPosition(pointAIndex)).normalise().dot(gameParameters.GravityNormal);

        // This amount of water falls in a second; 
        // a value too high causes all the water to be stuffed into the lowest node
        static constexpr float velocity = 0.60f;
                
        // Calculate amount of water that falls from highest point to lowest point
        float correction = mSprings.GetWaterPermeability(springIndex) * (velocity * GameParameters::SimulationStepTimeDuration<float>)
            * cos_theta  * (cos_theta > 0.0f ? mPoints.GetWater(pointAIndex) : mPoints.GetWater(pointBIndex));

        // TODO: use code below and store at Spring::WaterGravityFactorA/B
        ////float cos_theta_select = (1.0f + cos_theta) / 2.0f;
        ////float correction = 0.60f * cos_theta_select * pointA->GetWater() - 0.60f * (1.0f - cos_theta_select) * pointB->GetWater();
        ////correction *= GameParameters::SimulationStepTimeDuration<float>;

        mPoints.GetWater(pointAIndex) -= correction;
        mPoints.GetWater(pointBIndex) += correction;
    }
}

// TODOOLD
void Ship::BalancePressure(GameParameters const & /*gameParameters*/)
{
    //
    // If there's too much water in this node, try and push it into the others
    // (This needs to iterate over multiple frames for pressure waves to spread through water)
    //
    // Note: we don't take any shortcuts when a point has no water, as that would cause the speed of the 
    // simulation to change over time.
    //

    // Visit all connected non-hull points - i.e. non-hull springs
    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        float const aWater = mPoints.GetWater(pointAIndex);

        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);
        float const bWater = mPoints.GetWater(pointBIndex);

        if (aWater < 1 && bWater < 1)   // if water content below threshold, no need to force water out
            continue;

        // This amount of water difference propagates in 1 second
        static constexpr float velocity = 2.5f;

        // Move water from more wet to less wet
        float const correction = mSprings.GetWaterPermeability(springIndex) * (bWater - aWater) * (velocity * GameParameters::SimulationStepTimeDuration<float>);
        mPoints.GetWater(pointAIndex) += correction;
        mPoints.GetWater(pointBIndex) -= correction;
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Electrical Dynamics
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateElectricalDynamics(
    VisitSequenceNumber currentVisitSequenceNumber,
    GameParameters const & gameParameters)
{
    // Invoked regardless of dirty elements, as generators might become wet
    UpdateElectricalConnectivity(currentVisitSequenceNumber);

    mElectricalElements.Update(
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
    float const adjustmentCoefficient = powf(1.0f - gameParameters.LightDiffusionAdjustment, 2.0f);

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

///////////////////////////////////////////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////////////////////////////////////////

void Ship::DetectConnectedComponents(VisitSequenceNumber currentVisitSequenceNumber)
{
    mConnectedComponentSizes.clear();

    ConnectedComponentId currentConnectedComponentId = 0;
    std::queue<ElementIndex> pointsToVisitForConnectedComponents;

    // Visit all points
    for (auto pointIndex : mPoints)
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

void Ship::BombBlastHandler(
    vec2f const & blastPosition,
    ConnectedComponentId connectedComponentId,
    int blastSequenceNumber,
    int blastSequenceCount,
    GameParameters const & gameParameters)
{
    // 
    // Go through all the connected component's points and, for each point in radius:
    // - Keep closest to blast position, which we'll Destroy later (if this is the fist frame of the 
    //   blast sequence)
    // - Flip over the point outside of the radius
    //

    // Blast radius: lastSequenceNumber makes it from 0.6 to BombBlastRadius
    float blastRadius = 0.6f + (std::max(gameParameters.BombBlastRadius - 0.6f, 0.0f)) * static_cast<float>(blastSequenceNumber + 1) / static_cast<float>(blastSequenceCount);
    float squareBlastRadius = blastRadius * blastRadius;

    float closestPointSquareDistance = std::numeric_limits<float>::max();
    ElementIndex closestPointIndex = NoneElementIndex;

    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex)
            && mPoints.GetConnectedComponentId(pointIndex) == connectedComponentId)
        {
            vec2f pointRadius = mPoints.GetPosition(pointIndex) - blastPosition;
            float squarePointDistance = pointRadius.squareLength();
            if (squarePointDistance < squareBlastRadius)
            {
                // Check whether this point is the closest
                if (squarePointDistance < closestPointSquareDistance)
                {
                    closestPointSquareDistance = squarePointDistance;
                    closestPointIndex = pointIndex;
                }

                // Flip the point
                vec2f flippedRadius = pointRadius.normalise() * (blastRadius + (blastRadius - pointRadius.length()));
                vec2f newPosition = blastPosition + flippedRadius;                
                mPoints.GetVelocity(pointIndex) = (newPosition - mPoints.GetPosition(pointIndex)) / GameParameters::MechanicalDynamicsSimulationStepTimeDuration<float>;
                mPoints.GetPosition(pointIndex) = newPosition;
            }
        }
    }

    //
    // Eventually destroy the closest point
    //

    if (0 == blastSequenceNumber
        && NoneElementIndex != closestPointIndex)
    {
        // Destroy point
        mPoints.Destroy(closestPointIndex);
    }
}

void Ship::PointDestroyHandler(ElementIndex pointElementIndex)
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

    // Remember our elements are now dirty
    mAreElementsDirty = true;
}

void Ship::SpringDestroyHandler(
    ElementIndex springElementIndex,
    bool destroyAllTriangles)
{
    auto const pointAIndex = mSprings.GetPointAIndex(springElementIndex);
    auto const pointBIndex = mSprings.GetPointBIndex(springElementIndex);

    // Make endpoints leak
    mPoints.SetLeaking(pointAIndex);
    mPoints.SetLeaking(pointBIndex);

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
        DestroyConnectedTriangles(pointAIndex, pointBIndex);
    }


    //
    // Remove the spring from its endpoints
    //

    mPoints.RemoveConnectedSpring(pointAIndex, springElementIndex);
    mPoints.RemoveConnectedSpring(pointBIndex, springElementIndex);


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

}