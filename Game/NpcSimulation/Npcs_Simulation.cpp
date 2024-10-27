/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2023-07-23
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include <GameCore/Conversions.h>
#include <GameCore/GameMath.h>

#include <array>
#include <limits>

#pragma warning(disable : 4324) // std::optional of StateType gets padded because of alignment requirements

namespace Physics {

void Npcs::InternalEndMoveNpc(
    NpcId id,
    float currentSimulationTime,
    NpcInitializationOptions options)
{
    assert(mStateBuffer[id].has_value());

    auto & npc = *mStateBuffer[id];

    assert(npc.CurrentRegime == StateType::RegimeType::BeingPlaced);

    ResetNpcStateToWorld(
        npc,
        currentSimulationTime,
        options);

    OnMayBeNpcRegimeChanged(
        StateType::RegimeType::BeingPlaced,
        npc);

    npc.BeingPlacedState.reset();

#ifdef IN_BARYLAB
    // Select NPC's primary particle
    SelectParticle(npc.ParticleMesh.Particles[0].ParticleIndex);
#endif
}

void Npcs::InternalCompleteNewNpc(
    NpcId id,
    float currentSimulationTime,
    NpcInitializationOptions options)
{
    InternalEndMoveNpc(id, currentSimulationTime, options);
}

void Npcs::ResetNpcStateToWorld(
    StateType & npc,
    float currentSimulationTime,
    NpcInitializationOptions options)
{
    //
    // Find topmost triangle - among all ships - which contains this NPC
    //

    // Take the position of the primary particle as representative of the NPC
    auto const & primaryPosition = mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex);

    auto const topmostTriangle = FindTopmostWorkableTriangleContaining(primaryPosition);
    if (topmostTriangle.has_value())
    {
        // Primary is in a triangle!

        assert(mShips[topmostTriangle->GetShipId()].has_value());

        TransferNpcToShip(
            npc,
            topmostTriangle->GetShipId());

        ResetNpcStateToWorld(
            npc,
            currentSimulationTime,
            mShips[topmostTriangle->GetShipId()]->HomeShip,
            topmostTriangle->GetLocalObjectId(),
            options);
    }
    else
    {
        // No luck; means we're free, and pick topmost ship for that

        auto const topmostShipId = GetTopmostShipId();

        assert(mShips[topmostShipId].has_value());

        TransferNpcToShip(
            npc,
            topmostShipId);

        ResetNpcStateToWorld(
            npc,
            currentSimulationTime,
            mShips[GetTopmostShipId()]->HomeShip,
            std::nullopt,
            options);
    }
}

void Npcs::ResetNpcStateToWorld(
    StateType & npc,
    float currentSimulationTime,
    Ship const & homeShip,
    std::optional<ElementIndex> primaryParticleTriangleIndex,
    NpcInitializationOptions options)
{
    // Plane ID, connected component ID

    if (primaryParticleTriangleIndex.has_value())
    {
        // Constrained

        // Use the plane ID of this triangle
        ElementIndex const trianglePointIndex = homeShip.GetTriangles().GetPointAIndex(*primaryParticleTriangleIndex);
        npc.CurrentPlaneId = homeShip.GetPoints().GetPlaneId(trianglePointIndex);

        // Use the connected component ID of this triangle
        npc.CurrentConnectedComponentId = homeShip.GetPoints().GetConnectedComponentId(trianglePointIndex);
    }
    else
    {
        // Primary is free, hence this NPC is on the topmost plane ID of its current ship;
        // fine to stick to this plane so that if new planes come up, they will cover the NPC!
        npc.CurrentPlaneId = homeShip.GetMaxPlaneId();

        // Primary is free, hence this NPC does not belong to any connected components
        npc.CurrentConnectedComponentId.reset();
    }

    // Particles

    for (size_t p = 0; p < npc.ParticleMesh.Particles.size(); ++p)
    {
        ElementIndex const particleIndex = npc.ParticleMesh.Particles[p].ParticleIndex;

        if (p == 0)
        {
            // Primary
            npc.ParticleMesh.Particles[p].ConstrainedState = CalculateParticleConstrainedState(
                mParticles.GetPosition(particleIndex),
                homeShip,
                primaryParticleTriangleIndex,
                std::nullopt); // No need to search
        }
        else
        {
            // Secondaries

            if (!npc.ParticleMesh.Particles[0].ConstrainedState.has_value())
            {
                // When primary is free, also secondary is free (and thus whole NPC)
                if (npc.ParticleMesh.Particles[p].ConstrainedState.has_value())
                {
                    npc.ParticleMesh.Particles[p].ConstrainedState.reset();
                }
            }
            else
            {
                npc.ParticleMesh.Particles[p].ConstrainedState = CalculateParticleConstrainedState(
                    mParticles.GetPosition(particleIndex),
                    homeShip,
                    std::nullopt,
                    npc.CurrentConnectedComponentId); // Constrain this secondary's triangle to NPC's connected component ID
            }
        }
    }

    if ((options & NpcInitializationOptions::GainMeshVelocity) != NpcInitializationOptions::None)
    {
        // Give all particles the velocity of the primary's mesh

        if (npc.ParticleMesh.Particles[0].ConstrainedState.has_value())
        {
            vec2f const primaryMeshVelocity = homeShip.GetPoints().GetVelocity(
                homeShip.GetTriangles().GetPointAIndex(
                    npc.ParticleMesh.Particles[0].ConstrainedState->CurrentBCoords.TriangleElementIndex));

            for (size_t p = 1; p < npc.ParticleMesh.Particles.size(); ++p)
            {
                mParticles.SetVelocity(
                    npc.ParticleMesh.Particles[p].ParticleIndex,
                    primaryMeshVelocity);
            }
        }
    }

    // Regime

    npc.CurrentRegime = CalculateRegime(npc);

    // Kind specific

    switch (npc.Kind)
    {
        case NpcKindType::Furniture:
        {
            // Nop

            break;
        }

        case NpcKindType::Human:
        {
            // Init behavior
            npc.KindSpecificState.HumanNpcState.TransitionToState(
                CalculateHumanBehavior(npc),
                currentSimulationTime);

            break;
        }
    }

#ifdef IN_BARYLAB
    Publish();
#endif
}

void Npcs::TransitionParticleToConstrainedState(
    StateType & npc,
    int npcParticleOrdinal,
    StateType::NpcParticleStateType::ConstrainedStateType constrainedState)
{
    // Transition
    npc.ParticleMesh.Particles[npcParticleOrdinal].ConstrainedState = constrainedState;

    // Regime
    auto const oldRegime = npc.CurrentRegime;
    npc.CurrentRegime = CalculateRegime(npc);
    OnMayBeNpcRegimeChanged(oldRegime, npc);

    // We'll update plane ID for constrained NPCs at end of Update()
}

void Npcs::TransitionParticleToFreeState(
    StateType & npc,
    int npcParticleOrdinal,
    Ship const & homeShip)
{
    // Transition
    npc.ParticleMesh.Particles[npcParticleOrdinal].ConstrainedState.reset();
    if (npcParticleOrdinal == 0)
    {
        //
        // When primary is free, also secondaries are free - and thus transition whole NPC
        //

        for (size_t p = 1; p < npc.ParticleMesh.Particles.size(); ++p)
        {
            if (npc.ParticleMesh.Particles[p].ConstrainedState.has_value())
            {
                npc.ParticleMesh.Particles[p].ConstrainedState.reset();
            }
        }

        npc.CurrentPlaneId = homeShip.GetMaxPlaneId();
        npc.CurrentConnectedComponentId.reset();
    }

    // Regime
    auto const oldRegime = npc.CurrentRegime;
    npc.CurrentRegime = CalculateRegime(npc);
    OnMayBeNpcRegimeChanged(oldRegime, npc);
}

std::optional<Npcs::StateType::NpcParticleStateType::ConstrainedStateType> Npcs::CalculateParticleConstrainedState(
    vec2f const & position,
    Ship const & homeShip,
    std::optional<ElementIndex> triangleIndex,
    std::optional<ConnectedComponentId> constrainedConnectedComponentId)
{
    std::optional<StateType::NpcParticleStateType::ConstrainedStateType> constrainedState;

    if (!triangleIndex.has_value())
    {
        triangleIndex = FindWorkableTriangleContaining(position, homeShip, constrainedConnectedComponentId);
    }

    assert(triangleIndex.has_value());
    // At this point, if we have a triangle it's not deleted (nor folded)
    assert(*triangleIndex == NoneElementIndex || !homeShip.GetTriangles().IsDeleted(*triangleIndex));

    if (triangleIndex != NoneElementIndex)
    {
        bcoords3f const barycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinatesFromWithinTriangle(
            position,
            *triangleIndex,
            homeShip.GetPoints());

        assert(barycentricCoords.is_on_edge_or_internal());

        return Npcs::StateType::NpcParticleStateType::ConstrainedStateType(
            *triangleIndex,
            barycentricCoords);
    }

    return std::nullopt;
}

void Npcs::OnMayBeNpcRegimeChanged(
    StateType::RegimeType oldRegime,
    StateType const & npc)
{
    assert(oldRegime != StateType::RegimeType::BeingPlaced || npc.BeingPlacedState.has_value());
    if (oldRegime == StateType::RegimeType::BeingPlaced && npc.BeingPlacedState->PreviousRegime.has_value())
    {
        oldRegime = *(npc.BeingPlacedState->PreviousRegime);
    }

    if (oldRegime == npc.CurrentRegime)
    {
        // Nothing to do
        return;
    }

    if (npc.Kind == NpcKindType::Human)
    {
        //
        // Update stats
        //

        bool doPublishStats = false;
        if (oldRegime == StateType::RegimeType::Constrained)
        {
            assert(mConstrainedRegimeHumanNpcCount > 0);
            --mConstrainedRegimeHumanNpcCount;
            doPublishStats = true;
        }
        else if (oldRegime == StateType::RegimeType::Free)
        {
            assert(mFreeRegimeHumanNpcCount > 0);
            --mFreeRegimeHumanNpcCount;
            doPublishStats = true;
        }

        if (npc.CurrentRegime == StateType::RegimeType::Constrained)
        {
            ++mConstrainedRegimeHumanNpcCount;
            doPublishStats = true;
        }
        else if (npc.CurrentRegime == StateType::RegimeType::Free)
        {
            ++mFreeRegimeHumanNpcCount;
            doPublishStats = true;
        }

        if (doPublishStats)
        {
            PublishHumanNpcStats();
        }
    }
}

Npcs::StateType::RegimeType Npcs::CalculateRegime(StateType const & npc)
{
    // Constrained iff primary is constrained
    assert(npc.ParticleMesh.Particles.size() > 0);
    return npc.ParticleMesh.Particles[0].ConstrainedState.has_value()
        ? StateType::RegimeType::Constrained
        : StateType::RegimeType::Free;
}

void Npcs::UpdateNpcs(
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    LogNpcDebug("----------------------------------");
    LogNpcDebug("----------------------------------");
    LogNpcDebug("----------------------------------");

    //
    // 1. Reset buffers
    //

    // Note: no need to reset PreliminaryForces as we'll recalculate all of them

    //
    // 2. Low-frequency and high-frequency updates of Npc and NpcParticle attributes
    // 3. Check if a particle's constrained state is still valid (deleted/folded triangles)
    // 4. Check if a free secondary particle should become constrained
    // 5. Calculate preliminary forces
    // 6. Calculate spring forces
    // 7. Update physical state
    // 8. Maintain world bounds
    //

    // Calculate all physics constants needed for physics update

    float const effectiveAirDensity = Formulae::CalculateAirDensity(
        gameParameters.AirTemperature + stormParameters.AirTemperatureDelta,
        gameParameters);

    vec2f const globalWindForce = Formulae::WindSpeedToForceDensity(
        Conversions::KmhToMs(mParentWorld.GetCurrentWindSpeed()),
        effectiveAirDensity);

    ////// FUTUREWORK
    ////float const effectiveWaterDensity = Formulae::CalculateWaterDensity(
    ////    gameParameters.WaterTemperature,
    ////    gameParameters);

    // Visit all NPCs

    for (auto & npcState : mStateBuffer)
    {
        if (npcState.has_value())
        {
            assert(mShips[npcState->CurrentShipId].has_value());
            auto & homeShip = mShips[npcState->CurrentShipId]->HomeShip;

            // Invariant checks

            assert((npcState->CurrentRegime == StateType::RegimeType::BeingPlaced) == npcState->BeingPlacedState.has_value());
            assert(npcState->ParticleMesh.Particles.size() > 0);

            // Low-frequency updates

            unsigned int constexpr LowFrequencyUpdatePeriod = 4;
            if (mCurrentSimulationSequenceNumber.IsStepOf(npcState->Id % LowFrequencyUpdatePeriod, LowFrequencyUpdatePeriod))
            {
                // Waterness, Water Velocity, Combustion

                bool atLeastOneNpcParticleOnFire = false;
                bool atLeastOneNpcParticleInFreeWater = false;

                for (auto p = 0; p < npcState->ParticleMesh.Particles.size(); ++p)
                {
                    auto const & particle = npcState->ParticleMesh.Particles[p];

                    if (particle.ConstrainedState.has_value())
                    {
                        auto const t = particle.ConstrainedState->CurrentBCoords.TriangleElementIndex;

                        float totalWaterness = 0.0f;
                        vec2f totalWaterVelocity = vec2f::zero();
                        float waterablePointCount = 0.0f;
                        bool isAtLeastOneMeshPointOnFire = false;
                        for (int v = 0; v < 3; ++v)
                        {
                            ElementIndex const pointElementIndex = homeShip.GetTriangles().GetPointIndices(t)[v];

                            float const w = std::min(homeShip.GetPoints().GetWater(pointElementIndex), 1.0f);
                            totalWaterness += w;

                            totalWaterVelocity += homeShip.GetPoints().GetWaterVelocity(pointElementIndex) * w;

                            if (!homeShip.GetPoints().GetIsHull(pointElementIndex))
                                waterablePointCount += 1.0f;

                            if (homeShip.GetPoints().GetTemperature(pointElementIndex) >= mParticles.GetMaterial(particle.ParticleIndex).IgnitionTemperature * gameParameters.IgnitionTemperatureAdjustment
                                || homeShip.GetPoints().IsBurning(pointElementIndex))
                            {
                                isAtLeastOneMeshPointOnFire = true;
                            }
                        }

                        float const meshWaterness = totalWaterness / (std::max(waterablePointCount, 1.0f));
                        mParticles.SetMeshWaterness(particle.ParticleIndex, meshWaterness);

                        vec2f const meshWaterVelocity = totalWaterVelocity / (std::max(waterablePointCount, 1.0f));
                        mParticles.SetMeshWaterVelocity(particle.ParticleIndex, meshWaterVelocity);

                        if (meshWaterness < 0.4f) // Otherwise too much water for fire
                        {
                            atLeastOneNpcParticleOnFire = atLeastOneNpcParticleOnFire || isAtLeastOneMeshPointOnFire;
                        }
                    }
                    else
                    {
                        // Free - check if underwater (using AnyWaterness as proxy)
                        if (mParticles.GetAnyWaterness(particle.ParticleIndex) > 0.4f)
                        {
                            atLeastOneNpcParticleInFreeWater = true;
                        }
                    }
                } // For all NPC particles

                // Update NPC's fire progress

                if (atLeastOneNpcParticleInFreeWater)
                {
                    // Smother immediately
                    npcState->CombustionProgress += (-1.0f - npcState->CombustionProgress) * 0.1f;
                }
                else
                {
                    if (atLeastOneNpcParticleOnFire)
                    {
                        // Increase
                        npcState->CombustionProgress += (1.0f - npcState->CombustionProgress) * 0.3f;
                    }
                    else
                    {
                        // Decrease
                        npcState->CombustionProgress += (-1.0f - npcState->CombustionProgress) * 0.007f;
                    }
                }
            }

            // High-frequency updates

            {
                // Combustion state machine: ignite or smother

                if (npcState->CombustionProgress > 0.0f)
                {
                    // See if we've just ignited
                    if (!npcState->CombustionState.has_value())
                    {
                        // Init state (will be evolved right now)
                        npcState->CombustionState.emplace(
                            vec2f(0.0f, 1.0f),
                            0.0f);

                        // Add to burning set
                        auto & shipNpcs = *mShips[npcState->CurrentShipId];
                        assert(std::find(shipNpcs.BurningNpcs.cbegin(), shipNpcs.BurningNpcs.cend(), npcState->Id) == shipNpcs.BurningNpcs.cend());
                        shipNpcs.BurningNpcs.push_back(npcState->Id);

                        // Emit event
                        mGameEventHandler->OnPointCombustionBegin();
                    }

                    // Update flame progress
                    ElementIndex reprParticleIndex = npcState->Kind == NpcKindType::Human // Approx
                        ? npcState->ParticleMesh.Particles[1].ParticleIndex
                        : npcState->ParticleMesh.Particles[0].ParticleIndex;
                    Formulae::EvolveFlameGeometry(
                        npcState->CombustionState->FlameVector,
                        npcState->CombustionState->FlameWindRotationAngle,
                        mParticles.GetPosition(reprParticleIndex),
                        // Exhaggerate; using absolute V (instead of more correct rel) to be in sync w/Ship
                        mParticles.GetVelocity(reprParticleIndex) * 4.0f,
                        mParentWorld.GetCurrentWindSpeed(),
                        mParentWorld.GetCurrentRadialWindField());
                }
                else
                {
                    // See if we've stopped
                    if (npcState->CombustionState.has_value())
                    {
                        // Reset combustion state
                        npcState->CombustionState.reset();

                        // Remove from burning set
                        auto & shipNpcs = *mShips[npcState->CurrentShipId];
                        auto npcIt = std::find(shipNpcs.BurningNpcs.begin(), shipNpcs.BurningNpcs.end(), npcState->Id);
                        assert(npcIt != shipNpcs.BurningNpcs.end());
                        shipNpcs.BurningNpcs.erase(npcIt);

                        // Emit event
                        mGameEventHandler->OnPointCombustionEnd();
                    }
                }
            }

            // Check validity of constrained triangles, and calculate preliminary forces

            for (auto p = 0; p < npcState->ParticleMesh.Particles.size(); ++p)
            {
                auto const & particleConstrainedState = npcState->ParticleMesh.Particles[p].ConstrainedState;
                if (particleConstrainedState.has_value())
                {
                    // Constrained any particle: check if its triangle is still valid

                    // If triangle is not workable anymore, become free
                    if (homeShip.GetTriangles().IsDeleted(particleConstrainedState->CurrentBCoords.TriangleElementIndex)
                        || IsTriangleFolded(particleConstrainedState->CurrentBCoords.TriangleElementIndex, homeShip))
                    {
                        TransitionParticleToFreeState(*npcState, p, homeShip);
                    }
                }
                else if (p > 0)
                {
                    // Secondary free: check if should become constrained

                    if (npcState->ParticleMesh.Particles[0].ConstrainedState.has_value())
                    {
                        auto newConstrainedState = CalculateParticleConstrainedState(
                            mParticles.GetPosition(npcState->ParticleMesh.Particles[p].ParticleIndex),
                            homeShip,
                            std::nullopt,
                            npcState->CurrentConnectedComponentId); // Constrain search to NPC's connected component

                        if (newConstrainedState.has_value())
                        {
                            // Make this secondary constrained
                            TransitionParticleToConstrainedState(*npcState, static_cast<int>(p), std::move(*newConstrainedState));
                        }
                    }
                }

                // Preliminary forces

                CalculateNpcParticlePreliminaryForces(
                    *npcState,
                    p,
                    globalWindForce,
                    gameParameters);
            }

            // Spring forces for whole NPC

            CalculateNpcParticleSpringForces(*npcState);

            // Update physical state for all particles and maintain world bounds

            for (auto p = 0; p < npcState->ParticleMesh.Particles.size(); ++p)
            {
                UpdateNpcParticlePhysics(
                    *npcState,
                    static_cast<int>(p),
                    homeShip,
                    currentSimulationTime,
                    gameParameters);

                MaintainInWorldBounds(
                    *npcState,
                    p,
                    homeShip,
                    gameParameters);

                if (npcState->CurrentRegime == StateType::RegimeType::Free)
                {
                    // Only maintain over land if _all_ particles are free
                    MaintainOverLand(
                        *npcState,
                        p,
                        homeShip,
                        gameParameters);
                }
            }
        }
    }

    //
    // 9. Update behavioral state machines
    // 10. Update animation
    //

    LogNpcDebug("----------------------------------");

    for (auto & npcState : mStateBuffer)
    {
        if (npcState.has_value())
        {
            assert(mShips[npcState->CurrentShipId].has_value());
            auto & homeShip = mShips[npcState->CurrentShipId]->HomeShip;

            // Behavior

            if (npcState->Kind == NpcKindType::Human)
            {
                UpdateHuman(
                    *npcState,
                    currentSimulationTime,
                    homeShip,
                    gameParameters);
            }

            // Animation

            UpdateNpcAnimation(
                *npcState,
                currentSimulationTime,
                homeShip);
        }
    }
}

