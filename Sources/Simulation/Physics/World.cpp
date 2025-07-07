/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include <Core/GameRandomEngine.h>

#include <algorithm>
#include <cassert>
#include <numeric>

namespace Physics {

World::World(
    OceanFloorHeightMap && oceanFloorHeightMap,
    FishSpeciesDatabase const & fishSpeciesDatabase,
    NpcDatabase const & npcDatabase,
    SimulationEventDispatcher & simulationEventDispatcher,
    SimulationParameters const & simulationParameters)
    : mCurrentSimulationTime(0.0f)
    //
    , mSimulationEventHandler(simulationEventDispatcher)
    , mEventRecorder(nullptr)
    //
    , mAllShips()
    , mStars()
    , mStorm(*this, mSimulationEventHandler)
    , mWind(mSimulationEventHandler)
    , mClouds()
    , mOceanSurface(*this, mSimulationEventHandler)
    , mOceanFloor(std::move(oceanFloorHeightMap))
    , mFishes(fishSpeciesDatabase, mSimulationEventHandler)
    , mNpcs(std::make_unique<Npcs>(*this, npcDatabase, mSimulationEventHandler, simulationParameters))
    //
    , mAllShipExternalAABBs()
{
    // Initialize world pieces that need to be initialized now
    mStars.Update(mCurrentSimulationTime, simulationParameters);
    mStorm.Update(simulationParameters);
    mWind.Update(mStorm.GetParameters(), simulationParameters);
    mClouds.Update(mCurrentSimulationTime, mWind.GetBaseAndStormSpeedMagnitude(), mStorm.GetParameters(), simulationParameters);
    mOceanSurface.Update(mCurrentSimulationTime, mWind, simulationParameters);
    mOceanFloor.Update(simulationParameters);
}

ShipId World::GetNextShipId() const
{
    // FUTUREWORK: for now this is OK as we do not remove ships; when we do, however,
    // this could re-use an existing ID, hence the algo here will need to change
    return static_cast<ShipId>(mAllShips.size());
}

void World::AddShip(std::unique_ptr<Ship> ship)
{
    auto const shipExternalAABBs = ship->CalculateExternalAABBs();

    // Store ship
    assert(ship->GetId() == static_cast<ShipId>(mAllShips.size()));
    mAllShips.push_back(std::move(ship));

    // Tell NPCs
    assert(mNpcs);
    mNpcs->OnShipAdded(*mAllShips.back());

    // Update AABBSet
    for (auto const & aabb : shipExternalAABBs.GetItems())
    {
        mAllShipExternalAABBs.Add(aabb);
    }
}

void World::Announce()
{
    for (auto & ship : mAllShips)
    {
        ship->Announce();
    }

    mNpcs->Announce();
}

void World::SetEventRecorder(EventRecorder * eventRecorder)
{
    mEventRecorder = eventRecorder;

    // Set in all ships
    for (auto & ship : mAllShips)
    {
        ship->SetEventRecorder(eventRecorder);
    }
}

void World::ReplayRecordedEvent(
    RecordedEvent const & event,
    SimulationParameters const & simulationParameters)
{
    for (auto & ship : mAllShips)
    {
        if (ship->ReplayRecordedEvent(event, simulationParameters))
        {
            break;
        }
    }
}

size_t World::GetShipCount() const
{
    return mAllShips.size();
}

size_t World::GetShipPointCount(ShipId shipId) const
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->GetPointCount();
}

size_t World::GetAllShipSpringCount() const
{
    return std::accumulate(
        mAllShips.cbegin(),
        mAllShips.cend(),
        size_t(0),
        [](size_t total, auto const & ship)
        {
            return total + ship->GetSprings().GetElementCount();
        });
}

size_t World::GetAllShipTriangleCount() const
{
    return std::accumulate(
        mAllShips.cbegin(),
        mAllShips.cend(),
        size_t(0),
        [](size_t total, auto const & ship)
        {
            return total + ship->GetTriangles().GetElementCount();
        });
}

std::optional<Geometry::AABB> World::GetLargestShipExternalAABB() const
{
    std::optional<Geometry::AABB> largestAABB;

    for (auto const & aabb : mAllShipExternalAABBs.GetItems())
    {
        if (!largestAABB.has_value() || aabb.CalculateArea() > largestAABB->CalculateArea())
        {
            largestAABB = aabb;
        }
    }

    return largestAABB;
}

Geometry::AABB World::CalculateAllShipParticleAABB() const
{
    Geometry::AABB uberAABB;
    for (auto const & ship : mAllShips)
    {
        uberAABB.ExtendTo(ship->CalculateParticleAABB());
    }

    return uberAABB;
}

