/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-10-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "../Physics.h"

#include "../StockColors.h"

#include <GameCore/Colors.h>
#include <GameCore/GameGeometry.h>

#ifdef _MSC_VER
#pragma warning(disable : 4324) // std::optional of StateType gets padded because of alignment requirements
#endif

namespace Physics {

float constexpr ParticleSize = 0.30f; // For rendering, mostly - given that particles have zero dimensions

/*
Main principles:
    - Global damping: when constrained, we only apply it to velocity *relative* to the mesh ("air moves with the ship")
*/

void Npcs::Update(
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    //
    // Update parameters
    //

    if (gameParameters.GlobalDampingAdjustment != mCurrentGlobalDampingAdjustment)
    {
        mCurrentGlobalDampingAdjustment = gameParameters.GlobalDampingAdjustment;

        RecalculateGlobalDampingFactor();
    }

    if (gameParameters.NpcSizeMultiplier != mCurrentSizeMultiplier
        || gameParameters.NpcSpringReductionFractionAdjustment != mCurrentSpringReductionFractionAdjustment
        || gameParameters.NpcSpringDampingCoefficientAdjustment != mCurrentSpringDampingCoefficientAdjustment
#ifdef IN_BARYLAB
        || gameParameters.MassAdjustment != mCurrentMassAdjustment
        || gameParameters.BuoyancyAdjustment != mCurrentBuoyancyAdjustment
        || gameParameters.GravityAdjustment != mCurrentGravityAdjustment
#endif
        )
    {
        mCurrentSizeMultiplier = gameParameters.NpcSizeMultiplier;
        mCurrentSpringReductionFractionAdjustment = gameParameters.NpcSpringReductionFractionAdjustment;
        mCurrentSpringDampingCoefficientAdjustment = gameParameters.NpcSpringDampingCoefficientAdjustment;
#ifdef IN_BARYLAB
        mCurrentMassAdjustment = gameParameters.MassAdjustment;
        mCurrentBuoyancyAdjustment = gameParameters.BuoyancyAdjustment;
        mCurrentGravityAdjustment = gameParameters.GravityAdjustment;
#endif

        RecalculateSizeAndMassParameters();
    }

    if (gameParameters.StaticFrictionAdjustment != mCurrentStaticFrictionAdjustment
        || gameParameters.KineticFrictionAdjustment != mCurrentKineticFrictionAdjustment
        || gameParameters.NpcFrictionAdjustment != mCurrentNpcFrictionAdjustment)
    {
        mCurrentStaticFrictionAdjustment = gameParameters.StaticFrictionAdjustment;
        mCurrentKineticFrictionAdjustment = gameParameters.KineticFrictionAdjustment;
        mCurrentNpcFrictionAdjustment = gameParameters.NpcFrictionAdjustment;

        RecalculateFrictionTotalAdjustments();
    }

    if (gameParameters.HumanNpcWalkingSpeedAdjustment != mCurrentHumanNpcWalkingSpeedAdjustment)
    {
        mCurrentHumanNpcWalkingSpeedAdjustment = gameParameters.HumanNpcWalkingSpeedAdjustment;
    }

    //
    // Update NPCs' state
    //

    // Advance the current simulation sequence
    ++mCurrentSimulationSequenceNumber;

    UpdateNpcs(currentSimulationTime, stormParameters, gameParameters);
}

void Npcs::UpdateEnd()
{
    UpdateNpcsEnd();
}

void Npcs::Upload(Render::RenderContext & renderContext) const
{
#ifdef IN_BARYLAB
    if (renderContext.GetNpcRenderMode() == NpcRenderModeType::Physical)
    {
        renderContext.UploadNpcParticlesStart();
        renderContext.UploadNpcSpringsStart();

        for (ShipId shipId = 0; shipId < mShips.size(); ++shipId)
        {
            if (mShips[shipId].has_value())
            {
                Render::ShipRenderContext & shipRenderContext = renderContext.GetShipRenderContext(shipId);

                for (NpcId const npcId : mShips[shipId]->Npcs)
                {
                    assert(mStateBuffer[npcId].has_value());
                    auto const & state = *mStateBuffer[npcId];

                    auto const planeId = (state.CurrentRegime == StateType::RegimeType::BeingPlaced)
                        ? mShips[shipId]->HomeShip.GetMaxPlaneId()
                        : state.CurrentPlaneId;

                    // Particles
                    for (auto const & particle: state.ParticleMesh.Particles)
                    {
                        shipRenderContext.UploadNpcParticle(
                            planeId,
                            mParticles.GetPosition(particle.ParticleIndex),
                            mParticles.GetRenderColor(particle.ParticleIndex),
                            1.0f,
                            particle.ParticleIndex == mCurrentlySelectedParticle ? rgbaColor(0x80, 0, 0, 0xff) : rgbaColor::zero());
                    }

                    // Springs
                    for (auto const & spring : state.ParticleMesh.Springs)
                    {
                        renderContext.UploadNpcSpring(
                            planeId,
                            mParticles.GetPosition(spring.EndpointAIndex),
                            mParticles.GetPosition(spring.EndpointBIndex),
                            rgbaColor(0x4a, 0x4a, 0x4a, 0xff));
                    }
                }
            }
        }

        renderContext.UploadNpcSpringsEnd();
        renderContext.UploadNpcParticlesEnd();

        return;
    }

    assert(renderContext.GetNpcRenderMode() != NpcRenderModeType::Physical);

#endif

    for (ShipId shipId = 0; shipId < mShips.size(); ++shipId)
    {
        if (mShips[shipId].has_value())
        {
            auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

            shipRenderContext.UploadNpcsStart(
                mShips[shipId]->TotalNpcStats.FurnitureNpcCount			// Furniture: one single quad
                + mShips[shipId]->TotalNpcStats.HumanNpcCount * (6 + 2)); // Human: max 8 quads (limbs)

            for (NpcId const npcId : mShips[shipId]->Npcs)
            {
                assert(mStateBuffer[npcId].has_value());
                auto const & state = *mStateBuffer[npcId];

                RenderNpc(
                    state,
                    renderContext,
                    shipRenderContext);
            }

            shipRenderContext.UploadNpcsEnd();
        }
    }

#ifdef IN_BARYLAB

    //
    // Particle trajectories
    //

    renderContext.UploadParticleTrajectoriesStart();

    if (mCurrentParticleTrajectoryNotification)
    {
        renderContext.UploadParticleTrajectory(
            mParticles.GetPosition(mCurrentParticleTrajectoryNotification->ParticleIndex),
            mCurrentParticleTrajectoryNotification->TargetPosition,
            rgbaColor(0xc0, 0xc0, 0xc0, 0xff));
    }

    if (mCurrentParticleTrajectory)
    {
        renderContext.UploadParticleTrajectory(
            mParticles.GetPosition(mCurrentParticleTrajectory->ParticleIndex),
            mCurrentParticleTrajectory->TargetPosition,
            rgbaColor(0x99, 0x99, 0x99, 0xff));
    }

    renderContext.UploadParticleTrajectoriesEnd();

#endif
}

void Npcs::UploadFlames(
    ShipId shipId,
    Render::ShipRenderContext & shipRenderContext) const
{
    size_t const s = static_cast<size_t>(shipId);

    // We know about this ship
    assert(s < mShips.size());
    assert(mShips[s].has_value());

    for (auto const burningNpcId : mShips[s]->BurningNpcs)
    {
        assert(mStateBuffer[burningNpcId].has_value());

        auto const & npc = *mStateBuffer[burningNpcId];

        // It's burning
        assert(npc.CombustionState.has_value());

        vec2f position;
        if (npc.Kind == NpcKindType::Human)
        {
            // Head
            assert(npc.ParticleMesh.Particles.size() == 2);
            position = mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex);
        }
        else
        {
            // Center
            position = vec2f::zero();
            for (auto const & p : npc.ParticleMesh.Particles)
            {
                position += mParticles.GetPosition(p.ParticleIndex);
            }
            assert(npc.ParticleMesh.Particles.size() > 0);
            position /= static_cast<float>(npc.ParticleMesh.Particles.size());
        }

        shipRenderContext.UploadNpcFlame(
            npc.CurrentPlaneId,
            position,
            npc.CombustionState->FlameVector,
            npc.CombustionState->FlameWindRotationAngle,
            npc.CombustionProgress * mCurrentSizeMultiplier, // Scale
            (npc.RandomNormalizedUniformSeed + 1.0f) / 2.0f);
    }
}

///////////////////////////////

bool Npcs::HasNpcs() const
{
    // Working NPCs only

    return std::any_of(
        mShips.cbegin(),
        mShips.cend(),
        [](auto const & ship)
        {
            if (ship.has_value())
            {
                return ship->WorkingNpcStats.FurnitureNpcCount > 0
                    || ship->WorkingNpcStats.HumanNpcCount > 0;
            }
            else
            {
                return false;
            }
        });
}

bool Npcs::HasNpc(NpcId npcId) const
{
    // Working NPC only

    return mStateBuffer[npcId].has_value()
        && mStateBuffer[npcId]->CurrentRegime != StateType::RegimeType::BeingRemoved;
}

Geometry::AABB Npcs::GetNpcAABB(NpcId npcId) const
{
    assert(mStateBuffer[npcId].has_value());
    assert(mStateBuffer[npcId]->CurrentRegime != StateType::RegimeType::BeingRemoved);

    Geometry::AABB aabb;
    for (auto const & particle : mStateBuffer[npcId]->ParticleMesh.Particles)
    {
        aabb.ExtendTo(mParticles.GetPosition(particle.ParticleIndex));
    }

    return aabb;
}

///////////////////////////////

void Npcs::OnShipAdded(Ship & ship)
{
    size_t const s = static_cast<size_t>(ship.GetId());

    // Make room for ship
    if (s >= mShips.size())
    {
        mShips.resize(s + 1);
    }

    // We do not know about this ship yet
    assert(!mShips[s].has_value());

    // Initialize NPC Ship
    mShips[s].emplace(ship);
}

void Npcs::OnShipRemoved(ShipId shipId)
{
    size_t const s = static_cast<size_t>(shipId);

    // We know about this ship
    assert(s < mShips.size());
    assert(mShips[s].has_value());

    //
    // Handle destruction of all NPCs of this NPC ship
    //

    bool humanNpcStatsUpdated = false;

    for (auto const npcId : mShips[s]->Npcs)
    {
        assert(mStateBuffer[npcId].has_value());

        if (mStateBuffer[npcId]->CurrentRegime == StateType::RegimeType::BeingRemoved)
        {
            //
            // Remove from deferred NPCs
            //

            auto deferredRemovalNpcIt = std::find(mDeferredRemovalNpcs.begin(), mDeferredRemovalNpcs.end(), npcId);
            assert(deferredRemovalNpcIt != mDeferredRemovalNpcs.end());
            mDeferredRemovalNpcs.erase(deferredRemovalNpcIt);

            //
            // Update ship stats
            //

            mShips[s]->TotalNpcStats.Remove(*mStateBuffer[npcId]);

            // Not burning
            assert(std::find(mShips[s]->BurningNpcs.cbegin(), mShips[s]->BurningNpcs.cend(), npcId) == mShips[s]->BurningNpcs.cend());

            // Not selected
            assert(mCurrentlySelectedNpc != npcId);
        }
        else
        {
            // Not in deferred NPCs
            assert(std::find(mDeferredRemovalNpcs.cbegin(), mDeferredRemovalNpcs.cend(), npcId) == mDeferredRemovalNpcs.cend());

            //
            // Update ship stats
            //

            mShips[s]->WorkingNpcStats.Remove(*mStateBuffer[npcId]);
            mShips[s]->TotalNpcStats.Remove(*mStateBuffer[npcId]);

            if (mStateBuffer[npcId]->Kind == NpcKindType::Human)
            {
                if (mStateBuffer[npcId]->CurrentRegime == StateType::RegimeType::Constrained)
                {
                    assert(mConstrainedRegimeHumanNpcCount > 0);
                    --mConstrainedRegimeHumanNpcCount;
                    humanNpcStatsUpdated = true;
                }
                else if (mStateBuffer[npcId]->CurrentRegime == StateType::RegimeType::Free)
                {
                    assert(mFreeRegimeHumanNpcCount > 0);
                    --mFreeRegimeHumanNpcCount;
                    humanNpcStatsUpdated = true;
                }
            }

            //
            // Remove from burning set, if there
            //

            auto burningNpcIt = std::find(mShips[s]->BurningNpcs.begin(), mShips[s]->BurningNpcs.end(), npcId);
            if (burningNpcIt != mShips[s]->BurningNpcs.end())
            {
                mShips[s]->BurningNpcs.erase(burningNpcIt);
            }

            //
            // Deselect, if selected
            //

            if (mCurrentlySelectedNpc == npcId)
            {
                mCurrentlySelectedNpc.reset();
                PublishSelection();
            }
        }
    }

    PublishCount();

    if (humanNpcStatsUpdated)
    {
        PublishHumanNpcStats();
    }

    //
    // Destroy NPC ship
    //

    mShips[s].reset();
}

void Npcs::OnShipConnectivityChanged(ShipId shipId)
{
    //
    // The connected component IDs of the ship have changed; do the following:
    //  - Re-assign constrained NPCs to the (possibly new) PlaneId and ConnectedComponentID,
    //    via the primary particle's triangle;
    //  - Transition to free those constrained non-primaries that are now severed from primary
    //    (i.e. whose current (real) conn comp ID is different than current (real) conn comp ID of primary);
    //  - Assign (possibly new) MaxPlaneId/ConnectedComponentID to each free NPC.
    //

    size_t const s = static_cast<size_t>(shipId);

    // We know about this ship
    assert(s < mShips.size());
    assert(mShips[s].has_value());

    auto const & homeShip = mShips[s]->HomeShip;

    for (auto const npcId : mShips[s]->Npcs)
    {
        assert(mStateBuffer[npcId].has_value());
        auto & npcState = *mStateBuffer[npcId];

        if (npcState.CurrentRegime != StateType::RegimeType::BeingPlaced
            && npcState.CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            assert(npcState.ParticleMesh.Particles.size() > 0);
            auto const & primaryParticle = npcState.ParticleMesh.Particles[0];
            if (primaryParticle.ConstrainedState.has_value())
            {
                // NPC is constrained
                assert(npcState.CurrentRegime == StateType::RegimeType::Constrained);

                // Assign NPC's plane/ccid to the primary's
                auto const primaryTriangleRepresentativePoint = homeShip.GetTriangles().GetPointAIndex(primaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex);
                npcState.CurrentPlaneId = homeShip.GetPoints().GetPlaneId(primaryTriangleRepresentativePoint);
                npcState.CurrentConnectedComponentId = homeShip.GetPoints().GetConnectedComponentId(primaryTriangleRepresentativePoint);

                // Now visit all constained secondaries and transition to free those that have been severed from primary
                for (size_t p = 1; p < npcState.ParticleMesh.Particles.size(); ++p)
                {
                    auto & secondaryParticle = npcState.ParticleMesh.Particles[p];
                    if (secondaryParticle.ConstrainedState.has_value())
                    {
                        auto const secondaryTriangleRepresentativePoint = homeShip.GetTriangles().GetPointAIndex(secondaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex);
                        if (homeShip.GetPoints().GetConnectedComponentId(secondaryTriangleRepresentativePoint) != npcState.CurrentConnectedComponentId)
                        {
                            TransitionParticleToFreeState(npcState, static_cast<int>(p), homeShip);
                        }
                    }
                }
            }
            else
            {
                // NPC is free
                assert(npcState.CurrentRegime == StateType::RegimeType::Free);

                // Re-assign plane ID to this NPC
                npcState.CurrentPlaneId = homeShip.GetMaxPlaneId();
                assert(!npcState.CurrentConnectedComponentId.has_value());
            }
        }
    }
}

NpcKindType Npcs::GetNpcKind(NpcId id)
{
    assert(mStateBuffer[id].has_value());
    return mStateBuffer[id]->Kind;
}

std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> Npcs::BeginPlaceNewFurnitureNpc(
    std::optional<NpcSubKindIdType> subKind,
    vec2f const & worldCoordinates,
    bool doMoveWholeMesh,
    float currentSimulationTime)
{
    int constexpr ParticleOrdinal = 0; // We use primary for furniture

    //
    // Check if there are too many NPCs
    //

    if (CalculateTotalNpcCount() >= mMaxNpcs)
    {
        return { std::nullopt, NpcCreationFailureReasonType::TooManyNpcs };
    }

    //
    // Create NPC
    //

    if (!subKind.has_value())
    {
        subKind = ChooseSubKind(NpcKindType::Furniture, std::nullopt);
    }

    auto const & furnitureMaterial = mNpcDatabase.GetFurnitureMaterial(*subKind);

    StateType::ParticleMeshType particleMesh;

    switch (mNpcDatabase.GetFurnitureParticleMeshKindType(*subKind))
    {
        case NpcDatabase::ParticleMeshKindType::Dipole:
        {
            // Check if there are enough particles

            if (mParticles.GetRemainingParticlesCount() < 2)
            {
                return { std::nullopt, NpcCreationFailureReasonType::TooManyNpcs };
            }

            // TODO
            throw GameException("Dipoles not yet supported!");
        }

        case NpcDatabase::ParticleMeshKindType::Particle:
        {
            // Check if there are enough particles

            if (mParticles.GetRemainingParticlesCount() < 1)
            {
                return { std::nullopt, NpcCreationFailureReasonType::TooManyNpcs };
            }

            // Primary

            float const mass = CalculateParticleMass(
                furnitureMaterial.GetMass(),
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentMassAdjustment
#endif
            );

            float const buoyancyVolumeFill = mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).BuoyancyVolumeFill;

            float const buoyancyFactor = CalculateParticleBuoyancyFactor(
                buoyancyVolumeFill,
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentBuoyancyAdjustment
#endif
            );

            float const staticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
                mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).FrictionSurfaceAdjustment,
                mCurrentNpcFrictionAdjustment,
                mCurrentStaticFrictionAdjustment);

            float const kineticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
                mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).FrictionSurfaceAdjustment,
                mCurrentNpcFrictionAdjustment,
                mCurrentKineticFrictionAdjustment);

            auto const primaryParticleIndex = mParticles.Add(
                mass,
                buoyancyVolumeFill,
                buoyancyFactor,
                &furnitureMaterial,
                staticFrictionTotalAdjustment,
                kineticFrictionTotalAdjustment,
                worldCoordinates,
                furnitureMaterial.RenderColor);

            particleMesh.Particles.emplace_back(primaryParticleIndex, std::nullopt);

            break;
        }

        case NpcDatabase::ParticleMeshKindType::Quad:
        {
            // Check if there are enough particles

            if (mParticles.GetRemainingParticlesCount() < 4)
            {
                return { std::nullopt, NpcCreationFailureReasonType::TooManyNpcs };
            }

            // Create Particles

            float const baseWidth = mNpcDatabase.GetFurnitureGeometry(*subKind).Width;
            float const baseHeight = mNpcDatabase.GetFurnitureGeometry(*subKind).Height;

            float const mass = CalculateParticleMass(
                furnitureMaterial.GetMass(),
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentMassAdjustment
#endif
            );

            float const baseDiagonal = std::sqrtf(baseWidth * baseWidth + baseHeight * baseHeight);

            // Positions: primary @ placing position, others following
            //
            // 0 - 1
            // |   |
            // 3 - 2

            float const width = CalculateSpringLength(baseWidth, mCurrentSizeMultiplier);
            float const height = CalculateSpringLength(baseHeight, mCurrentSizeMultiplier);
            for (int p = 0; p < 4; ++p)
            {
                // CW order
                vec2f particlePosition = worldCoordinates;

                if (p == 1 || p == 2)
                {
                    particlePosition.x += width;
                }

                if (p == 2 || p == 3)
                {
                    particlePosition.y -= height;
                }

                float const buoyancyVolumeFill = mNpcDatabase.GetFurnitureParticleAttributes(*subKind, p).BuoyancyVolumeFill;

                float const buoyancyFactor = CalculateParticleBuoyancyFactor(
                    buoyancyVolumeFill,
                    mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                    , mCurrentBuoyancyAdjustment
#endif
                );

                float const staticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
                    mNpcDatabase.GetFurnitureParticleAttributes(*subKind, p).FrictionSurfaceAdjustment,
                    mCurrentNpcFrictionAdjustment,
                    mCurrentStaticFrictionAdjustment);

                float const kineticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
                    mNpcDatabase.GetFurnitureParticleAttributes(*subKind, p).FrictionSurfaceAdjustment,
                    mCurrentNpcFrictionAdjustment,
                    mCurrentKineticFrictionAdjustment);

                auto const particleIndex = mParticles.Add(
                    mass,
                    buoyancyVolumeFill,
                    buoyancyFactor * GameRandomEngine::GetInstance().GenerateUniformReal(0.99f, 1.01f), // Make sure rotates while floating
                    &furnitureMaterial,
                    staticFrictionTotalAdjustment,
                    kineticFrictionTotalAdjustment,
                    particlePosition,
                    furnitureMaterial.RenderColor);

                particleMesh.Particles.emplace_back(particleIndex, std::nullopt);
            }

            // Springs

            // 0 - 1
            {
                particleMesh.Springs.emplace_back(
                    particleMesh.Particles[0].ParticleIndex,
                    particleMesh.Particles[1].ParticleIndex,
                    baseWidth,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).SpringReductionFraction + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 1).SpringReductionFraction) / 2.0f,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).SpringDampingCoefficient + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 1).SpringDampingCoefficient) / 2.0f);
            }

            // 0 | 3
            {
                particleMesh.Springs.emplace_back(
                    particleMesh.Particles[0].ParticleIndex,
                    particleMesh.Particles[3].ParticleIndex,
                    baseHeight,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).SpringReductionFraction + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 3).SpringReductionFraction) / 2.0f,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).SpringDampingCoefficient + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 3).SpringDampingCoefficient) / 2.0f);
            }

            // 0 \ 2
            {
                particleMesh.Springs.emplace_back(
                    particleMesh.Particles[0].ParticleIndex,
                    particleMesh.Particles[2].ParticleIndex,
                    baseDiagonal,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).SpringReductionFraction + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 2).SpringReductionFraction) / 2.0f,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 0).SpringDampingCoefficient + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 2).SpringDampingCoefficient) / 2.0f);
            }

            // 1 | 2
            {
                particleMesh.Springs.emplace_back(
                    particleMesh.Particles[1].ParticleIndex,
                    particleMesh.Particles[2].ParticleIndex,
                    baseHeight,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 1).SpringReductionFraction + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 2).SpringReductionFraction) / 2.0f,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 1).SpringDampingCoefficient + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 2).SpringDampingCoefficient) / 2.0f);
            }

            // 2 - 3
            {
                particleMesh.Springs.emplace_back(
                    particleMesh.Particles[2].ParticleIndex,
                    particleMesh.Particles[3].ParticleIndex,
                    baseWidth,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 2).SpringReductionFraction + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 3).SpringReductionFraction) / 2.0f,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 2).SpringDampingCoefficient + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 3).SpringDampingCoefficient) / 2.0f);
            }

            // 1 / 3
            {
                particleMesh.Springs.emplace_back(
                    particleMesh.Particles[1].ParticleIndex,
                    particleMesh.Particles[3].ParticleIndex,
                    baseDiagonal,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 1).SpringReductionFraction + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 3).SpringReductionFraction) / 2.0f,
                    (mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 1).SpringDampingCoefficient + mNpcDatabase.GetFurnitureParticleAttributes(*subKind, 3).SpringDampingCoefficient) / 2.0f);
            }

            CalculateSprings(
                mCurrentSizeMultiplier,
#ifdef IN_BARYLAB
                mCurrentMassAdjustment,
#endif
                mCurrentSpringReductionFractionAdjustment,
                mCurrentSpringDampingCoefficientAdjustment,
                mParticles,
                particleMesh);

            break;
        }
    }

    // Furniture

    StateType::KindSpecificStateType::FurnitureNpcStateType furnitureState = StateType::KindSpecificStateType::FurnitureNpcStateType(
        *subKind,
        mNpcDatabase.GetFurnitureRole(*subKind),
        mNpcDatabase.GetFurnitureTextureCoordinatesQuad(*subKind),
        StateType::KindSpecificStateType::FurnitureNpcStateType::BehaviorType::Default,
        currentSimulationTime);

    //
    // Store NPC
    //

    NpcId const npcId = GetNewNpcId();

    // This NPC begins its journey on the topmost ship, just
    // to make sure it's at the nearest Z
    ShipId const shipId = GetTopmostShipId();

    mStateBuffer[npcId].emplace(
        npcId,
        NpcKindType::Furniture,
        mNpcDatabase.GetFurnitureRenderColor(*subKind).toVec3f(),
        shipId, // Topmost ship ID
        0, // PlaneID: irrelevant as long as BeingPlaced
        std::nullopt, // Connected component: irrelevant as long as BeingPlaced
        StateType::RegimeType::BeingPlaced,
        std::move(particleMesh),
        StateType::KindSpecificStateType(std::move(furnitureState)),
        StateType::BeingPlacedStateType({ ParticleOrdinal, doMoveWholeMesh}));

    assert(mShips[shipId].has_value());
    mShips[shipId]->AddNpc(npcId);

    //
    // Update ship stats
    //

    mShips[shipId]->WorkingNpcStats.Add(*mStateBuffer[npcId]);
    mShips[shipId]->TotalNpcStats.Add(*mStateBuffer[npcId]);
    PublishCount();

    return { PickedNpc(npcId, ParticleOrdinal, vec2f::zero()), NpcCreationFailureReasonType::Success };
}