void Npcs::UpdateNpcsEnd()
{
    // We consume this one
    mParticles.ResetExternalForces();
}

void Npcs::UpdateNpcParticlePhysics(
    StateType & npc,
    int npcParticleOrdinal,
    Ship & homeShip,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Here be dragons!
    //

    auto & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];

    LogNpcDebug("----------------------------------");
    LogNpcDebug("  Particle ", npcParticleOrdinal);

    float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;
    float const particleMass = mParticles.GetMass(npcParticle.ParticleIndex);

    vec2f const particleStartAbsolutePosition = mParticles.GetPosition(npcParticle.ParticleIndex);

    // Calculate physical displacement - once and for all, as whole loop
    // will attempt to move to trajectory that always ends here

    vec2f physicsDeltaPos;
#ifdef IN_BARYLAB
    if (mCurrentParticleTrajectory.has_value() && npcParticle.ParticleIndex == mCurrentParticleTrajectory->ParticleIndex)
    {
        // Consume externally-supplied trajectory

        physicsDeltaPos = mCurrentParticleTrajectory->TargetPosition - particleStartAbsolutePosition;
        mCurrentParticleTrajectory.reset();
    }
    else
#endif
    {
        // Integrate forces

        vec2f const physicalForces = CalculateNpcParticleDefinitiveForces(
            npc,
            npcParticleOrdinal,
            gameParameters);

        physicsDeltaPos = mParticles.GetVelocity(npcParticle.ParticleIndex) * dt + (physicalForces / particleMass) * dt * dt;

        LogNpcDebug("    physicsDeltaPos=", physicsDeltaPos, " ( v=", mParticles.GetVelocity(npcParticle.ParticleIndex) * dt,
            " f=", (physicalForces / particleMass) * dt * dt, ")");
    }


    if (npc.CurrentRegime == StateType::RegimeType::BeingPlaced)
    {
        //
        // Being placed
        //


        UpdateNpcParticle_BeingPlaced(
            npc,
            npcParticleOrdinal,
            physicsDeltaPos,
            mParticles);
    }
    else if (!npcParticle.ConstrainedState.has_value())
    {
        //
        // Particle is free
        //

        LogNpcDebug("    Free: velocity=", mParticles.GetVelocity(npcParticle.ParticleIndex), " prelimF=", mParticles.GetPreliminaryForces(npcParticle.ParticleIndex), " physicsDeltaPos=", physicsDeltaPos);
        LogNpcDebug("    StartPosition=", particleStartAbsolutePosition, " StartVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex));

        UpdateNpcParticle_Free(
            npcParticle,
            particleStartAbsolutePosition,
            particleStartAbsolutePosition + physicsDeltaPos,
            mParticles);

        LogNpcDebug("    EndPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " EndVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex));

        // Update total distance traveled
        if (npc.Kind == NpcKindType::Human
            && npcParticleOrdinal == 0) // Human is represented by primary particle
        {
            npc.KindSpecificState.HumanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition += physicsDeltaPos.length();
        }

        // We're done
    }
    else
    {
        //
        // Constrained
        //

        assert(npcParticle.ConstrainedState.has_value());

        // Triangle is workable
        assert(!homeShip.GetTriangles().IsDeleted(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex));
        assert(!IsTriangleFolded(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex, homeShip));

        // Loop tracing trajectory from TrajectoryStart (== current bary coords in new mesh state) to TrajectoryEnd (== start absolute pos + deltaPos);
        // each step moves the next TrajectoryStart a bit ahead.
        // Each iteration of the loop either exits (completes), or moves current bary coords (and calcs remaining dt) when it wants
        // to "continue" an impact while on edge-moving-against-it, i.e. when it wants to recalculate a new flattened traj.
        //    - In this case, the iteration doesn't change current absolute position nor velocity; it only updates current bary coords
        //      to what will become the next TrajectoryStart
        //    - In this case, at next iteration:
        //          - TrajectoryStart (== current bary coords) is new
        //          - TrajectoryEnd (== start absolute pos + physicsDeltaPos) is same as before
        //
        // Each iteration of the loop performs either an "inertial ray-tracing step" - i.e. with the particle free to move
        // around the inside of a triangle - or a "non-inertial step" - i.e. with the particle pushed against an edge, and
        // thus moving in a non-inertial frame as the mesh acceleration spawns the appearance of apparent forces.
        // - After an inertial iteration we won't enter a non-inertial iteration
        // - A non-inertial iteration might be followed by an inertial one

        // Initialize trajectory start (wrt current triangle) as the absolute pos of the particle as if it just
        // moved with the mesh, staying in its position wrt its triangle; in other words, it's the new theoretical
        // position after just mesh displacement
        vec2f trajectoryStartAbsolutePosition = homeShip.GetTriangles().FromBarycentricCoordinates(
            npcParticle.ConstrainedState->CurrentBCoords.BCoords,
            npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
            homeShip.GetPoints());

        // Calculate mesh velocity for the whole loop as the pure displacement of the triangle containing this particle
        // (end in-triangle position - start absolute position)
        vec2f const meshVelocity = (trajectoryStartAbsolutePosition - particleStartAbsolutePosition) / dt;

        LogNpcDebug("    Constrained: velocity=", mParticles.GetVelocity(npcParticle.ParticleIndex), " meshVelocity=", meshVelocity, " physicsDeltaPos=", physicsDeltaPos);

        // Machinery to detect 2- or 3-iteration paths that don't move particle (positional well,
        // aka gravity well)

        std::array<std::optional<AbsoluteTriangleBCoords>, 2> pastBarycentricPositions = { // A queue, insert at zero - initialized with now
            npcParticle.ConstrainedState->CurrentBCoords,
            std::nullopt
        };

        // Total displacement walked along the edge - as a sum of the vectors from the individual steps
        //
        // We will consider the particle's starting position to incrementally move by this,
        // in order to keep trajectory directory invariant with walk
        vec2f totalEdgeWalkedActual = vec2f::zero();

        // And here's for something seemingly obscure.
        //
        // At our first trajectory flattening for a non-inertial step we take the calculated
        // absolute flattened trajectory length (including walk) as the maximum (absolute)
        // distance that we're willing to travel in the (remaining) time quantum.
        // After all this is really the projection of the real and apparent forces acting on
        // the particle onto the (first) edge, and the one we should keep as the particle's
        // movement vector is now tied to the edge.
        //
        // At times an iteration might want to travel more than what we had decided is the
        // max we're willing to, for example because we've traveled a lot almost orthogonally
        // to the theoretical trajectory, and while there's little left along the flattened
        // trajectory, the trajectory end point might still be quite far from where we are.
        // If we stop being constrained by the edge that causes the travel to be almost
        // orthogonal to the trajectory, we might become subject to an abnormal quantity of
        // displacement - yielding also an abnormal velocity calculation.
        //
        // We thus clamp the magnitude of flattened trajectory vectors so to never exceed
        // this max distance we're willing to travel - hence the nickname "budget".

        std::optional<float> edgeDistanceToTravelMax;
        float edgeDistanceTraveledTotal = 0.0f; // To keep track of total distance, and of whether we have moved or not

        // The edge - wrt the current particle's triangle - that we are currently traveling on.
        // Meaningful only during Constrained-NonInertial phases.
        //
        // We begin by not knowing where we are on; when we don't know, we figure this out
        // either by means of vertex navigation (if we are on a vertex) - eventually involving
        // a floor choice in case we are a walking human NPC - or by checking on which floors
        // we are incident (if we are not on a vertex, or navigation of the vertex yields
        // a non-definite edge).
        // From them on, as long as we determine we're surely on an edge (i.e. non-inertial), we remember
        // the edge.

        std::optional<int> currentNonInertialFloorEdgeOrdinal;

        for (float remainingDt = dt; ; )
        {
            assert(remainingDt > 0.0f);

            LogNpcDebug("    ------------------------");
            LogNpcDebug("    New iter: CurrentBCoords=", npcParticle.ConstrainedState->CurrentBCoords, " RemainingDt=", remainingDt);

            //
            // We ray-trace the particle along a trajectory that starts at the position at which the particle
            // would be if it moved with the mesh (i.e. staying at its position wrt its triangle) and ends
            // at the position at which the particle would be if it were only subject to physical forces
            // and to walking
            //

            //
            // Calculate trajectory
            //
            // Trajectory is the apparent displacement, i.e. the displacement of the particle
            // *from the point of view of the mesh* (or of the particle itself), thus made up of
            // both the physical displacement and the mesh displacement.
            //
            // Given that trajectory discounts physics move, trajectory is the displacement
            // caused by the apparent forces. In fact, we've verified that when the particle has
            // the same velocity as the mesh, trajectory is zero.
            //
            // Note: when there's no physics displacement, this amounts to pure mesh move
            // (PartPos - FromBary(PartPosBary)). On the other hand, when the mesh is at rest, this
            // amounts to pure physical displacement.
            //

            // Absolute position of particle if it only moved due to physical forces and walking
            vec2f const trajectoryEndAbsolutePosition = particleStartAbsolutePosition + physicsDeltaPos + totalEdgeWalkedActual;

            // Trajectory
            // Note: on first iteration this is the same as physicsDeltaPos + meshDisplacement
            // TrajectoryStartAbsolutePosition changes at each iteration to be the absolute translation of the current particle bary coords
            vec2f const trajectory = trajectoryEndAbsolutePosition - trajectoryStartAbsolutePosition;

            LogNpcDebug("    TrajectoryStartAbsolutePosition=", trajectoryStartAbsolutePosition, " PhysicsDeltaPos=", physicsDeltaPos, " TotalEdgeWalkedActual=", totalEdgeWalkedActual,
                " => TrajectoryEndAbsolutePosition=", trajectoryEndAbsolutePosition, " Trajectory=", trajectory);

            //
            // Determine whether we are insisting on an edge - i.e. whether we're non-inertial
            //
            // If this is the first time, we determine the edge via vertex navigation (if we
            // are at a vertex) or via floor normals (if we are on an edge, or at a vertex
            // but unsure of the floor as the trajectory might hint to the inside of a triangle).
            //
            // If it's not the first time, then we know we;re at one of these two:
            //  - We cannot determine the edge (because the trajectory might hint to the inside of a triangle)
            //  - We know we are on an edge
            //

            // The edge we determine now - of the triangle @ npcParticle.ConstrainedState->CurrentBCoords; this is a floor
            int nonInertialEdgeOrdinal = -1;

            if (!currentNonInertialFloorEdgeOrdinal.has_value())
            {
                // First time
                if (std::optional<int> const vertexOrdinal = npcParticle.ConstrainedState->CurrentBCoords.BCoords.try_get_vertex();
                    vertexOrdinal.has_value())
                {
                    //
                    // We are on a vertex (*two* edges)
                    //
                    // There are two conditions that bring us here:
                    //  1. After a bounce or an impact continuation
                    //  2. Initial (e.g. placed by chance on a vertex)
                    //

                    LogNpcDebug("    On a vertex: ", *vertexOrdinal, " of triangle ", npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex);

                    // Check what comes next

                    bcoords3f const trajectoryEndBarycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinates(
                        trajectoryEndAbsolutePosition,
                        npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                        homeShip.GetPoints());

                    auto const outcome = NavigateVertex(
                        npc,
                        npcParticleOrdinal,
                        npcParticle.ConstrainedState->CurrentVirtualFloor,
                        *vertexOrdinal,
                        trajectory,
                        trajectoryEndAbsolutePosition,
                        trajectoryEndBarycentricCoords,
                        homeShip,
                        mParticles);

                    switch (outcome.Type)
                    {
                        case NavigateVertexOutcome::OutcomeType::BecomeFree:
                        {
                            // Transition to free immediately

                            TransitionParticleToFreeState(npc, npcParticleOrdinal, homeShip);

                            UpdateNpcParticle_Free(
                                npcParticle,
                                particleStartAbsolutePosition,
                                trajectoryEndAbsolutePosition,
                                mParticles);

                            remainingDt = 0.0f;

                            break;
                        }

                        case NavigateVertexOutcome::OutcomeType::ContinueAlongFloor:
                        {
                            //
                            // This either tells us that our trajectory points us towards this floor (if not walking),
                            // or that we're walking and decided for this floor: in either case we are non-inertial on this floor
                            //

                            assert(outcome.FloorEdgeOrdinal >= 0);
                            nonInertialEdgeOrdinal = outcome.FloorEdgeOrdinal;

                            // Move to NavigationOutcome
                            npcParticle.ConstrainedState->CurrentBCoords = outcome.TriangleBCoords;

                            break;
                        }

                        case NavigateVertexOutcome::OutcomeType::ContinueToInterior:
                        {
                            //
                            // This tells us that the trajectory appears to tell that we're
                            // moving into a triangle or along an edge of it, but this could
                            // be a numerical illusion.
                            // Need to use floor normals to see if we are
                            // incident to a triangle
                            //

                            assert(outcome.FloorEdgeOrdinal == -1);
                            nonInertialEdgeOrdinal = -1; // Determine via floor-normal checks

                            // Move to NavigationOutcome
                            npcParticle.ConstrainedState->CurrentBCoords = outcome.TriangleBCoords;

#ifdef _DEBUG

                            // Assert dot-with-normal is negative or very small for all edges we touch
                            // (after all the numerical illusion is just lack of precision)
                            for (int vi = 0; vi < 3; ++vi)
                            {
                                if (npcParticle.ConstrainedState->CurrentBCoords.BCoords[vi] == 0.0f)
                                {
                                    int const edgeOrdinal = (vi + 1) % 3;

                                    vec2f const edgeVector = homeShip.GetTriangles().GetSubSpringVector(
                                        npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                                        edgeOrdinal,
                                        homeShip.GetPoints());
                                    vec2f const edgeDir = edgeVector.normalise();
                                    vec2f const edgeNormal = edgeDir.to_perpendicular(); // Points outside of triangle (i.e. towards floor)
                                    assert(trajectory.dot(edgeNormal) < 0.01f);
                                }
                            }
#endif
                            break;
                        }

                        case NavigateVertexOutcome::OutcomeType::ImpactOnFloor:
                        {
                            // Let the subsequent non-inertial ray tracing take care of this

                            assert(outcome.FloorEdgeOrdinal >= 0);
                            nonInertialEdgeOrdinal = outcome.FloorEdgeOrdinal;

                            // Do not move to NavigationOutcome, we'll get there via ray tracing

                            break;
                        }
                    }

                    // Follow-up free conversion
                    if (remainingDt == 0.0f)
                    {
                        break;
                    }
                }
            }
            else
            {
                LogNpcDebug("    We remember from earlier that we are on a floor edge/inside triangle: ", *currentNonInertialFloorEdgeOrdinal, " of triangle ", npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex);

                nonInertialEdgeOrdinal = *currentNonInertialFloorEdgeOrdinal;
            }

            if (nonInertialEdgeOrdinal == -1)
            {
                LogNpcDebug("    Determining floor edge with floor normals");

                // Find edge that trajectory is against the most
                float bestHeadsOnNess = std::numeric_limits<float>::lowest();
                for (int vi = 0; vi < 3; ++vi)
                {
                    if (npcParticle.ConstrainedState->CurrentBCoords.BCoords[vi] == 0.0f)
                    {
                        // We are on this edge

                        int const edgeOrdinal = (vi + 1) % 3;

                        LogNpcDebug("    On an edge: ", edgeOrdinal, " of triangle ", npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex);

                        // Check if this is really a floor to this particle
                        if (IsEdgeFloorToParticle(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex, edgeOrdinal, npc, npcParticleOrdinal, mParticles, homeShip))
                        {
                            // The edge is a floor

                            LogNpcDebug("      The edge is a floor");

                            // Check now whether we're moving *against* the floor

                            vec2f const edgeVector = homeShip.GetTriangles().GetSubSpringVector(
                                npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                                edgeOrdinal,
                                homeShip.GetPoints());
                            vec2f const edgeNormal = edgeVector.to_perpendicular(); // Points outside of triangle (i.e. towards floor)
                            float const headsOnNess = trajectory.dot(edgeNormal);
                            if (trajectory.dot(edgeNormal) >= 0.0f) // If 0, no normal force - hence no friction; however we want to take this codepath anyways for consistency
                            {
                                //
                                // We are insisting against this edge floor
                                //

                                LogNpcDebug("      We are moving against this floor (alignment=", headsOnNess, ")");

                                if (headsOnNess > bestHeadsOnNess)
                                {
                                    // Winner
                                    nonInertialEdgeOrdinal = edgeOrdinal;
                                    bestHeadsOnNess = headsOnNess;
                                }
                            }
                            else
                            {
                                LogNpcDebug("      We are not moving against this floor (alignment=", headsOnNess, ")");
                            }
                        }
                        else
                        {
                            LogNpcDebug("      The edge is not a floor");
                        }
                    }
                }
            }

            if (nonInertialEdgeOrdinal >= 0)
            {
                //
                // Case 1: Non-inertial: on edge and moving against (or along) it, pushed by it
                //

                LogNpcDebug("    ConstrainedNonInertial: triangle=", npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex, " nonInertialEdgeOrdinal=", nonInertialEdgeOrdinal, " bCoords=", npcParticle.ConstrainedState->CurrentBCoords.BCoords, " trajectory=", trajectory);
                LogNpcDebug("    StartPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " StartVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex), " MeshVelocity=", meshVelocity, " StartMRVelocity=", npcParticle.ConstrainedState->MeshRelativeVelocity);

                // Save the coords of the touch point
                AbsoluteTriangleBCoords const edgeTouchPointBCoords = npcParticle.ConstrainedState->CurrentBCoords;
                assert(edgeTouchPointBCoords.BCoords.is_on_edge());

                // Set now our current edge-ness for the rest of this simulation step,
                // mostly for animation purposes.
                //
                // In fact, the subsequent UpdateNpcParticle_ConstrainedNonInertial call has 5 outcomes:
                //  - We end up inside the triangle - along this same edge;
                //  - We travel up to a vertex, and then:
                //      - End navigation at the interior of a triangle: in this case we might have reached a new triangle,
                //        but we'll go through this again and become inertial (resetting this edge-ness);
                //      - Ready to continue on a new edge: we could update this edge-ness to the new edge, but we'll go
                //        through this again, and thus we save the effort;
                //      - Become free: no more constrained state anyways;
                //      - Hit floor: in this case we _do_ want the current virtual floor to be the one _before_ the bounce.

                npcParticle.ConstrainedState->CurrentVirtualFloor.emplace(
                    npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                    nonInertialEdgeOrdinal);

                //
                // We're moving against the floor, hence we are in a non-inertial frame...
                // ...take friction into account and flaten trajectory
                //

                //
                // Calculate magnitude of flattened trajectory - i.e. component of trajectory
                // along (i.e. tangent) edge, positive when in the direction of the edge
                //

                vec2f const edgeDir = homeShip.GetTriangles().GetSubSpringVector(
                    npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                    nonInertialEdgeOrdinal,
                    homeShip.GetPoints()).normalise();

                float trajectoryT = trajectory.dot(edgeDir);

                //
                // Update tangential component of trajectory with friction
                //

                {
                    // Normal trajectory: apparent (integrated) force against the floor;
                    // positive when against the floor

                    float const trajectoryN = trajectory.dot(edgeDir.to_perpendicular());

                    //
                    // Choose between kinetic and static friction
                    //
                    // Note that we want to check actual velocity here, not
                    // *intention* to move (which is trajectory)
                    //

                    // Get edge's material (by taking arbitrarily the material of one of its endpoints)
                    auto const & meshMaterial = homeShip.GetPoints().GetStructuralMaterial(
                        homeShip.GetTriangles().GetPointIndices(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)[nonInertialEdgeOrdinal]);

                    float frictionCoefficient;
                    if (std::abs(npcParticle.ConstrainedState->MeshRelativeVelocity.dot(edgeDir)) > 0.01f) // Magic number
                    {
                        // Kinetic friction
                        frictionCoefficient =
                            (mParticles.GetMaterial(npcParticle.ParticleIndex).KineticFrictionCoefficient + meshMaterial.KineticFrictionCoefficient) / 2.0f
                            * mParticles.GetKineticFrictionTotalAdjustment(npcParticle.ParticleIndex);
                    }
                    else
                    {
                        // Static friction
                        frictionCoefficient =
                            (mParticles.GetMaterial(npcParticle.ParticleIndex).StaticFrictionCoefficient + meshMaterial.StaticFrictionCoefficient) / 2.0f
                            * mParticles.GetStaticFrictionTotalAdjustment(npcParticle.ParticleIndex);
                    }

                    // Calculate friction (integrated) force magnitude (along edgeDir),
                    // which is the same as apparent force, up to max friction threshold
                    float tFriction = std::min(std::abs(trajectoryT), frictionCoefficient * std::max(trajectoryN, 0.0f));
                    if (trajectoryT >= 0.0f)
                    {
                        tFriction *= -1.0f;
                    }

                    LogNpcDebug("        friction: trajectoryN=", trajectoryN, " relVel=", npcParticle.ConstrainedState->MeshRelativeVelocity,
                        " trajectoryT=", trajectoryT, " tFriction=", tFriction);

                    // Update trajectory with friction
                    trajectoryT += tFriction;
                }

                // The (signed) edge length that we plan to travel independently from walking
                float const edgePhysicalTraveledPlanned = trajectoryT;

                //
                // Calculate displacement due to walking, if any
                //

                // The (signed) edge length that we plan to travel exclusively via walking
                float edgeWalkedPlanned = 0.0f;

                if (npc.Kind == NpcKindType::Human
                    && npc.KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking
                    && npcParticleOrdinal == 0)
                {
                    //
                    // Walking displacement projected along edge - what's needed to reach total desired walking
                    // displacement, together with physical
                    // - Never reduces physical
                    // - Never adds to physical so much as to cause resultant to be faster than walking speed
                    //

                    vec2f const idealWalkDir = vec2f(npc.KindSpecificState.HumanNpcState.CurrentFaceDirectionX, 0.0f);
                    assert(idealWalkDir.length() == 1.0f);

                    float const idealWalkMagnitude = CalculateActualHumanWalkingAbsoluteSpeed(npc.KindSpecificState.HumanNpcState) * remainingDt;

                    vec2f walkDir; // Actual absolute direction of walk - along the edge
                    if (idealWalkDir.dot(edgeDir) >= 0.0f)
                    {
                        // Same direction as edge (ahead is towards larger)

                        walkDir = edgeDir;
                        edgeWalkedPlanned = Clamp(idealWalkMagnitude - edgePhysicalTraveledPlanned, 0.0f, idealWalkMagnitude);
                    }
                    else
                    {
                        // Opposite direction as edge (ahead is towards smaller)

                        walkDir = -edgeDir;
                        edgeWalkedPlanned = Clamp(-idealWalkMagnitude - edgePhysicalTraveledPlanned, -idealWalkMagnitude, 0.0f);
                    }

                    // Apply gravity resistance: too steep slopes (wrt vertical) are gently clamped to zero,
                    // to prevent walking on floors that are too steep
                    //
                    // Note: walkDir.y is sin(slope angle between horiz and dir)
                    float constexpr NeighborhoodWidth = 1.0f - GameParameters::MaxHumanNpcWalkSinSlope;
                    float constexpr ResistanceSinSlopeStart = GameParameters::MaxHumanNpcWalkSinSlope - NeighborhoodWidth / 2.0f;
                    if (walkDir.y >= ResistanceSinSlopeStart) // walkDir.y is component along vertical, pointing up
                    {
                        float const y2 = (walkDir.y - ResistanceSinSlopeStart) / NeighborhoodWidth;
                        float const gravityResistance = std::max(1.0f - y2, 0.0f);

                        LogNpcDebug("        gravityResistance=", gravityResistance, " (walkDir.y=", walkDir.y, ")");

                        edgeWalkedPlanned *= gravityResistance;
                    }

                    if (npc.KindSpecificState.HumanNpcState.CurrentBehaviorState.Constrained_Walking.CurrentWalkMagnitude != 0.0f)
                    {
                        LogNpcDebug("        idealWalkMagnitude=", idealWalkMagnitude, " => edgeWalkedPlanned=", edgeWalkedPlanned, " (@", npc.KindSpecificState.HumanNpcState.CurrentBehaviorState.Constrained_Walking.CurrentWalkMagnitude, ")");
                    }
                }

                //
                // Calculate total (signed) displacement we plan on undergoing
                //

                float const edgeTraveledPlanned = edgePhysicalTraveledPlanned + edgeWalkedPlanned; // Resultant

                if (!edgeDistanceToTravelMax)
                {
                    edgeDistanceToTravelMax = std::abs(edgeTraveledPlanned);

                    LogNpcDebug("        initialized distance budget: edgeDistanceToTravelMax=", *edgeDistanceToTravelMax);
                }

                // Make sure we don't travel more than what we're willing to

                float adjustedEdgeTraveledPlanned;

                float const remainingDistanceBudget = *edgeDistanceToTravelMax - edgeDistanceTraveledTotal;
                assert(remainingDistanceBudget >= 0.0f);
                if (std::abs(edgeTraveledPlanned) > remainingDistanceBudget)
                {
                    if (edgeTraveledPlanned >= 0.0f)
                    {
                        adjustedEdgeTraveledPlanned = std::min(edgeTraveledPlanned, remainingDistanceBudget);
                    }
                    else
                    {
                        adjustedEdgeTraveledPlanned = std::max(edgeTraveledPlanned, -remainingDistanceBudget);
                    }

                    LogNpcDebug("        travel exceeds budget (edgeTraveledPlanned=", edgeTraveledPlanned, " budget=", remainingDistanceBudget,
                        " => adjustedEdgeTraveledPlanned=", adjustedEdgeTraveledPlanned);
                }
                else
                {
                    adjustedEdgeTraveledPlanned = edgeTraveledPlanned;
                }

                //
                // Recover flattened trajectory as a vector
                //

                vec2f const flattenedTrajectory = edgeDir * adjustedEdgeTraveledPlanned;

                //
                // Calculate trajectory target
                //

                vec2f flattenedTrajectoryEndAbsolutePosition = trajectoryStartAbsolutePosition + flattenedTrajectory;

                bcoords3f flattenedTrajectoryEndBarycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinates(
                    flattenedTrajectoryEndAbsolutePosition,
                    npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                    homeShip.GetPoints());

                // Due to numerical slack, ensure target barycentric coords are still along edge
                int const edgeVertexOrdinal = (nonInertialEdgeOrdinal + 2) % 3;
                flattenedTrajectoryEndBarycentricCoords[edgeVertexOrdinal] = 0.0f;
                flattenedTrajectoryEndBarycentricCoords[(edgeVertexOrdinal + 1) % 3] = 1.0f - flattenedTrajectoryEndBarycentricCoords[(edgeVertexOrdinal + 2) % 3];

                LogNpcDebug("        flattenedTrajectory=", flattenedTrajectory, " flattenedTrajectoryEndAbsolutePosition=", flattenedTrajectoryEndAbsolutePosition, " flattenedTrajectoryEndBarycentricCoords=", flattenedTrajectoryEndBarycentricCoords);

                //
                // Ray-trace using non-inertial physics;
                // will return when completed or when current edge is over
                //
                // If needs to continue, returns the (signed) actual edge traveled, which is implicitly the
                // (signed) actual edge physically traveled plus the (signed) actual edge walked during the
                // consumed dt portion of the remaning dt
                //
                // Fact: at each iteration, the actual movement of the particle will be the result of phys traj and imposed walk displacement
                // Fact: phys traj displacement (planned) is itself dependant from remaining_dt, because of advancement of particle's current bary coords
                // Fat: walk displacement (planned) is also dependant from remaining_dt, because we use walk velocity
                // Fact: so, the actual movement includes the consumed_dt's portion (fraction) of both phys traj and imposed walk
                //

                LogNpcDebug("        edgePhysicalTraveledPlanned=", edgePhysicalTraveledPlanned, " edgeWalkedPlanned=", edgeWalkedPlanned);

                auto const nonInertialOutcome = UpdateNpcParticle_ConstrainedNonInertial(
                    npc,
                    npcParticleOrdinal,
                    nonInertialEdgeOrdinal,
                    edgeDir,
                    particleStartAbsolutePosition,
                    trajectoryStartAbsolutePosition,
                    flattenedTrajectoryEndAbsolutePosition,
                    flattenedTrajectoryEndBarycentricCoords,
                    flattenedTrajectory,
                    adjustedEdgeTraveledPlanned,
                    (edgeDistanceTraveledTotal > 0.0f), // hasMovedInStep
                    meshVelocity,
                    remainingDt,
                    homeShip,
                    mParticles,
                    currentSimulationTime,
                    gameParameters);

                if (npcParticle.ConstrainedState.has_value())
                {
                    LogNpcDebug("    EndBCoords=", npcParticle.ConstrainedState->CurrentBCoords);
                }
                else
                {
                    LogNpcDebug("    Became free");
                }

                LogNpcDebug("    Actual edge traveled in non-inertial step: ", nonInertialOutcome.EdgeTraveled);

                if (nonInertialOutcome.DoStop)
                {
                    // Completed
                    remainingDt = 0.0f;

                    if (!nonInertialOutcome.HasBounced)
                    {
                        // We have reached destination and thus we're lying on this edge;
                        // impart mass onto mesh, divided among the two vertices

                        int const edgeVertex1Ordinal = nonInertialEdgeOrdinal;
                        ElementIndex const edgeVertex1PointIndex = homeShip.GetTriangles().GetPointIndices(edgeTouchPointBCoords.TriangleElementIndex)[edgeVertex1Ordinal];
                        float const vertex1InterpCoeff = edgeTouchPointBCoords.BCoords[edgeVertex1Ordinal];
                        homeShip.GetPoints().AddTransientAdditionalMass(edgeVertex1PointIndex, particleMass * vertex1InterpCoeff);

                        int const edgeVertex2Ordinal = (nonInertialEdgeOrdinal + 1) % 3;
                        ElementIndex const edgeVertex2PointIndex = homeShip.GetTriangles().GetPointIndices(edgeTouchPointBCoords.TriangleElementIndex)[edgeVertex2Ordinal];
                        float const vertex2InterpCoeff = edgeTouchPointBCoords.BCoords[edgeVertex2Ordinal];
                        homeShip.GetPoints().AddTransientAdditionalMass(edgeVertex2PointIndex, particleMass * vertex2InterpCoeff);
                    }
                }
                else
                {
                    if (nonInertialOutcome.EdgeTraveled == 0.0f)
                    {
                        // No movement

                        // Check if we're in a well
                        if (pastBarycentricPositions[0] == npcParticle.ConstrainedState->CurrentBCoords
                            || pastBarycentricPositions[1] == npcParticle.ConstrainedState->CurrentBCoords)
                        {
                            //
                            // Well - stop here
                            //

                            LogNpcDebug("    Detected well - stopping here");

                            // Update particle's physics, considering that we are in a well and thus still (wrt mesh)

                            vec2f const particleEndAbsolutePosition = homeShip.GetTriangles().FromBarycentricCoordinates(
                                npcParticle.ConstrainedState->CurrentBCoords.BCoords,
                                npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                                homeShip.GetPoints());

                            mParticles.SetPosition(npcParticle.ParticleIndex, particleEndAbsolutePosition);

                            // No (relative) velocity (so just mesh velocity, and no global damping)
                            mParticles.SetVelocity(npcParticle.ParticleIndex, meshVelocity);
                            npcParticle.ConstrainedState->MeshRelativeVelocity = vec2f::zero();

                            // Consume the whole time quantum
                            remainingDt = 0.0f;
                        }
                        else
                        {
                            // Resultant of physical and walked is zero => no movement at all
                            // Note: either physical and/or walked could individually be non-zero
                            //
                            // For now, assume no dt consumed and continue
                        }
                    }
                    else
                    {
                        // We have moved

                        // Calculate consumed dt
                        assert(nonInertialOutcome.EdgeTraveled * adjustedEdgeTraveledPlanned >= 0.0f); // Should have same sign
                        float const dtFractionConsumed = adjustedEdgeTraveledPlanned != 0.0f
                            ? std::min(nonInertialOutcome.EdgeTraveled / adjustedEdgeTraveledPlanned, 1.0f) // Signs should agree anyway
                            : 1.0f; // If we were planning no travel, any movement is a whole consumption
                        LogNpcDebug("        dtFractionConsumed=", dtFractionConsumed);
                        remainingDt *= (1.0f - dtFractionConsumed);

                        // Reset well detection machinery
                        static_assert(pastBarycentricPositions.size() == 2);
                        pastBarycentricPositions[0].reset();
                        pastBarycentricPositions[1].reset();
                    }
                }

                // Update total (absolute) distance traveled along (an) edge
                edgeDistanceTraveledTotal += std::abs(nonInertialOutcome.EdgeTraveled);

                // If we haven't completed, there is still some distance remaining in the budget
                //
                // Note: if this doesn't hold, at the next iteration we'll move by zero and we'll reset
                // velocity to zero, even though we have moved in this step, thus yielding an erroneous
                // zero velocity
                assert(remainingDt == 0.0f || edgeDistanceTraveledTotal < *edgeDistanceToTravelMax);

                // Update total human distance traveled
                if (npc.Kind == NpcKindType::Human
                    && npcParticleOrdinal == 0) // Human is represented by primary particle
                {
                    // Note: does not include eventual distance traveled after becoming free; fine because we will transition and wipe out total traveled
                    npc.KindSpecificState.HumanNpcState.TotalDistanceTraveledOnEdgeSinceStateTransition += std::abs(nonInertialOutcome.EdgeTraveled);
                }

                // Update total vector walked along edge
                // Note: we use unadjusted edge traveled planned, as edge walked planned is also unadjusted,
                // and we are only interested in the ratio anyway
                float const edgeWalkedActual = edgeTraveledPlanned != 0.0f
                    ? nonInertialOutcome.EdgeTraveled * (edgeWalkedPlanned / edgeTraveledPlanned)
                    : 0.0f; // Unlikely, but read above for rationale behind 0.0
                totalEdgeWalkedActual += edgeDir * edgeWalkedActual;
                LogNpcDebug("        edgeWalkedActual=", edgeWalkedActual, " totalEdgeWalkedActual=", totalEdgeWalkedActual);

                // Update well detection machinery
                if (npcParticle.ConstrainedState.has_value()) // We might have left constrained state (not to return to it anymore)
                {
                    pastBarycentricPositions[1] = pastBarycentricPositions[0];
                    pastBarycentricPositions[0] = npcParticle.ConstrainedState->CurrentBCoords;
                }

                // Update current floor edge we're walking on, or inside triangle
                currentNonInertialFloorEdgeOrdinal = nonInertialOutcome.FloorEdgeOrdinal;

                if (npcParticle.ConstrainedState.has_value())
                {
                    LogNpcDebug("    EndPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " EndVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex), " EndMRVelocity=", npcParticle.ConstrainedState->MeshRelativeVelocity);
                }
                else
                {
                    LogNpcDebug("    EndPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " EndVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex));
                }
            }
            else
            {
                //
                // Case 2: Inertial: not on edge or on edge but not moving against (nor along) it
                //

                LogNpcDebug("    ConstrainedInertial: CurrentBCoords=", npcParticle.ConstrainedState->CurrentBCoords, " physicsDeltaPos=", physicsDeltaPos);
                LogNpcDebug("    StartPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " StartVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex), " MeshVelocity=", meshVelocity, " StartMRVelocity=", npcParticle.ConstrainedState->MeshRelativeVelocity);

                // We are not on an edge floor (nor we'll got on it now)
                npcParticle.ConstrainedState->CurrentVirtualFloor.reset();
                currentNonInertialFloorEdgeOrdinal.reset();

                //
                // Calculate target barycentric coords
                //

                bcoords3f const trajectoryEndBarycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinates(
                    trajectoryEndAbsolutePosition,
                    npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                    homeShip.GetPoints());

                //
                // Move towards target bary coords
                //

                float totalTraveled = UpdateNpcParticle_ConstrainedInertial(
                    npc,
                    npcParticleOrdinal,
                    particleStartAbsolutePosition,
                    trajectoryStartAbsolutePosition, // segmentTrajectoryStartAbsolutePosition
                    trajectoryEndAbsolutePosition,
                    trajectoryEndBarycentricCoords,
                    (edgeDistanceTraveledTotal > 0.0f),
                    meshVelocity,
                    remainingDt,
                    homeShip,
                    mParticles,
                    currentSimulationTime,
                    gameParameters);

                if (npcParticle.ConstrainedState.has_value())
                {
                    LogNpcDebug("    EndBCoords=", npcParticle.ConstrainedState->CurrentBCoords);
                }
                else
                {
                    LogNpcDebug("    Became free");
                }

                // Update total traveled
                if (npc.Kind == NpcKindType::Human
                    && npcParticleOrdinal == 0) // Human is represented by primary particle
                {
                    // Note: does not include eventual disance traveled after becoming free; fine because we will transition and wipe out total traveled
                    npc.KindSpecificState.HumanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition += std::abs(totalTraveled);
                }

                if (npcParticle.ConstrainedState.has_value())
                {
                    LogNpcDebug("    EndPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " EndVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex), " EndMRVelocity=", npcParticle.ConstrainedState->MeshRelativeVelocity);
                }
                else
                {
                    LogNpcDebug("    EndPosition=", mParticles.GetPosition(npcParticle.ParticleIndex), " EndVelocity=", mParticles.GetVelocity(npcParticle.ParticleIndex));
                }

                // Consume whole time quantum and stop
                remainingDt = 0.0f;
            }

            if (remainingDt <= 0.0f)
            {
                assert(remainingDt > -0.0001f); // If negative, it's only because of numerical slack

                // Consumed whole time quantum, loop completed
                break;
            }

            //
            // Update trajectory start position for next iteration
            //

            // Current (virtual, not yet real) position of this particle
            trajectoryStartAbsolutePosition = homeShip.GetTriangles().FromBarycentricCoordinates(
                npcParticle.ConstrainedState->CurrentBCoords.BCoords,
                npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
                homeShip.GetPoints());
        }
    }