bool World::IsUnderwater(GlobalElementId elementId) const
{
    auto const shipId = elementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->IsUnderwater(elementId.GetLocalObjectId());
}

void World::OnBlast(
    ShipId shipId, // None if global
    vec2f const & centerPosition,
    float blastForceMagnitude, // N
    float blastForceRadius, // m
    float blastHeat, // KJ/s
    float blastHeatRadius, // m
    ExplosionType explosionType,
    SimulationParameters const & simulationParameters)
{
    //
    // Blast NPCs
    //

    mNpcs->ApplyBlast(
        shipId,
        centerPosition,
        blastForceMagnitude,
        blastForceRadius,
        blastHeat,
        blastHeatRadius,
        explosionType,
        simulationParameters);

    //
    // Blast ocean surface displacement
    //

    if (simulationParameters.DoDisplaceWater)
    {
        // Explosion depth (positive when underwater)
        float const explosionDepth = mOceanSurface.GetDepth(centerPosition);
        float const absExplosionDepth = std::abs(explosionDepth);

        // No effect when abs depth greater than this
        float constexpr MaxDepth = 20.0f;

        // Calculate (lateral) radius: depends on depth (abs)
        //  radius(depth) = ax + b
        //  radius(0) = maxRadius
        //  radius(maxDepth) = MinRadius;
        float constexpr MinRadius = 1.0f;
        float const maxRadius = 20.0f * blastForceRadius; // Spectacular, spectacular
        float const radius = maxRadius + (absExplosionDepth / MaxDepth * (MinRadius - maxRadius));

        // Calculate displacement: depends on depth
        //  displacement(depth) =  ax^2 + bx + c
        //  f(MaxDepth) = 0
        //  f(0) = MaxDisplacement
        //  f'(MaxDepth) = 0
        float constexpr MaxDisplacement = 6.0f; // Max displacement
        float constexpr a = -MaxDisplacement / (MaxDepth * MaxDepth);
        float constexpr b = 2.0f * MaxDisplacement / MaxDepth;
        float constexpr c = -MaxDisplacement;
        float const displacement =
            (a * absExplosionDepth * absExplosionDepth + b * absExplosionDepth + c)
            * (absExplosionDepth > MaxDepth ? 0.0f : 1.0f) // Turn off at far-away depths
            * (explosionDepth <= 0.0f ? 1.0f : -1.0f); // Follow depth sign

        // Displace
        for (float r = 0.0f; r <= radius; r += 0.5f)
        {
            float const d = displacement * (1.0f - r / radius);
            DisplaceOceanSurfaceAt(centerPosition.x - r, d);
            DisplaceOceanSurfaceAt(centerPosition.x + r, d);
        }
    }

    //
    // Scare fishes
    //

    DisturbOceanAt(
        centerPosition,
        blastForceRadius * 125.0f,
        std::chrono::milliseconds(150));
}


//////////////////////////////////////////////////////////////////////////////
// Interactions
//////////////////////////////////////////////////////////////////////////////

void World::PickConnectedComponentToMove(
    vec2f const & pickPosition,
    float searchRadius,
    std::optional<GlobalConnectedComponentId> & connectedComponentId) const
{
    for (auto & ship : mAllShips)
    {
        auto candidateConnectedComponentId = ship->PickConnectedComponentToMove(
            pickPosition,
            searchRadius);

        if (!!candidateConnectedComponentId)
        {
            connectedComponentId = GlobalConnectedComponentId(ship->GetId(), *candidateConnectedComponentId);
            return;
        }
    }

    connectedComponentId = std::nullopt;
}

void World::MoveBy(
    GlobalConnectedComponentId connectedComponentId,
    vec2f const & moveOffset,
    vec2f const & inertialVelocity,
    SimulationParameters const & simulationParameters)
{
    auto const shipId = connectedComponentId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    // Ship
    mAllShips[shipId]->MoveBy(
        connectedComponentId.GetLocalObjectId(),
        moveOffset,
        inertialVelocity,
        simulationParameters);

    // NPCs
    mNpcs->MoveShipBy(
        shipId,
        connectedComponentId.GetLocalObjectId(),
        moveOffset,
        inertialVelocity,
        simulationParameters);
}