std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> Npcs::BeginPlaceNewHumanNpc(
    std::optional<NpcSubKindIdType> subKind,
    vec2f const & worldCoordinates,
    bool doMoveWholeMesh,
    float currentSimulationTime)
{
    int constexpr ParticleOrdinal = 1; // We use head for humans

    //
    // Check if there are enough NPCs and particles
    //

    if (CalculateTotalNpcCount() >= mMaxNpcs || mParticles.GetRemainingParticlesCount() < 2)
    {
        return { std::nullopt, NpcCreationFailureReasonType::TooManyNpcs };
    }

    //
    // Create NPC
    //

    if (!subKind.has_value())
    {
        subKind = ChooseSubKind(NpcKindType::Human, std::nullopt);
    }

    StateType::ParticleMeshType particleMesh;

    // Calculate height

    float const baseHeight =
        GameRandomEngine::GetInstance().GenerateNormalReal(
            GameParameters::HumanNpcGeometry::BodyLengthMean,
            GameParameters::HumanNpcGeometry::BodyLengthStdDev)
        * mNpcDatabase.GetHumanSizeMultiplier(*subKind);

    float const height = CalculateSpringLength(baseHeight, mCurrentSizeMultiplier);

    // Feet (primary)

    auto const & feetMaterial = mNpcDatabase.GetHumanFeetMaterial(*subKind);
    auto const & feetParticleAttributes = mNpcDatabase.GetHumanFeetParticleAttributes(*subKind);

    float const feetMass = CalculateParticleMass(
        feetMaterial.GetMass(),
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentMassAdjustment
#endif
    );

    float const feetBuoyancyFactor = CalculateParticleBuoyancyFactor(
        feetParticleAttributes.BuoyancyVolumeFill,
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentBuoyancyAdjustment
#endif
    );

    float const feetStaticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
        mNpcDatabase.GetHumanFeetParticleAttributes(*subKind).FrictionSurfaceAdjustment,
        mCurrentNpcFrictionAdjustment,
        mCurrentStaticFrictionAdjustment);

    float const feetKineticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
        mNpcDatabase.GetHumanFeetParticleAttributes(*subKind).FrictionSurfaceAdjustment,
        mCurrentNpcFrictionAdjustment,
        mCurrentKineticFrictionAdjustment);

    auto const primaryParticleIndex = mParticles.Add(
        feetMass,
        feetParticleAttributes.BuoyancyVolumeFill,
        feetBuoyancyFactor,
        &feetMaterial,
        feetStaticFrictionTotalAdjustment,
        feetKineticFrictionTotalAdjustment,
        worldCoordinates - vec2f(0.0f, height),
        feetMaterial.RenderColor);

    particleMesh.Particles.emplace_back(primaryParticleIndex, std::nullopt);

    // Head (secondary)

    auto const & headMaterial = mNpcDatabase.GetHumanHeadMaterial(*subKind);
    auto const & headParticleAttributes = mNpcDatabase.GetHumanHeadParticleAttributes(*subKind);

    float const headMass = CalculateParticleMass(
        headMaterial.GetMass(),
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentMassAdjustment
#endif
    );

    float const headBuoyancyFactor = CalculateParticleBuoyancyFactor(
        headParticleAttributes.BuoyancyVolumeFill,
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentBuoyancyAdjustment
#endif
    );

    float const headStaticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
        mNpcDatabase.GetHumanHeadParticleAttributes(*subKind).FrictionSurfaceAdjustment,
        mCurrentNpcFrictionAdjustment,
        mCurrentStaticFrictionAdjustment);

    float const headKineticFrictionTotalAdjustment = CalculateFrictionTotalAdjustment(
        mNpcDatabase.GetHumanHeadParticleAttributes(*subKind).FrictionSurfaceAdjustment,
        mCurrentNpcFrictionAdjustment,
        mCurrentKineticFrictionAdjustment);

    auto const secondaryParticleIndex = mParticles.Add(
        headMass,
        headParticleAttributes.BuoyancyVolumeFill,
        headBuoyancyFactor,
        &headMaterial,
        headStaticFrictionTotalAdjustment,
        headKineticFrictionTotalAdjustment,
        worldCoordinates,
        headMaterial.RenderColor);

    particleMesh.Particles.emplace_back(secondaryParticleIndex, std::nullopt);

    // Dipole spring

    particleMesh.Springs.emplace_back(
        primaryParticleIndex,
        secondaryParticleIndex,
        baseHeight,
        (headParticleAttributes.SpringReductionFraction + feetParticleAttributes.SpringReductionFraction) / 2.0f,
        (headParticleAttributes.SpringDampingCoefficient + feetParticleAttributes.SpringDampingCoefficient) / 2.0f);

    CalculateSprings(
        mCurrentSizeMultiplier,
#ifdef IN_BARYLAB
        mCurrentMassAdjustment,
#endif
        mCurrentSpringReductionFractionAdjustment,
        mCurrentSpringDampingCoefficientAdjustment,
        mParticles,
        particleMesh);

    // Human

    float widthMultiplier;
    if (GameRandomEngine::GetInstance().Choose(2) == 0)
    {
        // Narrow
        widthMultiplier = 1.0f -
            std::min(
                std::abs(GameRandomEngine::GetInstance().GenerateNormalReal(0.0f, GameParameters::HumanNpcGeometry::BodyWidthNarrowMultiplierStdDev)),
                1.8f * GameParameters::HumanNpcGeometry::BodyWidthNarrowMultiplierStdDev)
            * mNpcDatabase.GetHumanBodyWidthRandomizationSensitivity(*subKind);
    }
    else
    {
        // Wide
        widthMultiplier = 1.0f +
            std::min(
                std::abs(GameRandomEngine::GetInstance().GenerateNormalReal(0.0f, GameParameters::HumanNpcGeometry::BodyWidthWideMultiplierStdDev)),
                2.3f * GameParameters::HumanNpcGeometry::BodyWidthWideMultiplierStdDev)
            * mNpcDatabase.GetHumanBodyWidthRandomizationSensitivity(*subKind);
    }

    float const walkingSpeedBase =
        1.0f
        * baseHeight / 1.65f; // Just comes from 1m/s looking good when human is 1.65

    StateType::KindSpecificStateType::HumanNpcStateType humanState = StateType::KindSpecificStateType::HumanNpcStateType(
        *subKind,
        mNpcDatabase.GetHumanRole(*subKind),
        widthMultiplier,
        walkingSpeedBase,
        mNpcDatabase.GetHumanTextureCoordinatesQuads(*subKind),
        mNpcDatabase.GetHumanTextureGeometry(*subKind),
        StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::BeingPlaced,
        currentSimulationTime);

    // Frontal
    humanState.CurrentFaceOrientation = 1.0f;
    humanState.CurrentFaceDirectionX = 0.0f;

    //
    // Store NPC
    //

    NpcId const npcId = GetNewNpcId();

    // This NPC begins its journey on the topmost ship, just
    // to make sure it's at the nearest Z
    ShipId const shipId = GetTopmostShipId();

    mStateBuffer[npcId].emplace(
        npcId,
        NpcKindType::Human,
        mNpcDatabase.GetHumanRenderColor(*subKind).toVec3f(),
        shipId, // Topmost ship ID
        0, // PlaneID: irrelevant as long as BeingPlaced
        std::nullopt, // Connected component: irrelevant as long as BeingPlaced
        StateType::RegimeType::BeingPlaced,
        std::move(particleMesh),
        StateType::KindSpecificStateType(std::move(humanState)),
        StateType::BeingPlacedStateType({ ParticleOrdinal, doMoveWholeMesh })); // Human: anchor is head (second particle)

    assert(mShips[shipId].has_value());
    mShips[shipId]->AddNpc(npcId);

    //
    // Update ship stats
    //

    mShips[shipId]->WorkingNpcStats.Add(*mStateBuffer[npcId]);
    mShips[shipId]->TotalNpcStats.Add(*mStateBuffer[npcId]);
    PublishCount();

    return { PickedNpc(npcId, ParticleOrdinal, vec2f::zero()), NpcCreationFailureReasonType::Success };
}

std::optional<PickedNpc> Npcs::ProbeNpcAt(
    vec2f const & position,
    float radius,
    GameParameters const & gameParameters) const
{
    float const squareSearchRadius = radius * radius * gameParameters.NpcSizeMultiplier;

    struct NearestNpcType
    {
        NpcId Id{ NoneNpcId };
        int ParticleOrdinal{ 0 };
        float SquareDistance{ std::numeric_limits<float>::max() };
    };

    NearestNpcType nearestOnPlaneNpc;
    NearestNpcType nearestOffPlaneNpc;

    //
    // Determine ship and plane of this position - if any
    //

    std::pair<ShipId, PlaneId> probeDepth;

    // Find topmost triangle containing this position
    auto const topmostTriangle = FindTopmostWorkableTriangleContaining(position);
    if (topmostTriangle)
    {
        assert(topmostTriangle->GetShipId() < mShips.size());
        assert(mShips[topmostTriangle->GetShipId()].has_value());
        auto const & ship = *mShips[topmostTriangle->GetShipId()];

        ElementIndex const trianglePointIndex = ship.HomeShip.GetTriangles().GetPointAIndex(topmostTriangle->GetLocalObjectId());
        PlaneId const planeId = ship.HomeShip.GetPoints().GetPlaneId(trianglePointIndex);

        probeDepth = { ship.HomeShip.GetId(), planeId };
    }
    else
    {
        probeDepth = { 0, 0 }; // Bottommost
    }

    //
    // Visit all NPCs and find winner, if any
    //

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved) // BeingRemoved NPCs are invisible
        {
            switch (npc->Kind)
            {
                case NpcKindType::Furniture:
                {
                    // Proximity search for all particles

                    bool aParticleWasFound = false;
                    for (size_t p = 0; p < npc->ParticleMesh.Particles.size(); ++p)
                    {
                        auto const & particle = npc->ParticleMesh.Particles[p];

                        vec2f const candidateNpcPosition = mParticles.GetPosition(particle.ParticleIndex);
                        float const squareDistance = (candidateNpcPosition - position).squareLength();
                        if (squareDistance < squareSearchRadius)
                        {
                            if (std::make_pair(npc->CurrentShipId, npc->CurrentPlaneId) >= probeDepth)
                            {
                                // It's on-plane
                                if (squareDistance < nearestOnPlaneNpc.SquareDistance)
                                {
                                    nearestOnPlaneNpc = { npc->Id, static_cast<int>(p), squareDistance };
                                    aParticleWasFound = true;
                                }
                            }
                            else
                            {
                                // It's off-plane
                                if (squareDistance < nearestOffPlaneNpc.SquareDistance)
                                {
                                    nearestOffPlaneNpc = { npc->Id, static_cast<int>(p), squareDistance };
                                    aParticleWasFound = true;
                                }
                            }
                        }
                    }

                    if (!aParticleWasFound)
                    {
                        // Polygon test
                        //
                        // From https://wrfranklin.org/Research/Short_Notes/pnpoly.html

                        bool isHit = false;
                        for (size_t i = 0, j = npc->ParticleMesh.Particles.size() - 1; i < npc->ParticleMesh.Particles.size(); j = i++)
                        {
                            vec2f const & pos_i = mParticles.GetPosition(npc->ParticleMesh.Particles[i].ParticleIndex);
                            vec2f const & pos_j = mParticles.GetPosition(npc->ParticleMesh.Particles[j].ParticleIndex);
                            if (((pos_i.y > position.y) != (pos_j.y > position.y)) &&
                                (position.x < (pos_j.x - pos_i.x) * (position.y - pos_i.y) / (pos_j.y - pos_i.y) + pos_i.x))
                            {
                                isHit = !isHit;
                            }
                        }

                        if (isHit)
                        {
                            if (std::make_pair(npc->CurrentShipId, npc->CurrentPlaneId) >= probeDepth)
                            {
                                // It's on-plane
                                nearestOnPlaneNpc = { npc->Id, 0, squareSearchRadius };
                            }
                            else
                            {
                                // It's off-plane
                                nearestOffPlaneNpc = { npc->Id, 0, squareSearchRadius };
                            }
                        }
                    }

                    break;
                }

                case NpcKindType::Human:
                {
                    float const squareDistance = Geometry::Segment::SquareDistanceToPoint(
                        mParticles.GetPosition(npc->ParticleMesh.Particles[0].ParticleIndex),
                        mParticles.GetPosition(npc->ParticleMesh.Particles[1].ParticleIndex),
                        position);
                    if (squareDistance < squareSearchRadius)
                    {
                        if (std::make_pair(npc->CurrentShipId, npc->CurrentPlaneId) >= probeDepth)
                        {
                            // It's on-plane
                            if (squareDistance < nearestOnPlaneNpc.SquareDistance)
                            {
                                nearestOnPlaneNpc = { npc->Id, 1, squareDistance };
                            }
                        }
                        else
                        {
                            // It's off-plane
                            if (squareDistance < nearestOffPlaneNpc.SquareDistance)
                            {
                                nearestOffPlaneNpc = { npc->Id, 1, squareDistance };
                            }
                        }
                    }

                    break;
                }
            }
        }
    }

    //
    // Pick a winner - on-plane has higher prio than off-place
    //

    NpcId foundId = NoneNpcId;
    int foundParticleOrdinal = 0;
    if (nearestOnPlaneNpc.Id != NoneNpcId)
    {
        foundId = nearestOnPlaneNpc.Id;
        foundParticleOrdinal = nearestOnPlaneNpc.ParticleOrdinal;
    }
    else if (nearestOffPlaneNpc.Id != NoneNpcId)
    {
        foundId = nearestOffPlaneNpc.Id;
        foundParticleOrdinal = nearestOffPlaneNpc.ParticleOrdinal;
    }

    if (foundId != NoneNpcId)
    {
        assert(mStateBuffer[foundId].has_value());

        ElementIndex referenceParticleIndex = mStateBuffer[foundId]->ParticleMesh.Particles[foundParticleOrdinal].ParticleIndex;

        return PickedNpc(
            foundId,
            foundParticleOrdinal,
            position - mParticles.GetPosition(referenceParticleIndex));
    }
    else
    {
        return std::nullopt;
    }
}

std::vector<NpcId> Npcs::ProbeNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2) const
{
    std::vector<NpcId> result;

    VisitNpcsInQuad(
        corner1,
        corner2,
        [&](NpcId id)
        {
            // BeingRemoved NPCs are invisible
            assert(mStateBuffer[id].has_value());
            if (mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved)
            {
                result.emplace_back(id);
            }
        });

    return result;
}

void Npcs::BeginMoveNpc(
    NpcId id,
    int particleOrdinal,
    float currentSimulationTime,
    bool doMoveWholeMesh)
{
    InternalBeginMoveNpc(
        id,
        particleOrdinal,
        currentSimulationTime,
        doMoveWholeMesh);
}

void Npcs::BeginMoveNpcs(
    std::vector<NpcId> const & ids,
    float currentSimulationTime)
{
    for (NpcId const id : ids)
    {
        InternalBeginMoveNpc(
            id,
            0, // Primary
            currentSimulationTime,
            true);
    }
}

void Npcs::MoveNpcTo(
    NpcId id,
    vec2f const & position,
    vec2f const & offset,
    bool doMoveWholeMesh)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime == StateType::RegimeType::BeingPlaced);
    assert(mStateBuffer[id]->BeingPlacedState.has_value());

    // Calculate delta movement for anchor particle
    ElementIndex anchorParticleIndex = mStateBuffer[id]->ParticleMesh.Particles[mStateBuffer[id]->BeingPlacedState->AnchorParticleOrdinal].ParticleIndex;
    vec2f const deltaAnchorPosition = (position - offset) - mParticles.GetPosition(anchorParticleIndex);

    InternalMoveNpcBy(
        id,
        deltaAnchorPosition,
        doMoveWholeMesh);
}

void Npcs::MoveNpcsBy(
    std::vector<NpcId> const & ids,
    vec2f const & stride)
{
    for (NpcId const id : ids)
    {
        InternalMoveNpcBy(
            id,
            stride,
            true);
    }
}

void Npcs::EndMoveNpc(
    NpcId id,
    float currentSimulationTime)
{
    InternalEndMoveNpc(id, currentSimulationTime);
}

void Npcs::CompleteNewNpc(
    NpcId id,
    float currentSimulationTime)
{
    InternalCompleteNewNpc(id, currentSimulationTime);
}

void Npcs::RemoveNpc(
    NpcId id,
    float currentSimulationTime)
{
    InternalBeginNpcRemoval(id, currentSimulationTime);
}

void Npcs::RemoveNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2,
    float currentSimulationTime)
{
    VisitNpcsInQuad(
        corner1,
        corner2,
        [&](NpcId id)
        {
            // BeingRemoved NPCs are invisible
            assert(mStateBuffer[id].has_value());
            if (mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved)
            {
                InternalBeginNpcRemoval(id, currentSimulationTime);
            }
        });
}