#ifdef BARYLAB_PROBING
    if (mCurrentlySelectedParticle == npcParticle.ParticleIndex)
    {
        // Publish final velocities

        vec2f const particleVelocity = (mParticles.GetPosition(npcParticle.ParticleIndex) - particleStartAbsolutePosition) / GameParameters::SimulationStepTimeDuration<float>;

        mGameEventHandler->OnCustomProbe("VelX", particleVelocity.x);
        mGameEventHandler->OnCustomProbe("VelY", particleVelocity.y);
    }
#endif
}

void Npcs::CalculateNpcParticlePreliminaryForces(
    StateType const & npc,
    int npcParticleOrdinal,
    vec2f const & globalWindForce,
    GameParameters const & gameParameters)
{
    auto & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];

    //
    // Calculate world forces
    //

    float const particleMass = mParticles.GetMass(npcParticle.ParticleIndex);
    vec2f const & particlePosition = mParticles.GetPosition(npcParticle.ParticleIndex);
    float const effectiveParticleWindReceptivity = std::min(
        mParticles.GetMaterial(npcParticle.ParticleIndex).WindReceptivity * gameParameters.NpcWindReceptivityAdjustment,
        1.0f);

    // 1. World forces - gravity

    vec2f preliminaryForces =
        GameParameters::Gravity