void World::MoveBy(
    ShipId shipId,
    vec2f const & moveOffset,
    vec2f const & inertialVelocity,
    SimulationParameters const & simulationParameters)
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    // Ship
    mAllShips[shipId]->MoveBy(
        moveOffset,
        inertialVelocity,
        simulationParameters);

    // NPCs
    mNpcs->MoveShipBy(
        shipId,
        std::nullopt,
        moveOffset,
        inertialVelocity,
        simulationParameters);
}

void World::RotateBy(
    GlobalConnectedComponentId connectedComponentId,
    float angle,
    vec2f const & center,
    float inertialAngle,
    SimulationParameters const & simulationParameters)
{
    auto const shipId = connectedComponentId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->RotateBy(
        connectedComponentId.GetLocalObjectId(),
        angle,
        center,
        inertialAngle,
        simulationParameters);

    mNpcs->RotateShipBy(
        shipId,
        connectedComponentId.GetLocalObjectId(),
        angle,
        center,
        inertialAngle,
        simulationParameters);
}

void World::RotateBy(
    ShipId shipId,
    float angle,
    vec2f const & center,
    float inertialAngle,
    SimulationParameters const & simulationParameters)
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->RotateBy(
        angle,
        center,
        inertialAngle,
        simulationParameters);

    mNpcs->RotateShipBy(
        shipId,
        std::nullopt,
        angle,
        center,
        inertialAngle,
        simulationParameters);
}

void World::MoveGrippedBy(
    std::vector<GrippedMoveParameters> const & moves,
    SimulationParameters const & simulationParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->MoveGrippedBy(
            moves,
            simulationParameters);
    }
}

void World::RotateGrippedBy(
    vec2f const & gripCenter,
    float gripRadius,
    float angle,
    float inertialAngle,
    SimulationParameters const & simulationParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->RotateGrippedBy(
            gripCenter,
            gripRadius,
            angle,
            inertialAngle,
            simulationParameters);
    }
}

void World::EndMoveGrippedBy(SimulationParameters const & simulationParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->EndMoveGrippedBy(simulationParameters);
    }
}

std::optional<GlobalElementId> World::PickObjectForPickAndPull(
    vec2f const & pickPosition,
    float searchRadius)
{
    for (auto & ship : mAllShips)
    {
        auto elementIndex = ship->PickObjectForPickAndPull(
            pickPosition,
            searchRadius);

        if (elementIndex.has_value())
        {
            return GlobalElementId(ship->GetId(), *elementIndex);
        }
    }

    // No luck
    return std::nullopt;
}

void World::Pull(
    GlobalElementId elementId,
    vec2f const & target,
    SimulationParameters const & simulationParameters)
{
    auto const shipId = elementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->Pull(
        elementId.GetLocalObjectId(),
        target,
        simulationParameters);
}

void World::DestroyAt(
    vec2f const & targetPos,
    float radius,
    SessionId const & sessionId,
    SimulationParameters const & simulationParameters)
{
    // Ships
    for (auto & ship : mAllShips)
    {
        ship->DestroyAt(
            targetPos,
            radius,
            sessionId,
            mCurrentSimulationTime,
            simulationParameters);
    }

    // NPCs
    mNpcs->DestroyAt(
        NoneShipId,
        targetPos,
        radius,
        sessionId,
        mCurrentSimulationTime,
        simulationParameters);

    // Scare fishes at bit
    mFishes.DisturbAt(
        targetPos,
        6.5f,
        std::chrono::milliseconds(0));
}

void World::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    SequenceNumber repairStepId,
    SimulationParameters const & simulationParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->RepairAt(
            targetPos,
            radiusMultiplier,
            repairStepId,
            mCurrentSimulationTime,
            simulationParameters);
    }
}

bool World::SawThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    bool isFirstSegment,
    SimulationParameters const & simulationParameters)
{
    bool atLeastOneCut = false;

    for (auto & ship : mAllShips)
    {
        bool const isCut = ship->SawThrough(
            startPos,
            endPos,
            isFirstSegment,
            mCurrentSimulationTime,
            simulationParameters);

        atLeastOneCut |= isCut;
    }

    return atLeastOneCut;
}

bool World::ApplyHeatBlasterAt(
    vec2f const & targetPos,
    HeatBlasterActionType action,
    float radius,
    SimulationParameters const & simulationParameters)
{
    bool atLeastOneShipApplied = false;

    for (auto & ship : mAllShips)
    {
        bool const isApplied = ship->ApplyHeatBlasterAt(
            targetPos,
            action,
            radius,
            simulationParameters);

        atLeastOneShipApplied |= isApplied;
    }

    // Npcs
    bool const atLeastOneNpcApplied = mNpcs->ApplyHeatBlasterAt(
        NoneShipId,
        targetPos,
        action,
        radius,
        simulationParameters);

    return (atLeastOneShipApplied || atLeastOneNpcApplied);
}