void Npcs::AbortNewNpc(NpcId id)
{
    assert(mStateBuffer[id].has_value());
    auto & npc = *mStateBuffer[id];

    assert(mShips[mStateBuffer[id]->CurrentShipId].has_value());
    auto & ship = *mShips[mStateBuffer[id]->CurrentShipId];

    // Not being removed
    assert(npc.CurrentRegime != StateType::RegimeType::BeingRemoved);
    assert(std::find(mDeferredRemovalNpcs.cbegin(), mDeferredRemovalNpcs.cend(), id) == mDeferredRemovalNpcs.cend());

    // Not burning
    assert(std::find(ship.BurningNpcs.cbegin(), ship.BurningNpcs.cend(), id) == ship.BurningNpcs.cend());

    //
    // Deselect, if selected
    //

    if (mCurrentlySelectedNpc == id)
    {
        mCurrentlySelectedNpc.reset();
        PublishSelection();
    }

    //
    // Update ship stats
    //

    ship.WorkingNpcStats.Remove(npc);
    ship.TotalNpcStats.Remove(npc);
    PublishCount();

    //
    // Remove from ship
    //

    ship.RemoveNpc(id);

    //
    // Reset NPC
    //

    mStateBuffer[id].reset();
}

std::tuple<std::optional<NpcId>, NpcCreationFailureReasonType> Npcs::AddNpcGroup(
    NpcKindType kind,
    VisibleWorld const & visibleWorld,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Choose a ship
    //

    assert(mShips.size() > 0);
    size_t const shipSearchStart = GameRandomEngine::GetInstance().Choose(mShips.size());

    ShipId shipId = 0;
    for (size_t s = shipSearchStart; ;)
    {
        if (mShips[s].has_value())
        {
            // Found!
            shipId = static_cast<ShipId>(s);
            break;
        }

        // Advance
        ++s;
        if (s >= mShips.size())
        {
            s = 0;
        }

        assert(s != shipSearchStart); // There's always at least one ship
    }

    Points const & points = mShips[shipId]->HomeShip.GetPoints();
    Triangles const & triangles = mShips[shipId]->HomeShip.GetTriangles();

    //
    // Build set of candidate triangles with the best score
    //

    std::vector<ElementIndex> candidateTriangles;
    size_t bestTriangleScore = 0;

    for (ElementIndex t : triangles)
    {
        // Check triangle viability
        if (!triangles.IsDeleted(t))
        {
            ElementIndex const pA = triangles.GetPointAIndex(t);
            ElementIndex const pB = triangles.GetPointBIndex(t);
            ElementIndex const pC = triangles.GetPointCIndex(t);

            vec2f const aPosition = points.GetPosition(pA);
            vec2f const bPosition = points.GetPosition(pB);
            vec2f const cPosition = points.GetPosition(pC);

            if (aPosition.x >= visibleWorld.TopLeft.x && aPosition.x <= visibleWorld.BottomRight.x
                && aPosition.y >= visibleWorld.BottomRight.y && aPosition.y <= visibleWorld.TopLeft.y
                && bPosition.x >= visibleWorld.TopLeft.x && bPosition.x <= visibleWorld.BottomRight.x
                && bPosition.y >= visibleWorld.BottomRight.y && bPosition.y <= visibleWorld.TopLeft.y
                && cPosition.x >= visibleWorld.TopLeft.x && cPosition.x <= visibleWorld.BottomRight.x
                && cPosition.y >= visibleWorld.BottomRight.y && cPosition.y <= visibleWorld.TopLeft.y
                && !IsTriangleFolded(aPosition, bPosition, cPosition))
            {
                // Minimally viable

                //
                // Calculate score
                //

                size_t score = 1;

                // Water
                float constexpr MaxWater = 0.05f; // Arbitrary
                if (points.GetWater(pA) < MaxWater && points.GetWater(pB) < MaxWater && points.GetWater(pC) < MaxWater)
                {
                    ++score;
                }

                // Fire
                if (!points.IsBurning(pA) && !points.IsBurning(pB) && !points.IsBurning(pC))
                {
                    ++score;
                }

                // Floor underneath and at least one edge that can walk through and out
                {
                    vec2f const centerPosition = (aPosition + bPosition + cPosition) / 3.0f;
                    bcoords3f underneathBCoords = triangles.ToBarycentricCoordinates(centerPosition + vec2f(0.0f, -2.0f), t, points);

                    // Heuristic: we consider as "it's gonna be our floor" any edge that has its corresponding bcoord < 0, and viceversa
                    bool hasRightFloorUnderneath = false;
                    bool hasAtLeastOneEdgeToWalkOut = false;
                    for (int v = 0; v < 3; ++v)
                    {
                        int const edgeOrdinal = (v + 1) % 3;
                        if (underneathBCoords[v] < 0.0f && triangles.GetSubSpringNpcFloorKind(t, edgeOrdinal) != NpcFloorKindType::NotAFloor)
                        {
                            hasRightFloorUnderneath = true;
                        }
                        else if (underneathBCoords[v] > 0.0f && triangles.GetSubSpringNpcFloorKind(t, edgeOrdinal) == NpcFloorKindType::NotAFloor)
                        {
                            auto const & oppositeTriangleInfo = triangles.GetOppositeTriangle(t, edgeOrdinal);
                            if (oppositeTriangleInfo.TriangleElementIndex != NoneElementIndex && !triangles.IsDeleted(oppositeTriangleInfo.TriangleElementIndex))
                            {
                                hasAtLeastOneEdgeToWalkOut = true;
                            }
                        }
                    }

                    if (hasRightFloorUnderneath)
                    {
                        ++score;

                        if (hasAtLeastOneEdgeToWalkOut)
                        {
                            ++score;
                        }
                    }
                }

                //
                // Check if improved best score
                //

                if (score > bestTriangleScore)
                {
                    candidateTriangles.clear();
                    bestTriangleScore = score;
                }

                //
                // Store candidate
                //

                if (score == bestTriangleScore)
                {
                    candidateTriangles.push_back(t);
                }
            }
        }
    }

    //
    // Create group
    //

    // Triangles already chosen - we'll try to avoid cramming multiple NPCs in the same triangle
    std::vector<ElementIndex> alreadyChosenTriangles;
    alreadyChosenTriangles.reserve(gameParameters.NpcsPerGroup);

    size_t nNpcsAdded = 0;
    NpcId firstNpcId;
    for (; nNpcsAdded < gameParameters.NpcsPerGroup; ++nNpcsAdded)
    {
        //
        // Decide sub-kind
        //

        NpcSubKindIdType const subKind = ChooseSubKind(kind, shipId);

        //
        // Find triangle - if none, we'll go free
        //

        ElementIndex chosenTriangle = NoneElementIndex;
        if (!candidateTriangles.empty())
        {
            for (int t = 0; t < 10; ++t)
            {
                chosenTriangle = candidateTriangles[GameRandomEngine::GetInstance().Choose(static_cast<ElementCount>(candidateTriangles.size()))];

                if (auto const searchIt = std::find(alreadyChosenTriangles.cbegin(), alreadyChosenTriangles.cend(), chosenTriangle);
                    searchIt == alreadyChosenTriangles.cend())
                {
                    break;
                }
            }

            // Remember this was chosen
            alreadyChosenTriangles.push_back(chosenTriangle);
        }

        //
        // Choose position
        //

        vec2f npcPosition;
        if (chosenTriangle != NoneElementIndex)
        {
            // Center
            vec2f const aPosition = points.GetPosition(triangles.GetPointAIndex(chosenTriangle));
            vec2f const bPosition = points.GetPosition(triangles.GetPointBIndex(chosenTriangle));
            vec2f const cPosition = points.GetPosition(triangles.GetPointCIndex(chosenTriangle));
            npcPosition = (aPosition + bPosition + cPosition) / 3.0f;
        }
        else
        {
            // Choose freely
            npcPosition = vec2f(
                GameRandomEngine::GetInstance().GenerateUniformReal(visibleWorld.TopLeft.x, visibleWorld.BottomRight.x),
                GameRandomEngine::GetInstance().GenerateUniformReal(visibleWorld.BottomRight.y, visibleWorld.TopLeft.y));
        }

        //
        // Create NPC
        //

        std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> placementOutcome;

        switch (kind)
        {
            case NpcKindType::Furniture:
            {
                vec2f position;
                switch (mNpcDatabase.GetFurnitureParticleMeshKindType(subKind))
                {
                    case NpcDatabase::ParticleMeshKindType::Dipole:
                    {
                        // TODO
                        throw GameException("Dipoles not yet supported!");
                    }

                    case NpcDatabase::ParticleMeshKindType::Particle:
                    {
                        position = npcPosition;
                        break;
                    }

                    case NpcDatabase::ParticleMeshKindType::Quad:
                    {
                        // Position we give is of primary (top-left), but we want bottom (h-center) to be here
                        float const width = mNpcDatabase.GetFurnitureGeometry(subKind).Width;
                        float const height = mNpcDatabase.GetFurnitureGeometry(subKind).Height;
                        position = npcPosition + vec2f(-width / 2.0f, height);
                        break;
                    }
                }

                placementOutcome = BeginPlaceNewFurnitureNpc(
                    subKind,
                    position,
                    false,
                    currentSimulationTime);

                break;
            }

            case NpcKindType::Human:
            {
                // Position is of feet

                float const height = CalculateSpringLength(
                    GameParameters::HumanNpcGeometry::BodyLengthMean * mNpcDatabase.GetHumanSizeMultiplier(subKind),
                    mCurrentSizeMultiplier);

                placementOutcome = BeginPlaceNewHumanNpc(
                    subKind,
                    npcPosition + vec2f(0.0, height), // Head
                    false, // DoWholeMesh
                    currentSimulationTime);

                break;
            }
        }

        if (!std::get<0>(placementOutcome).has_value())
        {
            // Couldn't add NPC, so we're done
            break;
        }

        InternalCompleteNewNpc(
            std::get<0>(placementOutcome)->Id,
            currentSimulationTime);

        if (nNpcsAdded == 0)
        {
            firstNpcId = std::get<0>(placementOutcome)->Id;
        }
    }

    return (nNpcsAdded > 0)
        ? std::make_tuple(firstNpcId, NpcCreationFailureReasonType::Success)
        : std::make_tuple(std::optional<NpcId>(), NpcCreationFailureReasonType::TooManyNpcs);
}

void Npcs::TurnaroundNpc(NpcId id)
{
    InternalTurnaroundNpc(id);
}

void Npcs::TurnaroundNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2)
{
    VisitNpcsInQuad(
        corner1,
        corner2,
        [&](NpcId id)
        {
            // BeingRemoved NPCs are invisible
            assert(mStateBuffer[id].has_value());
            if (mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved)
            {
                InternalTurnaroundNpc(id);
            }
        });
}

std::optional<NpcId> Npcs::GetCurrentlySelectedNpc() const
{
    return mCurrentlySelectedNpc;
}

void Npcs::SelectFirstNpc()
{
    // Assuming an NPC exists
    assert(HasNpcs());

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved) // BeingRemoved NPCs are invisible
        {
            // Found!
            SelectNpc(npc->Id);

            return;
        }
    }

    assert(false);
}

void Npcs::SelectNextNpc()
{
    // Assuming an NPC exists
    assert(HasNpcs());

    // If we don't have any selected, select first
    if (!mCurrentlySelectedNpc.has_value())
    {
        SelectFirstNpc();
        return;
    }

    // Start searching for an NPC from next
    for (NpcId newId = *mCurrentlySelectedNpc + 1; ; ++newId)
    {
        if (static_cast<size_t>(newId) == mStateBuffer.size())
        {
            newId = 0;
        }

        if (mStateBuffer[newId].has_value()
            && mStateBuffer[newId]->CurrentRegime != StateType::RegimeType::BeingRemoved) // BeingRemoved NPCs are invisible
        {
            // Found!
            SelectNpc(newId);
            return;
        }
    }
}

void Npcs::SelectNpc(std::optional<NpcId> id)
{
    assert(!id.has_value() || (mStateBuffer[*id].has_value() && mStateBuffer[*id]->CurrentRegime != StateType::RegimeType::BeingRemoved));

    mCurrentlySelectedNpc = id;
    mCurrentlySelectedNpcWallClockTimestamp = GameWallClock::GetInstance().Now();
    PublishSelection();

#ifdef IN_BARYLAB
    Publish();
#endif
}

void Npcs::HighlightNpcs(std::vector<NpcId> const & ids)
{
    for (auto const id : ids)
    {
        InternalHighlightNpc(id);
    }
}

void Npcs::HighlightNpcsInRect(
    vec2f const & corner1,
    vec2f const & corner2)
{
    VisitNpcsInQuad(
        corner1,
        corner2,
        [&](NpcId id)
        {
            assert(mStateBuffer[id].has_value());
            if (mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved) // BeingRemoved NPCs are invisible
            {
                InternalHighlightNpc(id);
            }
        });
}

void Npcs::Announce()
{
    PublishCount();
    PublishSelection();
}

/////////////////////////////////////////

void Npcs::MoveBy(
    ShipId shipId,
    std::optional<ConnectedComponentId> connectedComponent,
    vec2f const & offset,
    vec2f const & inertialVelocity,
    GameParameters const & gameParameters)
{
    vec2f const actualInertialVelocity =
        inertialVelocity
        * gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    assert(mShips[shipId].has_value());
    auto const & homeShip = mShips[shipId]->HomeShip;
    for (auto npcId : mShips[shipId]->Npcs)
    {
        assert(mStateBuffer[npcId].has_value());

        if (mStateBuffer[npcId]->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            // Check if this NPC is in scope: it is iff:
            //	- We're moving all, OR
            //	- The primary is constrained and in this connected component
            auto const & primaryParticle = mStateBuffer[npcId]->ParticleMesh.Particles[0];
            if (!connectedComponent
                || (primaryParticle.ConstrainedState.has_value()
                    && homeShip.GetPoints().GetConnectedComponentId(homeShip.GetTriangles().GetPointAIndex(primaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)) == *connectedComponent))
            {
                // In scope - move all of its particles
                for (size_t particleOrdinal = 0; particleOrdinal < mStateBuffer[npcId]->ParticleMesh.Particles.size(); ++particleOrdinal)
                {
                    ElementIndex const particleIndex = mStateBuffer[npcId]->ParticleMesh.Particles[particleOrdinal].ParticleIndex;
                    mParticles.SetPosition(particleIndex, mParticles.GetPosition(particleIndex) + offset);
                    mParticles.SetVelocity(particleIndex, actualInertialVelocity);

                    // Zero-out already-existing forces
                    mParticles.SetExternalForces(particleIndex, vec2f::zero());

                    // Maintain world bounds
                    MaintainInWorldBounds(
                        *mStateBuffer[npcId],
                        static_cast<int>(particleOrdinal),
                        homeShip,
                        gameParameters);
                }
            }
        }
    }
}

void Npcs::RotateBy(
    ShipId shipId,
    std::optional<ConnectedComponentId> connectedComponent,
    float angle,
    vec2f const & center,
    float inertialAngle,
    GameParameters const & gameParameters)
{
    vec2f const rotX(cos(angle), sin(angle));
    vec2f const rotY(-sin(angle), cos(angle));

    float const inertiaMagnitude =
        gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f const inertialRotX(cos(inertialAngle), sin(inertialAngle));
    vec2f const inertialRotY(-sin(inertialAngle), cos(inertialAngle));

    assert(mShips[shipId].has_value());
    auto const & homeShip = mShips[shipId]->HomeShip;
    for (auto npcId : mShips[shipId]->Npcs)
    {
        assert(mStateBuffer[npcId].has_value());

        if (mStateBuffer[npcId]->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            // Check if this NPC is in scope: it is iff:
            //	- We're rotating all, OR
            //	- The primary is constrained and in this connected component
            auto const & primaryParticle = mStateBuffer[npcId]->ParticleMesh.Particles[0];
            if (!connectedComponent
                || (primaryParticle.ConstrainedState.has_value()
                    && homeShip.GetPoints().GetConnectedComponentId(homeShip.GetTriangles().GetPointAIndex(primaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)) == *connectedComponent))
            {
                for (size_t particleOrdinal = 0; particleOrdinal < mStateBuffer[npcId]->ParticleMesh.Particles.size(); ++particleOrdinal)
                {
                    ElementIndex const particleIndex = mStateBuffer[npcId]->ParticleMesh.Particles[particleOrdinal].ParticleIndex;
                    vec2f const centeredPos = mParticles.GetPosition(particleIndex) - center;
                    vec2f const newPosition = vec2f(centeredPos.dot(rotX), centeredPos.dot(rotY)) + center;
                    mParticles.SetPosition(particleIndex, newPosition);

                    vec2f const linearInertialVelocity = (vec2f(centeredPos.dot(inertialRotX), centeredPos.dot(inertialRotY)) - centeredPos) * inertiaMagnitude;
                    mParticles.SetVelocity(particleIndex, linearInertialVelocity);

                    // Zero-out already-existing forces
                    mParticles.SetExternalForces(particleIndex, vec2f::zero());

                    // Maintain world bounds
                    MaintainInWorldBounds(
                        *mStateBuffer[npcId],
                        static_cast<int>(particleOrdinal),
                        homeShip,
                        gameParameters);
                }
            }
        }
    }
}

void Npcs::SmashAt(
    vec2f const & targetPos,
    float radius,
    float currentSimulationTime)
{
    //
    // Transition all humans in radius which have not transitioned yet
    //

    float const squareRadius = radius * radius;

    for (auto & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->Kind == NpcKindType::Human
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            bool hasOneParticleInRadius = false;
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                float const pointSquareDistance = (mParticles.GetPosition(npcParticle.ParticleIndex) - targetPos).squareLength();
                if (pointSquareDistance < squareRadius)
                {
                    hasOneParticleInRadius = true;
                    break;
                }
            }

            if (hasOneParticleInRadius)
            {
                auto & humanState = npc->KindSpecificState.HumanNpcState;
                if (humanState.CurrentBehavior != StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::ConstrainedOrFree_Smashed)
                {
                    // Transition to Smashed
                    humanState.TransitionToState(StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::ConstrainedOrFree_Smashed, currentSimulationTime);

                    // Turn front/back iff side-looking
                    if (humanState.CurrentFaceOrientation == 0.0f)
                    {
                        humanState.CurrentFaceOrientation = GameRandomEngine::GetInstance().GenerateUniformBoolean(0.5f) ? +1.0f : -1.0f;
                        humanState.CurrentFaceDirectionX = 0.0f;
                    }

                    // Futurework: sound
                }
                else
                {
                    // Prolong stay
                    humanState.CurrentBehaviorState.ConstrainedOrFree_Smashed.Reset();
                }
            }
        }
    }
}

void Npcs::DrawTo(
    vec2f const & targetPos,
    float strength)
{
    //
    // F = ForceStrength/sqrt(distance), along radius
    //

    // Recalibrate force for NPCs
    strength *= 0.2f;

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                vec2f displacement = (targetPos - mParticles.GetPosition(npcParticle.ParticleIndex));
                float forceMagnitude = strength / sqrtf(0.1f + displacement.length());

                mParticles.AddExternalForce(
                    npcParticle.ParticleIndex,
                    displacement.normalise() * forceMagnitude);
            }
        }
    }
}

void Npcs::SwirlAt(
    vec2f const & targetPos,
    float strength)
{
    //
    // Just some magic mix of radial and centripetal forces
    //

    float const radialForce = strength;
    float const centripetalForce = std::abs(strength) * 8.5f; // To

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                vec2f displacement = (targetPos - mParticles.GetPosition(npcParticle.ParticleIndex));
                vec2f displacementDir = displacement.normalise_approx();

                mParticles.AddExternalForce(
                    npcParticle.ParticleIndex,
                    displacementDir.to_perpendicular() * radialForce + displacementDir * centripetalForce);
            }
        }
    }
}

void Npcs::ApplyBlast(
    ShipId shipId,
    vec2f const & centerPosition,
    float blastRadius,
    float blastForce, // N
    GameParameters const & /*gameParameters*/)
{
    //
    // Only NPCs of this ship, or free regime of any ship
    //

    float const actualBlastRadius = blastRadius * 4.0f;
    float const squareRadius = actualBlastRadius * actualBlastRadius;

    // The specified blast is for damage to the ship; here we want a lower
    // force and a larger radius - as if only caused by air - and thus we
    // make the force ~proportional to the particle's mass so we have ~constant
    // runaway speeds
    //
    // Anchor points:
    //   Human: F=35000 == 1000*mass
    float const blastAcceleration = blastForce / 3750.0f; // This yields a blast force of 35000, i.e. an acceleration of 1000 on a human particle

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                if (!npcParticle.ConstrainedState.has_value()
                    || npc->CurrentShipId == shipId)
                {
                    vec2f const particleRadius = mParticles.GetPosition(npcParticle.ParticleIndex) - centerPosition;
                    float const squareParticleDistance = particleRadius.squareLength();
                    if (squareParticleDistance < squareRadius)
                    {
                        float const particleRadiusLength = std::sqrt(squareParticleDistance);

                        //
                        // Apply blast force
                        //

                        float const particleBlastForce = blastAcceleration * 6.0f * std::sqrt(mParticles.GetMass(npcParticle.ParticleIndex));

                        mParticles.AddExternalForce(
                            npcParticle.ParticleIndex,
                            particleRadius.normalise(particleRadiusLength) * particleBlastForce / (particleRadiusLength + 2.0f));
                    }
                }
            }
        }
    }
}