#ifdef IN_BARYLAB
        * mCurrentGravityAdjustment
#endif
        * particleMass;

    if (npc.CurrentRegime != StateType::RegimeType::BeingPlaced)
    {
        // Calculate waterness of this point: from mesh waterness if we're free, from water in mesh if we're constrained

        float anyWaterness;
        if (npcParticle.ConstrainedState.has_value())
        {
            // Constrained - use ship points' water

            anyWaterness = mParticles.GetMeshWaterness(npcParticle.ParticleIndex);

            // Converge particle's mesh-relative velocity to mesh water velocity
            //
            // Now, our target is that the point's relative velocity ends up like the resultant water velocity;
            // that is reached by adding (resultantWaterVelocity - pointRelVel) to pointRelVel. This increment
            // also ends up being the same increment to the *absolute* point velocity, hence we can calculate
            // the absolute point's velocity delta as (resultantWaterVelocity - pointRelVel).
            // Note that we use the point's *prior* relative velocity

            vec2f const & waterVelocity = mParticles.GetMeshWaterVelocity(npcParticle.ParticleIndex);
            float const waterVelocityMagnitude = waterVelocity.length();
            vec2f const waterVelocityDir = waterVelocity.normalise_approx(waterVelocityMagnitude);
            float const dampedWaterVelocityMagnitude = std::min(waterVelocityMagnitude, 4.0f);

            vec2f const & particleVelocity = npcParticle.ConstrainedState->MeshRelativeVelocity;
            float const particleVelocityMagnitude = particleVelocity.length();
            vec2f const particleVelocityDir = particleVelocity.normalise_approx(particleVelocityMagnitude);

            float constexpr MaxParticleVelocityForApplyingWaterVelocity = 15.0f;

            float velocityIncrement;
            float const particleVelocityDirAlongWaterDir = particleVelocityDir.dot(waterVelocityDir);
            if (particleVelocityDirAlongWaterDir >= 0.0f)
            {
                // The particle's relative velocity is in the same direction as the water; fill-in the remaining part
                // (but don't slow it down)

                // Water velocity accrual is zero as the particle's velocity (in the water direction) approaches
                // a maximum - so that we don't accelerate particles to the crazy velocities of the water
                float const damper = std::max(
                    (MaxParticleVelocityForApplyingWaterVelocity - particleVelocityMagnitude) / MaxParticleVelocityForApplyingWaterVelocity,
                    0.0f);

                velocityIncrement =
                    std::max(dampedWaterVelocityMagnitude - particleVelocityMagnitude * particleVelocityDirAlongWaterDir, 0.0f)
                    * damper;
            }
            else
            {
                // The particle's relative velocity is opposite water; add what it takes to match it, but converge slowly

                velocityIncrement = std::min(
                    (dampedWaterVelocityMagnitude - particleVelocityMagnitude * particleVelocityDirAlongWaterDir) * 0.3f,
                    MaxParticleVelocityForApplyingWaterVelocity);
            }

            // Make it harder for orthogonal directions to change velocity,
            // so to avoid too-quick vortices
            float const orthoDamper = particleVelocityDirAlongWaterDir * particleVelocityDirAlongWaterDir;

            vec2f const absoluteVelocityDelta =
                waterVelocityDir
                * velocityIncrement
                * orthoDamper;

            // 2. World forces - inside water tide

            // Since we do forces here, we apply this as a force - but not dependent on the mass of the particle
            // (because heavy particles should practically not move)
            preliminaryForces +=
                absoluteVelocityDelta
                / GameParameters::SimulationStepTimeDuration<float>
                * anyWaterness // Mess with velocity only if enough water
                * (1.0f - SmoothStep(0.0f, 0.9f, waterVelocityDir.y)) // Lower acceleration with verticality - water close to surface pushes up and we don't like that
                * std::min(particleMass, 35.0f); // This magic number is to ensure the numbers above perform OK on the reference human particles
        }
        else
        {
            // Free - there is waterness if we are underwater

            float constexpr BuoyancyInterfaceWidth = 0.4f; // Nature abhorrs discontinuities

            vec2f testParticlePosition = particlePosition;
            if (npc.Kind == NpcKindType::Human && npcParticleOrdinal == 1)
            {
                // Head - a little bit of a hack to make them float with the head above water,
                // use an empirical offset - a fixed one to account for the equilibrium waterness
                // (where the head particle is in equilibrium) plus the chin offset
                //
                // The actual waterness that sets the system in equilibrium is 0.81,
                // calculated by means of masses and buoyancy factors. Here we target defaults
                testParticlePosition.y +=
                    - BuoyancyInterfaceWidth * 0.81f
                    +
                    (mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex).y - particlePosition.y)
                    * GameParameters::HumanNpcGeometry::HeadLengthFraction;
            }

            float const waterHeight = mParentWorld.GetOceanSurface().GetHeightAt(testParticlePosition.x);
            float const particleDepth = waterHeight - testParticlePosition.y;
            anyWaterness = Clamp(particleDepth, 0.0f, BuoyancyInterfaceWidth) / BuoyancyInterfaceWidth; // Same as uwCoefficient

            // 3. World forces - wind: iff free and above-water

            preliminaryForces +=
                globalWindForce
                * effectiveParticleWindReceptivity
                * (1.0f - anyWaterness); // Only above-water (modulated)

            // Generate waves if on the air-water interface, magnitude
            // proportional to (signed) vertical velocity

            float const verticalVelocity = mParticles.GetVelocity(npcParticle.ParticleIndex).y;
            float const particleDepthBefore = waterHeight - (particlePosition.y - verticalVelocity * GameParameters::SimulationStepTimeDuration<float>);
            if (particleDepth * particleDepthBefore < 0.0f) // Check if we've just entered/left the air-water interface
            {
                float const waveDisplacement =
                    SmoothStep(0.0f, 6.0f, std::abs(verticalVelocity))
                    * SignStep(0.0f, verticalVelocity) // Displacement has same sign as vertical velocity
                    * std::min(1.0f, 2.0f / static_cast<float>(npc.ParticleMesh.Particles.size())) // Other particles in this mesh will generate waves
                    * 0.6f; // Magic number

                mParentWorld.DisplaceOceanSurfaceAt(particlePosition.x, waveDisplacement);
            }
        }

        // Store it for future use
        mParticles.SetAnyWaterness(npcParticle.ParticleIndex, anyWaterness);

        if (anyWaterness > 0.0f)
        {
            // Underwater (even if partially)

            // 4. World forces - buoyancy

            preliminaryForces.y +=
                mParticles.GetBuoyancyFactor(npcParticle.ParticleIndex)
                * anyWaterness;

            // 5. World forces - water drag

            preliminaryForces +=
                -mParticles.GetVelocity(npcParticle.ParticleIndex)
                * GameParameters::WaterFrictionDragCoefficient
                * gameParameters.WaterFrictionDragAdjustment;
        }
        else
        {
            // Completely above-water

            // 6. World forces - radial wind (if any)

            auto const & radialWindField = mParentWorld.GetCurrentRadialWindField();
            if (radialWindField.has_value())
            {
                vec2f const displacement = particlePosition - radialWindField->SourcePos;
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
                        * effectiveParticleWindReceptivity;

                    // Apply force
                    preliminaryForces += force;
                }
            }
        }
    }

    // 7. External forces

    preliminaryForces += mParticles.GetExternalForces(npcParticle.ParticleIndex);

    mParticles.SetPreliminaryForces(npcParticle.ParticleIndex, preliminaryForces);
}

void Npcs::CalculateNpcParticleSpringForces(StateType const & npc)
{
    for (auto const & spring : npc.ParticleMesh.Springs)
    {
        vec2f const springDisplacement = mParticles.GetPosition(spring.EndpointAIndex) - mParticles.GetPosition(spring.EndpointBIndex); // Towards A
        float const springDisplacementLength = springDisplacement.length();
        vec2f const springDir = springDisplacement.normalise_approx(springDisplacementLength);

        //
        // 3a. Hooke's law
        //

        // Calculate spring force on this particle
        float const fSpring =
            (springDisplacementLength - spring.RestLength)
            * spring.SpringStiffnessFactor;

        //
        // 3b. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on this particle
        vec2f const relVelocity = mParticles.GetVelocity(spring.EndpointAIndex) - mParticles.GetVelocity(spring.EndpointBIndex);
        float const fDamp =
            relVelocity.dot(springDir)
            * spring.SpringDampingFactor;

        //
        // Apply forces
        //

        vec2f const springForce = springDir * (fSpring + fDamp);

        mParticles.SetPreliminaryForces(spring.EndpointAIndex, mParticles.GetPreliminaryForces(spring.EndpointAIndex) - springForce);
        mParticles.SetPreliminaryForces(spring.EndpointBIndex, mParticles.GetPreliminaryForces(spring.EndpointBIndex) + springForce);
    }
}

vec2f Npcs::CalculateNpcParticleDefinitiveForces(
    StateType const & npc,
    int npcParticleOrdinal,
    GameParameters const & gameParameters) const
{
    float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

    auto & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];

    vec2f definitiveForces = mParticles.GetPreliminaryForces(npcParticle.ParticleIndex);

    //
    // Human Equlibrium Torque
    //

    if (npc.Kind == NpcKindType::Human
        && npc.KindSpecificState.HumanNpcState.EquilibriumTorque != 0.0f
        && npcParticleOrdinal > 0)
    {
        ElementIndex const primaryParticleIndex = npc.ParticleMesh.Particles[0].ParticleIndex;
        ElementIndex const secondaryParticleIndex = npcParticle.ParticleIndex;

        vec2f const feetPosition = mParticles.GetPosition(primaryParticleIndex);

        // Given that we apply torque onto the secondary particle *after* the primary has been simulated
        // (so that we take into account the primary's new position), and thus we see the primary where it
        // is at the end of the step - possibly far away if mesh velocity is high, we want to _predict_
        // where the secondary will be by its own velocity

        vec2f const headPredictedPosition =
            mParticles.GetPosition(secondaryParticleIndex)
            + mParticles.GetVelocity(secondaryParticleIndex) * dt;

        vec2f const humanDir = (headPredictedPosition - feetPosition).normalise_approx();

        // Calculate radial force direction
        vec2f radialDir = humanDir.to_perpendicular(); // CCW
        if (humanDir.x < 0.0f)
        {
            // Head is to the left of the vertical
            radialDir *= -1.0f;
        }

        //
        // First component: Hookean force proportional to length of arc to destination, directed towards the ideal head position
        //  - But we approximate the arc with the chord, i.e.the distance between source and destination
        //

        assert(npc.ParticleMesh.Springs.size() == 1);
        vec2f const idealHeadPosition = feetPosition + vec2f(0.0f, npc.ParticleMesh.Springs[0].RestLength);

        float const stiffnessCoefficient =
            gameParameters.HumanNpcEquilibriumTorqueStiffnessCoefficient
            + std::min(npc.KindSpecificState.HumanNpcState.ResultantPanicLevel, 1.0f) * 0.0005f;

        float const force1Magnitude =
            (idealHeadPosition - headPredictedPosition).length()
            * stiffnessCoefficient;

        //
        // Second component: damp force proportional to component of relative velocity that is orthogonal to human vector, opposite that velocity
        //

        vec2f const relativeVelocity = mParticles.GetVelocity(secondaryParticleIndex) - mParticles.GetVelocity(primaryParticleIndex);
        float const orthoRelativeVelocity = relativeVelocity.dot(radialDir);

        float const dampCoefficient = gameParameters.HumanNpcEquilibriumTorqueDampingCoefficient;

        float const force2Magnitude =
            -orthoRelativeVelocity
            * dampCoefficient;

        //
        // Combine
        //

        vec2f const equilibriumTorqueForce =
            radialDir
            * (force1Magnitude + force2Magnitude)
            / mCurrentSizeMultiplier // Note: we divide by size adjustment to maintain torque independent from lever length
            * mParticles.GetMass(npcParticle.ParticleIndex) / (dt * dt);

        definitiveForces += equilibriumTorqueForce;
    }

    return definitiveForces;
}