bool World::ExtinguishFireAt(
    vec2f const & targetPos,
    float strengthMultiplier,
    float radius,
    SimulationParameters const & simulationParameters)
{
    bool atLeastOneShipApplied = false;

    for (auto & ship : mAllShips)
    {
        bool isApplied = ship->ExtinguishFireAt(
            targetPos,
            strengthMultiplier,
            radius,
            simulationParameters);

        atLeastOneShipApplied |= isApplied;
    }

    // Npcs
    bool const atLeastOneNpcApplied = mNpcs->ExtinguishFireAt(
        NoneShipId,
        targetPos,
        strengthMultiplier,
        radius,
        simulationParameters);

    return (atLeastOneShipApplied || atLeastOneNpcApplied);
}

void World::ApplyBlastAt(
    vec2f const & targetPos,
    float radius,
    float forceMultiplier,
    SimulationParameters const & simulationParameters)
{
    // Calculate blast force magnitude
    float const blastForceMagnitude =
        75.0f * 50000.0f // Magic number
        * forceMultiplier
        * simulationParameters.BlastToolForceAdjustment
        * (simulationParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto & ship : mAllShips)
    {
        ship->ApplyBlastAt(
            targetPos,
            radius,
            blastForceMagnitude,
            simulationParameters);
    }

    // Apply side-effects
    OnBlast(
        NoneShipId, // Global
        targetPos,
        blastForceMagnitude,
        radius,
        0.0f, // No heat
        0.0f, // No heat
        ExplosionType::Deflagration, // Arbitrary - this gives us side effects we want (forces)
        simulationParameters);
}

bool World::ApplyElectricSparkAt(
    vec2f const & targetPos,
    std::uint64_t counter,
    float lengthMultiplier,
    SimulationParameters const & simulationParameters)
{
    bool atLeastOneShipApplied = false;

    for (auto & ship : mAllShips)
    {
        bool isApplied = ship->ApplyElectricSparkAt(
            targetPos,
            counter,
            lengthMultiplier,
            mCurrentSimulationTime,
            simulationParameters);

        atLeastOneShipApplied |= isApplied;
    }

    return atLeastOneShipApplied;
}

void World::ApplyRadialWindFrom(
    vec2f const & sourcePos,
    float preFrontRadius,
    float preFrontWindSpeed, // m/s
    float mainFrontRadius,
    float mainFrontWindSpeed, // m/s
    SimulationParameters const & simulationParameters)
{
    //
    // Store in Wind, after translating
    //

    float const effectiveAirDensity = Formulae::CalculateAirDensity(
        simulationParameters.AirTemperature,
        simulationParameters);

    // Convert to wind force

    float const preFrontWindForceMagnitude = Formulae::WindSpeedToForceDensity(preFrontWindSpeed, effectiveAirDensity);
    float const mainFrontWindForceMagnitude = Formulae::WindSpeedToForceDensity(mainFrontWindSpeed, effectiveAirDensity);

    // Give to wind
    mWind.SetRadialWindField(
        Wind::RadialWindField(
            sourcePos,
            preFrontRadius,
            preFrontWindForceMagnitude,
            mainFrontRadius,
            mainFrontWindForceMagnitude));
}

bool World::ApplyLaserCannonThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    float strength,
    SimulationParameters const & simulationParameters)
{
    bool atLeastOneShipCut = false;

    // Apply to ships
    for (auto & ship : mAllShips)
    {
        bool const isCut = ship->ApplyLaserCannonThrough(
            startPos,
            endPos,
            strength,
            mCurrentSimulationTime,
            simulationParameters);

        atLeastOneShipCut |= isCut;
    }

    // Npcs
    mNpcs->ApplyLaserCannonThrough(
        NoneShipId,
        startPos,
        endPos,
        strength,
        simulationParameters);

    return atLeastOneShipCut;
}