void Npcs::ApplyAntiMatterBombPreimplosion(
    ShipId shipId,
    vec2f const & centerPosition,
    float radius,
    float radiusThickness,
    GameParameters const & gameParameters)
{
    //
    // Only NPCs of this ship, or free regime of any ship
    //

    float const strength =
        5000.0f // Magic number
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                if (!npcParticle.ConstrainedState.has_value()
                    || npc->CurrentShipId == shipId)
                {
                    vec2f const particleRadius = mParticles.GetPosition(npcParticle.ParticleIndex) - centerPosition;
                    float const particleDistanceFromRadius = particleRadius.length() - radius;
                    float const absoluteParticleDistanceFromRadius = std::abs(particleDistanceFromRadius);
                    if (absoluteParticleDistanceFromRadius <= radiusThickness)
                    {
                        float const forceDirection = particleDistanceFromRadius >= 0.0f ? 1.0f : -1.0f;

                        float const forceStrength = strength * (1.0f - absoluteParticleDistanceFromRadius / radiusThickness);

                        mParticles.AddExternalForce(
                            npcParticle.ParticleIndex,
                            particleRadius.normalise() * forceStrength * forceDirection);
                    }
                }
            }
        }
    }
}

void Npcs::ApplyAntiMatterBombImplosion(
    ShipId shipId,
    vec2f const & centerPosition,
    float sequenceProgress,
    GameParameters const & gameParameters)
{
    //
    // Only NPCs of this ship, or free regime of any ship
    //

    float const strength =
        (sequenceProgress * sequenceProgress)
        * gameParameters.AntiMatterBombImplosionStrength
        * 3000.0f // Magic number
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                if (!npcParticle.ConstrainedState.has_value()
                    || npc->CurrentShipId == shipId)
                {
                    vec2f displacement = centerPosition - mParticles.GetPosition(npcParticle.ParticleIndex);
                    float const displacementLength = displacement.length();
                    vec2f normalizedDisplacement = displacement.normalise(displacementLength);

                    // Make final acceleration somewhat independent from mass
                    float const massNormalization = mParticles.GetMass(npcParticle.ParticleIndex) / 50.0f;

                    // Angular (constant)
                    mParticles.AddExternalForce(
                        npcParticle.ParticleIndex,
                        vec2f(-normalizedDisplacement.y, normalizedDisplacement.x)
                        * strength
                        * massNormalization
                        / 10.0f); // Magic number

                    // Radial (stronger when closer)
                    mParticles.AddExternalForce(
                        npcParticle.ParticleIndex,
                        normalizedDisplacement
                        * strength
                        / (0.2f + 0.5f * sqrt(displacementLength))
                        * massNormalization
                        * 10.0f); // Magic number
                }
            }
        }
    }
}

void Npcs::ApplyAntiMatterBombExplosion(
    ShipId shipId,
    vec2f const & centerPosition,
    GameParameters const & gameParameters)
{
    //
    // Only NPCs of this ship, or free regime of any ship
    //

    float const strength =
        30000.0f // Magic number
        * (gameParameters.IsUltraViolentMode ? 50.0f : 1.0f);

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value()
            && npc->CurrentRegime != StateType::RegimeType::BeingRemoved)
        {
            for (auto const & npcParticle : npc->ParticleMesh.Particles)
            {
                if (!npcParticle.ConstrainedState.has_value()
                    || npc->CurrentShipId == shipId)
                {
                    vec2f displacement = mParticles.GetPosition(npcParticle.ParticleIndex) - centerPosition;
                    float forceMagnitude = strength / sqrtf(0.1f + displacement.length());

                    mParticles.AddExternalForce(
                        npcParticle.ParticleIndex,
                        displacement.normalise() * forceMagnitude);
                }
            }
        }
    }
}

void Npcs::OnShipTriangleDestroyed(
    ShipId shipId,
    ElementIndex triangleElementIndex)
{
    assert(shipId < mShips.size());
    assert(mShips[shipId].has_value());

    Ship const & homeShip = mShips[shipId]->HomeShip;

    // Check pre-conditions
    //
    // Since this loop might be taxing - especially under widespread destruction - we
    // want to run only on "first break" of an area

    for (int e = 0; e < 3; ++e)
    {
        auto const oppositeTriangleIndex = homeShip.GetTriangles().GetOppositeTriangle(triangleElementIndex, e).TriangleElementIndex;
        if (oppositeTriangleIndex != NoneElementIndex && homeShip.GetTriangles().IsDeleted(oppositeTriangleIndex))
        {
            return;
        }
    }

    //
    // Visit all NPCs on this ship and scare the close ones that are walking
    //

    ElementIndex trianglePointElementIndex = homeShip.GetTriangles().GetPointAIndex(triangleElementIndex); // Representative
    ConnectedComponentId const triangleConnectedComponentId = homeShip.GetPoints().GetConnectedComponentId(trianglePointElementIndex);
    vec2f const & trianglePosition = homeShip.GetPoints().GetPosition(trianglePointElementIndex);

    float constexpr Radius = 10.0f;
    float constexpr SquareRadius = Radius * Radius;

    for (auto const npcId : mShips[shipId]->Npcs)
    {
        assert(mStateBuffer[npcId].has_value());

        auto & npc = *mStateBuffer[npcId];

        if (npc.CurrentConnectedComponentId == triangleConnectedComponentId
            && npc.CurrentRegime != StateType::RegimeType::BeingRemoved
            && npc.Kind == NpcKindType::Human
            && npc.KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking)
        {
            auto & humanNpcState = npc.KindSpecificState.HumanNpcState;

            assert(npc.ParticleMesh.Particles.size() >= 2);
            vec2f const & npcPosition = mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex); // Head, arbitrarily
            float const squareDistance = (npcPosition - trianglePosition).squareLength();
            if (squareDistance <= SquareRadius)
            {
                // Scare this NPC, unless we've just scared it
                if (humanNpcState.MiscPanicLevel < 0.6)
                {
                    // Time to flip if we're going towards it
                    if ((trianglePosition.x - npcPosition.x) * humanNpcState.CurrentFaceDirectionX >= 0.0f)
                    {
                        humanNpcState.CurrentFaceDirectionX *= -1.0f;
                    }
                }

                // Panic
                humanNpcState.MiscPanicLevel = 1.0f;
            }
        }
    }
}

/////////////////////////////// Barylab-specific

#ifdef IN_BARYLAB

bool Npcs::AddHumanNpc(
    NpcSubKindIdType subKind,
    vec2f const & worldCoordinates,
    float currentSimulationTime)
{
    auto const result = BeginPlaceNewHumanNpc(
        subKind,
        worldCoordinates,
        currentSimulationTime,
        false);

    if (std::get<0>(result).has_value())
    {
        CompleteNewNpc(
            std::get<0>(result)->Id,
            currentSimulationTime);

        return true;
    }
    else
    {
        return false;
    }
}

void Npcs::FlipHumanWalk(int npcIndex)
{
    if (npcIndex < mStateBuffer.size()
        && mStateBuffer[npcIndex].has_value()
        && mStateBuffer[npcIndex]->Kind == NpcKindType::Human
        && mStateBuffer[npcIndex]->KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking)
    {
        FlipHumanWalk(mStateBuffer[npcIndex]->KindSpecificState.HumanNpcState, StrongTypedTrue<_DoImmediate>);
    }
}

void Npcs::FlipHumanFrontBack(int npcIndex)
{
    if (npcIndex < mStateBuffer.size()
        && mStateBuffer[npcIndex].has_value()
        && mStateBuffer[npcIndex]->Kind == NpcKindType::Human
        && mStateBuffer[npcIndex]->KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking)
    {
        auto & humanState = mStateBuffer[npcIndex]->KindSpecificState.HumanNpcState;

        if (humanState.CurrentFaceOrientation != 0.0f)
        {
            humanState.CurrentFaceOrientation *= -1.0f;
        }
    }
}

void Npcs::MoveParticleBy(
    ElementIndex particleIndex,
    vec2f const & offset,
    float currentSimulationTime)
{
    //
    // Move particle
    //

    mParticles.SetPosition(
        particleIndex,
        mParticles.GetPosition(particleIndex) + offset);

    mParticles.SetVelocity(
        particleIndex,
        vec2f::zero()); // Zero-out velocity

    //
    // Re-initialize state of NPC that contains this particle
    //

    for (auto & state : mStateBuffer)
    {
        if (state.has_value())
        {
            if (std::any_of(
                state->ParticleMesh.Particles.cbegin(),
                state->ParticleMesh.Particles.cend(),
                [particleIndex](auto const & p) { return p.ParticleIndex == particleIndex;}))
            {
                auto const oldRegime = state->CurrentRegime;

                ResetNpcStateToWorld(*state, currentSimulationTime);

                OnMayBeNpcRegimeChanged(oldRegime, *state);

                //
                // Select particle
                //

                SelectParticle(particleIndex);

                //
                // Reset trajectories
                //

                mCurrentParticleTrajectory.reset();
                mCurrentParticleTrajectoryNotification.reset();

                break;
            }
        }
    }
}

void Npcs::RotateParticlesWithShip(
    vec2f const & centerPos,
    float cosAngle,
    float sinAngle)
{
    //
    // Rotate particles
    //

    for (auto const & ship : mShips)
    {
        if (ship.has_value())
        {
            for (auto const n : ship->Npcs)
            {
                auto & state = mStateBuffer[n];
                assert(state.has_value());

                for (auto & particle : state->ParticleMesh.Particles)
                {
                    RotateParticleWithShip(
                        particle,
                        centerPos,
                        cosAngle,
                        sinAngle,
                        ship->HomeShip);
                }
            }
        }
    }
}

void Npcs::RotateParticleWithShip(
    StateType::NpcParticleStateType const & npcParticleState,
    vec2f const & centerPos,
    float cosAngle,
    float sinAngle,
    Ship const & homeShip)
{
    vec2f newPosition;

    if (npcParticleState.ConstrainedState.has_value())
    {
        // Simply set position from current bary coords

        newPosition = homeShip.GetTriangles().FromBarycentricCoordinates(
            npcParticleState.ConstrainedState->CurrentBCoords.BCoords,
            npcParticleState.ConstrainedState->CurrentBCoords.TriangleElementIndex,
            homeShip.GetPoints());
    }
    else
    {
        // Rotate particle

        vec2f const centeredPos = mParticles.GetPosition(npcParticleState.ParticleIndex) - centerPos;
        vec2f const rotatedPos = vec2f(
            centeredPos.x * cosAngle - centeredPos.y * sinAngle,
            centeredPos.x * sinAngle + centeredPos.y * cosAngle);

        newPosition = rotatedPos + centerPos;
    }

    mParticles.SetPosition(npcParticleState.ParticleIndex, newPosition);
}

void Npcs::OnPointMoved(float currentSimulationTime)
{
    //
    // Recalculate state of all NPCs
    //

    for (auto & state : mStateBuffer)
    {
        if (state.has_value())
        {
            auto const oldRegime = state->CurrentRegime;

            ResetNpcStateToWorld(*state, currentSimulationTime);

            OnMayBeNpcRegimeChanged(oldRegime, *state);
        }
    }
}

bool Npcs::IsTriangleConstrainingCurrentlySelectedParticle(ElementIndex triangleIndex) const
{
#ifdef _DEBUG
    if (mCurrentlySelectedParticle.has_value())
    {
        for (auto const & state : mStateBuffer)
        {
            if (state.has_value())
            {
                if (std::any_of(
                    state->ParticleMesh.Particles.cbegin(),
                    state->ParticleMesh.Particles.cend(),
                    [triangleIndex, selectedParticleIndex = *mCurrentlySelectedParticle](auto const & p)
                    {
                        return p.ParticleIndex == selectedParticleIndex
                            && p.ConstrainedState.has_value()
                            && p.ConstrainedState->CurrentBCoords.TriangleElementIndex == triangleIndex;
                    }))
                {
                    return true;
                }
            }
        }
    }
#else
    (void)triangleIndex;
#endif

    return false;
}

bool Npcs::IsSpringHostingCurrentlySelectedParticle(ElementIndex springIndex) const
{
#ifdef _DEBUG
    if (mCurrentlySelectedParticle.has_value())
    {
        for (auto const & ship : mShips)
        {
            if (ship.has_value())
            {
                for (auto const n : ship->Npcs)
                {
                    auto & state = mStateBuffer[n];
                    assert(state.has_value());

                    for (auto const & particle : state->ParticleMesh.Particles)
                    {
                        if (particle.ParticleIndex == *mCurrentlySelectedParticle
                            && particle.ConstrainedState.has_value()
                            && particle.ConstrainedState->CurrentVirtualFloor.has_value()
                            && ship->HomeShip.GetTriangles()
                            .GetSubSprings(particle.ConstrainedState->CurrentVirtualFloor->TriangleElementIndex).SpringIndices[particle.ConstrainedState->CurrentVirtualFloor->EdgeOrdinal] == springIndex)
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }
#else
    (void)springIndex;
#endif

    return false;
}

void Npcs::Publish() const
{
#ifdef _DEBUG
    std::optional<AbsoluteTriangleBCoords> constrainedRegimeParticleProbe;
    std::optional<int> constrainedRegimeLastEnteredFloorDepth;
    std::optional<bcoords3f> subjectParticleBarycentricCoordinatesWrtOriginTriangleChanged;
    std::optional<PhysicsParticleProbe> physicsParticleProbe;

    if (mCurrentlySelectedParticle.has_value())
    {
        for (size_t n = 0; n < mStateBuffer.size(); ++n)
        {
            if (mStateBuffer[n].has_value())
            {
                auto const & state = *mStateBuffer[n];

                assert(mShips[state.CurrentShipId].has_value());
                auto const & homeShip = mShips[state.CurrentShipId]->HomeShip;

                for (auto const & particle : state.ParticleMesh.Particles)
                {
                    if (particle.ParticleIndex == *mCurrentlySelectedParticle)
                    {
                        if (particle.ConstrainedState.has_value())
                        {
                            constrainedRegimeParticleProbe.emplace(particle.ConstrainedState->CurrentBCoords);

                            if (mCurrentOriginTriangle.has_value())
                            {
                                subjectParticleBarycentricCoordinatesWrtOriginTriangleChanged = homeShip.GetTriangles().ToBarycentricCoordinates(
                                    mParticles.GetPosition(particle.ParticleIndex),
                                    *mCurrentOriginTriangle,
                                    homeShip.GetPoints());
                            }
                        }

                        physicsParticleProbe.emplace(mParticles.GetVelocity(particle.ParticleIndex));
                    }
                }
            }
        }
    }

    mGameEventHandler->OnSubjectParticleConstrainedRegimeUpdated(constrainedRegimeParticleProbe);
    mGameEventHandler->OnSubjectParticleBarycentricCoordinatesWrtOriginTriangleChanged(subjectParticleBarycentricCoordinatesWrtOriginTriangleChanged);
    mGameEventHandler->OnSubjectParticlePhysicsUpdated(physicsParticleProbe);

    if (mCurrentlySelectedNpc.has_value()
        && mStateBuffer[*mCurrentlySelectedNpc].has_value())
    {
        if (mStateBuffer[*mCurrentlySelectedNpc]->Kind == NpcKindType::Human)
        {
            switch (mStateBuffer[*mCurrentlySelectedNpc]->KindSpecificState.HumanNpcState.CurrentBehavior)
            {
                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::BeingPlaced:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("BeingPlaced");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Aerial:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Aerial");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Electrified:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Electrified");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Equilibrium:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Equilibrium");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Falling:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Falling");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_InWater:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_InWater");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_KnockedOut:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_KnockedOut");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_PreRising:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_PreRising");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Rising:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Rising");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Swimming_Style1:
                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Swimming_Style2:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Swimming");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_Walking");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_WalkingUndecided:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Constrained_WalkingUndecided");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Free_Aerial:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Free_Aerial");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Free_InWater:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Free_InWater");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Free_KnockedOut:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Free_KnockedOut");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Free_Swimming_Style1:
                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Free_Swimming_Style2:
                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Free_Swimming_Style3:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("Free_Swimming");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::ConstrainedOrFree_Smashed:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("ConstrainedOrFree_Smashed");
                    break;
                }

                case StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::BeingRemoved:
                {
                    mGameEventHandler->OnHumanNpcBehaviorChanged("BeingRemoved");
                    break;
                }
            }
        }
    }
#endif
}

#endif

///////////////////////////////

void Npcs::VisitNpcsInQuad(
    vec2f const & corner1,
    vec2f const & corner2,
    std::function<void(NpcId)> action) const
{
    float const minX = std::min(corner1.x, corner2.x);
    float const maxX = std::max(corner1.x, corner2.x);
    float const minY = std::min(corner1.y, corner2.y);
    float const maxY = std::max(corner1.y, corner2.y);

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value())
        {
            // We visit the NPC if at least one of its particles is in the quad

            bool isChosen = false;

            for (size_t p = 0; p < npc->ParticleMesh.Particles.size(); ++p)
            {
                vec2f const & pos = mParticles.GetPosition(npc->ParticleMesh.Particles[p].ParticleIndex);
                if (pos.x >= minX && pos.x <= maxX && pos.y >= minY && pos.y <= maxY)
                {
                    // Found!
                    isChosen = true;
                    break;
                }
            }

            if (isChosen)
            {
                action(npc->Id);
            }
        }
    }
}

void Npcs::InternalBeginMoveNpc(
    NpcId id,
    int particleOrdinal,
    float currentSimulationTime,
    bool doMoveWholeMesh)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved);
    assert(std::find(mDeferredRemovalNpcs.cbegin(), mDeferredRemovalNpcs.cend(), id) == mDeferredRemovalNpcs.cend());

    auto & npc = *mStateBuffer[id];

    //
    // Move NPC to topmost ship
    //

    TransferNpcToShip(npc, GetTopmostShipId());
    npc.CurrentPlaneId = 0; // Irrelevant as long as it's in BeingPlaced

    //
    // Move NPC to BeingPlaced
    //

    auto const oldRegime = npc.CurrentRegime;

    // All particles become free
    for (auto & particle : npc.ParticleMesh.Particles)
    {
        particle.ConstrainedState.reset();
    }

    // Setup being placed state
    npc.BeingPlacedState = StateType::BeingPlacedStateType({
        particleOrdinal,
        doMoveWholeMesh,
        oldRegime
        });

    // Change regime
    npc.CurrentRegime = StateType::RegimeType::BeingPlaced;
    // Do not check regime change

    if (npc.Kind == NpcKindType::Human)
    {
        // Change behavior
        npc.KindSpecificState.HumanNpcState.TransitionToState(
            StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::BeingPlaced,
            currentSimulationTime);
    }
}

void Npcs::InternalMoveNpcBy(
    NpcId id,
    vec2f const & deltaAnchorPosition,
    bool doMoveWholeMesh)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime == StateType::RegimeType::BeingPlaced);
    assert(mStateBuffer[id]->BeingPlacedState.has_value());

    auto & npc = *mStateBuffer[id];

    // Calculate absolute velocity for this delta movement - we want it clamped
    vec2f const targetAbsoluteVelocity = (deltaAnchorPosition / GameParameters::SimulationStepTimeDuration<float> *mGlobalDampingFactor).clamp_length_upper(GameParameters::MaxNpcToolMoveVelocityMagnitude);

    // Move particles
    for (size_t p = 0; p < npc.ParticleMesh.Particles.size(); ++p)
    {
        auto const particleIndex = npc.ParticleMesh.Particles[p].ParticleIndex;

        if (doMoveWholeMesh || p == static_cast<size_t>(npc.BeingPlacedState->AnchorParticleOrdinal))
        {
            mParticles.SetPosition(particleIndex, mParticles.GetPosition(particleIndex) + deltaAnchorPosition);
            mParticles.SetVelocity(particleIndex, targetAbsoluteVelocity);
        }

        // No worries about mesh-relative velocity
        assert(!npc.ParticleMesh.Particles[p].ConstrainedState.has_value());
    }

    // Update state
    npc.BeingPlacedState->DoMoveWholeMesh = doMoveWholeMesh;
}

void Npcs::InternalBeginNpcRemoval(
    NpcId id,
    float currentSimulationTime)
{
    InternalBeginDeferredDeletion(id, currentSimulationTime);

    // Change behavior
    assert(mStateBuffer[id].has_value());
    auto & npc = *mStateBuffer[id];
    switch (npc.Kind)
    {
        case NpcKindType::Furniture:
        {
            npc.KindSpecificState.FurnitureNpcState.TransitionToState(
                StateType::KindSpecificStateType::FurnitureNpcStateType::BehaviorType::BeingRemoved,
                currentSimulationTime);

            break;
        }

        case NpcKindType::Human:
        {
            npc.KindSpecificState.HumanNpcState.TransitionToState(
                StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::BeingRemoved,
                currentSimulationTime);

#ifdef BARYLAB_PROBING
            if (npc.Id == mCurrentlySelectedNpc)
            {
                mGameEventHandler->OnHumanNpcBehaviorChanged("BeingRemoved");
            }
#endif

            break;
        }
    }
}

void Npcs::InternalBeginDeferredDeletion(
    NpcId id,
    float /*currentSimulationTime*/)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved);
    assert(std::find(mDeferredRemovalNpcs.cbegin(), mDeferredRemovalNpcs.cend(), id) == mDeferredRemovalNpcs.cend());

    auto & npc = *mStateBuffer[id];

    //
    // Move NPC to BeingRemoved
    //

    auto const oldRegime = npc.CurrentRegime;

    // Change regime
    npc.CurrentRegime = StateType::RegimeType::BeingRemoved;
    OnMayBeNpcRegimeChanged(oldRegime, npc);

    //
    // Update ship stats
    //

    assert(mShips[npc.CurrentShipId].has_value());
    auto & ship = *(mShips[npc.CurrentShipId]);

    ship.WorkingNpcStats.Remove(npc);
    PublishCount();

    //
    // Remove from burning set, if there
    //

    auto burningNpcIt = std::find(ship.BurningNpcs.begin(), ship.BurningNpcs.end(), id);
    if (burningNpcIt != ship.BurningNpcs.end())
    {
        ship.BurningNpcs.erase(burningNpcIt);
    }

    //
    // Deselect, if selected
    //

    if (mCurrentlySelectedNpc == id)
    {
        mCurrentlySelectedNpc.reset();
        PublishSelection();
    }
}