void Npcs::RecalculateSizeAndMassParameters()
{
    for (auto & state : mStateBuffer)
    {
        if (state.has_value())
        {
            //
            // Particles
            //

            for (auto & particle : state->ParticleMesh.Particles)
            {
                mParticles.SetMass(particle.ParticleIndex,
                    CalculateParticleMass(
                        mParticles.GetMaterial(particle.ParticleIndex).GetMass(),
                        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                        , mCurrentMassAdjustment
#endif
                    ));

                mParticles.SetBuoyancyFactor(particle.ParticleIndex,
                    CalculateParticleBuoyancyFactor(
                        mParticles.GetBuoyancyVolumeFill(particle.ParticleIndex),
                        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                        , mCurrentBuoyancyAdjustment
#endif
                    ));
            }

            //
            // Springs
            //

            CalculateSprings(
                mCurrentSizeMultiplier,
#ifdef IN_BARYLAB
                mCurrentMassAdjustment,
#endif
                mCurrentSpringReductionFractionAdjustment,
                mCurrentSpringDampingCoefficientAdjustment,
                mParticles,
                state->ParticleMesh);
        }
    }
}

float Npcs::CalculateParticleMass(
    float baseMass,
    float sizeMultiplier
#ifdef IN_BARYLAB
    , float massAdjustment
#endif
    )
{
    float const particleMass =
        baseMass
        * (sizeMultiplier * sizeMultiplier) // 2D "volume" adjustment
#ifdef IN_BARYLAB
        * massAdjustment
#endif
        ;

    return particleMass;
}

float Npcs::CalculateParticleBuoyancyFactor(
    float baseBuoyancyVolumeFill,
    float sizeMultiplier
#ifdef IN_BARYLAB
    , float buoyancyAdjustment
#endif
)
{
    float const buoyancyFactor =
        GameParameters::GravityMagnitude * 1000.0f
        * (sizeMultiplier * sizeMultiplier) // 2D "volume" adjustment
        * baseBuoyancyVolumeFill
#ifdef IN_BARYLAB
        * buoyancyAdjustment
#endif
        ;

    return buoyancyFactor;
}

void Npcs::RecalculateFrictionTotalAdjustments()
{
    for (auto & state : mStateBuffer)
    {
        if (state.has_value())
        {
            for (int p = 0; p < state->ParticleMesh.Particles.size(); ++p)
            {
                auto const particleIndex = state->ParticleMesh.Particles[p].ParticleIndex;

                switch (state->Kind)
                {
                    case NpcKindType::Furniture:
                    {
                        mParticles.SetStaticFrictionTotalAdjustment(particleIndex,
                            CalculateFrictionTotalAdjustment(
                                mNpcDatabase.GetFurnitureParticleAttributes(state->KindSpecificState.FurnitureNpcState.SubKindId, p).FrictionSurfaceAdjustment,
                                mCurrentNpcFrictionAdjustment,
                                mCurrentStaticFrictionAdjustment));

                        mParticles.SetKineticFrictionTotalAdjustment(particleIndex,
                            CalculateFrictionTotalAdjustment(
                                mNpcDatabase.GetFurnitureParticleAttributes(state->KindSpecificState.FurnitureNpcState.SubKindId, p).FrictionSurfaceAdjustment,
                                mCurrentNpcFrictionAdjustment,
                                mCurrentKineticFrictionAdjustment));

                        break;
                    }

                    case NpcKindType::Human:
                    {
                        assert(p == 0 || p == 1);

                        float const frictionSurfaceAdjustment = (p == 0)
                            ? mNpcDatabase.GetHumanFeetParticleAttributes(state->KindSpecificState.FurnitureNpcState.SubKindId).FrictionSurfaceAdjustment
                            : mNpcDatabase.GetHumanHeadParticleAttributes(state->KindSpecificState.FurnitureNpcState.SubKindId).FrictionSurfaceAdjustment;

                        mParticles.SetStaticFrictionTotalAdjustment(particleIndex,
                            CalculateFrictionTotalAdjustment(
                                frictionSurfaceAdjustment,
                                mCurrentNpcFrictionAdjustment,
                                mCurrentStaticFrictionAdjustment));

                        mParticles.SetKineticFrictionTotalAdjustment(particleIndex,
                            CalculateFrictionTotalAdjustment(
                                frictionSurfaceAdjustment,
                                mCurrentNpcFrictionAdjustment,
                                mCurrentKineticFrictionAdjustment));

                        break;
                    }
                }
            }
        }
    }
}

float Npcs::CalculateFrictionTotalAdjustment(
    float npcSurfaceFrictionAdjustment,
    float npcAdjustment,
    float globalAdjustment)
{
    return
        npcSurfaceFrictionAdjustment
        * npcAdjustment
        * globalAdjustment;
}

void Npcs::CalculateSprings(
    float sizeMultiplier,
#ifdef IN_BARYLAB
    float massAdjustment,
#endif
    float springReductionFractionAdjustment,
    float springDampingCoefficientAdjustment,
    NpcParticles const & particles,
    StateType::ParticleMeshType & mesh)
{
    float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

    for (auto & spring : mesh.Springs)
    {
        // Spring rest length

        spring.RestLength = CalculateSpringLength(spring.BaseRestLength, sizeMultiplier);

        // Spring force factors

        float const baseMass1 = particles.GetMaterial(spring.EndpointAIndex).GetMass();
        float const baseMass2 = particles.GetMaterial(spring.EndpointBIndex).GetMass();

        float const baseMassFactor =
            (baseMass1 * baseMass2)
            / (baseMass1 + baseMass2);

        spring.SpringStiffnessFactor =
            spring.BaseSpringReductionFraction
            * springReductionFractionAdjustment
            * baseMassFactor
#ifdef IN_BARYLAB
            * massAdjustment
#endif
            * (sizeMultiplier * sizeMultiplier) // 2D
            / (dt * dt);

        spring.SpringDampingFactor =
            spring.BaseSpringDampingCoefficient
            * springDampingCoefficientAdjustment
            * baseMassFactor
#ifdef IN_BARYLAB
            * massAdjustment
#endif
            * (sizeMultiplier * sizeMultiplier) // 2D
            / dt;
    }
}

float Npcs::CalculateSpringLength(
    float baseLength,
    float sizeMultiplier)
{
    return baseLength * sizeMultiplier;
}

void Npcs::RecalculateGlobalDampingFactor()
{
    mGlobalDampingFactor = 1.0f - GameParameters::NpcDamping * mCurrentGlobalDampingAdjustment;
}

void Npcs::UpdateNpcParticle_BeingPlaced(
    StateType & npc,
    int npcParticleOrdinal,
    vec2f physicsDeltaPos,
    NpcParticles & particles) const
{
    assert(npc.CurrentRegime == StateType::RegimeType::BeingPlaced);

    LogNpcDebug("    Being placed");

    if (npc.BeingPlacedState->DoMoveWholeMesh || npcParticleOrdinal == npc.BeingPlacedState->AnchorParticleOrdinal)
    {
        //
        // Particle is moved independently and fixed - nothing to do
        //
    }
    else
    {
        //
        // Particle is moved dependently
        //

        auto const & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];
        assert(!npcParticle.ConstrainedState.has_value());

        vec2f const & startPosition = particles.GetPosition(npcParticle.ParticleIndex);

        //
        // Strive to maintain spring lengths (fight stretching)
        //

        if (npc.ParticleMesh.Springs.size() > 0)
        {
            assert(npcParticleOrdinal != npc.BeingPlacedState->AnchorParticleOrdinal);

            vec2f adjustedDeltaPosSum = vec2f::zero();
            float adjustedDeltaPosCount = 0.0f;

            // Pair this particle with:
            // a) Each particle already done,
            // b) The anchor particle
            for (int p = 0; p < npc.ParticleMesh.Particles.size(); ++p)
            {
                if (p < npcParticleOrdinal || (p > npcParticleOrdinal && p == npc.BeingPlacedState->AnchorParticleOrdinal))
                {
                    // Adjust physicsDeltaPos to maintain spring length after we've traveled it

                    int const s = GetSpringAmongEndpoints(npcParticleOrdinal, p, npc.ParticleMesh);
                    float const targetSpringLength = npc.ParticleMesh.Springs[s].RestLength;
                    vec2f const & otherPPosition = mParticles.GetPosition(npc.ParticleMesh.Particles[p].ParticleIndex);
                    vec2f const particleAdjustedPosition =
                        otherPPosition
                        + (startPosition + physicsDeltaPos - otherPPosition).normalise() * targetSpringLength;

                    adjustedDeltaPosSum += particleAdjustedPosition - startPosition;
                    adjustedDeltaPosCount += 1.0f;
                }
            }

            assert(adjustedDeltaPosCount != 0.0f);
            physicsDeltaPos = adjustedDeltaPosSum / adjustedDeltaPosCount;
        }

        //
        // Update physics
        //

        float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

        // Update position
        particles.SetPosition(
            npcParticle.ParticleIndex,
            startPosition + physicsDeltaPos);

        // Update velocity
        //
        // Note: we clamp it exactly as the anchor's is clamped
        particles.SetVelocity(
            npcParticle.ParticleIndex,
            ClampPlacementVelocity(physicsDeltaPos / dt * mGlobalDampingFactor));

        //
        // Unfold if folded quad (fight long strides)
        //

        if (npc.ParticleMesh.Particles.size() == 4)
        {
            //
            // Check folds along the two diagonals - which one to use depends
            // on the particle
            //

            std::array<std::pair<int, int>, 4> const diagonalEndpoints{ { // tail->head, in the order that makes the triangle with this particle CW
                {1, 3},
                {2, 0},
                {3, 1},
                {0, 2}
            } };

            ElementIndex const diagTailParticleIndex = npc.ParticleMesh.Particles[diagonalEndpoints[npcParticleOrdinal].first].ParticleIndex;
            ElementIndex const diagHeadParticleIndex = npc.ParticleMesh.Particles[diagonalEndpoints[npcParticleOrdinal].second].ParticleIndex;

            vec2f const & diagTailPosition = particles.GetPosition(diagTailParticleIndex);

            vec2f const diagonalVector =
                particles.GetPosition(diagHeadParticleIndex)
                - diagTailPosition;

            vec2f const particleVector =
                particles.GetPosition(npcParticle.ParticleIndex)
                - diagTailPosition;

            if (particleVector.cross(diagonalVector) < 0.0f)
            {
                // This particle is on the wrong size of the diagonal
                LogNpcDebug("BeingMoved Quad ", npc.Id, ":", npcParticleOrdinal, ": folded");

                //
                // Move particle to other side of diagonal
                //
                // We do so by moving particle twice along normal to diagonal
                //
                // Note: the cross product we have is the dot product of the
                // particle vector with the perpendicular to the diagonal;
                // almost what we need, with an extra |diagonal| on top of it.
                // Since we have to calculate a length and then perform a division,
                // for clarity we keep the formulation of P - 2*diag_n
                //

                vec2f const dNorm = diagonalVector.to_perpendicular().normalise_approx();
                float const particleVectorAlongDiagonalNormal = particleVector.dot(dNorm);
                assert(particleVectorAlongDiagonalNormal > 0.0f); // Because we know cross < 0
                vec2f const newPos =
                    particles.GetPosition(npcParticle.ParticleIndex)
                    - dNorm * 2.0f * particleVectorAlongDiagonalNormal;

                particles.SetPosition(npcParticle.ParticleIndex, newPos);
            }
        }
    }
}

void Npcs::UpdateNpcParticle_Free(
    StateType::NpcParticleStateType & particle,
    vec2f const & startPosition,
    vec2f const & endPosition,
    NpcParticles & particles) const
{
    float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

    assert(!particle.ConstrainedState.has_value());

    // Update position
    particles.SetPosition(
        particle.ParticleIndex,
        endPosition);

    // Update velocity
    // Use whole time quantum for velocity, as communicated start/end positions are those planned for whole dt
    particles.SetVelocity(
        particle.ParticleIndex,
        (endPosition - startPosition) / dt * mGlobalDampingFactor);
}