void World::DrawTo(
    vec2f const & targetPos,
    float strengthFraction,
    SimulationParameters const & simulationParameters)
{
    // Calculate draw force
    float const strength =
        SimulationParameters::DrawForce
        * strengthFraction
        * (simulationParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    // Apply to ships
    if (simulationParameters.DoApplyPhysicsToolsToShips)
    {
        for (auto & ship : mAllShips)
        {
            ship->DrawTo(
                targetPos,
                strength);
        }
    }

    // Apply to NPCs
    if (simulationParameters.DoApplyPhysicsToolsToNpcs)
    {
        assert(mNpcs);
        mNpcs->DrawTo(
            targetPos,
            strength);
    }
}

void World::SwirlAt(
    vec2f const & targetPos,
    float strengthFraction,
    SimulationParameters const & simulationParameters)
{
    // Calculate swirl strength
    float const strength =
        SimulationParameters::SwirlForce
        * strengthFraction
        * (simulationParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    // Apply to ships
    if (simulationParameters.DoApplyPhysicsToolsToShips)
    {
        for (auto & ship : mAllShips)
        {
            ship->SwirlAt(
                targetPos,
                strength);
        }
    }

    // Apply to NPCs
    if (simulationParameters.DoApplyPhysicsToolsToNpcs)
    {
        assert(mNpcs);
        mNpcs->SwirlAt(
            targetPos,
            strength);
    }
}

void World::TogglePinAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully pins or unpins a point
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->TogglePinAt(targetPos, simulationParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::RemoveAllPins()
{
    for (auto & ship : mAllShips)
    {
        ship->RemoveAllPins();
    }
}

std::optional<ToolApplicationLocus> World::InjectPressureAt(
    vec2f const & targetPos,
    float pressureQuantityMultiplier,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully injects pressure
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        auto const applicationLocus = (*it)->InjectPressureAt(
            targetPos,
            pressureQuantityMultiplier,
            simulationParameters);

        if (applicationLocus.has_value())
        {
            // Found!
            return applicationLocus;
        }

        // No luck...
        // search other ships
    }

    // Couldn't inject pressure...
    // ...stop at first ship that successfully injects bubbles now
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        auto const applicationLocus = (*it)->InjectBubblesAt(
            targetPos,
            mCurrentSimulationTime,
            simulationParameters);

        if (applicationLocus.has_value())
        {
            // Found!
            return applicationLocus;
        }

        // No luck...
        // search other ships
    }

    return std::nullopt;
}

bool World::FloodAt(
    vec2f const & targetPos,
    float radius,
    float flowSign,
    SimulationParameters const & simulationParameters)
{
    // Flood all ships
    bool anyHasFlooded = false;
    for (auto & ship : mAllShips)
    {
        bool hasFlooded = ship->FloodAt(
            targetPos,
            radius,
            flowSign,
            simulationParameters);

        anyHasFlooded |= hasFlooded;
    }

    return anyHasFlooded;
}

void World::ToggleAntiMatterBombAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleAntiMatterBombAt(targetPos, simulationParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleFireExtinguishingBombAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleFireExtinguishingBombAt(targetPos, simulationParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleImpactBombAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleImpactBombAt(targetPos, simulationParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

std::optional<bool> World::TogglePhysicsProbeAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully places or removes a probe
    size_t iShip = mAllShips.size();
    do
    {
        --iShip;

        auto result = mAllShips[iShip]->TogglePhysicsProbeAt(targetPos, simulationParameters);
        if (result.has_value())
        {
            // The probe has been placed or removed on this ship

            if (*result)
            {
                // The probe has been placed on this ship, remove it from all others
                for (size_t iShip2 = 0; iShip2 < mAllShips.size(); ++iShip2)
                {
                    if (iShip2 != iShip)
                    {
                        mAllShips[iShip2]->RemovePhysicsProbe();
                    }
                }
            }

            return result;
        }

        // No luck...
        // ...continue with other ships
    } while(iShip > 0);

    return std::nullopt;
}

void World::ToggleRCBombAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleRCBombAt(targetPos, simulationParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleTimerBombAt(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleTimerBombAt(targetPos, simulationParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::DetonateRCBombs(SimulationParameters const & simulationParameters)
{
    for (auto const & ship : mAllShips)
    {
        ship->DetonateRCBombs(mCurrentSimulationTime, simulationParameters);
    }
}

void World::DetonateAntiMatterBombs()
{
    for (auto const & ship : mAllShips)
    {
        ship->DetonateAntiMatterBombs();
    }
}

void World::AdjustOceanSurfaceTo(
    vec2f const & worldCoordinates,
    float worldRadius)
{
    mOceanSurface.AdjustTo(
        worldCoordinates,
        worldRadius);
}

std::optional<bool> World::AdjustOceanFloorTo(
    float x1,
    float targetY1,
    float x2,
    float targetY2)
{
    return mOceanFloor.AdjustTo(x1, targetY1, x2, targetY2);
}

bool World::ScrubThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    SimulationParameters const & simulationParameters)
{
    // Scrub all ships
    bool anyHasScrubbed = false;
    for (auto & ship : mAllShips)
    {
        bool const hasScrubbed = ship->ScrubThrough(
            startPos,
            endPos,
            simulationParameters);

        anyHasScrubbed |= hasScrubbed;
    }

    return anyHasScrubbed;
}

bool World::RotThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    SimulationParameters const & simulationParameters)
{
    // Rot all ships
    bool anyHasRotted = false;
    for (auto & ship : mAllShips)
    {
        bool const hasRotted = ship->RotThrough(
            startPos,
            endPos,
            simulationParameters);

        anyHasRotted |= hasRotted;
    }

    return anyHasRotted;
}

void World::ApplyThanosSnap(
    float centerX,
    float radius,
    float leftFrontX,
    float rightFrontX,
    bool isSparseMode,
    SimulationParameters const & simulationParameters)
{
    // Apply to all ships
    for (auto & ship : mAllShips)
    {
        ship->ApplyThanosSnap(
            centerX,
            radius,
            leftFrontX,
            rightFrontX,
            isSparseMode,
            mCurrentSimulationTime,
            simulationParameters);
    }

    // Apply to ocean surface
    mOceanSurface.ApplyThanosSnap(leftFrontX, rightFrontX);

    // Apply to fishes

    float constexpr DisturbanceRadius = 100.0f;

    mFishes.DisturbAt(
        vec2f(leftFrontX, 0.0f),
        DisturbanceRadius,
        std::chrono::milliseconds(0));

    mFishes.DisturbAt(
        vec2f(rightFrontX, 0.0f),
        DisturbanceRadius,
        std::chrono::milliseconds(0));
}

std::optional<GlobalElementId> World::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    std::optional<GlobalElementId> bestPointId;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const & ship : mAllShips)
    {
        auto shipBestPointIndex = ship->GetNearestPointAt(targetPos, radius);
        if (NoneElementIndex != shipBestPointIndex)
        {
            float squareDistance = (ship->GetPoints().GetPosition(shipBestPointIndex) - targetPos).squareLength();
            if (squareDistance < bestSquareDistance)
            {
                bestPointId = GlobalElementId(ship->GetId(), shipBestPointIndex);
                bestSquareDistance = squareDistance;
            }
        }
    }

    return bestPointId;
}

void World::QueryNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    // Stop at first ship that successfully queries
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->QueryNearestPointAt(targetPos, radius))
            return;
    }
}