void Npcs::InternalEndMoveNpc(
    NpcId id,
    float currentSimulationTime)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime == StateType::RegimeType::BeingPlaced);

    auto & npc = *mStateBuffer[id];

    ResetNpcStateToWorld(
        npc,
        currentSimulationTime);

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
    float currentSimulationTime)
{
    InternalEndMoveNpc(id, currentSimulationTime);
}

void Npcs::InternalTurnaroundNpc(NpcId id)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved);

    switch (mStateBuffer[id]->Kind)
    {
        case NpcKindType::Human:
        {
            if (mStateBuffer[id]->KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking)
            {
                // Flip walk
                FlipHumanWalk(mStateBuffer[id]->KindSpecificState.HumanNpcState, StrongTypedTrue<_DoImmediate>);
            }
            else
            {
                // Just change orientation/direction
                if (mStateBuffer[id]->KindSpecificState.HumanNpcState.CurrentFaceDirectionX != 0.0f)
                {
                    mStateBuffer[id]->KindSpecificState.HumanNpcState.CurrentFaceDirectionX *= -1.0f;
                }
                else
                {
                    mStateBuffer[id]->KindSpecificState.HumanNpcState.CurrentFaceOrientation *= -1.0f;
            }
        }

            break;
        }

        case NpcKindType::Furniture:
        {
            mStateBuffer[id]->KindSpecificState.FurnitureNpcState.CurrentFaceDirectionX *= -1.0f;

            break;
        }
    }
}

void Npcs::InternalHighlightNpc(NpcId id)
{
    assert(mStateBuffer[id].has_value());
    assert(mStateBuffer[id]->CurrentRegime != StateType::RegimeType::BeingRemoved);

    mStateBuffer[id]->IsHighlightedForRendering = true;
}

void Npcs::PublishCount()
{
    mGameEventHandler->OnNpcCountsUpdated(CalculateWorkingNpcCount());
}

void Npcs::PublishSelection()
{
    mGameEventHandler->OnNpcSelectionChanged(mCurrentlySelectedNpc);
}

NpcId Npcs::GetNewNpcId()
{
    // See if we can find a hole, so we stay compact
    for (size_t n = 0; n < mStateBuffer.size(); ++n)
    {
        if (!mStateBuffer[n].has_value())
        {
            return static_cast<NpcId>(n);
        }
    }

    // No luck, add new entry
    NpcId const newNpcId = static_cast<NpcId>(mStateBuffer.size());
    mStateBuffer.emplace_back(std::nullopt);
    return newNpcId;
}

NpcSubKindIdType Npcs::ChooseSubKind(
    NpcKindType kind,
    std::optional<ShipId> shipId) const
{
    switch (kind)
    {
        case NpcKindType::Furniture:
        {
            // Furniture
            auto const & furnitureRoles = mNpcDatabase.GetFurnitureSubKindIdsByRole()[static_cast<size_t>(NpcFurnitureRoleType::Furniture)];
            size_t iSubKind = GameRandomEngine::GetInstance().Choose(furnitureRoles.size());
            return furnitureRoles[iSubKind];
        }

        case NpcKindType::Human:
        {
            // Check whether ship already has a captain
            if (shipId.has_value() && mShips[*shipId]->WorkingNpcStats.HumanCaptainNpcCount == 0)
            {
                // Choose a captain
                auto const & captainRoles = mNpcDatabase.GetHumanSubKindIdsByRole()[static_cast<size_t>(NpcHumanRoleType::Captain)];
                size_t iSubKind = GameRandomEngine::GetInstance().Choose(captainRoles.size());
                return captainRoles[iSubKind];
            }
            else
            {
                // Choose a role first
                if (GameRandomEngine::GetInstance().GenerateUniformBoolean(0.3f)) // ~1/3 is crew
                {
                    // Crew
                    auto const & crewRoles = mNpcDatabase.GetHumanSubKindIdsByRole()[static_cast<size_t>(NpcHumanRoleType::Crew)];
                    size_t iSubKind = GameRandomEngine::GetInstance().Choose(crewRoles.size());
                    return crewRoles[iSubKind];
                }
                else
                {
                    // Passengers
                    auto const & passengerRoles = mNpcDatabase.GetHumanSubKindIdsByRole()[static_cast<size_t>(NpcHumanRoleType::Passenger)];
                    size_t iSubKind = GameRandomEngine::GetInstance().Choose(passengerRoles.size());
                    return passengerRoles[iSubKind];
                }
            }
        }
    }

    assert(false);
    return 0;
}

size_t Npcs::CalculateWorkingNpcCount() const
{
    size_t totalCount = 0;

    for (auto const & s : mShips)
    {
        if (s.has_value())
        {
            totalCount += s->WorkingNpcStats.FurnitureNpcCount;
            totalCount += s->WorkingNpcStats.HumanNpcCount;
        }
    }

    return totalCount;
}

size_t Npcs::CalculateTotalNpcCount() const
{
    size_t totalCount = 0;

    for (auto const & s : mShips)
    {
        if (s.has_value())
        {
            totalCount += s->TotalNpcStats.FurnitureNpcCount;
            totalCount += s->TotalNpcStats.HumanNpcCount;
        }
    }

    return totalCount;
}

ShipId Npcs::GetTopmostShipId() const
{
    assert(mShips.size() > 0);

    for (size_t s = mShips.size() - 1; ;)
    {
        if (mShips[s].has_value())
        {
            return static_cast<ShipId>(s);
        }

        if (s == 0)
        {
            break;
        }

        --s;
    }

    assert(false);
    return 0;
}

std::optional<GlobalElementId> Npcs::FindTopmostWorkableTriangleContaining(vec2f const & position) const
{
    // Visit all ships in reverse ship ID order (i.e. from topmost to bottommost)
    assert(mShips.size() > 0);
    for (size_t s = mShips.size() - 1; ;)
    {
        if (mShips[s].has_value())
        {
            // Find the triangle in this ship containing this position and having the highest plane ID

            auto const & homeShip = mShips[s]->HomeShip;

            std::optional<ElementIndex> bestTriangleIndex;
            PlaneId bestPlaneId = std::numeric_limits<PlaneId>::lowest();
            for (auto const triangleIndex : homeShip.GetTriangles())
            {
                if (!homeShip.GetTriangles().IsDeleted(triangleIndex))
                {
                    // Arbitrary representative for plane and connected component
                    auto const pointAIndex = homeShip.GetTriangles().GetPointAIndex(triangleIndex);

                    vec2f const aPosition = homeShip.GetPoints().GetPosition(pointAIndex);
                    vec2f const bPosition = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointBIndex(triangleIndex));
                    vec2f const cPosition = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointCIndex(triangleIndex));

                    if (Geometry::IsPointInTriangle(position, aPosition, bPosition, cPosition)
                        && (!bestTriangleIndex || homeShip.GetPoints().GetPlaneId(pointAIndex) > bestPlaneId)
                        && !IsTriangleFolded(aPosition, bPosition, cPosition))
                    {
                        bestTriangleIndex = triangleIndex;
                        bestPlaneId = homeShip.GetPoints().GetPlaneId(pointAIndex);
                    }
                }
            }

            if (bestTriangleIndex)
            {
                // Found a triangle on this ship
                return GlobalElementId(static_cast<ShipId>(s), *bestTriangleIndex);
            }
        }

        if (s == 0)
            break;
        --s;
    }

    // No triangle found
    return std::nullopt;
}

ElementIndex Npcs::FindWorkableTriangleContaining(
    vec2f const & position,
    Ship const & homeShip,
    std::optional<ConnectedComponentId> constrainedConnectedComponentId)
{
    for (auto const triangleIndex : homeShip.GetTriangles())
    {
        if (!homeShip.GetTriangles().IsDeleted(triangleIndex))
        {
            // Arbitrary representative for plane and connected component
            auto const pointAIndex = homeShip.GetTriangles().GetPointAIndex(triangleIndex);

            vec2f const aPosition = homeShip.GetPoints().GetPosition(pointAIndex);
            vec2f const bPosition = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointBIndex(triangleIndex));
            vec2f const cPosition = homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointCIndex(triangleIndex));

            if (Geometry::IsPointInTriangle(position, aPosition, bPosition, cPosition)
                && !IsTriangleFolded(aPosition, bPosition, cPosition)
                && (!constrainedConnectedComponentId.has_value() || homeShip.GetPoints().GetConnectedComponentId(pointAIndex) == *constrainedConnectedComponentId))
            {
                return triangleIndex;
            }
        }
    }

    return NoneElementIndex;
}

void Npcs::TransferNpcToShip(
    StateType & npc,
    ShipId newShip)
{
    if (npc.CurrentShipId == newShip)
    {
        return;
    }

    //
    // Remove from old ship and add to new ship
    //

    assert(mShips[npc.CurrentShipId].has_value());
    mShips[npc.CurrentShipId]->RemoveNpc(npc.Id);

    assert(mShips[newShip].has_value());
    mShips[newShip]->AddNpc(npc.Id);

    //
    // Maintain stats
    //

    mShips[npc.CurrentShipId]->WorkingNpcStats.Remove(npc);
    mShips[npc.CurrentShipId]->TotalNpcStats.Remove(npc);
    mShips[newShip]->WorkingNpcStats.Add(npc);
    mShips[newShip]->TotalNpcStats.Add(npc);

    //
    // Set ShipId in npc
    //

    npc.CurrentShipId = newShip;
}

void Npcs::PublishHumanNpcStats()
{
    mGameEventHandler->OnHumanNpcCountsUpdated(
        mConstrainedRegimeHumanNpcCount,
        mFreeRegimeHumanNpcCount);
}

void Npcs::RenderNpc(
    StateType const & npc,
    Render::RenderContext & renderContext,
    Render::ShipRenderContext & shipRenderContext) const
{
    switch (renderContext.GetNpcRenderMode())
    {
        case NpcRenderModeType::Texture:
        {
            RenderNpc<NpcRenderModeType::Texture>(npc, renderContext, shipRenderContext);
            break;
        }

        case NpcRenderModeType::QuadWithRoles:
        {
            RenderNpc<NpcRenderModeType::QuadWithRoles>(npc, renderContext, shipRenderContext);
            break;
        }

        case NpcRenderModeType::QuadFlat:
        {
            RenderNpc<NpcRenderModeType::QuadFlat>(npc, renderContext, shipRenderContext);
            break;
        }

#ifdef IN_BARYLAB
        case NpcRenderModeType::Physical:
        {
            // Taken care of elsewhere
            assert(false);
            break;
        }
#endif
    }
}