Npcs::ConstrainedNonInertialOutcome Npcs::UpdateNpcParticle_ConstrainedNonInertial(
    StateType & npc,
    int npcParticleOrdinal,
    int edgeOrdinal,
    vec2f const & edgeDir,
    vec2f const & particleStartAbsolutePosition,
    vec2f const & trajectoryStartAbsolutePosition,
    vec2f const & flattenedTrajectoryEndAbsolutePosition,
    bcoords3f flattenedTrajectoryEndBarycentricCoords,
    vec2f const & flattenedTrajectory,
    float edgeTraveledPlanned,
    bool hasMovedInStep,
    vec2f const meshVelocity,
    float dt,
    Ship & homeShip,
    NpcParticles & particles,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    auto & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];
    assert(npcParticle.ConstrainedState.has_value());
    auto & npcParticleConstrainedState = *npcParticle.ConstrainedState;

    //
    // Ray-trace along the flattened trajectory, ending only at one of the following three conditions:
    // 1. Reached destination: terminate
    // 2. Becoming free: do free movement and terminate
    // 3. Impact:
    //      - With bounce: impart bounce velocity and terminate
    //      - Without bounce: advance simulation by how much walked, re-flatten trajectory, and continue
    //

    //
    // If target is on/in triangle, we move to target
    //

    if (flattenedTrajectoryEndBarycentricCoords.is_on_edge_or_internal())
    {
        LogNpcDebug("      Target is on/in triangle, moving to target");

        //
        // Update particle and exit - consuming whole time quantum
        //

        npcParticleConstrainedState.CurrentBCoords.BCoords = flattenedTrajectoryEndBarycentricCoords;
        particles.SetPosition(npcParticle.ParticleIndex, flattenedTrajectoryEndAbsolutePosition);

        //
        // Velocity: given that we've completed *along the edge*, then we can calculate
        // our (relative) velocity based on the distance traveled along this (remaining) time quantum
        //
        // We take into account only the edge traveled at this moment, divided by the length of this time quantum:
        // V = signed_edge_traveled_actual * edgeDir / this_dt
        //
        // Now: consider that at this moment we've reached the planned end of the iteration's sub-trajectory;
        // we can then assume that signed_edge_traveled_actual == signed_edge_traveled_planned (verified via assert)
        //

        assert(std::abs((flattenedTrajectoryEndAbsolutePosition - trajectoryStartAbsolutePosition).dot(edgeDir) - edgeTraveledPlanned) < 0.001f);

        vec2f const relativeVelocity =
            edgeDir
            * edgeTraveledPlanned
            / dt;

        vec2f const absoluteVelocity =
            // Do not damp velocity if we're trying to maintain equilibrium
            relativeVelocity * ((npc.Kind != NpcKindType::Human || npc.KindSpecificState.HumanNpcState.EquilibriumTorque == 0.0f) ? mGlobalDampingFactor : 1.0f)
            + meshVelocity;

        particles.SetVelocity(npcParticle.ParticleIndex, absoluteVelocity);
        npcParticleConstrainedState.MeshRelativeVelocity = relativeVelocity;

        LogNpcDebug("        edgeTraveleded (==planned)=", edgeTraveledPlanned, " absoluteVelocity=", particles.GetVelocity(npcParticle.ParticleIndex));

        // Complete
        return ConstrainedNonInertialOutcome::MakeStopOutcome(edgeTraveledPlanned, false);
    }

    //
    // Target is outside triangle
    //

    LogNpcDebug("      Target is outside triangle");

    //
    // Find closest intersection in the direction of the trajectory, which is
    // a vertex of this triangle
    //
    // Guaranteed to exist and within trajectory, because target is outside
    // of triangle and we're on an edge
    //

    // Here we take advantage of the fact that we know we're on an edge

    assert(npcParticleConstrainedState.CurrentBCoords.BCoords[(edgeOrdinal + 2) % 3] == 0.0f);

    int intersectionVertexOrdinal;
    if (flattenedTrajectoryEndBarycentricCoords[edgeOrdinal] < 0.0f)
    {
        // It's the vertex at the end of our edge
        assert(flattenedTrajectoryEndBarycentricCoords[(edgeOrdinal + 1) % 3] > 0.0f); // Because target is outside of triangle
        intersectionVertexOrdinal = (edgeOrdinal + 1) % 3;
    }
    else
    {
        // It's the vertex before our edge
        assert(flattenedTrajectoryEndBarycentricCoords[(edgeOrdinal + 1) % 3] < 0.0f);
        intersectionVertexOrdinal = edgeOrdinal;
    }

    //
    // Move to intersection vertex
    //
    // Note that except for the very first iteration, any other iteration will travel
    // zero distance at this moment
    //

    bcoords3f intersectionBarycentricCoords = bcoords3f::zero();
    intersectionBarycentricCoords[intersectionVertexOrdinal] = 1.0f;

    LogNpcDebug("      Moving to intersection vertex ", intersectionVertexOrdinal, ": ", intersectionBarycentricCoords);

    // Calculate (signed) edge traveled
    float edgeTraveled;
    if (intersectionBarycentricCoords == npcParticleConstrainedState.CurrentBCoords.BCoords)
    {
        // We haven't moved - ensure pure zero
        edgeTraveled = 0.0f;
    }
    else
    {
        vec2f const intersectionAbsolutePosition = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointIndices(npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex)[intersectionVertexOrdinal]);
        assert(intersectionAbsolutePosition == homeShip.GetTriangles().FromBarycentricCoordinates(
            intersectionBarycentricCoords,
            npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex,
            homeShip.GetPoints()));

        edgeTraveled = (intersectionAbsolutePosition - trajectoryStartAbsolutePosition).dot(edgeDir);
    }

    LogNpcDebug("        edgeTraveled=", edgeTraveled);

    // Move
    npcParticleConstrainedState.CurrentBCoords.BCoords = intersectionBarycentricCoords;

    //
    // Navigate this vertex now
    //

    auto const navigationOutcome = NavigateVertex(
        npc,
        npcParticleOrdinal,
        TriangleAndEdge(npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex, edgeOrdinal),
        intersectionVertexOrdinal,
        flattenedTrajectory,
        flattenedTrajectoryEndAbsolutePosition,
        flattenedTrajectoryEndBarycentricCoords,
        homeShip,
        particles);

    switch (navigationOutcome.Type)
    {
        case NavigateVertexOutcome::OutcomeType::BecomeFree:
        {
            // Become free

            TransitionParticleToFreeState(npc, npcParticleOrdinal, homeShip);

            UpdateNpcParticle_Free(
                npcParticle,
                particleStartAbsolutePosition,
                flattenedTrajectoryEndAbsolutePosition,
                particles);

            // Terminate
            return ConstrainedNonInertialOutcome::MakeStopOutcome(edgeTraveled, false);
        }

        case NavigateVertexOutcome::OutcomeType::ContinueAlongFloor:
        {
            // Impact continuation, continue

            // Move to NavigationOutcome
            npcParticleConstrainedState.CurrentBCoords = navigationOutcome.TriangleBCoords;

            // Continue
            return ConstrainedNonInertialOutcome::MakeContinueOutcome(
                edgeTraveled,
                navigationOutcome.FloorEdgeOrdinal); // This is the edge we want to be on, we may bypass the next NavigateVertex
        }

        case NavigateVertexOutcome::OutcomeType::ContinueToInterior:
        {
            // Continue

            // Move to NavigationOutcome
            npcParticleConstrainedState.CurrentBCoords = navigationOutcome.TriangleBCoords;

            // Continue
            return ConstrainedNonInertialOutcome::MakeContinueOutcome(
                edgeTraveled,
                -1); // Must determine next edge via normals
        }

        case NavigateVertexOutcome::OutcomeType::ImpactOnFloor:
        {
            // Bounce

            // Move to bounce position - so that we can eventually make a fresh new choice
            // now that we have acquired a velocity in a different direction
            npcParticleConstrainedState.CurrentBCoords = navigationOutcome.TriangleBCoords;

            vec2f const bounceAbsolutePosition = homeShip.GetTriangles().FromBarycentricCoordinates(
                navigationOutcome.TriangleBCoords.BCoords,
                navigationOutcome.TriangleBCoords.TriangleElementIndex,
                homeShip.GetPoints());

            BounceConstrainedNpcParticle(
                npc,
                npcParticleOrdinal,
                flattenedTrajectory,
                hasMovedInStep || (edgeTraveled != 0.0f),
                bounceAbsolutePosition,
                navigationOutcome.FloorEdgeOrdinal,
                meshVelocity,
                dt,
                homeShip,
                particles,
                currentSimulationTime,
                gameParameters);

            // Terminate
            return ConstrainedNonInertialOutcome::MakeStopOutcome(edgeTraveled, true);
        }
    }

    assert(false);
    return ConstrainedNonInertialOutcome::MakeStopOutcome(edgeTraveled, false); // Just to return something
}

float Npcs::UpdateNpcParticle_ConstrainedInertial(
    StateType & npc,
    int npcParticleOrdinal,
    vec2f const & particleStartAbsolutePosition, // Since beginning of whole time quantum, not just this step
    vec2f const & segmentTrajectoryStartAbsolutePosition,
    vec2f const & segmentTrajectoryEndAbsolutePosition,
    bcoords3f segmentTrajectoryEndBarycentricCoords, // In current triangle; mutable
    bool hasMovedInStep,
    vec2f const meshVelocity,
    float segmentDt,
    Ship & homeShip,
    NpcParticles & particles,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    auto & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];
    assert(npcParticle.ConstrainedState.has_value());
    auto & npcParticleConstrainedState = *npcParticle.ConstrainedState;

    //
    // Ray-trace along the specified trajectory, ending only at one of the following three conditions:
    // 1. Reached destination: terminate
    // 2. Becoming free: do free movement and terminate
    // 3. Impact with bounce: impart bounce velocity and terminate
    //

    for (int iIter = 0; ; ++iIter)
    {
        assert(iIter < 32); // Detect and debug-break on infinite loops

        assert(npcParticleConstrainedState.CurrentBCoords.BCoords.is_on_edge_or_internal());

        LogNpcDebug("    SegmentTrace ", iIter);
        LogNpcDebug("      triangle=", npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex, " bCoords=", npcParticleConstrainedState.CurrentBCoords.BCoords,
            " segmentTrajEndBCoords=", segmentTrajectoryEndBarycentricCoords);

        //
        // If target is on/in triangle, we move to target
        //

        if (segmentTrajectoryEndBarycentricCoords.is_on_edge_or_internal())
        {
            LogNpcDebug("      Target is on/in triangle, moving to target");

            //
            // Update particle and exit - consuming whole time quantum
            //

            // Move particle to end of trajectory
            npcParticleConstrainedState.CurrentBCoords.BCoords = segmentTrajectoryEndBarycentricCoords;
            particles.SetPosition(npcParticle.ParticleIndex, segmentTrajectoryEndAbsolutePosition);

            // Use whole time quantum for velocity, as particleStartAbsolutePosition is fixed at t0
            vec2f const totalAbsoluteTraveledVector = segmentTrajectoryEndAbsolutePosition - particleStartAbsolutePosition;
            vec2f const relativeVelocity = totalAbsoluteTraveledVector / GameParameters::SimulationStepTimeDuration<float> - meshVelocity;

            // Do not damp velocity if we're trying to maintain equilibrium
            vec2f const absoluteVelocity =
                relativeVelocity * ((npc.Kind != NpcKindType::Human || npc.KindSpecificState.HumanNpcState.EquilibriumTorque == 0.0f) ? mGlobalDampingFactor : 1.0f)
                + meshVelocity;

            particles.SetVelocity(npcParticle.ParticleIndex, absoluteVelocity);
            npcParticleConstrainedState.MeshRelativeVelocity = relativeVelocity;

            LogNpcDebug("        totalAbsoluteTraveledVector=", totalAbsoluteTraveledVector, " absoluteVelocity=", particles.GetVelocity(npcParticle.ParticleIndex));

            // Return (mesh-relative) distance traveled with this move
            return (segmentTrajectoryEndAbsolutePosition - segmentTrajectoryStartAbsolutePosition).length();
        }

        //
        // We're inside or on triangle, and target is outside triangle;
        // if we're on edge, trajectory is along this edge
        //

        LogNpcDebug("      Target is outside triangle");

        //
        // Find closest intersection in the direction of the trajectory
        //
        // Guaranteed to exist and within trajectory, because target is outside
        // of triangle and we're inside or on an edge
        //

#ifdef _DEBUG

        struct EdgeIntersectionDiag
        {
            float Den;
            float T;
            bcoords3f IntersectionPoint;

            EdgeIntersectionDiag(
                float den,
                float t)
                : Den(den)
                , T(t)
                , IntersectionPoint()
            {}
        };

        std::array<std::optional<EdgeIntersectionDiag>, 3> diags;

#endif // DEBUG

        int intersectionVertexOrdinal = -1;
        float minIntersectionT = std::numeric_limits<float>::max();

        for (int vi = 0; vi < 3; ++vi)
        {
            // Only consider edges that we ancounter ahead along the trajectory
            if (segmentTrajectoryEndBarycentricCoords[vi] < 0.0f)
            {
                float const den = npcParticleConstrainedState.CurrentBCoords.BCoords[vi] - segmentTrajectoryEndBarycentricCoords[vi];
                float const t = npcParticleConstrainedState.CurrentBCoords.BCoords[vi] / den;

#ifdef _DEBUG
                diags[vi].emplace(den, t);
                diags[vi]->IntersectionPoint =
                    npcParticleConstrainedState.CurrentBCoords.BCoords
                    + (segmentTrajectoryEndBarycentricCoords - npcParticleConstrainedState.CurrentBCoords.BCoords) * t;
#endif

                assert(t > -Epsilon<float>); // Some numeric slack, trajectory is here guaranteed to be pointing into this edge

                LogNpcDebug("        t[v", vi, " e", ((vi + 1) % 3), "] = ", t);

                if (t < minIntersectionT)
                {
                    intersectionVertexOrdinal = vi;
                    minIntersectionT = t;
                }
            }
        }

        assert(intersectionVertexOrdinal >= 0); // Guaranteed to exist
        assert(minIntersectionT > -Epsilon<float> && minIntersectionT <= 1.0f); // Guaranteed to exist, and within trajectory

        int const intersectionEdgeOrdinal = (intersectionVertexOrdinal + 1) % 3;

        // Calculate intersection barycentric coordinates

        bcoords3f intersectionBarycentricCoords;
        intersectionBarycentricCoords[intersectionVertexOrdinal] = 0.0f;
        float const lNext = Clamp( // Barycentric coord of next vertex at intersection; enforcing it's within triangle
            npcParticleConstrainedState.CurrentBCoords.BCoords[(intersectionVertexOrdinal + 1) % 3] * (1.0f - minIntersectionT)
            + segmentTrajectoryEndBarycentricCoords[(intersectionVertexOrdinal + 1) % 3] * minIntersectionT,
            0.0f,
            1.0f);
        intersectionBarycentricCoords[(intersectionVertexOrdinal + 1) % 3] = lNext;
        intersectionBarycentricCoords[(intersectionVertexOrdinal + 2) % 3] = 1.0f - lNext;

        assert(intersectionBarycentricCoords.is_on_edge_or_internal());

        //
        // Move to intersection, by moving barycentric coords
        //

        LogNpcDebug("      Moving bary coords to intersection with edge ", intersectionEdgeOrdinal, " ", intersectionBarycentricCoords);

        npcParticleConstrainedState.CurrentBCoords.BCoords = intersectionBarycentricCoords;

        //
        // Check if impacted with floor
        //

        vec2f const intersectionAbsolutePosition = homeShip.GetTriangles().FromBarycentricCoordinates(
            intersectionBarycentricCoords,
            npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex,
            homeShip.GetPoints());

        if (IsEdgeFloorToParticle(npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex, intersectionEdgeOrdinal, npc, npcParticleOrdinal, mParticles, homeShip))
        {
            //
            // Impact and bounce
            //

            LogNpcDebug("      Impact and bounce");

            //
            // Calculate bounce response, using the *apparent* (trajectory)
            // velocity - since this one includes the mesh velocity
            //

            vec2f const trajectory = segmentTrajectoryEndAbsolutePosition - segmentTrajectoryStartAbsolutePosition;

            vec2f const intersectionEdgeDir =
                homeShip.GetTriangles().GetSubSpringVector(
                    npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex,
                    intersectionEdgeOrdinal,
                    homeShip.GetPoints())
                .normalise();
            vec2f const intersectionEdgeNormal = intersectionEdgeDir.to_perpendicular();

            BounceConstrainedNpcParticle(
                npc,
                npcParticleOrdinal,
                trajectory,
                hasMovedInStep || ((intersectionAbsolutePosition - segmentTrajectoryStartAbsolutePosition).length() != 0.0f),
                intersectionAbsolutePosition,
                intersectionEdgeOrdinal,
                meshVelocity,
                segmentDt,
                homeShip,
                particles,
                currentSimulationTime,
                gameParameters);

            // Remember that - at least for this frame - we are non-inertial on this floor
            //
            // We do this to interrupt long streaks off-floor while we bounce multiple times on the floor
            // while walking; without interruptions the multiple bounces, with no intervening non-inertial
            // step, would cause the NPC to stop walking. Afte rall we're not lying as we're really
            // non-inertial on this floor
            npcParticleConstrainedState.CurrentVirtualFloor.emplace(npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex, intersectionEdgeOrdinal);

            // Return (mesh-relative) distance traveled with this move
            return (segmentTrajectoryEndAbsolutePosition - segmentTrajectoryStartAbsolutePosition).length();
        }

        //
        // Not floor, climb over edge
        //

        LogNpcDebug("      Climbing over non-floor edge");

        // Find opposite triangle
        auto const & oppositeTriangleInfo = homeShip.GetTriangles().GetOppositeTriangle(npcParticleConstrainedState.CurrentBCoords.TriangleElementIndex, intersectionEdgeOrdinal);
        if (oppositeTriangleInfo.TriangleElementIndex == NoneElementIndex || homeShip.GetTriangles().IsDeleted(oppositeTriangleInfo.TriangleElementIndex))
        {
            //
            // Become free
            //

            LogNpcDebug("      No opposite triangle found, becoming free");

            //
            // Move to endpoint and exit, consuming whole quantum
            //

            TransitionParticleToFreeState(npc, npcParticleOrdinal, homeShip);

            UpdateNpcParticle_Free(
                npcParticle,
                particleStartAbsolutePosition,
                segmentTrajectoryEndAbsolutePosition,
                particles);

            vec2f const totalTraveledVector = intersectionAbsolutePosition - particleStartAbsolutePosition; // We consider constrained portion only
            return totalTraveledVector.length();
        }
        else
        {
            //
            // Move to edge of opposite triangle
            //

            LogNpcDebug("      Moving to edge ", oppositeTriangleInfo.EdgeOrdinal, " of opposite triangle ", oppositeTriangleInfo.TriangleElementIndex);

            // Calculate new current barycentric coords (wrt new triangle)
            bcoords3f newBarycentricCoords; // In new triangle
            newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 2) % 3] = 0.0f;
            newBarycentricCoords[oppositeTriangleInfo.EdgeOrdinal] = npcParticleConstrainedState.CurrentBCoords.BCoords[(intersectionEdgeOrdinal + 1) % 3];
            newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 1) % 3] = npcParticleConstrainedState.CurrentBCoords.BCoords[intersectionEdgeOrdinal];

            LogNpcDebug("      B-Coords: ", npcParticleConstrainedState.CurrentBCoords.BCoords, " -> ", newBarycentricCoords);

            assert(newBarycentricCoords.is_on_edge_or_internal());

            // Move to triangle and coords
            npcParticleConstrainedState.CurrentBCoords = AbsoluteTriangleBCoords(oppositeTriangleInfo.TriangleElementIndex, newBarycentricCoords);

            // Translate target coords to this triangle, for next iteration
            auto const oldSegmentTrajectoryEndBarycentricCoords = segmentTrajectoryEndBarycentricCoords; // For logging
            // Note: here we introduce a lot of error - the target bary coords are not anymore
            // guaranteed to lie exactly on the (continuation of the) edge
            segmentTrajectoryEndBarycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinatesInsideEdge(
                segmentTrajectoryEndAbsolutePosition,
                oppositeTriangleInfo.TriangleElementIndex,
                homeShip.GetPoints(),
                oppositeTriangleInfo.EdgeOrdinal);

            LogNpcDebug("      TrajEndB-Coords: ", oldSegmentTrajectoryEndBarycentricCoords, " -> ", segmentTrajectoryEndBarycentricCoords);

            // Continue
        }
    }
}