void World::QueryNearestNpcAt(
    vec2f const & targetPos,
    float radius) const
{
    mNpcs->QueryNearestNpcAt(targetPos, radius);
}

std::optional<vec2f> World::FindSuitableLightningTarget() const
{
    // Try all ships until a target is found
    for (auto const & ship : mAllShips)
    {
        auto target = ship->FindSuitableLightningTarget();
        if (!!target)
            return target;
    }

    return std::nullopt;
}

void World::ApplyLightning(
    vec2f const & targetPos,
    SimulationParameters const & simulationParameters)
{
    // Apply to all ships
    for (auto & ship : mAllShips)
    {
        ship->ApplyLightning(targetPos, mCurrentSimulationTime, simulationParameters);
    }

    // Apply to fishes
    DisturbOceanAt(
        targetPos,
        500.0f,
        std::chrono::milliseconds(0));
}

void World::TriggerTsunami()
{
    mOceanSurface.TriggerTsunami(mCurrentSimulationTime);

    DisturbOcean(std::chrono::milliseconds(0));
}

void World::TriggerStorm()
{
    mStorm.TriggerStorm();
}

void World::TriggerLightning(SimulationParameters const & simulationParameters)
{
    mStorm.TriggerLightning(simulationParameters);
}

void World::TriggerLightningAt(vec2f const & targetWorldPosition)
{
    mStorm.TriggerForegroundLightningAt(targetWorldPosition);
}

void World::TriggerRogueWave()
{
    mOceanSurface.TriggerRogueWave(
        mCurrentSimulationTime,
        mWind);
}

void World::HighlightElectricalElement(GlobalElectricalElementId electricalElementId)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->HighlightElectricalElement(electricalElementId);
}

void World::SetSwitchState(
    GlobalElectricalElementId electricalElementId,
    ElectricalState switchState,
    SimulationParameters const & simulationParameters)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->SetSwitchState(
        electricalElementId,
        switchState,
        simulationParameters);
}