template<NpcRenderModeType RenderMode>
void Npcs::RenderNpc(
    StateType const & npc,
    Render::RenderContext & renderContext,
    Render::ShipRenderContext & shipRenderContext) const
{
    assert(mShips[npc.CurrentShipId].has_value());

    bool constexpr IsTextureMode = (RenderMode == NpcRenderModeType::Texture);

    switch (npc.Kind)
    {
        case NpcKindType::Human:
        {
            assert(npc.ParticleMesh.Particles.size() == 2);
            assert(npc.ParticleMesh.Springs.size() == 1);
            auto const & humanNpcState = npc.KindSpecificState.HumanNpcState;
            auto const & animationState = humanNpcState.AnimationState;

            // Prepare static attributes
            Render::ShipRenderContext::NpcStaticAttributes staticAttribs{
                (npc.CurrentRegime == StateType::RegimeType::BeingPlaced)
                    ? static_cast<float>(mShips[npc.CurrentShipId]->HomeShip.GetMaxPlaneId())
                    : static_cast<float>(npc.CurrentPlaneId),
                animationState.Alpha,
                npc.IsHighlightedForRendering ? 1.0f : 0.0f,
                animationState.RemovalProgress
            };

            // Geometry:
            //
            //  ---  HeadTopHat (might be == HeadTop)
            //   |
            //  ---  HeadTop                   ---
            //   |                              | HeadLengthFraction
            //  ---  HeadBottom == TorsoTop    ---      ---
            //   |                              |        |   Shoulder offset (magic)
            //  -|-  ArmTop                     |       ---
            //   |                              |
            //   |                              | TorsoLengthFraction
            //   |                              |
            //   |                              |
            //  ---  LegTop                    ---      ---
            //   |                              |        |   Crotch offset (magic)
            //  -|-  TorsoBottom                |       ---
            //   |                              | LegLengthFraction * CrotchHeightMultiplier
            //   |                              |
            //   |                              |
            //  ---  Feet                      ---
            //
            //
            // - All based on current dipole length - anchor points are feet - except for
            //   arm lengths, leg lengths, and all widths, whose values are based on ideal (NPC) height (incl.adjustment),
            //   thus unaffected by current dipole length
            //

            vec2f const feetPosition = mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex);
            vec2f const headPosition = mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex);

            vec2f const actualBodyVector = headPosition - feetPosition; // From feet to head
            float const actualBodyLength = actualBodyVector.length();
            vec2f const actualBodyVDir = -actualBodyVector.normalise_approx(actualBodyLength); // From head to feet - facilitates arm and length angle-making
            vec2f const actualBodyHDir = actualBodyVDir.to_perpendicular(); // Points R (of the screen)

            vec2f const legTop = feetPosition
                + actualBodyVector
                    * (IsTextureMode ? humanNpcState.TextureGeometry.LegLengthFraction : GameParameters::HumanNpcGeometry::LegLengthFraction)
                    * animationState.CrotchHeightMultiplier;
            vec2f const torsoBottom = legTop - actualBodyVector * (GameParameters::HumanNpcGeometry::LegLengthFraction / 20.0f); // Magic hip
            vec2f const torsoTop = legTop
                + actualBodyVector
                    * (IsTextureMode ? humanNpcState.TextureGeometry.TorsoLengthFraction : GameParameters::HumanNpcGeometry::TorsoLengthFraction);
            vec2f const headBottom = torsoTop;
            vec2f const armTop = headBottom - actualBodyVector * (GameParameters::HumanNpcGeometry::ArmLengthFraction / 8.0f); // Magic shoulder
            vec2f const headTop = headBottom
                + actualBodyVector
                    * (IsTextureMode ? humanNpcState.TextureGeometry.HeadLengthFraction : GameParameters::HumanNpcGeometry::HeadLengthFraction);

            float const adjustedIdealHumanHeight = npc.ParticleMesh.Springs[0].RestLength;

            float const headWidthMultiplier = 1.0f + (humanNpcState.WidthMultipier - 1.0f) * 0.5f; // Head doesn'w widen/narrow like body does
            float const headWidthFraction = IsTextureMode
                ? humanNpcState.TextureGeometry.HeadLengthFraction * humanNpcState.TextureGeometry.HeadWHRatio
                : GameParameters::HumanNpcGeometry::QuadModeHeadWidthFraction;
            float const halfHeadW = (adjustedIdealHumanHeight * headWidthFraction * headWidthMultiplier) / 2.0f;

            float const torsoWidthFraction = IsTextureMode
                ? humanNpcState.TextureGeometry.TorsoLengthFraction * humanNpcState.TextureGeometry.TorsoWHRatio
                : GameParameters::HumanNpcGeometry::QuadModeTorsoWidthFraction;
            float const halfTorsoW = (adjustedIdealHumanHeight * torsoWidthFraction * humanNpcState.WidthMultipier) / 2.0f;

            float const leftArmLength =
                adjustedIdealHumanHeight
                * (IsTextureMode ? humanNpcState.TextureGeometry.ArmLengthFraction : GameParameters::HumanNpcGeometry::ArmLengthFraction)
                * animationState.LimbLengthMultipliers.LeftArm;
            float const rightArmLength =
                adjustedIdealHumanHeight
                * (IsTextureMode ? humanNpcState.TextureGeometry.ArmLengthFraction : GameParameters::HumanNpcGeometry::ArmLengthFraction)
                * animationState.LimbLengthMultipliers.RightArm;

            float const armWidthFraction = IsTextureMode
                ? humanNpcState.TextureGeometry.ArmLengthFraction * humanNpcState.TextureGeometry.ArmWHRatio
                : GameParameters::HumanNpcGeometry::QuadModeArmWidthFraction;
            float const halfArmW = (adjustedIdealHumanHeight * armWidthFraction * humanNpcState.WidthMultipier) / 2.0f;

            float const leftLegLength =
                adjustedIdealHumanHeight
                * (IsTextureMode ? humanNpcState.TextureGeometry.LegLengthFraction : GameParameters::HumanNpcGeometry::LegLengthFraction)
                * animationState.LimbLengthMultipliers.LeftLeg;
            float const rightLegLength =
                adjustedIdealHumanHeight
                * (IsTextureMode ? humanNpcState.TextureGeometry.LegLengthFraction : GameParameters::HumanNpcGeometry::LegLengthFraction)
                * animationState.LimbLengthMultipliers.RightLeg;

            float const legWidthFraction = IsTextureMode
                ? humanNpcState.TextureGeometry.LegLengthFraction * humanNpcState.TextureGeometry.LegWHRatio
                : GameParameters::HumanNpcGeometry::QuadModeLegWidthFraction;
            float const halfLegW = (adjustedIdealHumanHeight * legWidthFraction * humanNpcState.WidthMultipier) / 2.0f;

            // Prepare texture coords for quad mode
            float const x = humanNpcState.CurrentFaceDirectionX + humanNpcState.CurrentFaceOrientation;
            assert(x == -1.0f || x == 1.0f);
            Render::TextureCoordinatesQuad quadModeTextureCoordinates = {
                -x, x,
                -1.0f, 1.0f
            };

            // Prepare light
            float const _lowerLight = mParticles.GetLight(npc.ParticleMesh.Particles[0].ParticleIndex);
            std::array<float, 4> const lowerLight = { _lowerLight, _lowerLight, _lowerLight, _lowerLight };
            float const _upperLight = mParticles.GetLight(npc.ParticleMesh.Particles[1].ParticleIndex);
            std::array<float, 4> const upperLight = { _upperLight, _upperLight, _upperLight, _upperLight };

            if (humanNpcState.CurrentFaceOrientation != 0.0f)
            {
                //
                // Front-back
                //

                // Head
                {
                    auto & quad = shipRenderContext.UploadNpcPosition();
                    Geometry::MakeQuadInto(
                        headTop,
                        headBottom,
                        actualBodyHDir,
                        halfHeadW,
                        quad);
                    if constexpr (IsTextureMode)
                        shipRenderContext.UploadNpcTextureAttributes(
                            humanNpcState.CurrentFaceOrientation > 0.0f ? humanNpcState.TextureFrames.HeadFront : humanNpcState.TextureFrames.HeadBack,
                            upperLight,
                            staticAttribs);
                    else
                        shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                            quadModeTextureCoordinates,
                            upperLight,
                            staticAttribs,
                            npc.RenderColor);
                }

                // Arms and legs

                vec2f const leftArmJointPosition = armTop - actualBodyHDir * (halfTorsoW - halfTorsoW / 4.0f);
                vec2f const rightArmJointPosition = armTop + actualBodyHDir * (halfTorsoW - halfTorsoW / 4.0f);
                vec2f const leftLegJointPosition = legTop - actualBodyHDir * halfTorsoW / 4.0f;
                vec2f const rightLegJointPosition = legTop + actualBodyHDir * halfTorsoW / 4.0f;

                if (humanNpcState.CurrentFaceOrientation > 0.0f)
                {
                    // Front

                    // Left arm (on left side of the screen)
                    vec2f const leftArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftArm, animationState.LimbAnglesSin.LeftArm);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            leftArmJointPosition,
                            leftArmJointPosition + leftArmDir * leftArmLength,
                            leftArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.ArmFront,
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Right arm (on right side of the screen)
                    vec2f const rightArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightArm, animationState.LimbAnglesSin.RightArm);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            rightArmJointPosition,
                            rightArmJointPosition + rightArmDir * rightArmLength,
                            rightArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.ArmFront.FlipH(),
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Left leg (on left side of the screen)
                    vec2f const leftLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftLeg, animationState.LimbAnglesSin.LeftLeg);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            leftLegJointPosition,
                            leftLegJointPosition + leftLegDir * leftLegLength,
                            leftLegDir.to_perpendicular(),
                            halfLegW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.LegFront,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Right leg (on right side of the screen)
                    vec2f const rightLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightLeg, animationState.LimbAnglesSin.RightLeg);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            rightLegJointPosition,
                            rightLegJointPosition + rightLegDir * rightLegLength,
                            rightLegDir.to_perpendicular(),
                            halfLegW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.LegFront.FlipH(),
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                }
                else
                {
                    // Back

                    // Left arm (on right side of screen)
                    vec2f const leftArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftArm, -animationState.LimbAnglesSin.LeftArm);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            rightArmJointPosition,
                            rightArmJointPosition + leftArmDir * leftArmLength,
                            leftArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.ArmBack,
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Right arm (on left side of the screen)
                    vec2f const rightArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightArm, -animationState.LimbAnglesSin.RightArm);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            leftArmJointPosition,
                            leftArmJointPosition + rightArmDir * rightArmLength,
                            rightArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.ArmBack.FlipH(),
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Left leg (on right side of the screen)
                    vec2f const leftLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftLeg, -animationState.LimbAnglesSin.LeftLeg);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            rightLegJointPosition,
                            rightLegJointPosition + leftLegDir * leftLegLength,
                            leftLegDir.to_perpendicular(),
                            halfLegW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.LegBack,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Right leg (on left side of the screen)
                    vec2f const rightLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightLeg, -animationState.LimbAnglesSin.RightLeg);
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            leftLegJointPosition,
                            leftLegJointPosition + rightLegDir * rightLegLength,
                            rightLegDir.to_perpendicular(),
                            halfLegW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.TextureFrames.LegBack.FlipH(),
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                }

                // Torso
                {
                    auto & quad = shipRenderContext.UploadNpcPosition();
                    Geometry::MakeQuadInto(
                        torsoTop,
                        torsoBottom,
                        actualBodyHDir,
                        halfTorsoW,
                        quad);
                    if constexpr (IsTextureMode)
                        shipRenderContext.UploadNpcTextureAttributes(
                            humanNpcState.CurrentFaceOrientation > 0.0f ? humanNpcState.TextureFrames.TorsoFront : humanNpcState.TextureFrames.TorsoBack,
                            upperLight,
                            staticAttribs);
                    else
                        shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                            quadModeTextureCoordinates,
                            upperLight,
                            staticAttribs,
                            npc.RenderColor);
                }
            }
            else
            {
                //
                // Left-Right
                //

                struct TextureQuad
                {
                    Quad Position;
                    Render::TextureCoordinatesQuad TextureCoords;

                    TextureQuad() = default;
                };

                // Note: angles are with body vertical, regardless of L/R

                vec2f const leftArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftArm, animationState.LimbAnglesSin.LeftArm);
                vec2f const rightArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightArm, animationState.LimbAnglesSin.RightArm);

                vec2f const leftUpperLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftLeg, animationState.LimbAnglesSin.LeftLeg);
                vec2f const leftUpperLegVector = leftUpperLegDir * leftLegLength * animationState.UpperLegLengthFraction;
                vec2f const leftUpperLegTraverseDir = leftUpperLegDir.to_perpendicular();
                vec2f const leftKneeOrFootPosition = legTop + leftUpperLegVector; // When UpperLegLengthFraction is 1.0 (whole leg), this is the (virtual) foot
                TextureQuad leftUpperLegQuad;
                std::optional<TextureQuad> leftLowerLegQuad;

                vec2f const rightUpperLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightLeg, animationState.LimbAnglesSin.RightLeg);
                vec2f const rightUpperLegVector = rightUpperLegDir * rightLegLength * animationState.UpperLegLengthFraction;
                vec2f const rightUpperLegTraverseDir = rightUpperLegDir.to_perpendicular();
                vec2f const rightKneeOrFootPosition = legTop + rightUpperLegVector; // When UpperLegLengthFraction is 1.0 (whole leg), this is the (virtual) foot
                TextureQuad rightUpperLegQuad;
                std::optional<TextureQuad> rightLowerLegQuad;

                float const lowerLegLengthFraction = 1.0f - animationState.UpperLegLengthFraction;
                if (lowerLegLengthFraction != 0.0f)
                {
                    //
                    // Both upper and lower legs
                    //

                    // When UpperLegLengthFraction=1 (i.e. whole leg), kneeTextureY is bottom;
                    // otherwise, it's in-between top and bottom
                    float const kneeTextureY = IsTextureMode
                        ? (humanNpcState.TextureFrames.LegSide.TopY - animationState.UpperLegLengthFraction * (humanNpcState.TextureFrames.LegSide.TopY - humanNpcState.TextureFrames.LegSide.BottomY))
                        : 1.0f - animationState.UpperLegLengthFraction * 2.0f;

                    Render::TextureCoordinatesQuad const upperLegTextureQuad = IsTextureMode
                        ? Render::TextureCoordinatesQuad({
                            humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.LeftX : humanNpcState.TextureFrames.LegSide.RightX,
                            humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.RightX : humanNpcState.TextureFrames.LegSide.LeftX,
                            kneeTextureY,
                            humanNpcState.TextureFrames.LegSide.TopY })
                        : Render::TextureCoordinatesQuad({
                            quadModeTextureCoordinates.LeftX,
                            quadModeTextureCoordinates.RightX,
                            kneeTextureY,
                            1.0f });


                    Render::TextureCoordinatesQuad const lowerLegTextureQuad = IsTextureMode
                        ? Render::TextureCoordinatesQuad({
                            humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.LeftX : humanNpcState.TextureFrames.LegSide.RightX,
                            humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.RightX : humanNpcState.TextureFrames.LegSide.LeftX,
                            humanNpcState.TextureFrames.LegSide.BottomY,
                            kneeTextureY })
                        : Render::TextureCoordinatesQuad({
                            quadModeTextureCoordinates.LeftX,
                            quadModeTextureCoordinates.RightX,
                            -1.0f,
                            kneeTextureY });

                    // We extrude the corners to make them join nicely to the previous
                    // and next segments. The calculation of the extrusion (J) between two
                    // segments is based on these observations:
                    //  * The direction of the extrusion is along the resultant of the normals
                    //    to the two segments
                    //  * The magnitude of the extrusion is (W/2) / cos(alpha), where alpha is
                    //    the angle between a normal and the direction of the extrusion

                    float constexpr MinJ = 0.8f; // Prevents too pointy angles

                    vec2f const leftLowerLegDir = (feetPosition - leftKneeOrFootPosition).normalise_approx();
                    vec2f const leftLowerLegVector = leftLowerLegDir * leftLegLength * lowerLegLengthFraction;
                    vec2f const leftLowerLegTraverseDir = leftLowerLegDir.to_perpendicular();
                    vec2f const leftLegResultantNormal = (leftUpperLegTraverseDir + leftLowerLegTraverseDir).normalise_approx();
                    vec2f const leftLegJ = leftLegResultantNormal / std::max(MinJ, leftUpperLegTraverseDir.dot(leftLegResultantNormal)) * halfLegW;

                    leftUpperLegQuad = {
                        {
                            legTop - leftUpperLegTraverseDir * halfLegW,
                            leftKneeOrFootPosition - leftLegJ,
                            legTop + leftUpperLegTraverseDir * halfLegW,
                            leftKneeOrFootPosition + leftLegJ
                        },
                        upperLegTextureQuad };

                    leftLowerLegQuad = {
                        {
                            leftKneeOrFootPosition - leftLegJ,
                            leftKneeOrFootPosition + leftLowerLegVector - leftLowerLegTraverseDir * halfLegW,
                            leftKneeOrFootPosition + leftLegJ,
                            leftKneeOrFootPosition + leftLowerLegVector + leftLowerLegTraverseDir * halfLegW
                        },
                        lowerLegTextureQuad };

                    vec2f const rightLowerLegDir = (feetPosition - rightKneeOrFootPosition).normalise_approx();
                    vec2f const rightLowerLegVector = rightLowerLegDir * rightLegLength * lowerLegLengthFraction;
                    vec2f const rightLowerLegTraverseDir = rightLowerLegDir.to_perpendicular();
                    vec2f const rightLegResultantNormal = (rightUpperLegTraverseDir + rightLowerLegTraverseDir).normalise_approx();
                    vec2f const rightLegJ = rightLegResultantNormal / std::max(MinJ, rightUpperLegTraverseDir.dot(rightLegResultantNormal)) * halfLegW;

                    rightUpperLegQuad = {
                        {
                            legTop - rightUpperLegTraverseDir * halfLegW,
                            rightKneeOrFootPosition - rightLegJ,
                            legTop + rightUpperLegTraverseDir * halfLegW,
                            rightKneeOrFootPosition + rightLegJ
                        },
                        upperLegTextureQuad };

                    rightLowerLegQuad = {
                        {
                            rightKneeOrFootPosition - rightLegJ,
                            rightKneeOrFootPosition + rightLowerLegVector - rightLowerLegTraverseDir * halfLegW,
                            rightKneeOrFootPosition + rightLegJ,
                            rightKneeOrFootPosition + rightLowerLegVector + rightLowerLegTraverseDir * halfLegW
                        },
                        lowerLegTextureQuad };
                }
                else
                {
                    // Just upper leg, which is leg in its entirety

                    leftUpperLegQuad = {
                        Geometry::MakeQuad(
                            legTop,
                            leftKneeOrFootPosition,
                            leftUpperLegTraverseDir,
                            halfLegW),
                        IsTextureMode
                            ? humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide : humanNpcState.TextureFrames.LegSide.FlipH()
                            : quadModeTextureCoordinates
                    };

                    rightUpperLegQuad = {
                        Geometry::MakeQuad(
                            legTop,
                            rightKneeOrFootPosition,
                            rightUpperLegTraverseDir,
                            halfLegW),
                        IsTextureMode
                            ? humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide : humanNpcState.TextureFrames.LegSide.FlipH()
                            : quadModeTextureCoordinates
                    };
                }

                // Arm and leg far

                if (humanNpcState.CurrentFaceDirectionX > 0.0f)
                {
                    // Left leg
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = leftUpperLegQuad.Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                leftUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                leftUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                    if (leftLowerLegQuad.has_value())
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = leftLowerLegQuad->Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                leftLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                leftLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Left arm
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            armTop,
                            armTop + leftArmDir * leftArmLength,
                            leftArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.ArmSide : humanNpcState.TextureFrames.ArmSide.FlipH(),
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                }
                else
                {
                    // Right leg
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = rightUpperLegQuad.Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                rightUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                rightUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                    if (rightLowerLegQuad.has_value())
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = rightLowerLegQuad->Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                rightLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                rightLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Right arm
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            armTop,
                            armTop + rightArmDir * rightArmLength,
                            rightArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.ArmSide : humanNpcState.TextureFrames.ArmSide.FlipH(),
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                }

                // Head
                {
                    auto & quad = shipRenderContext.UploadNpcPosition();
                    Geometry::MakeQuadInto(
                        headTop,
                        headBottom,
                        actualBodyHDir,
                        halfHeadW,
                        quad);
                    if constexpr (IsTextureMode)
                        shipRenderContext.UploadNpcTextureAttributes(
                            humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.HeadSide : humanNpcState.TextureFrames.HeadSide.FlipH(),
                            upperLight,
                            staticAttribs);
                    else
                        shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                            quadModeTextureCoordinates,
                            upperLight,
                            staticAttribs,
                            npc.RenderColor);
                }

                // Torso
                {
                    auto & quad = shipRenderContext.UploadNpcPosition();
                    Geometry::MakeQuadInto(
                        torsoTop,
                        torsoBottom,
                        actualBodyHDir,
                        halfTorsoW,
                        quad);
                    if constexpr (IsTextureMode)
                        shipRenderContext.UploadNpcTextureAttributes(
                            humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.TorsoSide : humanNpcState.TextureFrames.TorsoSide.FlipH(),
                            upperLight,
                            staticAttribs);
                    else
                        shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                            quadModeTextureCoordinates,
                            upperLight,
                            staticAttribs,
                            npc.RenderColor);
                }

                // Arm and leg near

                if (humanNpcState.CurrentFaceDirectionX > 0.0f)
                {
                    // Right leg
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = rightUpperLegQuad.Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                rightUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                rightUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                    if (rightLowerLegQuad.has_value())
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = rightLowerLegQuad->Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                rightLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                rightLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Right arm
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            armTop,
                            armTop + rightArmDir * rightArmLength,
                            rightArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.ArmSide : humanNpcState.TextureFrames.ArmSide.FlipH(),
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                }
                else
                {
                    // Left leg
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = leftUpperLegQuad.Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                leftUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                leftUpperLegQuad.TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                    if (leftLowerLegQuad.has_value())
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        quad = leftLowerLegQuad->Position;
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                leftLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                leftLowerLegQuad->TextureCoords,
                                lowerLight,
                                staticAttribs,
                                npc.RenderColor);
                    }

                    // Left arm
                    {
                        auto & quad = shipRenderContext.UploadNpcPosition();
                        Geometry::MakeQuadInto(
                            armTop,
                            armTop + leftArmDir * leftArmLength,
                            leftArmDir.to_perpendicular(),
                            halfArmW,
                            quad);
                        if constexpr (IsTextureMode)
                            shipRenderContext.UploadNpcTextureAttributes(
                                humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.ArmSide : humanNpcState.TextureFrames.ArmSide.FlipH(),
                                upperLight,
                                staticAttribs);
                        else
                            shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                                quadModeTextureCoordinates,
                                upperLight,
                                staticAttribs,
                                npc.RenderColor);
                    }
                }
            }

            // Selection

            if (npc.Id == mCurrentlySelectedNpc)
            {
                vec2f const centerPosition = (headPosition + feetPosition) / 2.0f;

                renderContext.UploadRectSelection(
                    centerPosition,
                    actualBodyVDir,
                    halfTorsoW * 2.0f,
                    actualBodyLength,
                    Render::StockColors::Red1,
                    GameWallClock::GetInstance().ElapsedAsFloat(mCurrentlySelectedNpcWallClockTimestamp));
            }

            break;
        }

        case NpcKindType::Furniture:
        {
            auto const & furnitureNpcState = npc.KindSpecificState.FurnitureNpcState;
            auto const & animationState = furnitureNpcState.AnimationState;

            // Prepare static attributes
            Render::ShipRenderContext::NpcStaticAttributes staticAttribs{
                (npc.CurrentRegime == StateType::RegimeType::BeingPlaced)
                    ? static_cast<float>(mShips[npc.CurrentShipId]->HomeShip.GetMaxPlaneId())
                    : static_cast<float>(npc.CurrentPlaneId),
                animationState.Alpha,
                npc.IsHighlightedForRendering ? 1.0f : 0.0f,
                animationState.RemovalProgress
            };

            Render::TextureCoordinatesQuad textureCoords;
            if constexpr (RenderMode == NpcRenderModeType::Texture)
            {
                textureCoords = furnitureNpcState.CurrentFaceDirectionX > 0.0f ?
                    furnitureNpcState.TextureCoordinatesQuad
                    : furnitureNpcState.TextureCoordinatesQuad.FlipH();
            }
            else
            {
                textureCoords = {
                    -furnitureNpcState.CurrentFaceDirectionX, furnitureNpcState.CurrentFaceDirectionX,
                    -1.0f, 1.0f
                };
            }

            if (npc.ParticleMesh.Particles.size() == 4)
            {
                // Just one quad
                auto & quad = shipRenderContext.UploadNpcPosition();
                quad.V.TopLeft = mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex);
                quad.V.TopRight = mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex);
                quad.V.BottomRight = mParticles.GetPosition(npc.ParticleMesh.Particles[2].ParticleIndex),
                quad.V.BottomLeft = mParticles.GetPosition(npc.ParticleMesh.Particles[3].ParticleIndex);

                std::array<float, 4> const light = {
                    mParticles.GetLight(npc.ParticleMesh.Particles[0].ParticleIndex),
                    mParticles.GetLight(npc.ParticleMesh.Particles[1].ParticleIndex),
                    mParticles.GetLight(npc.ParticleMesh.Particles[2].ParticleIndex),
                    mParticles.GetLight(npc.ParticleMesh.Particles[3].ParticleIndex) };

                if constexpr (RenderMode == NpcRenderModeType::Texture)
                    shipRenderContext.UploadNpcTextureAttributes(
                        textureCoords,
                        light,
                        staticAttribs);
                else
                    shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                        textureCoords,
                        light,
                        staticAttribs,
                        npc.RenderColor);
            }
            else
            {
                // Bunch-of-particles (each a quad)

                for (auto const & particle : npc.ParticleMesh.Particles)
                {
                    float constexpr ParticleHalfWidth  = ParticleSize / 2.0f;
                    vec2f const position = mParticles.GetPosition(particle.ParticleIndex);
                    float const _light = mParticles.GetLight(particle.ParticleIndex);
                    std::array<float, 4> const light = { _light, _light, _light, _light };

                    auto & quad = shipRenderContext.UploadNpcPosition();
                    quad.V.TopLeft = vec2f(position.x - ParticleHalfWidth, position.y + ParticleHalfWidth);
                    quad.V.TopRight = vec2f(position.x + ParticleHalfWidth, position.y + ParticleHalfWidth);
                    quad.V.BottomLeft = vec2f(position.x - ParticleHalfWidth, position.y - ParticleHalfWidth);
                    quad.V.BottomRight = vec2f(position.x + ParticleHalfWidth, position.y - ParticleHalfWidth);
                    if constexpr (RenderMode == NpcRenderModeType::Texture)
                        shipRenderContext.UploadNpcTextureAttributes(
                            textureCoords,
                            light,
                            staticAttribs);
                    else
                        shipRenderContext.UploadNpcQuadAttributes<RenderMode>(
                            textureCoords,
                            light,
                            staticAttribs,
                            npc.RenderColor);
                }
            }

            // Selection

            if (npc.Id == mCurrentlySelectedNpc)
            {
                // Calculate center position
                vec2f centerPosition = vec2f::zero();
                for (auto const & particle : npc.ParticleMesh.Particles)
                {
                    centerPosition += mParticles.GetPosition(particle.ParticleIndex);
                }
                centerPosition /= static_cast<float>(npc.ParticleMesh.Particles.size());

                // Calculate vertical dir
                vec2f verticalDir;
                if (npc.ParticleMesh.Particles.size() > 1)
                {
                    // Take arbitrarily normal to first two particles' positions
                    vec2f const firstVector =
                        mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex)
                        - mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex);
                    verticalDir = firstVector.normalise_approx().to_perpendicular();
                }
                else
                {
                    verticalDir = vec2f(0.0f, -1.0f);
                }

                // Calculate dimensions
                float const width = std::max(
                    CalculateSpringLength(
                        mNpcDatabase.GetFurnitureGeometry(npc.KindSpecificState.FurnitureNpcState.SubKindId).Width,
                        mCurrentSizeMultiplier),
                    ParticleSize);
                float const height = std::max(
                    CalculateSpringLength(
                        mNpcDatabase.GetFurnitureGeometry(npc.KindSpecificState.FurnitureNpcState.SubKindId).Height,
                        mCurrentSizeMultiplier),
                    ParticleSize);

                renderContext.UploadRectSelection(
                    centerPosition,
                    verticalDir,
                    width,
                    height,
                    Render::StockColors::Red1,
                    GameWallClock::GetInstance().ElapsedAsFloat(mCurrentlySelectedNpcWallClockTimestamp));
            }

            break;
        }
    }

    // Reset highlight state
    npc.IsHighlightedForRendering = false;
}

void Npcs::UpdateFurnitureNpcAnimation(
    StateType & npc,
    float currentSimulationTime,
    Ship const & /*homeShip*/)
{
    assert(npc.Kind == NpcKindType::Furniture);

    auto & furnitureNpcState = npc.KindSpecificState.FurnitureNpcState;
    auto & animationState = furnitureNpcState.AnimationState;
    using FurnitureNpcStateType = StateType::KindSpecificStateType::FurnitureNpcStateType;

    switch (furnitureNpcState.CurrentBehavior)
    {
        case FurnitureNpcStateType::BehaviorType::BeingRemoved:
        {
            // Alpha and RemovalProgress

            float const elapsed = currentSimulationTime - furnitureNpcState.CurrentStateTransitionSimulationTimestamp;

            animationState.RemovalProgress = std::min(elapsed / FurnitureRemovalDuration, 1.0f);
            animationState.Alpha = 1.0f - animationState.RemovalProgress;

            break;
        }

        case FurnitureNpcStateType::BehaviorType::Default:
        {
            // Nop
            break;
        }
    }
}