inline Npcs::NavigateVertexOutcome Npcs::NavigateVertex(
    StateType const & npc,
    int npcParticleOrdinal,
    std::optional<TriangleAndEdge> const & walkedEdge,
    int vertexOrdinal, // Mutable
    vec2f const & trajectory,
    vec2f const & trajectoryEndAbsolutePosition,
    bcoords3f trajectoryEndBarycentricCoords, // Mutable
    Ship const & homeShip,
    NpcParticles const & particles)
{
    // Rules of the code:
    // - We communicate the particle's final state (triangle & bcoords) upon leaving
    //      - And most importantly, we don't move
    // - We take into account *actual* (resultant physical) movement (i.e. trajectory), rather than *intended* (walkdir) movement

    //
    // See whether this particle is the primary (feet) of a walking human
    //

    auto const & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];
    assert(npcParticle.ConstrainedState.has_value());

    if (npc.Kind == NpcKindType::Human
        && npc.KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking
        && walkedEdge.has_value()
        && npcParticleOrdinal == 0)
    {
        //
        // Navigate around this vertex according to CW/CCW and choose floors to walk on; we return any of these:
        // - We are directed inside a triangle (iff no floors have been found)
        // - We have chosen a floor
        // - We have detected an impact against a floor (iff no floors have been found)
        // - We become free
        //

        //
        // When in walking state and arriving at a vertex at which there are more than two *viable* floors there (incl.incoming, so >= 2 + 1), choose which one to take
        //    - It's like "not seeing" certain floors
        // Note: *viable* == with right slope for walking on it
        //    - We only consider those floors that are in a sector centered around walk(face) dir, up amplitude equal to = / -MaxSlopeForWalking, and down amplitude less than vertical
        //    - This allows us to take a down "stair" that is almost vertical
        //    - We only choose in our direction because the simulation is still very much physical, i.e.informed by trajectory
        //
        // We divide floor candidates in two groups: easy slope and hard slope (i.e. almost falling); we only choose among hard slopes if there
        // are no easy slopes. Rationale: NPC going about S and hitting wall-floor conjunction; we don't want it to take wall going down
        //      |
        //   ---|
        //    */|
        //    / |
        //

        struct AbsoluteTriangleBCoordsAndEdge
        {
            AbsoluteTriangleBCoords TriangleBCoords;
            int EdgeOrdinal;
        };

        std::array<AbsoluteTriangleBCoordsAndEdge, GameParameters::MaxSpringsPerPoint> floorCandidatesEasySlope;
        size_t floorCandidatesEasySlopeCount = 0;
        std::array<AbsoluteTriangleBCoordsAndEdge, GameParameters::MaxSpringsPerPoint> floorCandidatesHardSlope;
        size_t floorCandidatesHardSlopeCount = 0;
        std::optional<AbsoluteTriangleBCoordsAndEdge> firstBounceableFloor;
        std::optional<AbsoluteTriangleBCoords> firstTriangleInterior;

        AbsoluteTriangleBCoords currentAbsoluteBCoords = npcParticle.ConstrainedState->CurrentBCoords;

        vec2f const vertexAbsolutePosition = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointIndices(currentAbsoluteBCoords.TriangleElementIndex)[vertexOrdinal]);

        // The two vertices around the vertex we are on - seen in clockwise order
        int nextVertexOrdinal = (vertexOrdinal + 1) % 3;
        int prevVertexOrdinal = (vertexOrdinal + 2) % 3;

        // Determine orientation of our visit (CW vs CCW)
        //
        // Assumption is that trajectoryEndBarycentricCoords is *outside* triangle,
        // and we are at the last vertex possible in this triangle before leaving it
        //
        //   0    * b[0] > b[2]
        //   |\  /
        //   | \
        //   | *\
        //  2----1
        //      /
        //     *  b[0] < b[2]

        RotationDirectionType const orientation = (trajectoryEndBarycentricCoords[prevVertexOrdinal] <= trajectoryEndBarycentricCoords[nextVertexOrdinal])
            ? RotationDirectionType::CounterClockwise
            : RotationDirectionType::Clockwise;

        // Remember floor geometry of starting edge
        NpcFloorGeometryType const initialFloorGeometry = homeShip.GetTriangles().GetSubSpringNpcFloorGeometry(walkedEdge->TriangleElementIndex, walkedEdge->EdgeOrdinal);

        for (int iIter = 0; ; ++iIter)
        {
            LogNpcDebug("    NavigateVertex_Walking: iter=", iIter, " orientation=", orientation == RotationDirectionType::Clockwise ? "CW" : "CCW",
                " nCandidatesEasy=", floorCandidatesEasySlopeCount, " nCandidatesHard=", floorCandidatesHardSlopeCount, " hasBounceableFloor=", firstBounceableFloor.has_value() ? "T" : "F",
                " hasFirstTriangleInterior=", firstTriangleInterior.has_value() ? "T" : "F");

            assert(iIter < GameParameters::MaxSpringsPerPoint); // Detect and debug-break on infinite loops

            // Pre-conditions: we are at this vertex
            assert(currentAbsoluteBCoords.BCoords[vertexOrdinal] == 1.0f);
            assert(currentAbsoluteBCoords.BCoords[nextVertexOrdinal] == 0.0f);
            assert(currentAbsoluteBCoords.BCoords[prevVertexOrdinal] == 0.0f);

            LogNpcDebug("      Triangle=", currentAbsoluteBCoords.TriangleElementIndex, " Vertex=", vertexOrdinal, " BCoords=", currentAbsoluteBCoords.BCoords,
                " TrajectoryEndBarycentricCoords=", trajectoryEndBarycentricCoords);

            //
            // Check whether we are directed towards the *interior* of this triangle, if we don't
            // know yet which triangle we'd be going inside
            //

            if (!firstTriangleInterior.has_value()
                && trajectoryEndBarycentricCoords[prevVertexOrdinal] >= 0.0f
                && trajectoryEndBarycentricCoords[nextVertexOrdinal] >= 0.0f)
            {
                LogNpcDebug("      Trajectory extends inside triangle, remembering it");

                // Remember these absolute BCoords as we'll go there if we don't have any candidates nor we bounce on a floor
                firstTriangleInterior = currentAbsoluteBCoords;

                // Continue, so that we may find candidates at a lower slope
            }

            //
            // Find next edge that we cross
            //

            int const crossedEdgeOrdinal = (orientation == RotationDirectionType::CounterClockwise)
                ? vertexOrdinal // Next edge
                : (vertexOrdinal + 2) % 3; // Previous edge

            LogNpcDebug("      Next crossed edge: ", crossedEdgeOrdinal);

            //
            // Check whether we've gone too far around
            //

            auto const & nextVertexPos = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointIndices(currentAbsoluteBCoords.TriangleElementIndex)[nextVertexOrdinal]);
            auto const & prevVertexPos = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointIndices(currentAbsoluteBCoords.TriangleElementIndex)[prevVertexOrdinal]);
            if (prevVertexPos.x <= vertexAbsolutePosition.x && nextVertexPos.x > vertexAbsolutePosition.x)
            {
                LogNpcDebug("      Gone too far - may stop search here");
                break;
            }

            //
            // Check whether this new edge is floor
            //

            if (IsEdgeFloorToParticle(currentAbsoluteBCoords.TriangleElementIndex, crossedEdgeOrdinal, npc, npcParticleOrdinal, particles, homeShip))
            {
                //
                // Encountered floor
                //

                auto const crossedEdgeFloorGeometry = homeShip.GetTriangles().GetSubSpringNpcFloorGeometry(currentAbsoluteBCoords.TriangleElementIndex, crossedEdgeOrdinal);

                LogNpcDebug("        Crossed edge is floor (geometry:", int(crossedEdgeFloorGeometry), ")");

                //
                // Check whether it's a viable floor, i.e. whether its direction is:
                //    - x: in direction of movement (including close to 0, i.e. almost vertical)
                //    - y: lower than MaxHumanNpcWalkSinSlope
                //
                // Notes:
                //  - Here we check viability wrt *actual* (resultant physical) movement, rather than *intended* (walkdir) movement
                //  - Viability is based on actual gravity direction - not on *apparent* gravity
                //

                vec2f const crossedEdgeDir = homeShip.GetTriangles().GetSubSpringVector(
                    currentAbsoluteBCoords.TriangleElementIndex,
                    crossedEdgeOrdinal,
                    homeShip.GetPoints()).normalise();

                bool const isViable =
                    crossedEdgeDir.x < 0.0f
                    && (
                        (orientation == RotationDirectionType::CounterClockwise)
                        ? crossedEdgeDir.y <= GameParameters::MaxHumanNpcWalkSinSlope
                        : -crossedEdgeDir.y <= GameParameters::MaxHumanNpcWalkSinSlope
                        );

                LogNpcDebug("          Edge is ", isViable ? "viable" : "non-viable", " (edgeDir=", crossedEdgeDir, ")");

                if (isViable)
                {
                    //
                    // Viable floor - add to candidates
                    //

                    if (std::abs(crossedEdgeDir.y) < 0.98f) // Magic slope
                    {
                        floorCandidatesEasySlope[floorCandidatesEasySlopeCount++] = { currentAbsoluteBCoords, crossedEdgeOrdinal };

                        LogNpcDebug("          Added to easy candidates: new count=", floorCandidatesEasySlopeCount);
                    }
                    else
                    {
                        floorCandidatesHardSlope[floorCandidatesHardSlopeCount++] = { currentAbsoluteBCoords, crossedEdgeOrdinal };

                        LogNpcDebug("          Added to hard candidates: new count=", floorCandidatesHardSlopeCount);
                    }
                }
                else
                {
                    //
                    // Not viable - see if it's bounceable
                    //
                    // Bounceable: iff we hit it according to our current direction
                    //   - Note: all edges are in our direction until we've found a triangle interior
                    //

                    if (!firstTriangleInterior.has_value())
                    {
                        LogNpcDebug("          Edge is bounceable");

                        //
                        // Bounceable: store it so we may bounce on it in case there are no candidates
                        //
                        // Note: we give same-depth priority over other-depth, because:
                        //  - If no other walls - nor candidates - exist, we're ok with bouncing on S at --> _\
                        //  - But when given a choice with vertical - e.g. --> _\| - we prefer bouncing on vertical, for better physics
                        //      - Thus honoring semi-invisible nature of S
                        //
                        // Also: we want to enforce a concept of entering depth 1 areas from depth 2 areas, hence,
                        // if we're on S, we want to remember the _last_ of H/V we encounter (which would be the
                        // second at most, as there cannot be any third bounceable H/V if we're on S), as that is
                        // consistent with being "inside a depth 1 area"
                        //

                        bool doRememberBounceableFloor = false;

                        if (!firstBounceableFloor.has_value())
                        {
                            doRememberBounceableFloor = true;
                        }
                        else
                        {
                            auto const currentBounceableFloorGeometry = homeShip.GetTriangles().GetSubSpringNpcFloorGeometry(firstBounceableFloor->TriangleBCoords.TriangleElementIndex, firstBounceableFloor->EdgeOrdinal);
                            if ((NpcFloorGeometryDepth(currentBounceableFloorGeometry) != NpcFloorGeometryDepth(initialFloorGeometry) && NpcFloorGeometryDepth(crossedEdgeFloorGeometry) == NpcFloorGeometryDepth(initialFloorGeometry))
                                ||
                                (
                                    NpcFloorGeometryDepth(initialFloorGeometry) == NpcFloorGeometryDepthType::Depth2
                                    && NpcFloorGeometryDepth(currentBounceableFloorGeometry) == NpcFloorGeometryDepthType::Depth1
                                    && NpcFloorGeometryDepth(crossedEdgeFloorGeometry) == NpcFloorGeometryDepthType::Depth1
                                    ))
                            {
                                doRememberBounceableFloor = true;
                            }
                        }

                        if (doRememberBounceableFloor)
                        {
                            LogNpcDebug("            Remembering bounceable floor");
                            firstBounceableFloor = { currentAbsoluteBCoords, crossedEdgeOrdinal };
                        }
                    }
                }

                //
                // Check now if it's an impenetrable wall
                //
                // Impenetrable: iff HonV or VonH
                //  - We allow SonS to be penetrable, so that while we go down ladder and encounter ladder going up, we may go beyond it if there's a floor behind it
                //

                if ((crossedEdgeFloorGeometry == NpcFloorGeometryType::Depth1H && initialFloorGeometry == NpcFloorGeometryType::Depth1V)
                    || (crossedEdgeFloorGeometry == NpcFloorGeometryType::Depth1V && initialFloorGeometry == NpcFloorGeometryType::Depth1H))
                {
                    LogNpcDebug("          Impenetrable, stopping here");

                    // If this was viable, we'll choose it; if it was not viable, we'll bounce on it if it was in direction
                    //  - Note: if it's not in direction (i.e. if we have an interior), and if we have no candidates and no bounceables - then we will take the interior

                    break;
                }
            }

            //
            // Climb over edge
            //

            // Find opposite triangle
            auto const & oppositeTriangleInfo = homeShip.GetTriangles().GetOppositeTriangle(currentAbsoluteBCoords.TriangleElementIndex, crossedEdgeOrdinal);
            if (oppositeTriangleInfo.TriangleElementIndex == NoneElementIndex || homeShip.GetTriangles().IsDeleted(oppositeTriangleInfo.TriangleElementIndex))
            {
                //
                // Found a free region
                //

                LogNpcDebug("      No opposite triangle found, found free region");

                // If we've found this free region before anything else, become free; otherwise,
                // stop search now and proceed with what we've found
                if (floorCandidatesEasySlopeCount + floorCandidatesHardSlopeCount != 0
                    || firstBounceableFloor.has_value()
                    || firstTriangleInterior.has_value())
                {
                    LogNpcDebug("        Free region after other options, stopping here");

                    break;
                }
                else
                {
                    // Detected free

                    LogNpcDebug("        Free region before anything else, detected free");

                    return NavigateVertexOutcome::MakeBecomeFreeOutcome();
                }
            }

            LogNpcDebug("      Opposite triangle found: ", oppositeTriangleInfo.TriangleElementIndex);

            // See whether we've gone around
            if (oppositeTriangleInfo.TriangleElementIndex == npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)
            {
                // Time to stop

                LogNpcDebug("        Opposite triangle is self (done full round), stopping search");

                // We must have found a triangle into which we go
                assert(firstTriangleInterior.has_value());

                break;
            }

            //
            // Move to triangle
            //

            LogNpcDebug("      Continuing search from edge ", oppositeTriangleInfo.EdgeOrdinal, " of opposite triangle ", oppositeTriangleInfo.TriangleElementIndex);

            // Calculate new current barycentric coords (wrt opposite triangle - note that we haven't moved)
            bcoords3f newBarycentricCoords; // In new triangle
            newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 2) % 3] = 0.0f;
            newBarycentricCoords[oppositeTriangleInfo.EdgeOrdinal] = currentAbsoluteBCoords.BCoords[(crossedEdgeOrdinal + 1) % 3];
            newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 1) % 3] = currentAbsoluteBCoords.BCoords[crossedEdgeOrdinal];

            LogNpcDebug("        B-Coords: ", currentAbsoluteBCoords.BCoords, " -> ", newBarycentricCoords);

            assert(newBarycentricCoords.is_on_edge_or_internal());

            // Move to triangle and b-coords
            currentAbsoluteBCoords = AbsoluteTriangleBCoords(oppositeTriangleInfo.TriangleElementIndex, newBarycentricCoords);

            // New vertex: we know that coord of vertex opposite of crossed edge (i.e. vertex with ordinal crossed_edge+2) is 0.0
            if (newBarycentricCoords[oppositeTriangleInfo.EdgeOrdinal] == 0.0f)
            {
                // Between edge and edge+1
                vertexOrdinal = (oppositeTriangleInfo.EdgeOrdinal + 1) % 3;
            }
            else
            {
                // Between edge and edge-1
                assert(newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 1) % 3] == 0.0f);
                vertexOrdinal = oppositeTriangleInfo.EdgeOrdinal;
            }

            // The two vertices around the vertex we are on - seen in clockwise order
            nextVertexOrdinal = (vertexOrdinal + 1) % 3;
            prevVertexOrdinal = (vertexOrdinal + 2) % 3;

            //
            // Translate target bary coords - if we still need them
            //

            if (!firstTriangleInterior.has_value())
            {
                trajectoryEndBarycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinatesInsideEdge(
                    trajectoryEndAbsolutePosition,
                    oppositeTriangleInfo.TriangleElementIndex,
                    homeShip.GetPoints(),
                    oppositeTriangleInfo.EdgeOrdinal);

                LogNpcDebug("        New TrajEndB-Coords: ", trajectoryEndBarycentricCoords);
            }
        }

        //
        // Process results
        //

        if (floorCandidatesEasySlopeCount > 0 || floorCandidatesHardSlopeCount > 0)
        {
            // Only choose among hard ones if there are no easy ones

            AbsoluteTriangleBCoordsAndEdge chosenFloor;
            if (floorCandidatesEasySlopeCount > 0)
            {
                size_t chosenCandidateIndex = (floorCandidatesEasySlopeCount == 1)
                    ? 0
                    : GameRandomEngine::GetInstance().Choose(floorCandidatesEasySlopeCount);
                chosenFloor = floorCandidatesEasySlope[chosenCandidateIndex];

                LogNpcDebug("    Chosen easy candidate ", chosenCandidateIndex, " (", chosenFloor.TriangleBCoords.TriangleElementIndex, ":", chosenFloor.TriangleBCoords.BCoords,
                    ", ", chosenFloor.EdgeOrdinal, ") out of ", floorCandidatesEasySlopeCount);
            }
            else
            {
                size_t chosenCandidateIndex = (floorCandidatesHardSlopeCount == 1)
                    ? 0
                    : GameRandomEngine::GetInstance().Choose(floorCandidatesHardSlopeCount);
                chosenFloor = floorCandidatesHardSlope[chosenCandidateIndex];

                LogNpcDebug("    Chosen hard candidate ", chosenCandidateIndex, " (", chosenFloor.TriangleBCoords.TriangleElementIndex, ":", chosenFloor.TriangleBCoords.BCoords,
                    ", ", chosenFloor.EdgeOrdinal, ") out of ", floorCandidatesEasySlopeCount);
            }

            return NavigateVertexOutcome::MakeContinueAlongFloorOutcome(
                chosenFloor.TriangleBCoords,
                chosenFloor.EdgeOrdinal);
        }
        else if (firstBounceableFloor.has_value())
        {
            // Impact on this floor

            LogNpcDebug("    Impact on floor ", firstBounceableFloor->TriangleBCoords.TriangleElementIndex, ":", firstBounceableFloor->EdgeOrdinal);

            return NavigateVertexOutcome::MakeImpactOnFloorOutcome(
                firstBounceableFloor->TriangleBCoords,
                firstBounceableFloor->EdgeOrdinal);
        }
        else
        {
            // Inside triangle

            assert(firstTriangleInterior.has_value());

            LogNpcDebug("    Going inside triangle ", *firstTriangleInterior);

            return NavigateVertexOutcome::MakeContinueToInteriorOutcome(*firstTriangleInterior);
        }
    }
    else
    {
        //
        // Navigate this vertex along the trajectory direction, until any of these:
        // - We are directed inside a triangle
        // - We have encountered a floor
        // - We have detected an impact against a floor
        // - We become free
        //

        AbsoluteTriangleBCoords currentAbsoluteBCoords = npcParticle.ConstrainedState->CurrentBCoords;

        for (int iIter = 0; ; ++iIter)
        {
            LogNpcDebug("    NavigateVertex_NonWalking: iter=", iIter);

            assert(iIter < 32); // Detect and debug-break on infinite loops

            // The two vertices around the vertex we are on - seen in clockwise order
            int const nextVertexOrdinal = (vertexOrdinal + 1) % 3;
            int const prevVertexOrdinal = (vertexOrdinal + 2) % 3;

            // Pre-conditions: we are at this vertex
            assert(currentAbsoluteBCoords.BCoords[vertexOrdinal] == 1.0f);
            assert(currentAbsoluteBCoords.BCoords[nextVertexOrdinal] == 0.0f);
            assert(currentAbsoluteBCoords.BCoords[prevVertexOrdinal] == 0.0f);

            LogNpcDebug("      Triangle=", currentAbsoluteBCoords.TriangleElementIndex, " Vertex=", vertexOrdinal, " TrajectoryEndBarycentricCoords=", trajectoryEndBarycentricCoords);

            //
            // Check whether we are directed towards the *interior* of this triangle - including its edges
            //

            if (trajectoryEndBarycentricCoords[prevVertexOrdinal] >= 0.0f
                && trajectoryEndBarycentricCoords[nextVertexOrdinal] >= 0.0f)
            {
                //
                // We go inside (or on) this triangle - stop where we are, we'll then check trajectory in new situation
                //

                LogNpcDebug("      Going inside triangle ", currentAbsoluteBCoords);

                return NavigateVertexOutcome::MakeContinueToInteriorOutcome(currentAbsoluteBCoords);
            }

            //
            // Find next edge that we intersect at this vertex
            //

            RotationDirectionType const orientation = (trajectoryEndBarycentricCoords[prevVertexOrdinal] <= trajectoryEndBarycentricCoords[nextVertexOrdinal])
                ? RotationDirectionType::CounterClockwise
                : RotationDirectionType::Clockwise;

            int const crossedEdgeOrdinal = (orientation == RotationDirectionType::CounterClockwise)
                ? vertexOrdinal // Next edge
                : (vertexOrdinal + 2) % 3; // Prev edge

            LogNpcDebug("      Trajectory crosses triangle: crossedEdgeOrdinal=", crossedEdgeOrdinal);

            //
            // Check whether this new edge is floor
            //

            if (IsEdgeFloorToParticle(currentAbsoluteBCoords.TriangleElementIndex, crossedEdgeOrdinal, npc, npcParticleOrdinal, particles, homeShip))
            {
                //
                // Encountered floor
                //

                LogNpcDebug("      Crossed edge is floor");

                // Determine viability of floor: check angle between desired (original) trajectory and edge

                vec2f const floorEdgeDir = homeShip.GetTriangles().GetSubSpringVector(
                    currentAbsoluteBCoords.TriangleElementIndex,
                    crossedEdgeOrdinal,
                    homeShip.GetPoints()).normalise();
                vec2f const floorEdgeNormal = floorEdgeDir.to_perpendicular();

                float const trajProjOntoEdgeNormal = trajectory.normalise().dot(floorEdgeNormal);
                bool const isViable = (trajProjOntoEdgeNormal <= 0.71f); // PI/4+

                LogNpcDebug("        Edge is ", isViable ? "viable" : "non-viable", " (floorEdgeNormal=", floorEdgeNormal, ")");

                if (isViable)
                {
                    return NavigateVertexOutcome::MakeContinueAlongFloorOutcome(currentAbsoluteBCoords, crossedEdgeOrdinal);
                }
                else
                {
                    return NavigateVertexOutcome::MakeImpactOnFloorOutcome(currentAbsoluteBCoords, crossedEdgeOrdinal);
                }
            }

            //
            // Not floor, climb over edge
            //

            // Find opposite triangle
            auto const & oppositeTriangleInfo = homeShip.GetTriangles().GetOppositeTriangle(currentAbsoluteBCoords.TriangleElementIndex, crossedEdgeOrdinal);
            if (oppositeTriangleInfo.TriangleElementIndex == NoneElementIndex || homeShip.GetTriangles().IsDeleted(oppositeTriangleInfo.TriangleElementIndex))
            {
                //
                // Become free
                //

                LogNpcDebug("      No opposite triangle found, becoming free");

                return NavigateVertexOutcome::MakeBecomeFreeOutcome();
            }

            //
            // Move to triangle
            //

            LogNpcDebug("      Moving to edge ", oppositeTriangleInfo.EdgeOrdinal, " of opposite triangle ", oppositeTriangleInfo.TriangleElementIndex);

            // Calculate new current barycentric coords (wrt opposite triangle - note that we haven't moved)
            bcoords3f newBarycentricCoords; // In new triangle
            newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 2) % 3] = 0.0f;
            newBarycentricCoords[oppositeTriangleInfo.EdgeOrdinal] = currentAbsoluteBCoords.BCoords[(crossedEdgeOrdinal + 1) % 3];
            newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 1) % 3] = currentAbsoluteBCoords.BCoords[crossedEdgeOrdinal];

            LogNpcDebug("      B-Coords: ", currentAbsoluteBCoords.BCoords, " -> ", newBarycentricCoords);

            assert(newBarycentricCoords.is_on_edge_or_internal());

            // Move to triangle and b-coords
            currentAbsoluteBCoords = AbsoluteTriangleBCoords(oppositeTriangleInfo.TriangleElementIndex, newBarycentricCoords);

            // New vertex: we know that coord of vertex opposite of crossed edge (i.e. vertex with ordinal crossed_edge+2) is 0.0
            if (newBarycentricCoords[oppositeTriangleInfo.EdgeOrdinal] == 0.0f)
            {
                // Between edge and edge+1
                vertexOrdinal = (oppositeTriangleInfo.EdgeOrdinal + 1) % 3;
            }
            else
            {
                // Between edge and edge-1
                assert(newBarycentricCoords[(oppositeTriangleInfo.EdgeOrdinal + 1) % 3] == 0.0f);
                vertexOrdinal = oppositeTriangleInfo.EdgeOrdinal;
            }

            //
            // Translate target bary coords
            //

            trajectoryEndBarycentricCoords = homeShip.GetTriangles().ToBarycentricCoordinatesInsideEdge(
                trajectoryEndAbsolutePosition,
                oppositeTriangleInfo.TriangleElementIndex,
                homeShip.GetPoints(),
                oppositeTriangleInfo.EdgeOrdinal);

            LogNpcDebug("      TrajEndB-Coords: ", trajectoryEndBarycentricCoords);
        }
    }
}