void World::SetEngineControllerState(
    GlobalElectricalElementId electricalElementId,
    float controllerValue,
    SimulationParameters const & simulationParameters)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->SetEngineControllerState(
        electricalElementId,
        controllerValue,
        simulationParameters);
}

void World::SetSilence(float silenceAmount)
{
    mWind.SetSilence(silenceAmount);
}

void World::ScareFish(
    vec2f const & position,
    float radius,
    std::chrono::milliseconds delay)
{
    mFishes.DisturbAt(position, radius, delay);
}

void World::AttractFish(
    vec2f const & position,
    float radius,
    std::chrono::milliseconds delay)
{
    mFishes.AttractAt(position, radius, delay);
}

NpcKindType World::GetNpcKind(NpcId id)
{
    assert(mNpcs);
    return mNpcs->GetNpcKind(id);
}

NpcPlacementOutcome World::BeginPlaceNewFurnitureNpc(
    std::optional<NpcSubKindIdType> subKind,
    vec2f const & position,
    bool doMoveWholeMesh)
{
    assert(mNpcs);
    return mNpcs->BeginPlaceNewFurnitureNpc(
        subKind,
        position,
        doMoveWholeMesh,
        mCurrentSimulationTime);
}

NpcPlacementOutcome World::BeginPlaceNewHumanNpc(
    std::optional<NpcSubKindIdType> subKind,
    vec2f const & position,
    bool doMoveWholeMesh)
{
    assert(mNpcs);
    return mNpcs->BeginPlaceNewHumanNpc(
        subKind,
        position,
        doMoveWholeMesh,
        mCurrentSimulationTime);
}

std::optional<PickedNpc> World::ProbeNpcAt(
    vec2f const & position,
    float radius,
    SimulationParameters const & simulationParameters) const
{
    assert(mNpcs);
    return mNpcs->ProbeNpcAt(
        position,
        radius,
        simulationParameters);
}

std::vector<NpcId> World::ProbeNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2) const
{
    assert(mNpcs);
    return mNpcs->ProbeNpcsInRect(
        corner1,
        corner2);
}

void World::BeginMoveNpc(
    NpcId id,
    int particleOrdinal,
    bool doMoveWholeMesh)
{
    assert(mNpcs);
    mNpcs->BeginMoveNpc(
        id,
        particleOrdinal,
        mCurrentSimulationTime,
        doMoveWholeMesh);
}

void World::BeginMoveNpcs(std::vector<NpcId> const & ids)
{
    assert(mNpcs);
    mNpcs->BeginMoveNpcs(
        ids,
        mCurrentSimulationTime);
}

void World::MoveNpcTo(
    NpcId id,
    vec2f const & position,
    vec2f const & offset,
    bool doMoveWholeMesh)
{
    assert(mNpcs);
    mNpcs->MoveNpcTo(
        id,
        position,
        offset,
        doMoveWholeMesh);
}

void World::MoveNpcsBy(
    std::vector<NpcId> const & ids,
    vec2f const & stride)
{
    assert(mNpcs);
    mNpcs->MoveNpcsBy(
        ids,
        stride);
}

void World::EndMoveNpc(NpcId id)
{
    assert(mNpcs);
    mNpcs->EndMoveNpc(id, mCurrentSimulationTime);
}

void World::CompleteNewNpc(NpcId id)
{
    assert(mNpcs);
    mNpcs->CompleteNewNpc(id, mCurrentSimulationTime);
}

void World::RemoveNpc(NpcId id)
{
    assert(mNpcs);
    mNpcs->RemoveNpc(id, mCurrentSimulationTime);
}

void World::RemoveNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2)
{
    assert(mNpcs);
    mNpcs->RemoveNpcsInRect(corner1, corner2, mCurrentSimulationTime);
}

void World::AbortNewNpc(NpcId id)
{
    assert(mNpcs);
    mNpcs->AbortNewNpc(id);
}

NpcPlacementFailureReasonType World::AddNpcGroup(
    NpcKindType kind,
    VisibleWorld const & visibleWorld,
    SimulationParameters const & simulationParameters)
{
    assert(mNpcs);
    return mNpcs->AddNpcGroup(kind, visibleWorld, mCurrentSimulationTime, simulationParameters);
}

void World::TurnaroundNpc(NpcId id)
{
    assert(mNpcs);
    mNpcs->TurnaroundNpc(id);
}

void World::TurnaroundNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2)
{
    assert(mNpcs);
    mNpcs->TurnaroundNpcsInRect(corner1, corner2);
}

std::optional<NpcId> World::GetSelectedNpc() const
{
    assert(mNpcs);
    return mNpcs->GetCurrentlySelectedNpc();
}