void Npcs::UpdateHumanNpcAnimation(
    StateType & npc,
    float currentSimulationTime,
    Ship const & homeShip)
{
    assert(npc.Kind == NpcKindType::Human);

    auto & humanNpcState = npc.KindSpecificState.HumanNpcState;
    auto & animationState = humanNpcState.AnimationState;
    using HumanNpcStateType = StateType::KindSpecificStateType::HumanNpcStateType;

    assert(npc.ParticleMesh.Particles.size() == 2);
    assert(npc.ParticleMesh.Springs.size() == 1);
    ElementIndex const primaryParticleIndex = npc.ParticleMesh.Particles[0].ParticleIndex;
    auto const & primaryContrainedState = npc.ParticleMesh.Particles[0].ConstrainedState;
    ElementIndex const secondaryParticleIndex = npc.ParticleMesh.Particles[1].ParticleIndex;

    //
    // Angles and thigh
    //

    // Target: begin with current
    FS_ALIGN16_BEG LimbVector targetAngles FS_ALIGN16_END = LimbVector(animationState.LimbAngles);

    float convergenceRate = 0.0f;

    // Stuff we calc in some cases and which we need again later for lengths
    float humanEdgeAngle = 0.0f;
    float adjustedStandardHumanHeight = 0.0f;
    vec2f edgp1 = vec2f::zero(), edgp2 = vec2f::zero(), edgVector, edgDir;
    vec2f feetPosition = vec2f::zero(), actualBodyVector = vec2f::zero(), actualBodyDir = vec2f::zero();
    float periodicValue = 0.0f;

    float targetUpperLegLengthFraction = 1.0f;

    // Angle of human wrt edge until which arm is angled to the max
    // (extent of early stage during rising)
    float constexpr MaxHumanEdgeAngleForArms = 0.40489178628508342331207292900944f;
    //static_assert(MaxHumanEdgeAngleForArms == std::atanf(GameParameters::HumanNpcGeometry::ArmLengthFraction / (1.0f - GameParameters::HumanNpcGeometry::HeadLengthFraction)));

    switch (humanNpcState.CurrentBehavior)
    {
        case HumanNpcStateType::BehaviorType::BeingPlaced:
        {
            // Being-placed dance

            float const arg =
                (
                    (currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp) * 1.0f
                    + humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition * 0.2f
                    ) * (1.0f + humanNpcState.ResultantPanicLevel * 0.2f)
                * (Pi<float> *2.0f + npc.RandomNormalizedUniformSeed * 4.0f);

            float const yArms = std::sinf(arg);
            targetAngles.RightArm = Pi<float> / 2.0f + Pi<float> / 2.0f * 0.7f * yArms;
            targetAngles.LeftArm = -targetAngles.RightArm;

            float const yLegs = std::sinf(arg + npc.RandomNormalizedUniformSeed * Pi<float> *2.0f);
            targetAngles.RightLeg = (1.0f + yLegs) / 2.0f * Pi<float> / 2.0f * 0.3f;
            targetAngles.LeftLeg = -targetAngles.RightLeg;

            convergenceRate = 0.3f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_PreRising:
        {
            // Move arms against floor (PI/2 wrt body)

            if (primaryContrainedState.has_value() && primaryContrainedState->CurrentVirtualFloor.has_value())
            {
                vec2f const edgeVector = homeShip.GetTriangles().GetSubSpringVector(
                    primaryContrainedState->CurrentVirtualFloor->TriangleElementIndex,
                    primaryContrainedState->CurrentVirtualFloor->EdgeOrdinal,
                    homeShip.GetPoints());
                vec2f const head = mParticles.GetPosition(secondaryParticleIndex);
                vec2f const feet = mParticles.GetPosition(primaryParticleIndex);

                float const humanFloorAlignment = (head - feet).dot(edgeVector);

                float constexpr MaxArmAngle = Pi<float> / 2.0f;
                float constexpr OtherArmDeltaAngle = 0.3f;

                if (humanFloorAlignment >= 0.0f)
                {
                    targetAngles.LeftArm = -MaxArmAngle;
                    targetAngles.RightArm = -MaxArmAngle + OtherArmDeltaAngle;
                }
                else
                {
                    targetAngles.RightArm = MaxArmAngle;
                    targetAngles.LeftArm = MaxArmAngle - OtherArmDeltaAngle;
                }
            }

            // Legs at zero
            targetAngles.LeftLeg = 0.0f;
            targetAngles.RightLeg = 0.0f;

            convergenceRate = 0.09f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Rising:
        {
            //
            // Leg and arm that are against floor "help"
            //

            if (primaryContrainedState.has_value() && primaryContrainedState->CurrentVirtualFloor.has_value())
            {
                // Remember the virtual edge that we're rising against, so we can survive
                // small bursts of being off the edge
                humanNpcState.CurrentBehaviorState.Constrained_Rising.VirtualEdgeRisingAgainst = *primaryContrainedState->CurrentVirtualFloor;
            }

            if (humanNpcState.CurrentBehaviorState.Constrained_Rising.VirtualEdgeRisingAgainst.TriangleElementIndex != NoneElementIndex)
            {
                // Calculate edge vector
                vec2f const edgeVector = homeShip.GetTriangles().GetSubSpringVector(
                    humanNpcState.CurrentBehaviorState.Constrained_Rising.VirtualEdgeRisingAgainst.TriangleElementIndex,
                    humanNpcState.CurrentBehaviorState.Constrained_Rising.VirtualEdgeRisingAgainst.EdgeOrdinal,
                    homeShip.GetPoints());
                vec2f const head = mParticles.GetPosition(secondaryParticleIndex);
                vec2f const feet = mParticles.GetPosition(primaryParticleIndex);

                // First off, we calculate the max possible human-edge vector, considering that
                // human converges towards full vertical (gravity-only :-( )
                float maxHumanEdgeAngle = edgeVector.angleCw(vec2f(0.0f, 1.0f)); // Also angle between edge and vertical

                // Calculate angle between human and edge (angle that we need to rotate human CW to get onto edge)
                humanEdgeAngle = edgeVector.angleCw(head - feet); // [0.0 ... PI]
                if (humanEdgeAngle < 0.0f)
                {
                    // Two possible inaccuracies here:
                    // o -8.11901e-06: this is basically 0.0
                    // o -3.14159: this is basically +PI

                    if (humanEdgeAngle >= -Pi<float> / 2.0f) // Just sentinel for side of inaccuracy
                    {
                        humanEdgeAngle = 0.0f;
                    }
                    else
                    {
                        humanEdgeAngle = Pi<float>;
                    }
                }

                bool isOnLeftSide; // Of screen - i.e. head to the left side of the edge (exploiting CWness of edge)
                if (humanEdgeAngle <= maxHumanEdgeAngle)
                {
                    isOnLeftSide = true;
                }
                else
                {
                    isOnLeftSide = false;

                    // Normalize to simplify math below
                    humanEdgeAngle = Pi<float> -humanEdgeAngle;
                    maxHumanEdgeAngle = Pi<float> -maxHumanEdgeAngle;
                }

                // Max angle of arm wrt body - kept until MaxAngle
                float constexpr MaxArmAngle = Pi<float> / 2.0f;

                // Rest angle of arm wrt body - reached when fully erect
                float constexpr RestArmAngle = HumanNpcStateType::AnimationStateType::InitialArmAngle * 0.3f;

                // DeltaAngle of other arm
                float constexpr OtherArmDeltaAngle = 0.3f;

                // AngleMultiplier of other leg when closing knees
                float constexpr OtherLegAlphaAngle = 0.87f;

                //  *  0 --> maxHumanEdgeAngle (which is PI/2 when edge is flat)
                //   \
                //   |\
                // -----

                //
                // Arm: at MaxArmAngle until MaxHumanEdgeAngleForArms, then goes down to rest
                //

                float targetArm;
                if (humanEdgeAngle <= MaxHumanEdgeAngleForArms)
                {
                    // Early stage

                    // Arms: leave them where they are (MaxArmAngle)
                    targetArm = MaxArmAngle;
                }
                else
                {
                    // Late stage: -> towards maxHumanEdgeAngle

                    // Arms: MaxArmAngle -> RestArmAngle
                    targetArm = MaxArmAngle + (MaxHumanEdgeAngleForArms - humanEdgeAngle) / (MaxHumanEdgeAngleForArms - maxHumanEdgeAngle) * (RestArmAngle - MaxArmAngle); // MaxArmAngle @ MaxHumanEdgeAngleForArms -> RestArmAngle @ maxHumanEdgeAngle
                }

                //
                // Legs: various phases: knee bending, then straightening
                //
                // Note: we only do legs if we're facing L/R
                //

                float targetLeg = 0.0f; // Start with legs closed - we'll change (if we're L/R)
                if (humanNpcState.CurrentFaceOrientation == 0.0f)
                {
                    float constexpr angle1 = MaxHumanEdgeAngleForArms * 0.9f; // Leg towards LegAngle0 until here
                    float constexpr LegAngle0 = Pi<float> * 0.37f;
                    float constexpr angle2 = MaxHumanEdgeAngleForArms * 1.5f; // Rest until here
                    float const angle3 = angle2 + (maxHumanEdgeAngle - angle2) * 5.0f / 6.0f; // Leg shrinking to zero until here; rest afterwards

                    if (humanEdgeAngle < angle1)
                    {
                        // Rise
                        targetLeg = humanEdgeAngle / angle1 * LegAngle0;
                        targetUpperLegLengthFraction = 0.5f;
                    }
                    else if (humanEdgeAngle < angle2)
                    {
                        // Rest
                        targetLeg = LegAngle0;
                        targetUpperLegLengthFraction = 0.5f;
                    }
                    else if (humanEdgeAngle < angle3)
                    {
                        // Decrease
                        targetLeg = LegAngle0 * (1.0f - (humanEdgeAngle - angle2) / (angle3 - angle2));
                        targetUpperLegLengthFraction = 0.5f;
                    }
                    else
                    {
                        // Zero
                        targetLeg = 0.0f;
                        targetUpperLegLengthFraction = 0.0f;
                    }

                    // Knees cannot bend backwards!
                    if ((humanNpcState.CurrentFaceDirectionX > 0.0f && isOnLeftSide)
                        || (humanNpcState.CurrentFaceDirectionX < 0.0f && !isOnLeftSide))
                    {
                        // Less angle on the opposite side
                        targetLeg *= -0.8f;
                    }
                }

                //
                // Finalize angles
                //

                if (isOnLeftSide)
                {
                    targetAngles.LeftArm = -targetArm;
                    targetAngles.RightArm = targetAngles.LeftArm + OtherArmDeltaAngle;

                    targetAngles.LeftLeg = -targetLeg;
                    targetAngles.RightLeg = targetAngles.LeftLeg * OtherLegAlphaAngle;
                }
                else
                {
                    targetAngles.RightArm = targetArm;
                    targetAngles.LeftArm = targetAngles.RightArm - OtherArmDeltaAngle;

                    targetAngles.RightLeg = targetLeg;
                    targetAngles.LeftLeg = targetAngles.RightLeg * OtherLegAlphaAngle;
                }
            }
            else
            {
                // Let's survive small bursts and keep current angles; after all we'll lose
                // this state very quickly if the burst is too long

                targetUpperLegLengthFraction = animationState.UpperLegLengthFraction;
            }

            convergenceRate = 0.45f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Equilibrium:
        {
            // Just small arms angle

            float constexpr ArmsAngle = HumanNpcStateType::AnimationStateType::InitialArmAngle;

            targetAngles.RightArm = ArmsAngle;
            targetAngles.LeftArm = -ArmsAngle;

            targetAngles.RightLeg = 0.0f;
            targetAngles.LeftLeg = 0.0f;

            convergenceRate = 0.1f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Walking:
        {
            //
            // Calculate leg angle based on distance traveled
            //

            // Add some dependency on walking speed
            float const actualWalkingSpeed = CalculateActualHumanWalkingAbsoluteSpeed(humanNpcState);
            float const MaxLegAngle =
                0.41f // std::atanf((GameParameters::HumanNpcGeometry::StepLengthFraction / 2.0f) / GameParameters::HumanNpcGeometry::LegLengthFraction)
                * std::sqrt(actualWalkingSpeed * 0.9f);

            adjustedStandardHumanHeight = npc.ParticleMesh.Springs[0].RestLength;
            float const stepLength = GameParameters::HumanNpcGeometry::StepLengthFraction * adjustedStandardHumanHeight;
            float const distance =
                humanNpcState.TotalDistanceTraveledOnEdgeSinceStateTransition
                + 0.3f * humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition;
            float const distanceInTwoSteps = FastMod(distance + 3.0f * stepLength / 2.0f, stepLength * 2.0f);

            float const legAngle = std::abs(stepLength - distanceInTwoSteps) / stepLength * 2.0f * MaxLegAngle - MaxLegAngle;

            targetAngles.RightLeg = legAngle;
            targetAngles.LeftLeg = -legAngle;

            // Arms depend on panic
            if (humanNpcState.ResultantPanicLevel < 0.32f)
            {
                // No panic: arms aperture depends on speed

                // At base speed (1m/s): 1.4
                // Swing more
                float const apertureMultiplier = 1.4f + (actualWalkingSpeed - 1.0f) * 0.4f;
                targetAngles.RightArm = targetAngles.LeftLeg * apertureMultiplier;
            }
            else
            {
                // Panic: arms raised up

                float const elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;
                float const halfPeriod = 1.0f - 0.6f * std::min(humanNpcState.ResultantPanicLevel, 4.0f) / 4.0f; // 1.0 @ no panic, 0.4 @ panic
                float const inPeriod = FastMod(elapsed, halfPeriod * 2.0f);

                float constexpr MaxAngle = Pi<float> / 2.0f;
                float const angle = std::abs(halfPeriod - inPeriod) / halfPeriod * 2.0f * MaxAngle - MaxAngle;

                // PanicMultiplier: p=0.0 => 0.7; p=2.0 => 0.4
                float const panicMultiplier = 0.4f + 0.3f * (1.0f - std::min(humanNpcState.ResultantPanicLevel, 2.0f) / 2.0f);
                targetAngles.RightArm = Pi<float> -(angle * panicMultiplier);
            }
            targetAngles.LeftArm = -targetAngles.RightArm;

            convergenceRate = 0.25f;

            if (primaryContrainedState.has_value() && primaryContrainedState->CurrentVirtualFloor.has_value())
            {
                //
                // We are walking on an edge - make sure feet don't look weird on sloped edges
                //

                // Calculate edge vector
                // Note: we do not care if not in CW order
                auto const t = primaryContrainedState->CurrentVirtualFloor->TriangleElementIndex;
                auto const e = primaryContrainedState->CurrentVirtualFloor->EdgeOrdinal;
                auto const p1 = homeShip.GetTriangles().GetPointIndices(t)[e];
                auto const p2 = homeShip.GetTriangles().GetPointIndices(t)[(e + 1) % 3];
                edgp1 = homeShip.GetPoints().GetPosition(p1);
                edgp2 = homeShip.GetPoints().GetPosition(p2);
                edgVector = edgp2 - edgp1;
                edgDir = edgVector.normalise_approx();

                //
                // 1. Limit leg angles if on slope
                //

                vec2f const headPosition = mParticles.GetPosition(secondaryParticleIndex);
                feetPosition = mParticles.GetPosition(primaryParticleIndex);
                actualBodyVector = feetPosition - headPosition; // From head to feet
                actualBodyDir = actualBodyVector.normalise_approx();

                float const bodyToVirtualEdgeAlignment = std::abs(edgDir.dot(actualBodyDir.to_perpendicular()));
                float const angleLimitFactor = bodyToVirtualEdgeAlignment * bodyToVirtualEdgeAlignment * bodyToVirtualEdgeAlignment;
                targetAngles.RightLeg *= angleLimitFactor;
                targetAngles.LeftLeg *= angleLimitFactor;
            }

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_WalkingUndecided:
        {
            float constexpr PhaseDurationFraction = 0.2f;

            //
            // Arms:
            //     fraction of duration : arms rising up
            //     fraction of duration : arms falling down
            //     remaining : nothing
            //

            float constexpr MaxArmAngle = Pi<float> / 2.0f * 0.75f;

            float const elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;

            float armAngle;
            if (elapsed < WalkingUndecidedDuration * PhaseDurationFraction)
            {
                armAngle = MaxArmAngle * elapsed / (WalkingUndecidedDuration * PhaseDurationFraction);
                convergenceRate = 0.15f;
            }
            else
            {
                armAngle = 0.0f;
                convergenceRate = 0.09f;
            }

            targetAngles.RightArm = armAngle;
            targetAngles.LeftArm = -armAngle;

            //
            // Legs:
            //     closed
            //

            targetAngles.RightLeg = 0.1f;
            targetAngles.LeftLeg = -0.1f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Electrified:
        {
            // Random dance with silly fast movements

            float const elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;
            int absolutePhase = static_cast<int>((elapsed + (2.0f + npc.RandomNormalizedUniformSeed) * 3.0f) / 0.09f);

            // Arms

            static float rArmAngles[5] = { Pi<float> / 2.0f, 3.0f * Pi<float> / 4.0f, Pi<float> / 5.0f, Pi<float> -0.01f, Pi<float> / 4.0f };
            static float lArmAngles[5] = { Pi<float> -0.01f, Pi<float> / 4.0f, Pi<float> / 2.0f, Pi<float> / 5.0f, 3.0f * Pi<float> / 4.0f };

            targetAngles.RightArm = rArmAngles[absolutePhase % 5];
            targetAngles.LeftArm = -lArmAngles[absolutePhase % 5];

            // Legs

            static float rLegAngles[4] = { Pi<float> / 2.0f, 0.0f, Pi<float> / 4.0f, 0.0f };
            static float lLegAngles[4] = { 0.0f, Pi<float> / 4.0f, 0.0f, Pi<float> / 2.0f };

            targetAngles.RightLeg = rLegAngles[absolutePhase % 4];
            targetAngles.LeftLeg = -lLegAngles[absolutePhase % 4];

            convergenceRate = 0.5f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Falling:
        {
            // Both arms in direction of face, depending on head velocity in that direction

            vec2f const headPosition = mParticles.GetPosition(secondaryParticleIndex);
            feetPosition = mParticles.GetPosition(primaryParticleIndex);
            actualBodyVector = feetPosition - headPosition; // From head to feet
            actualBodyDir = actualBodyVector.normalise_approx();

            // The extent to which we move arms depends on the avg velocity or head+feet

            vec2f const & headVelocity = npc.ParticleMesh.Particles[1].GetApplicableVelocity(mParticles);
            vec2f const & feetVelocity = npc.ParticleMesh.Particles[0].GetApplicableVelocity(mParticles);

            float const avgVelocityAlongBodyPerp = ((headVelocity + feetVelocity) / 2.0f).dot(actualBodyDir.to_perpendicular()); // When positive points to the right of the human vector
            float const targetDepth = LinearStep(0.0f, 0.8f, std::abs(avgVelocityAlongBodyPerp));

            if (humanNpcState.CurrentFaceDirectionX >= 0.0f)
            {
                targetAngles.RightArm = Pi<float> / 2.0f * targetDepth + 0.04f;
                targetAngles.LeftArm = targetAngles.RightArm - 0.08f;
            }
            else
            {
                targetAngles.LeftArm = -Pi<float> / 2.0f * targetDepth - 0.04f;
                targetAngles.RightArm = targetAngles.LeftArm + 0.08f;
            }

            // ~Close legs
            targetAngles.RightLeg = 0.05f;
            targetAngles.LeftLeg = -0.05f;

            convergenceRate = 0.1f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_KnockedOut:
        case HumanNpcStateType::BehaviorType::Free_KnockedOut:
        {
            // Arms: +/- PI or 0, depending on where they are now

            if (animationState.LimbAngles.RightArm >= -Pi<float> / 2.0f
                && animationState.LimbAngles.RightArm <= Pi<float> / 2.0f)
            {
                targetAngles.RightArm = 0.0f;
            }
            else
            {
                targetAngles.RightArm = Pi<float>;
            }

            if (animationState.LimbAngles.LeftArm >= -Pi<float> / 2.0f
                && animationState.LimbAngles.LeftArm <= Pi<float> / 2.0f)
            {
                targetAngles.LeftArm = 0.0f;
            }
            else
            {
                targetAngles.LeftArm = -Pi<float>;
            }

            // Legs: 0

            targetAngles.RightLeg = 0.0f;
            targetAngles.LeftLeg = 0.0f;

            convergenceRate = 0.2f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Aerial:
        case HumanNpcStateType::BehaviorType::Constrained_InWater:
        case HumanNpcStateType::BehaviorType::Free_Aerial:
        case HumanNpcStateType::BehaviorType::Free_InWater:
        {
            //
            // Rag doll
            //

            vec2f const headPosition = mParticles.GetPosition(secondaryParticleIndex);
            feetPosition = mParticles.GetPosition(primaryParticleIndex);
            actualBodyVector = feetPosition - headPosition;
            actualBodyDir = actualBodyVector.normalise_approx();

            // Arms: always up, unless horizontal or foot on the floor, in which case PI/2

            float const horizontality = std::abs(actualBodyDir.dot(GameParameters::GravityDir));

            float constexpr exceptionAngle = Pi<float> / 1.5f;
            float const armAngle = (primaryContrainedState.has_value() && primaryContrainedState->CurrentVirtualFloor.has_value())
                ? exceptionAngle
                : Pi<float> -(Pi<float> -exceptionAngle) / std::exp(horizontality * 2.2f);
            targetAngles.RightArm = armAngle;
            targetAngles.LeftArm = -targetAngles.RightArm;

            // Legs: inclined in direction opposite of resvel, by an amount proportional to resvel itself
            //
            // Res vel to the right (>0) => legs to the left
            // Res vel to the left (<0) => legs to the right

            vec2f const resultantVelocity = (mParticles.GetVelocity(primaryParticleIndex) + mParticles.GetVelocity(secondaryParticleIndex)) / 2.0f;
            float const resVelPerpToBody = resultantVelocity.dot(actualBodyDir.to_perpendicular()); // Positive when pointing towards right
            float const legAngle =
                SmoothStep(0.0f, 4.0f, std::abs(resVelPerpToBody)) * 0.8f
                * (resVelPerpToBody >= 0.0f ? -1.0f : 1.0f);
            float constexpr LegAperture = 0.6f;
            targetAngles.RightLeg = legAngle + LegAperture / 2.0f;
            targetAngles.LeftLeg = legAngle - LegAperture / 2.0f;

            convergenceRate = 0.1f;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Swimming_Style1:
        case HumanNpcStateType::BehaviorType::Free_Swimming_Style1:
        {
            //
            // Arms and legs up<->down
            //

            //
            // 1 period:
            //
            //  _----|         1.0
            // /     \
            // |      \_____|  0.0
            //              |
            //

            float constexpr Period1 = 3.00f;
            float constexpr Period2 = 1.00f;

            float elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;
            // Prolong first period
            float constexpr ActualLeadInTime = 6.0f;
            if (elapsed < ActualLeadInTime)
            {
                elapsed = elapsed / ActualLeadInTime * Period1;
            }
            else
            {
                elapsed = elapsed - Period1;
            }

            float const panicAccelerator = 1.0f + std::min(humanNpcState.ResultantPanicLevel, 2.0f) / 2.0f * 4.0f;

            float const arg =
                Period1 / 2.0f // Start some-halfway-through to avoid sudden extreme angles
                + elapsed * 2.6f * panicAccelerator
                + humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition * 0.7f;

            float const inPeriod = FastMod(arg, (Period1 + Period2));
            // y: [0.0 ... 1.0]
            float const y = (inPeriod < Period1)
                ? std::sqrt(inPeriod / Period1)
                : ((inPeriod - Period1) - Period2) * ((inPeriod - Period1) - Period2) / std::sqrt(Period2);

            // 0: 0, 2: 1, >+ INF: 1
            float const depthDamper = Clamp(mParentWorld.GetOceanSurface().GetDepth(mParticles.GetPosition(secondaryParticleIndex)) / 1.5f, 0.0f, 1.0f);

            // Arms: flapping around PI/2, with amplitude depending on depth
            float constexpr ArmAngleAmplitude = 2.9f; // Half of this on each side of center angle
            float const armCenterAngle = Pi<float> / 2.0f;
            float const armAngle =
                armCenterAngle
                + (y * 2.0f - 1.0f) * ArmAngleAmplitude / 2.0f * (depthDamper * 0.75f + 0.25f);
            targetAngles.RightArm = armAngle;
            targetAngles.LeftArm = -targetAngles.RightArm;

            // Legs:flapping around a (small) angle, which becomes even smaller
            // width depth amplitude depending on depth
            float constexpr LegAngleAmplitude = 0.25f * 2.0f; // Half of this on each side of center angle
            float const legCenterAngle = 0.25f * (depthDamper * 0.5f + 0.5f);
            float const legAngle =
                legCenterAngle
                + (y * 2.0f - 1.0f) * LegAngleAmplitude / 2.0f * (depthDamper * 0.35f + 0.65f);
            targetAngles.RightLeg = legAngle;
            targetAngles.LeftLeg = -targetAngles.RightLeg;

            // Convergence rate depends on how long we've been in this state
            float const MaxConvergenceWait = 3.5f;
            convergenceRate = 0.01f + Clamp(elapsed, 0.0f, MaxConvergenceWait) / MaxConvergenceWait * (0.25f - 0.01f);

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Swimming_Style2:
        {
            //
            // Arms alternating (narrowly) around normal to body (direction of face)
            // Legs alternating (narrowly) around opposite of feet velocity dir
            //
            // We are facing left or right
            //

            float constexpr Period = 3.00f;

            float elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;

            float const arg =
                elapsed * 2.3f
                + humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition * 0.7f;

            float const inPeriod = FastMod(arg, Period);

            // y: [0.0 ... 1.0]
            float const y = (inPeriod < Period / 2.0f)
                ? inPeriod / (Period / 2.0f)
                : 1.0f - (inPeriod - Period / 2.0f) / (Period / 2.0f);

            // Arms

            float const armCenterAngle = humanNpcState.CurrentFaceDirectionX * Pi<float> / 2.0f;
            float const armAperture = Pi<float> / 2.0f * (y - 0.5f);
            targetAngles.RightArm = armCenterAngle + armAperture;
            targetAngles.LeftArm = armCenterAngle - armAperture;

            // Legs

            vec2f const headPosition = mParticles.GetPosition(secondaryParticleIndex);
            feetPosition = mParticles.GetPosition(primaryParticleIndex);
            actualBodyVector = feetPosition - headPosition; // From head to feet

            // Angle between velocity and body
            assert(npc.ParticleMesh.Particles[0].ConstrainedState.has_value());
            vec2f const & feetVelocity = npc.ParticleMesh.Particles[0].ConstrainedState->MeshRelativeVelocity;
            float const velocityAngleWrtBody = actualBodyVector.angleCw(feetVelocity);

            // Leg center angle: opposite to velocity, but never too orthogonal
            float constexpr MaxLegCenterAngle = Pi<float> / 3.0f;
            float legCenterAngle;
            if (velocityAngleWrtBody >= 0.0f)
            {
                legCenterAngle = std::min(Pi<float> - velocityAngleWrtBody, MaxLegCenterAngle);
            }
            else
            {
                legCenterAngle = std::max(-Pi<float> -velocityAngleWrtBody, -MaxLegCenterAngle);
            }
            legCenterAngle *= LinearStep(0.0f, 3.0f, feetVelocity.length());

            targetAngles.RightLeg = legCenterAngle + armAperture;
            targetAngles.LeftLeg = legCenterAngle - armAperture;

            float const MaxConvergenceWait = 2.0f;
            convergenceRate = 0.01f + Clamp(elapsed, 0.0f, MaxConvergenceWait) / MaxConvergenceWait * (0.2f - 0.01f);

            break;
        }

        case HumanNpcStateType::BehaviorType::Free_Swimming_Style2:
        {
            //
            // Trappelen
            //

            float constexpr Period = 2.00f;

            float const elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;
            float const panicAccelerator = 1.0f + std::min(humanNpcState.ResultantPanicLevel, 2.0f) / 2.0f * 1.0f;

            float const arg =
                elapsed * 2.6f * panicAccelerator
                + humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition * 0.7f;

            float const inPeriod = FastMod(arg, Period);
            // periodicValue: [0.0 ... 1.0]
            periodicValue = (inPeriod < Period / 2.0f)
                ? inPeriod / (Period / 2.0f)
                : 1.0f - (inPeriod - (Period / 2.0f)) / (Period / 2.0f);

            // Arms: around a small angle
            targetAngles.RightArm = StateType::KindSpecificStateType::HumanNpcStateType::AnimationStateType::InitialArmAngle + (periodicValue - 0.5f) * Pi<float> / 8.0f;
            targetAngles.LeftArm = -targetAngles.RightArm;

            // Legs: perfectly vertical
            targetAngles.RightLeg = 0.0f;
            targetAngles.LeftLeg = 0.0f;

            // Convergence rate depends on how long we've been in this state
            float const MaxConvergenceWait = 3.5f;
            convergenceRate = 0.01f + Clamp(elapsed, 0.0f, MaxConvergenceWait) / MaxConvergenceWait * (0.25f - 0.01f);

            break;
        }

        case HumanNpcStateType::BehaviorType::Free_Swimming_Style3:
        {
            //
            // Trappelen
            //

            float constexpr Period = 2.00f;

            float const elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;
            float const panicAccelerator = 1.0f + std::min(humanNpcState.ResultantPanicLevel, 2.0f) / 2.0f * 2.0f;

            float const arg =
                elapsed * 2.6f * panicAccelerator
                + humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition * 0.7f;

            float const inPeriod = FastMod(arg, Period);
            // periodicValue: [0.0 ... 1.0]
            periodicValue = (inPeriod < Period / 2.0f)
                ? inPeriod / (Period / 2.0f)
                : 1.0f - (inPeriod - (Period / 2.0f)) / (Period / 2.0f);

            // Arms: one arm around around a large angle; the other fixed around a small angle
            float const angle1 = (Pi<float> -StateType::KindSpecificStateType::HumanNpcStateType::AnimationStateType::InitialArmAngle) + (periodicValue - 0.5f) * Pi<float> / 8.0f;
            float const angle2 = -StateType::KindSpecificStateType::HumanNpcStateType::AnimationStateType::InitialArmAngle;
            if (npc.RandomNormalizedUniformSeed >= 0.0f)
            {
                targetAngles.RightArm = angle1;
                targetAngles.LeftArm = angle2;
            }
            else
            {
                targetAngles.RightArm = -angle2;
                targetAngles.LeftArm = -angle1;
            }

            // Legs: perfectly vertical
            targetAngles.RightLeg = 0.0f;
            targetAngles.LeftLeg = 0.0f;

            // Convergence rate depends on how long we've been in this state
            float const MaxConvergenceWait = 3.5f;
            convergenceRate = 0.01f + Clamp(elapsed, 0.0f, MaxConvergenceWait) / MaxConvergenceWait * (0.25f - 0.01f);

            break;
        }

        case HumanNpcStateType::BehaviorType::ConstrainedOrFree_Smashed:
        {
            // Arms and legs at fixed angles
            targetAngles.RightArm = 3.0f / 4.0f * Pi<float>;
            targetAngles.LeftArm = -targetAngles.RightArm;
            targetAngles.RightLeg = 1.0f / 4.0f * Pi<float>;
            targetAngles.LeftLeg = -targetAngles.RightLeg;

            convergenceRate = 0.2f;

            break;
        }

        case HumanNpcStateType::BehaviorType::BeingRemoved:
        {
            auto & behaviorState = humanNpcState.CurrentBehaviorState.BeingRemoved;

            float const elapsed = currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp;
            float const relElapsed = elapsed - behaviorState.CurrentStateTransitionTimestamp;

            switch (behaviorState.CurrentState)
            {
                case HumanNpcStateType::BehaviorStateType::BeingRemovedStateType::StateType::Init:
                {
                    // Nop
                    break;
                }

                case HumanNpcStateType::BehaviorStateType::BeingRemovedStateType::StateType::GettingUpright:
                {
                    if (humanNpcState.CurrentFaceOrientation == 0.0f)
                    {
                        // On a side

                        // Arms, Legs: always opposite dir of viewing, but peaking in the ~middle (M)
                        float const progress = std::min(relElapsed / behaviorState.TotalUprightDuration, 1.0f);
                        float constexpr M = 0.7f;
                        float const depth = -(1.0f/(M*M)) * progress * progress + (2.0f/M) * progress;

                        float const targetArmAngle = - (Pi<float> / 4.0f * 0.7f) * humanNpcState.CurrentFaceDirectionX * depth;
                        targetAngles.RightArm = targetArmAngle;
                        targetAngles.LeftArm = targetArmAngle;

                        float const targetLegAngle = - (Pi<float> * 1.0f / 8.0f) * humanNpcState.CurrentFaceDirectionX * depth;
                        targetAngles.RightLeg = targetLegAngle;
                        targetAngles.LeftLeg = targetLegAngle;
                    }
                    else
                    {
                        // Front-back

                        // Arms->Pi/4
                        // Legs->0

                        float const targetArmAngle = Pi<float> / 4.0f;
                        targetAngles.RightArm = targetArmAngle;
                        targetAngles.LeftArm = -targetArmAngle;

                        float const targetLegAngle = 0.0f;
                        targetAngles.RightLeg = targetLegAngle;
                        targetAngles.LeftLeg = -targetLegAngle;
                    }

                    convergenceRate = 0.1f;

                    break;
                }

                case HumanNpcStateType::BehaviorStateType::BeingRemovedStateType::StateType::Rotating:
                {
                    assert(behaviorState.WorkingLimbFBAngles.has_value());
                    assert(behaviorState.WorkingLimbLRAngles.has_value());

                    //
                    // Arms:
                    //  FB: Arms->PI/2+rnd Legs->0
                    //  LR: Arms->0 Legs->0
                    //
                    // Since we're rotating, we converge immediately, hence the use of shadow "working" angles
                    //

                    float constexpr ConvergenceRate = 0.025f;

                    float const fbArmAngle = Pi<float> / 2.0f + npc.RandomNormalizedUniformSeed * 0.2f; // PI/2 slightly randomized
                    behaviorState.WorkingLimbFBAngles->ConvergeTo({ 0.0f, 0.0f, fbArmAngle, -fbArmAngle }, ConvergenceRate);
                    behaviorState.WorkingLimbLRAngles->ConvergeTo({ 0.0f, 0.0f, 0.0f, 0.0f }, ConvergenceRate);

                    if (humanNpcState.CurrentFaceOrientation == 0.0f)
                    {
                        // LR
                        targetAngles = *behaviorState.WorkingLimbLRAngles;

                        // Note: this should be taken care by rendering...
                        targetAngles.RightArm = std::fabs(targetAngles.RightArm) * humanNpcState.CurrentFaceDirectionX * -1.0f;
                        targetAngles.LeftArm = std::fabs(targetAngles.LeftArm) * humanNpcState.CurrentFaceDirectionX * -1.0f;
                        targetAngles.RightLeg = std::fabs(targetAngles.RightLeg) * humanNpcState.CurrentFaceDirectionX * -1.0f;
                        targetAngles.LeftLeg = std::fabs(targetAngles.LeftLeg) * humanNpcState.CurrentFaceDirectionX * -1.0f;
                    }
                    else
                    {
                        // FB
                        targetAngles = *behaviorState.WorkingLimbFBAngles;
                    }

                    convergenceRate = 1.0f;

                    // Alpha and RemovalProgress

                    // Removal highlight: from now until Duration
                    float constexpr RemovalDuration = 0.9f * HumanRemovalRotationDuration;
                    animationState.RemovalProgress = Clamp(relElapsed / RemovalDuration, 0.0f, 1.0f);

                    // Alpha: from Duration until end
                    animationState.Alpha = 1.0f - Clamp((relElapsed - RemovalDuration) / (HumanRemovalRotationDuration - RemovalDuration), 0.0f, 1.0f);

                    break;
                }
            }

            break;
        }
    }

    // Converge
    animationState.LimbAngles.ConvergeTo(targetAngles, convergenceRate);
    animationState.UpperLegLengthFraction = targetUpperLegLengthFraction;

    // Calculate sins and coss
    SinCos4(animationState.LimbAngles.fptr(), animationState.LimbAnglesSin.fptr(), animationState.LimbAnglesCos.fptr());

    //
    // Length Multipliers
    //

    FS_ALIGN16_BEG LimbVector targetLengthMultipliers FS_ALIGN16_END = LimbVector({ 1.0f, 1.0f, 1.0f, 1.0f });
    float limbLengthConvergenceRate = convergenceRate;

    float targetCrotchHeightMultiplier = 1.0f;

    float constexpr MinPrerisingArmLengthMultiplier = 0.35f;

    switch (humanNpcState.CurrentBehavior)
    {
        case HumanNpcStateType::BehaviorType::Constrained_PreRising:
        {
            // Retract arms
            targetLengthMultipliers.RightArm = MinPrerisingArmLengthMultiplier;
            targetLengthMultipliers.LeftArm = MinPrerisingArmLengthMultiplier;

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Rising:
        {
            if (humanNpcState.CurrentBehaviorState.Constrained_Rising.VirtualEdgeRisingAgainst.TriangleElementIndex != NoneElementIndex) // Locals guaranteed to be calc'd
            {
                // Recoil arms

                // For such a small angle, tan(x) ~= x
                float const targetArmLengthMultiplier =
                    MinPrerisingArmLengthMultiplier
                    + Clamp(humanEdgeAngle / MaxHumanEdgeAngleForArms, 0.0f, 1.0f) * (1.0f - MinPrerisingArmLengthMultiplier);

                targetLengthMultipliers.RightArm = targetArmLengthMultiplier;
                targetLengthMultipliers.LeftArm = targetArmLengthMultiplier;
            }
            else
            {
                // Survive small bursts of losing the edge
                targetLengthMultipliers.RightArm = animationState.LimbLengthMultipliers.RightArm;
                targetLengthMultipliers.LeftArm = animationState.LimbLengthMultipliers.LeftArm;
            }

            break;
        }

        case HumanNpcStateType::BehaviorType::Constrained_Walking:
        {
            // Lower crotch with gait
            targetCrotchHeightMultiplier = animationState.LimbAnglesCos.RightLeg;

            if (primaryContrainedState.has_value() && primaryContrainedState->CurrentVirtualFloor.has_value())
            {
                //
                // We are walking on an edge - make sure feet don't look weird on sloped edges
                //

                //
                // 2. Constrain feet onto edge - i.e. adjust leg lengths
                //

                //
                // Using parametric eq's (tl=scalar from leg1 to leg2, te=scalar from edg1 to edg2):
                //
                // leg1 + tl * (leg2 - leg1) = edg1 + te * (edg2 - edg1)
                // =>
                // tl = (edg1.y - leg1.y) * (edg2.x - edg1.x) + (leg1.x - edg1.x) * (edg2.y - edg1.y)
                //      -----------------------------------------------------------------------------
                //                                  edg X leg
                //

                float constexpr MaxLengthMultiplier = 1.4f;

                float const adjustedStandardLegLength = GameParameters::HumanNpcGeometry::LegLengthFraction * adjustedStandardHumanHeight;
                vec2f const crotchPosition = feetPosition - actualBodyVector * (GameParameters::HumanNpcGeometry::LegLengthFraction * targetCrotchHeightMultiplier);

                // leg*1 is crotchPosition
                float const numerator = (edgp1.y - crotchPosition.y) * (edgp2.x - edgp1.x) + (crotchPosition.x - edgp1.x) * (edgp2.y - edgp1.y);

                {
                    vec2f const legrVector = actualBodyDir.rotate(animationState.LimbAnglesCos.RightLeg, animationState.LimbAnglesSin.RightLeg) * adjustedStandardLegLength;
                    float const edgCrossRightLeg = edgVector.cross(legrVector);
                    if (std::abs(edgCrossRightLeg) > 0.0000001f)
                    {
                        //targetRightLegLengthMultiplier = numerator / edgCrossRightLeg;
                        float const candidate = numerator / edgCrossRightLeg;
                        if (candidate > 0.01f)
                        {
                            targetLengthMultipliers.RightLeg = std::min(candidate, MaxLengthMultiplier);
                        }
                    }
                }

                {
                    vec2f const leglVector = actualBodyDir.rotate(animationState.LimbAnglesCos.LeftLeg, animationState.LimbAnglesSin.LeftLeg) * adjustedStandardLegLength;
                    float const edgCrossLeftLeg = edgVector.cross(leglVector);
                    if (std::abs(edgCrossLeftLeg) > 0.0000001f)
                    {
                        //targetLeftLegLengthMultiplier = numerator / edgCrossLeftLeg;
                        float const candidate = numerator / edgCrossLeftLeg;
                        if (candidate > 0.01f)
                        {
                            targetLengthMultipliers.LeftLeg = std::min(candidate, MaxLengthMultiplier);
                        }
                    }
                }

                limbLengthConvergenceRate = 0.09f;
            }

            break;
        }

        case HumanNpcStateType::BehaviorType::Free_Swimming_Style2:
        {
            //
            // Trappelen lengths
            //

            float constexpr TrappelenExtent = 0.3f;
            targetLengthMultipliers.RightLeg = 1.0f - (1.0f - periodicValue) * TrappelenExtent;
            targetLengthMultipliers.LeftLeg = 1.0f - periodicValue * TrappelenExtent;

            break;
        }

        case HumanNpcStateType::BehaviorType::Free_Swimming_Style3:
        {
            //
            // Trappelen lengths
            //

            float constexpr TrappelenExtent = 0.3f;
            targetLengthMultipliers.RightLeg = 1.0f - (1.0f - periodicValue) * TrappelenExtent;
            targetLengthMultipliers.LeftLeg = 1.0f - periodicValue * TrappelenExtent;

            break;
        }

        case HumanNpcStateType::BehaviorType::BeingRemoved:
        {
            //
            // Bent arms from a side
            //

            if (humanNpcState.CurrentBehaviorState.BeingRemoved.CurrentState == HumanNpcStateType::BehaviorStateType::BeingRemovedStateType::StateType::Rotating)
            {
                if (humanNpcState.CurrentFaceOrientation == 0.0f)
                {
                    // We're looking L/R, make arms 3D considering F/B angles
                    assert(humanNpcState.CurrentBehaviorState.BeingRemoved.WorkingLimbFBAngles.has_value());
                    auto const & workingAngles = *humanNpcState.CurrentBehaviorState.BeingRemoved.WorkingLimbFBAngles;

                    targetLengthMultipliers.RightArm = std::cos(workingAngles.RightArm);
                    targetLengthMultipliers.LeftArm = std::cos(workingAngles.LeftArm);
                }
            }

            limbLengthConvergenceRate = 1.0f;

            break;
        }

        case HumanNpcStateType::BehaviorType::BeingPlaced:
        case HumanNpcStateType::BehaviorType::Constrained_Equilibrium:
        case HumanNpcStateType::BehaviorType::Constrained_WalkingUndecided:
        case HumanNpcStateType::BehaviorType::Constrained_Falling:
        case HumanNpcStateType::BehaviorType::Constrained_KnockedOut:
        case HumanNpcStateType::BehaviorType::Constrained_Aerial:
        case HumanNpcStateType::BehaviorType::Constrained_InWater:
        case HumanNpcStateType::BehaviorType::Constrained_Swimming_Style1:
        case HumanNpcStateType::BehaviorType::Constrained_Swimming_Style2:
        case HumanNpcStateType::BehaviorType::Constrained_Electrified:
        case HumanNpcStateType::BehaviorType::Free_Aerial:
        case HumanNpcStateType::BehaviorType::Free_KnockedOut:
        case HumanNpcStateType::BehaviorType::Free_InWater:
        case HumanNpcStateType::BehaviorType::Free_Swimming_Style1:
        case HumanNpcStateType::BehaviorType::ConstrainedOrFree_Smashed:
        {
            // Nop
            break;
        }
    }

    // Converge
    animationState.LimbLengthMultipliers.ConvergeTo(targetLengthMultipliers, limbLengthConvergenceRate);
    animationState.CrotchHeightMultiplier += (targetCrotchHeightMultiplier - animationState.CrotchHeightMultiplier) * convergenceRate;
}

}