void Npcs::BounceConstrainedNpcParticle(
    StateType & npc,
    int npcParticleOrdinal,
    vec2f const & trajectory,
    bool hasMovedInStep,
    vec2f const & bouncePosition,
    int bounceEdgeOrdinal,
    vec2f const meshVelocity,
    float dt,
    Ship & homeShip,
    NpcParticles & particles,
    float currentSimulationTime,
    GameParameters const & gameParameters) const
{
    auto & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];
    assert(npcParticle.ConstrainedState.has_value());

    // Calculate apparent velocity:
    //  - If we have made any movement in this step, then this is an actual collision
    //    after having traveled part of the (segment) trajectory; we calculate then the
    //    collision velocity by means of the (segment) trajectory we wanted to travel during
    //    the (segment) dt
    //      - Note: here we take into account new forces that sprung up just before this iteration
    //  - Otherwise, the previous iteration stopped just short of the bounce, either heads-on
    //    (in which case we have MRVelocity) or because we are resting here (in which case we
    //    have no MRVelocity); we thus infer whether this is a real collision or not by means
    //    of the particle's mesh-relative velocity
    //      - Note: at this moment the particle's mesh-relative velocity does _not_ include
    //        yet an eventual mesh displacement that took place between the previous step and this
    //        step, nor any new forces that sprung up just before this iteration
    //          - We assume new forces that sprung up just before this iteration have no effect on
    //            bounce, because particle is already on floor
    //
    // If we end up with zero apparent velocity, it means we're just resting on this edge,
    // at this position

    vec2f const floorEdgeDir =
        homeShip.GetTriangles().GetSubSpringVector(
            npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex,
            bounceEdgeOrdinal,
            homeShip.GetPoints())
        .normalise();
    vec2f const floorEdgeNormal = floorEdgeDir.to_perpendicular(); // Outside of triangle

    vec2f const apparentParticleVelocity = hasMovedInStep
        ? trajectory / dt
        : npcParticle.ConstrainedState->MeshRelativeVelocity;
    float const apparentParticleVelocityAlongNormal = apparentParticleVelocity.dot(floorEdgeNormal); // Should be positive

    LogNpcDebug("      BounceConstrainedNpcParticle: apparentParticleVelocity=", apparentParticleVelocity, " (hasMovedInStep=", hasMovedInStep,
        " meshRelativeVelocity=", npcParticle.ConstrainedState->MeshRelativeVelocity, ")");

    if (apparentParticleVelocityAlongNormal > 0.0f)
    {
        // Decompose apparent particle velocity into normal and tangential
        vec2f const normalVelocity = floorEdgeNormal * apparentParticleVelocityAlongNormal;
        vec2f const tangentialVelocity = apparentParticleVelocity - normalVelocity;

        // Get edge's material (by taking arbitrarily the material of one of its endpoints)
        auto const & meshMaterial = homeShip.GetPoints().GetStructuralMaterial(
            homeShip.GetTriangles().GetPointIndices(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)[bounceEdgeOrdinal]);

        // Calculate normal reponse: Vn' = -e*Vn (e = elasticity, [0.0 - 1.0])
        float const elasticityCoefficient = Clamp(
            (particles.GetMaterial(npcParticle.ParticleIndex).ElasticityCoefficient + meshMaterial.ElasticityCoefficient) / 2.0f * gameParameters.ElasticityAdjustment,
            0.0f, 1.0f);
        vec2f const normalResponse =
            -normalVelocity
            * elasticityCoefficient;

        // Calculate tangential response: Vt' = a*Vt (a = (1.0-friction), [0.0 - 1.0])
        float const materialFrictionCoefficient = (particles.GetMaterial(npcParticle.ParticleIndex).KineticFrictionCoefficient + meshMaterial.KineticFrictionCoefficient) / 2.0f;
        vec2f const tangentialResponse =
            tangentialVelocity
            * std::max(0.0f, 1.0f - materialFrictionCoefficient * particles.GetKineticFrictionTotalAdjustment(npcParticle.ParticleIndex));

        // Calculate whole response (which, given that we've been working in *apparent* space (we've calc'd the collision response to *trajectory* which is apparent displacement)),
        // is a relative velocity (relative to mesh)
        vec2f const resultantRelativeVelocity = (normalResponse + tangentialResponse);

        // Do not damp velocity if we're trying to maintain equilibrium
        vec2f const resultantAbsoluteVelocity =
            resultantRelativeVelocity * ((npc.Kind != NpcKindType::Human || npc.KindSpecificState.HumanNpcState.EquilibriumTorque == 0.0f) ? mGlobalDampingFactor : 1.0f)
            + meshVelocity;

        LogNpcDebug("        Impact: trajectory=", trajectory, " apparentParticleVelocity=", apparentParticleVelocity, " nr=", normalResponse, " tr=", tangentialResponse, " rr=", resultantRelativeVelocity);

        //
        // Set position and velocity
        //

        particles.SetPosition(npcParticle.ParticleIndex, bouncePosition);

        particles.SetVelocity(npcParticle.ParticleIndex, resultantAbsoluteVelocity);
        npcParticle.ConstrainedState->MeshRelativeVelocity = resultantRelativeVelocity;

        //
        // Impart force against edge - but only if hit was "substantial", so
        // we don't end up in infinite loops bouncing against a soft mesh
        //

        if (apparentParticleVelocityAlongNormal > 2.0f) // Magic number
        {
            // Calculate impact force: Dp/Dt
            //
            //  Dp = v2*m - v1*m (using _relative_ velocities)
            //  Dt = duration of impact - and here we are conservative, taking a whole simulation dt
            //       ...but then it's too much
            vec2f const impartedForce =
                (normalVelocity - normalResponse) // normalVelocity is directed outside of triangle
                * mParticles.GetMass(npcParticle.ParticleIndex)
                / GameParameters::SimulationStepTimeDuration<float>
                * 0.2f; // Magic damper

            // Divide among two vertices

            int const edgeVertex1Ordinal = bounceEdgeOrdinal;
            ElementIndex const edgeVertex1PointIndex = homeShip.GetTriangles().GetPointIndices(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)[edgeVertex1Ordinal];
            float const vertex1InterpCoeff = npcParticle.ConstrainedState->CurrentBCoords.BCoords[edgeVertex1Ordinal];
            homeShip.GetPoints().AddStaticForce(edgeVertex1PointIndex, impartedForce * vertex1InterpCoeff);

            int const edgeVertex2Ordinal = (bounceEdgeOrdinal + 1) % 3;
            ElementIndex const edgeVertex2PointIndex = homeShip.GetTriangles().GetPointIndices(npcParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)[edgeVertex2Ordinal];
            float const vertex2InterpCoeff = npcParticle.ConstrainedState->CurrentBCoords.BCoords[edgeVertex2Ordinal];
            homeShip.GetPoints().AddStaticForce(edgeVertex2PointIndex, impartedForce * vertex2InterpCoeff);
        }

        //
        // Publish impact
        //

        OnImpact(
            npc,
            npcParticleOrdinal,
            normalVelocity,
            floorEdgeNormal,
            currentSimulationTime);
    }
    else
    {
        LogNpcDebug("        Not an impact");

        //
        // Set position and velocity
        //

        particles.SetPosition(npcParticle.ParticleIndex, bouncePosition);

        particles.SetVelocity(npcParticle.ParticleIndex, meshVelocity);
        npcParticle.ConstrainedState->MeshRelativeVelocity = vec2f::zero();
    }
}

void Npcs::OnImpact(
    StateType & npc,
    int npcParticleOrdinal,
    vec2f const & normalResponse,
    vec2f const & bounceEdgeNormal, // Pointing outside of triangle
    float currentSimulationTime) const
{
    LogNpcDebug("    OnImpact(mag=", normalResponse.length(), ", bounceEdgeNormal=", bounceEdgeNormal, ")");

    // Human state machine
    if (npc.Kind == NpcKindType::Human)
    {
        OnHumanImpact(
            npc,
            npcParticleOrdinal,
            normalResponse,
            bounceEdgeNormal,
            currentSimulationTime);
    }
}

void Npcs::MaintainOverLand(
    StateType & npc,
    int npcParticleOrdinal,
    Ship const & homeShip,
    GameParameters const & gameParameters)
{
    ElementIndex const p = npc.ParticleMesh.Particles[npcParticleOrdinal].ParticleIndex;
    auto const & pos = mParticles.GetPosition(p);

    // Check if particle is below the sea floor
    //
    // At this moment the particle is guaranteed to be inside world boundaries
    OceanFloor const & oceanFloor = mParentWorld.GetOceanFloor();
    auto const [isUnderneathFloor, oceanFloorHeight, integralIndex] = oceanFloor.GetHeightIfUnderneathAt(pos.x, pos.y);
    if (isUnderneathFloor)
    {
        // Collision!

        //
        // Calculate post-bounce velocity
        //

        vec2f const particleVelocity = mParticles.GetVelocity(p);

        // Calculate sea floor anti-normal
        // (positive points down)
        vec2f const seaFloorAntiNormal = -oceanFloor.GetNormalAt(integralIndex);

        // Calculate the component of the particle's velocity along the anti-normal,
        // i.e. towards the interior of the floor...
        float const particleVelocityAlongAntiNormal = particleVelocity.dot(seaFloorAntiNormal);

        // ...if negative, it's already pointing outside the floor, hence we leave it as-is
        if (particleVelocityAlongAntiNormal > 0.0f)
        {
            // Decompose particle velocity into normal and tangential
            vec2f const normalVelocity = seaFloorAntiNormal * particleVelocityAlongAntiNormal;
            vec2f const tangentialVelocity = particleVelocity - normalVelocity;

            // Calculate normal reponse: Vn' = -e*Vn (e = elasticity, [0.0 - 1.0])
            float const elasticityFactor = Clamp(
                (mParticles.GetMaterial(p).ElasticityCoefficient + gameParameters.OceanFloorElasticityCoefficient) / 2.0f * gameParameters.ElasticityAdjustment,
                0.0f, 1.0f);
            vec2f const normalResponse =
                normalVelocity
                * -elasticityFactor;

            // Calculate tangential response: Vt' = a*Vt (a = (1.0-friction), [0.0 - 1.0])
            vec2f const tangentialResponse =
                tangentialVelocity
                * std::max(0.0f, 1.0f - mParticles.GetMaterial(p).KineticFrictionCoefficient * mParticles.GetKineticFrictionTotalAdjustment(p)); // For lazyness

            //
            // Impart final position and velocity
            //

            // Move point back along its velocity direction (i.e. towards where it was in the previous step,
            // which is guaranteed to be more towards the outside)
            vec2f deltaPos = particleVelocity * gameParameters.SimulationStepTimeDuration<float>;
            mParticles.SetPosition(
                p,
                pos - deltaPos);

            // Set velocity to resultant collision velocity
            mParticles.SetVelocity(
                p,
                (normalResponse + tangentialResponse));
        }

        // Become free - so to avoid bouncing back and forth
        TransitionParticleToFreeState(npc, npcParticleOrdinal, homeShip);
    }
}

}