void World::SelectFirstNpc()
{
    assert(mNpcs);
    mNpcs->SelectFirstNpc();
}

void World::SelectNextNpc()
{
    assert(mNpcs);
    mNpcs->SelectNextNpc();
}

void World::SelectNpc(std::optional<NpcId> id)
{
    assert(mNpcs);
    mNpcs->SelectNpc(id);
}

void World::HighlightNpcs(std::vector<NpcId> const & ids)
{
    assert(mNpcs);
    mNpcs->HighlightNpcs(ids);
}

void World::HighlightNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2)
{
    assert(mNpcs);
    mNpcs->HighlightNpcsInRect(corner1, corner2);
}

bool World::DestroyTriangle(GlobalElementId triangleId)
{
    auto const shipId = triangleId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->DestroyTriangle(triangleId.GetLocalObjectId());
}

bool World::RestoreTriangle(GlobalElementId triangleId)
{
    auto const shipId = triangleId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->RestoreTriangle(triangleId.GetLocalObjectId());
}

//////////////////////////////////////////////////////////////////////////////
// Simulation
//////////////////////////////////////////////////////////////////////////////

void World::Update(
    SimulationParameters const & simulationParameters,
    ViewModel const & viewModel,
    StressRenderModeType stressRenderMode,
    ThreadManager & threadManager,
    PerfStats & perfStats)
{
    // Update current time
    mCurrentSimulationTime += SimulationParameters::SimulationStepTimeDuration<float>;

    // Prepare all AABBs
    mAllShipExternalAABBs.Clear();

    //
    // Update all subsystems
    //

    mStars.Update(mCurrentSimulationTime, simulationParameters);

    mStorm.Update(simulationParameters);

    mWind.Update(mStorm.GetParameters(), simulationParameters);

    mClouds.Update(mCurrentSimulationTime, mWind.GetBaseAndStormSpeedMagnitude(), mStorm.GetParameters(), simulationParameters);

    mOceanSurface.Update(mCurrentSimulationTime, mWind, simulationParameters);

    mOceanFloor.Update(simulationParameters);

    for (auto & ship : mAllShips)
    {
        ship->Update(
            mCurrentSimulationTime,
            mStorm.GetParameters(),
            simulationParameters,
            stressRenderMode,
            mAllShipExternalAABBs,
            threadManager,
            perfStats);
    }

    {
        auto const startTime = std::chrono::steady_clock::now();

        assert(mNpcs);
        mNpcs->Update(mCurrentSimulationTime, mStorm.GetParameters(), simulationParameters);

        perfStats.Update<PerfMeasurement::TotalNpcUpdate>(std::chrono::steady_clock::now() - startTime);
    }

    {
        auto const startTime = std::chrono::steady_clock::now();

        mFishes.Update(mCurrentSimulationTime, mOceanSurface, mOceanFloor, simulationParameters, viewModel.GetVisibleWorld(), mAllShipExternalAABBs);

        perfStats.Update<PerfMeasurement::TotalFishUpdate>(std::chrono::steady_clock::now() - startTime);
    }

    //
    // Signal update end (for quantities that needed to persist during whole Update cycle)
    //

    mWind.UpdateEnd();

    for (auto & ship : mAllShips)
    {
        ship->UpdateEnd();
    }

    mNpcs->UpdateEnd();
}

void World::RenderUpload(
    SimulationParameters const & simulationParameters,
    RenderContext & renderContext)
{
    mStars.Upload(renderContext);

    mWind.Upload(renderContext);

    mStorm.Upload(renderContext);

    mClouds.Upload(renderContext);

    mOceanFloor.Upload(simulationParameters, renderContext);

    mOceanSurface.Upload(renderContext);

    mFishes.Upload(renderContext);

    // Ships
    {
        renderContext.UploadShipsStart();

        for (auto const & ship : mAllShips)
        {
            ship->RenderUpload(renderContext);
        }

        renderContext.UploadShipsEnd();
    }

    assert(mNpcs);
    mNpcs->Upload(renderContext);

    // AABBs
    if (renderContext.GetShowAABBs())
    {
        renderContext.UploadAABBsStart(mAllShipExternalAABBs.GetCount());

        auto constexpr ShipAABBColor = rgbaColor(18, 8, 255, 255).toVec4f();

        for (auto const & aabb : mAllShipExternalAABBs.GetItems())
        {
            renderContext.UploadAABB(
                aabb,
                ShipAABBColor);
        }

        renderContext.UploadAABBsEnd();
    }
}

}