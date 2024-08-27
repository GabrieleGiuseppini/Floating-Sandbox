/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-10-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/Colors.h>
#include <GameCore/GameGeometry.h>

#include <algorithm>
#include <cassert>

#pragma warning(disable : 4324) // std::optional of StateType gets padded because of alignment requirements

namespace Physics {

/*
Main principles:
    - Global damping: when constrained, we only apply it to velocity *relative* to the mesh ("air moves with the ship")
*/

void Npcs::Update(
    float currentSimulationTime,
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

    if (gameParameters.HumanNpcWalkingSpeedAdjustment != mCurrentHumanNpcWalkingSpeedAdjustment)
    {
        mCurrentHumanNpcWalkingSpeedAdjustment = gameParameters.HumanNpcWalkingSpeedAdjustment;
    }

    //
    // Update NPCs' state
    //

    // Advance the current simulation sequence
    ++mCurrentSimulationSequenceNumber;

    UpdateNpcs(currentSimulationTime, gameParameters);
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
                            state.Highlight);
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

    assert(renderContext.GetNpcRenderMode() == NpcRenderModeType::Limbs);

#endif

    for (ShipId shipId = 0; shipId < mShips.size(); ++shipId)
    {
        if (mShips[shipId].has_value())
        {
            auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

            shipRenderContext.UploadNpcTextureQuadsStart(
                mShips[shipId]->FurnitureNpcCount			// Furniture: one single quad
                + mShips[shipId]->HumanNpcCount * (6 + 2)); // Human: max 8 quads (limbs)

            for (NpcId const npcId : mShips[shipId]->Npcs)
            {
                assert(mStateBuffer[npcId].has_value());
                auto const & state = *mStateBuffer[npcId];

                RenderNpc(state, shipRenderContext);
            }

            shipRenderContext.UploadNpcTextureQuadsEnd();
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
            npc.CombustionProgress, // scale
            (npc.RandomNormalizedUniformSeed + 1.0f) / 2.0f);
    }
}

///////////////////////////////

Geometry::AABB Npcs::GetAABB(NpcId npcId) const
{
    assert(mStateBuffer[npcId].has_value());

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

    bool doPublishHumanNpcStats = false;

    for (auto const npcId : mShips[s]->Npcs)
    {
        assert(mStateBuffer[npcId].has_value());

        // Maintain stats
        if (mStateBuffer[npcId]->Kind == NpcKindType::Human)
        {
            if (mStateBuffer[npcId]->CurrentRegime == StateType::RegimeType::Constrained)
            {
                assert(mConstrainedRegimeHumanNpcCount > 0);
                --mConstrainedRegimeHumanNpcCount;
                doPublishHumanNpcStats = true;
            }
            else if (mStateBuffer[npcId]->CurrentRegime == StateType::RegimeType::Free)
            {
                assert(mFreeRegimeHumanNpcCount > 0);
                --mFreeRegimeHumanNpcCount;
                doPublishHumanNpcStats = true;
            }
        }

        // Remove from burning set
        auto npcIt = std::find(mShips[s]->BurningNpcs.begin(), mShips[s]->BurningNpcs.end(), npcId);
        if (npcIt != mShips[s]->BurningNpcs.end())
        {
            mShips[s]->BurningNpcs.erase(npcIt);
        }

        // Nuke NPC
        mStateBuffer[npcId].reset();
    }

    if (doPublishHumanNpcStats)
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
            for (int p = 1; p < npcState.ParticleMesh.Particles.size(); ++p)
            {
                auto & secondaryParticle = npcState.ParticleMesh.Particles[p];
                if (secondaryParticle.ConstrainedState.has_value())
                {
                    auto const secondaryTriangleRepresentativePoint = homeShip.GetTriangles().GetPointAIndex(secondaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex);
                    if (homeShip.GetPoints().GetConnectedComponentId(secondaryTriangleRepresentativePoint) != npcState.CurrentConnectedComponentId)
                    {
                        TransitionParticleToFreeState(npcState, p, homeShip);
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

std::optional<PickedObjectId<NpcId>> Npcs::BeginPlaceNewFurnitureNpc(
    NpcSubKindIdType subKind,
    vec2f const & worldCoordinates,
    float /*currentSimulationTime*/,
    bool doMoveWholeMesh)
{
    //
    // Check if there are too many NPCs
    //

    if (CalculateTotalNpcCount() >= GameParameters::MaxNpcs)
    {
        return std::nullopt;
    }

    //
    // Create NPC
    //

    auto const & furnitureMaterial = mNpcDatabase.GetFurnitureMaterial(subKind);

    StateType::ParticleMeshType particleMesh;
    vec2f pickAnchorOffset;

    switch (mNpcDatabase.GetFurnitureParticleMeshKindType(subKind))
    {
        case NpcDatabase::ParticleMeshKindType::Dipole:
        {
            // TODO
            throw GameException("Dipoles not yet supported!");
        }

        case NpcDatabase::ParticleMeshKindType::Particle:
        {
            // Check if there are enough particles

            if (mParticles.GetRemainingParticlesCount() < 1)
            {
                return std::nullopt;
            }

            // Primary

            float const mass = CalculateParticleMass(
                furnitureMaterial.GetMass(),
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentMassAdjustment
#endif
            );

            float const buoyancyFactor = CalculateParticleBuoyancyFactor(
                furnitureMaterial.NpcBuoyancyVolumeFill,
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentBuoyancyAdjustment
#endif
            );

            auto const primaryParticleIndex = mParticles.Add(
                mass,
                buoyancyFactor,
                &furnitureMaterial,
                worldCoordinates,
                furnitureMaterial.RenderColor);

            particleMesh.Particles.emplace_back(primaryParticleIndex, std::nullopt);

            pickAnchorOffset = vec2f(0.0f, 0.0f);

            break;
        }

        case NpcDatabase::ParticleMeshKindType::Quad:
        {
            // Check if there are enough particles

            if (mParticles.GetRemainingParticlesCount() < 4)
            {
                return std::nullopt;
            }

            // Create Particles

            float const baseWidth = mNpcDatabase.GetFurnitureDimensions(subKind).Width;
            float const baseHeight = mNpcDatabase.GetFurnitureDimensions(subKind).Height;

            float const mass = CalculateParticleMass(
                furnitureMaterial.GetMass(),
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentMassAdjustment
#endif
            );

            float const buoyancyFactor = CalculateParticleBuoyancyFactor(
                furnitureMaterial.NpcBuoyancyVolumeFill,
                mCurrentSizeMultiplier
#ifdef IN_BARYLAB
                , mCurrentBuoyancyAdjustment
#endif
            );

            float const baseDiagonal = std::sqrtf(baseWidth * baseWidth + baseHeight * baseHeight);

            // 0 - 1
            // |   |
            // 3 - 2
            float const width = CalculateSpringLength(baseWidth, mCurrentSizeMultiplier);
            float const height = CalculateSpringLength(baseHeight, mCurrentSizeMultiplier);
            for (int p = 0; p < 4; ++p)
            {
                // CW order
                vec2f particlePosition = worldCoordinates;

                if (p == 0 || p == 3)
                {
                    particlePosition.x -= width / 2.0f;
                }
                else
                {
                    particlePosition.x += width / 2.0f;
                }

                if (p == 0 || p == 1)
                {
                    particlePosition.y += height / 2.0f;
                }
                else
                {
                    particlePosition.y -= height / 2.0f;
                }

                auto const particleIndex = mParticles.Add(
                    mass,
                    buoyancyFactor * GameRandomEngine::GetInstance().GenerateUniformReal(0.99f, 1.01f), // Make sure rotates while floating
                    &furnitureMaterial,
                    particlePosition,
                    furnitureMaterial.RenderColor);

                particleMesh.Particles.emplace_back(particleIndex, std::nullopt);
            }

            // Springs

            StateType::NpcSpringStateType baseSpring(
                NoneElementIndex,
                NoneElementIndex,
                0,
                furnitureMaterial.NpcSpringReductionFraction,
                furnitureMaterial.NpcSpringDampingCoefficient);

            // 0 - 1
            {
                baseSpring.EndpointAIndex = particleMesh.Particles[0].ParticleIndex;
                baseSpring.EndpointBIndex = particleMesh.Particles[1].ParticleIndex;
                baseSpring.BaseRestLength = baseWidth;
                particleMesh.Springs.emplace_back(baseSpring);
            }

            // 0 | 3
            {
                baseSpring.EndpointAIndex = particleMesh.Particles[0].ParticleIndex;
                baseSpring.EndpointBIndex = particleMesh.Particles[3].ParticleIndex;
                baseSpring.BaseRestLength = baseHeight;
                particleMesh.Springs.emplace_back(baseSpring);
            }

            // 0 \ 2
            {
                baseSpring.EndpointAIndex = particleMesh.Particles[0].ParticleIndex;
                baseSpring.EndpointBIndex = particleMesh.Particles[2].ParticleIndex;
                baseSpring.BaseRestLength = baseDiagonal;
                particleMesh.Springs.emplace_back(baseSpring);
            }

            // 1 | 2
            {
                baseSpring.EndpointAIndex = particleMesh.Particles[1].ParticleIndex;
                baseSpring.EndpointBIndex = particleMesh.Particles[2].ParticleIndex;
                baseSpring.BaseRestLength = baseHeight;
                particleMesh.Springs.emplace_back(baseSpring);
            }

            // 2 - 3
            {
                baseSpring.EndpointAIndex = particleMesh.Particles[2].ParticleIndex;
                baseSpring.EndpointBIndex = particleMesh.Particles[3].ParticleIndex;
                baseSpring.BaseRestLength = baseWidth;
                particleMesh.Springs.emplace_back(baseSpring);
            }

            // 1 / 3
            {
                baseSpring.EndpointAIndex = particleMesh.Particles[1].ParticleIndex;
                baseSpring.EndpointBIndex = particleMesh.Particles[3].ParticleIndex;
                baseSpring.BaseRestLength = baseDiagonal;
                particleMesh.Springs.emplace_back(baseSpring);
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

            pickAnchorOffset = vec2f(width / 2.0f, -height / 2.0f);

            break;
        }
    }

    // Furniture

    StateType::KindSpecificStateType::FurnitureNpcStateType furnitureState = StateType::KindSpecificStateType::FurnitureNpcStateType(
        subKind,
        mNpcDatabase.GetFurnitureTextureCoordinatesQuad(subKind));

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
        shipId, // Topmost ship ID
        0, // PlaneID: irrelevant as long as BeingPlaced
        std::nullopt, // Connected component: irrelevant as long as BeingPlaced
        StateType::RegimeType::BeingPlaced,
        std::move(particleMesh),
        StateType::KindSpecificStateType(std::move(furnitureState)),
        StateType::BeingPlacedStateType({0, doMoveWholeMesh})); // Furniture: anchor is first particle

    assert(mShips[shipId].has_value());
    mShips[shipId]->Npcs.push_back(npcId);

    //
    // Update stats
    //

    ++(mShips[shipId]->FurnitureNpcCount);
    mGameEventHandler->OnNpcCountsUpdated(CalculateTotalNpcCount());

    return PickedObjectId<NpcId>(npcId, pickAnchorOffset);
}

std::optional<PickedObjectId<NpcId>> Npcs::BeginPlaceNewHumanNpc(
    NpcSubKindIdType subKind,
    vec2f const & worldCoordinates,
    float currentSimulationTime,
    bool doMoveWholeMesh)
{
    //
    // Check if there are enough NPCs and particles
    //

    if (CalculateTotalNpcCount() >= GameParameters::MaxNpcs || mParticles.GetRemainingParticlesCount() < 2)
    {
        return std::nullopt;
    }

    //
    // Create NPC
    //

    StateType::ParticleMeshType particleMesh;

    // Calculate height

    float const baseHeight =
        GameRandomEngine::GetInstance().GenerateNormalReal(
            GameParameters::HumanNpcGeometry::BodyLengthMean,
            GameParameters::HumanNpcGeometry::BodyLengthStdDev)
        * mNpcDatabase.GetHumanSizeMultiplier(subKind);

    float const height = CalculateSpringLength(baseHeight, mCurrentSizeMultiplier);

    // Feet (primary)

    auto const & feetMaterial = mNpcDatabase.GetHumanFeetMaterial(subKind);
    float const feetMass = CalculateParticleMass(
        feetMaterial.GetMass(),
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentMassAdjustment
#endif
    );

    float const feetBuoyancyFactor = CalculateParticleBuoyancyFactor(
        feetMaterial.NpcBuoyancyVolumeFill,
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentBuoyancyAdjustment
#endif
    );

    auto const primaryParticleIndex = mParticles.Add(
        feetMass,
        feetBuoyancyFactor,
        &feetMaterial,
        worldCoordinates - vec2f(0.0f, 1.0f) * height,
        feetMaterial.RenderColor);

    particleMesh.Particles.emplace_back(primaryParticleIndex, std::nullopt);

    // Head (secondary)

    auto const & headMaterial = mNpcDatabase.GetHumanHeadMaterial(subKind);
    float const headMass = CalculateParticleMass(
        headMaterial.GetMass(),
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentMassAdjustment
#endif
    );

    float const headBuoyancyFactor = CalculateParticleBuoyancyFactor(
        headMaterial.NpcBuoyancyVolumeFill,
        mCurrentSizeMultiplier
#ifdef IN_BARYLAB
        , mCurrentBuoyancyAdjustment
#endif
    );

    auto const secondaryParticleIndex = mParticles.Add(
        headMass,
        headBuoyancyFactor,
        &headMaterial,
        worldCoordinates,
        headMaterial.RenderColor);

    particleMesh.Particles.emplace_back(secondaryParticleIndex, std::nullopt);

    // Dipole spring

    particleMesh.Springs.emplace_back(
        primaryParticleIndex,
        secondaryParticleIndex,
        baseHeight,
        (headMaterial.NpcSpringReductionFraction + feetMaterial.NpcSpringReductionFraction) / 2.0f,
        (headMaterial.NpcSpringDampingCoefficient + feetMaterial.NpcSpringDampingCoefficient) / 2.0f);

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
                2.0f * GameParameters::HumanNpcGeometry::BodyWidthNarrowMultiplierStdDev)
            * mNpcDatabase.GetHumanBodyWidthRandomizationSensitivity(subKind);
    }
    else
    {
        // Wide
        widthMultiplier = 1.0f +
            std::min(
                std::abs(GameRandomEngine::GetInstance().GenerateNormalReal(0.0f, GameParameters::HumanNpcGeometry::BodyWidthWideMultiplierStdDev)),
                2.7f * GameParameters::HumanNpcGeometry::BodyWidthWideMultiplierStdDev)
            * mNpcDatabase.GetHumanBodyWidthRandomizationSensitivity(subKind);
    }

    float const walkingSpeedBase =
        1.0f
        * baseHeight / 1.65f; // Just comes from 1m/s looking good when human is 1.65

    StateType::KindSpecificStateType::HumanNpcStateType humanState = StateType::KindSpecificStateType::HumanNpcStateType(
        subKind,
        mNpcDatabase.GetHumanRole(subKind),
        widthMultiplier,
        walkingSpeedBase,
        mNpcDatabase.GetHumanDimensions(subKind),
        mNpcDatabase.GetHumanTextureCoordinatesQuads(subKind),
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
        shipId, // Topmost ship ID
        0, // PlaneID: irrelevant as long as BeingPlaced
        std::nullopt, // Connected component: irrelevant as long as BeingPlaced
        StateType::RegimeType::BeingPlaced,
        std::move(particleMesh),
        StateType::KindSpecificStateType(std::move(humanState)),
        StateType::BeingPlacedStateType({ 1, doMoveWholeMesh })); // Human: anchor is head (second particle)

    assert(mShips[shipId].has_value());
    mShips[shipId]->Npcs.push_back(npcId);

    //
    // Update stats
    //

    ++(mShips[shipId]->HumanNpcCount);
    mGameEventHandler->OnNpcCountsUpdated(CalculateTotalNpcCount());

    return PickedObjectId<NpcId>(npcId, vec2f::zero());
}

std::optional<PickedObjectId<NpcId>> Npcs::ProbeNpcAt(
    vec2f const & position,
    float radius,
    GameParameters const & gameParameters) const
{
    float const squareSearchRadius = radius * radius * gameParameters.NpcSizeMultiplier;

    struct NearestNpcType
    {
        NpcId Id{ NoneNpcId };
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
        if (npc.has_value())
        {
            switch (npc->Kind)
            {
                case NpcKindType::Furniture:
                {
                    // Proximity search for all particles

                    bool aParticleWasFound = false;
                    for (auto const & particle : npc->ParticleMesh.Particles)
                    {
                        vec2f const candidateNpcPosition = mParticles.GetPosition(particle.ParticleIndex);
                        float const squareDistance = (candidateNpcPosition - position).squareLength();
                        if (squareDistance < squareSearchRadius)
                        {
                            if (std::make_pair(npc->CurrentShipId, npc->CurrentPlaneId) >= probeDepth)
                            {
                                // It's on-plane
                                if (squareDistance < nearestOnPlaneNpc.SquareDistance)
                                {
                                    nearestOnPlaneNpc = { npc->Id, squareDistance };
                                    aParticleWasFound = true;
                                }
                            }
                            else
                            {
                                // It's off-plane
                                if (squareDistance < nearestOffPlaneNpc.SquareDistance)
                                {
                                    nearestOffPlaneNpc = { npc->Id, squareDistance };
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
                                nearestOnPlaneNpc = { npc->Id, squareSearchRadius };
                            }
                            else
                            {
                                // It's off-plane
                                nearestOffPlaneNpc = { npc->Id, squareSearchRadius };
                            }
                        }
                    }

                    break;
                }

                case NpcKindType::Human:
                {
                    float const squareDistance = Segment::SquareDistanceToPoint(
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
                                nearestOnPlaneNpc = { npc->Id, squareDistance };
                            }
                        }
                        else
                        {
                            // It's off-plane
                            if (squareDistance < nearestOffPlaneNpc.SquareDistance)
                            {
                                nearestOffPlaneNpc = { npc->Id, squareDistance };
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
    if (nearestOnPlaneNpc.Id != NoneNpcId)
    {
        foundId = nearestOnPlaneNpc.Id;
    }
    else if (nearestOffPlaneNpc.Id != NoneNpcId)
    {
        foundId = nearestOffPlaneNpc.Id;
    }

    if (foundId != NoneNpcId)
    {
        assert(mStateBuffer[foundId].has_value());

        int referenceParticleOrdinal = mStateBuffer[foundId]->Kind == NpcKindType::Furniture
            ? 0
            : 1;

        ElementIndex referenceParticleIndex = mStateBuffer[foundId]->ParticleMesh.Particles[referenceParticleOrdinal].ParticleIndex;

        return PickedObjectId<NpcId>(
            foundId,
            position - mParticles.GetPosition(referenceParticleIndex));
    }
    else
    {
        return std::nullopt;
    }
}

void Npcs::BeginMoveNpc(
    NpcId id,
    float currentSimulationTime,
    bool doMoveWholeMesh)
{
    assert(mStateBuffer[id].has_value());
    auto & npc = *mStateBuffer[id];

    //
    // Move NPC to topmost ship
    //

    TransferNpcToShip(npc, GetTopmostShipId());
    npc.CurrentPlaneId = 0; // Irrelevant as long as it's in BeingPlaced

    //
    // Move NPC to BeingPlaced
    //

    // All particles become free
    for (auto & particle : npc.ParticleMesh.Particles)
    {
        particle.ConstrainedState.reset();
    }

    // Setup being placed state
    npc.BeingPlacedState = StateType::BeingPlacedStateType({
        (npc.Kind == NpcKindType::Human) ? 1 : 0,
        doMoveWholeMesh });

    // Change regime
    auto const oldRegime = npc.CurrentRegime;
    npc.CurrentRegime = StateType::RegimeType::BeingPlaced;

    if (npc.Kind == NpcKindType::Human)
    {
        // Change behavior
        npc.KindSpecificState.HumanNpcState.TransitionToState(
            StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::BeingPlaced,
            currentSimulationTime);
    }

    //
    // Maintain stats
    //

    if (npc.Kind == NpcKindType::Human)
    {
        if (oldRegime == StateType::RegimeType::Constrained)
        {
            assert(mConstrainedRegimeHumanNpcCount > 0);
            --mConstrainedRegimeHumanNpcCount;
            PublishHumanNpcStats();
        }
        else if (oldRegime == StateType::RegimeType::Free)
        {
            assert(mFreeRegimeHumanNpcCount > 0);
            --mFreeRegimeHumanNpcCount;
            PublishHumanNpcStats();
        }
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

    // Defeat - we cannot make quads move nicely with our current spring length maintenance algorithm :-(
    doMoveWholeMesh = (mStateBuffer[id]->ParticleMesh.Particles.size() > 2)
        ? true
        : doMoveWholeMesh;

    // Calculate delta movement for anchor particle
    ElementIndex anchorParticleIndex = mStateBuffer[id]->ParticleMesh.Particles[mStateBuffer[id]->BeingPlacedState->AnchorParticleOrdinal].ParticleIndex;
    vec2f const deltaAnchorPosition = (position - offset) - mParticles.GetPosition(anchorParticleIndex);

    // Calculate absolute velocity for this delta movement
    float constexpr InertialVelocityFactor = 0.5f; // Magic number for how much velocity we impart
    vec2f const targetAbsoluteVelocity = deltaAnchorPosition / GameParameters::SimulationStepTimeDuration<float> * InertialVelocityFactor;

    // Move particles
    for (int p = 0; p < mStateBuffer[id]->ParticleMesh.Particles.size(); ++p)
    {
        if (doMoveWholeMesh || p == mStateBuffer[id]->BeingPlacedState->AnchorParticleOrdinal)
        {
            auto const particleIndex = mStateBuffer[id]->ParticleMesh.Particles[p].ParticleIndex;
            mParticles.SetPosition(particleIndex, mParticles.GetPosition(particleIndex) + deltaAnchorPosition);
            mParticles.SetVelocity(particleIndex, targetAbsoluteVelocity);

            if (mStateBuffer[id]->ParticleMesh.Particles[p].ConstrainedState.has_value())
            {
                // We can only assume here, and we assume the ship is still and since the user doesn't move with the ship,
                // all this velocity is also relative to mesh
                mStateBuffer[id]->ParticleMesh.Particles[p].ConstrainedState->MeshRelativeVelocity = targetAbsoluteVelocity;
            }
        }
    }

    // Update state
    mStateBuffer[id]->BeingPlacedState->DoMoveWholeMesh = doMoveWholeMesh;
}

void Npcs::EndMoveNpc(
    NpcId id,
    float currentSimulationTime)
{
    assert(mStateBuffer[id].has_value());

    auto & npc = *mStateBuffer[id];

    assert(npc.CurrentRegime == StateType::RegimeType::BeingPlaced);

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

void Npcs::CompleteNewNpc(
    NpcId id,
    float currentSimulationTime)
{
    EndMoveNpc(id, currentSimulationTime);
}

void Npcs::RemoveNpc(NpcId id)
{
    assert(mStateBuffer[id].has_value());
    assert(mShips[mStateBuffer[id]->CurrentShipId].has_value());

    //
    // Maintain stats
    //

    switch (mStateBuffer[id]->Kind)
    {
        case NpcKindType::Furniture:
        {
            --(mShips[mStateBuffer[id]->CurrentShipId]->FurnitureNpcCount);

            break;
        }

        case NpcKindType::Human:
        {
            --(mShips[mStateBuffer[id]->CurrentShipId]->HumanNpcCount);

            if (mStateBuffer[id]->CurrentRegime == StateType::RegimeType::Constrained)
            {
                assert(mConstrainedRegimeHumanNpcCount > 0);
                --mConstrainedRegimeHumanNpcCount;
                PublishHumanNpcStats();
            }
            else if (mStateBuffer[id]->CurrentRegime == StateType::RegimeType::Free)
            {
                assert(mFreeRegimeHumanNpcCount > 0);
                --mFreeRegimeHumanNpcCount;
                PublishHumanNpcStats();
            }

            break;
        }
    }

    mGameEventHandler->OnNpcCountsUpdated(CalculateTotalNpcCount());

    //
    // Update ship indices
    //

    assert(mShips[mStateBuffer[id]->CurrentShipId].has_value());

    auto it = std::find(
        mShips[mStateBuffer[id]->CurrentShipId]->Npcs.begin(),
        mShips[mStateBuffer[id]->CurrentShipId]->Npcs.end(),
        id);

    assert(it != mShips[mStateBuffer[id]->CurrentShipId]->Npcs.end());

    mShips[mStateBuffer[id]->CurrentShipId]->Npcs.erase(it);

    //
    // Get rid of NPC
    //

    mStateBuffer[id].reset();
}

void Npcs::AbortNewNpc(NpcId id)
{
    RemoveNpc(id);
}

void Npcs::HighlightNpc(
    NpcId id,
    NpcHighlightType highlight)
{
    assert(mStateBuffer[id].has_value());
    mStateBuffer[id]->Highlight = highlight;
}

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

        // Check if this NPC is in scope: it is iff:
        //	- We're moving all, OR
        //	- The primary is constrained and in this connected component
        auto const & primaryParticle = mStateBuffer[npcId]->ParticleMesh.Particles[0];
        if (!connectedComponent
            || (primaryParticle.ConstrainedState.has_value()
                && homeShip.GetPoints().GetConnectedComponentId(homeShip.GetTriangles().GetPointAIndex(primaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)) == *connectedComponent))
        {
            // In scope - move all of its particles
            for (auto particleOrdinal = 0; particleOrdinal < mStateBuffer[npcId]->ParticleMesh.Particles.size(); ++particleOrdinal)
            {
                ElementIndex const particleIndex = mStateBuffer[npcId]->ParticleMesh.Particles[particleOrdinal].ParticleIndex;
                mParticles.SetPosition(particleIndex, mParticles.GetPosition(particleIndex) + offset);
                mParticles.SetVelocity(particleIndex, actualInertialVelocity);

                // Zero-out already-existing forces
                mParticles.SetExternalForces(particleIndex, vec2f::zero());

                // Maintain world bounds
                MaintainInWorldBounds(
                    *mStateBuffer[npcId],
                    particleOrdinal,
                    homeShip,
                    gameParameters);
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

        // Check if this NPC is in scope: it is iff:
        //	- We're rotating all, OR
        //	- The primary is constrained and in this connected component
        auto const & primaryParticle = mStateBuffer[npcId]->ParticleMesh.Particles[0];
        if (!connectedComponent
            || (primaryParticle.ConstrainedState.has_value()
                && homeShip.GetPoints().GetConnectedComponentId(homeShip.GetTriangles().GetPointAIndex(primaryParticle.ConstrainedState->CurrentBCoords.TriangleElementIndex)) == *connectedComponent))
        {
            for (auto particleOrdinal = 0; particleOrdinal < mStateBuffer[npcId]->ParticleMesh.Particles.size(); ++particleOrdinal)
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
                    particleOrdinal,
                    homeShip,
                    gameParameters);
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

    // The specified blast is for damage to the ship; here we want
    // a lower force and a larger radius - as if only caused by air
    float const actualBlastRadius = blastRadius * 8.0f;
    float const actualBlastForce = std::min(blastForce / 15.0f, 35000.0f);

    float const squareRadius = actualBlastRadius * actualBlastRadius;

    for (auto const & npc : mStateBuffer)
    {
        if (npc.has_value())
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
                        // (inversely proportional to square root of distance, not second power as one would expect though)
                        //

                        mParticles.AddExternalForce(
                            npcParticle.ParticleIndex,
                            //particleRadius.normalise(particleRadiusLength) * actualBlastForce / std::sqrt(particleRadiusLength + 2.0f));
                            particleRadius.normalise(particleRadiusLength) * actualBlastForce / (particleRadiusLength + 2.0f));
                    }
                }
            }
        }
    }
}

void Npcs::SetGeneralizedPanicLevelForAllHumans(float panicLevel)
{
    for (auto & npc : mStateBuffer)
    {
        if (npc.has_value())
        {
            if (npc->Kind == NpcKindType::Human)
            {
                npc->KindSpecificState.HumanNpcState.GeneralizedPanicLevel = panicLevel;
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
    auto const npcId = BeginPlaceNewHumanNpc(
        subKind,
        worldCoordinates,
        currentSimulationTime,
        false);

    if (npcId.has_value())
    {
        CompleteNewNpc(
            npcId->ObjectId,
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
            }
        }
    }
#endif
}

#endif

///////////////////////////////

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

size_t Npcs::CalculateTotalNpcCount() const
{
    size_t totalCount = 0;

    for (auto const & s : mShips)
    {
        if (s.has_value())
        {
            totalCount += s->FurnitureNpcCount;
            totalCount += s->HumanNpcCount;
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

                    if (IsPointInTriangle(position, aPosition, bPosition, cPosition)
                        && (!bestTriangleIndex || homeShip.GetPoints().GetPlaneId(pointAIndex) > bestPlaneId)
                        && !IsTriangleFolded(triangleIndex, homeShip))
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

            if (IsPointInTriangle(position, aPosition, bPosition, cPosition)
                && !IsTriangleFolded(triangleIndex, homeShip)
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
    // Maintain indices
    //

    assert(mShips[npc.CurrentShipId].has_value());

    auto it = std::find(
        mShips[npc.CurrentShipId]->Npcs.begin(),
        mShips[npc.CurrentShipId]->Npcs.end(),
        npc.Id);

    assert(it != mShips[npc.CurrentShipId]->Npcs.end());

    mShips[npc.CurrentShipId]->Npcs.erase(it);

    assert(mShips[newShip].has_value());

    mShips[newShip]->Npcs.push_back(newShip);

    //
    // Maintain stats
    //

    switch (npc.Kind)
    {
        case NpcKindType::Furniture:
        {
            --(mShips[npc.CurrentShipId]->FurnitureNpcCount);
            ++(mShips[newShip]->FurnitureNpcCount);

            break;
        }

        case NpcKindType::Human:
        {
            --(mShips[npc.CurrentShipId]->HumanNpcCount);
            ++(mShips[newShip]->HumanNpcCount);

            break;
        }
    }

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
    Render::ShipRenderContext & shipRenderContext) const
{
    assert(mShips[npc.CurrentShipId].has_value());

    auto const planeId = (npc.CurrentRegime == StateType::RegimeType::BeingPlaced)
        ? mShips[npc.CurrentShipId]->HomeShip.GetMaxPlaneId()
        : npc.CurrentPlaneId;

    switch (npc.Kind)
    {
        case NpcKindType::Human:
        {
            assert(npc.ParticleMesh.Particles.size() == 2);
            assert(npc.ParticleMesh.Springs.size() == 1);
            auto const & humanNpcState = npc.KindSpecificState.HumanNpcState;
            auto const & animationState = humanNpcState.AnimationState;

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
            //   |                              | LegLengthFraction * LowerExtremityLengthMultiplier
            //   |                              |
            //   |                              |
            //  ---  Feet                      ---
            //
            //
            // - All based on current dipole length - anchor points are feet - except for
            //   arms, legs, and widths, whose values are based on ideal (NPC) height (incl.adjustment),
            //   thus unaffected by current dipole length
            //

            vec2f const feetPosition = mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex);

            vec2f const actualBodyVector = mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex) - feetPosition; // From feet to head
            vec2f const actualBodyVDir = -actualBodyVector.normalise_approx(); // From head to feet - facilitates arm and length angle-making
            vec2f const actualBodyHDir = actualBodyVDir.to_perpendicular(); // Points R (of the screen)

            vec2f const legTop = feetPosition + actualBodyVector * (GameParameters::HumanNpcGeometry::LegLengthFraction * animationState.LowerExtremityLengthMultiplier);
            vec2f const torsoBottom = legTop - actualBodyVector * (GameParameters::HumanNpcGeometry::LegLengthFraction / 8.0f); // Magic
            vec2f const torsoTop = legTop + actualBodyVector * GameParameters::HumanNpcGeometry::TorsoLengthFraction;
            vec2f const headBottom = torsoTop;
            vec2f const armTop = headBottom - actualBodyVector * (GameParameters::HumanNpcGeometry::ArmLengthFraction / 8.0f); // Magic
            vec2f const headTop = headBottom + actualBodyVector * (GameParameters::HumanNpcGeometry::HeadWidthFraction * humanNpcState.Dimensions.HeadWidthToHeightFactor);

            float const adjustedIdealHumanHeight = npc.ParticleMesh.Springs[0].RestLength;

            float const halfHeadW = (adjustedIdealHumanHeight * GameParameters::HumanNpcGeometry::HeadWidthFraction * humanNpcState.WidthMultipier) / 2.0f;
            float const torsoWidthFraction = GameParameters::HumanNpcGeometry::TorsoLengthFraction * humanNpcState.Dimensions.TorsoHeightToWidthFactor;
            float const halfTorsoW = (adjustedIdealHumanHeight * torsoWidthFraction * humanNpcState.WidthMultipier) / 2.0f;

            float const leftArmLength = adjustedIdealHumanHeight * GameParameters::HumanNpcGeometry::ArmLengthFraction * animationState.LimbLengthMultipliers.LeftArm;
            float const rightArmLength = adjustedIdealHumanHeight * GameParameters::HumanNpcGeometry::ArmLengthFraction * animationState.LimbLengthMultipliers.RightArm;
            float const armWidthFraction = GameParameters::HumanNpcGeometry::ArmLengthFraction * humanNpcState.Dimensions.ArmHeightToWidthFactor;
            float const halfArmW = (adjustedIdealHumanHeight * armWidthFraction * humanNpcState.WidthMultipier) / 2.0f;

            float const leftLegLength = adjustedIdealHumanHeight * GameParameters::HumanNpcGeometry::LegLengthFraction * animationState.LimbLengthMultipliers.LeftLeg;
            float const rightLegLength = adjustedIdealHumanHeight * GameParameters::HumanNpcGeometry::LegLengthFraction * animationState.LimbLengthMultipliers.RightLeg;
            float const legWidthFraction = GameParameters::HumanNpcGeometry::LegLengthFraction * humanNpcState.Dimensions.LegHeightToWidthFactor;
            float const halfLegW = (adjustedIdealHumanHeight * legWidthFraction * humanNpcState.WidthMultipier) / 2.0f;

            if (humanNpcState.CurrentFaceOrientation != 0.0f)
            {
                //
                // Front-back
                //

                // Head

                shipRenderContext.UploadNpcTextureQuad(
                    planeId,
                    headTop - actualBodyHDir * halfHeadW,
                    headTop + actualBodyHDir * halfHeadW,
                    headBottom - actualBodyHDir * halfHeadW,
                    headBottom + actualBodyHDir * halfHeadW,
                    humanNpcState.CurrentFaceOrientation > 0.0f ? humanNpcState.TextureFrames.HeadFront : humanNpcState.TextureFrames.HeadBack,
                    npc.Highlight);

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
                    vec2f const leftArmVector = leftArmDir * leftArmLength;
                    vec2f const leftArmTraverseVector = leftArmDir.to_perpendicular() * halfArmW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftArmJointPosition - leftArmTraverseVector,
                        leftArmJointPosition + leftArmTraverseVector,
                        leftArmJointPosition + leftArmVector - leftArmTraverseVector,
                        leftArmJointPosition + leftArmVector + leftArmTraverseVector,
                        humanNpcState.TextureFrames.ArmFront,
                        npc.Highlight);

                    // Right arm (on right side of the screen)
                    vec2f const rightArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightArm, animationState.LimbAnglesSin.RightArm);
                    vec2f const rightArmVector = rightArmDir * rightArmLength;
                    vec2f const rightArmTraverseVector = rightArmDir.to_perpendicular() * halfArmW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightArmJointPosition - rightArmTraverseVector,
                        rightArmJointPosition + rightArmTraverseVector,
                        rightArmJointPosition + rightArmVector - rightArmTraverseVector,
                        rightArmJointPosition + rightArmVector + rightArmTraverseVector,
                        humanNpcState.TextureFrames.ArmFront.FlipH(),
                        npc.Highlight);

                    // Left leg (on left side of the screen)
                    vec2f const leftLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftLeg, animationState.LimbAnglesSin.LeftLeg);
                    vec2f const leftLegVector = leftLegDir * leftLegLength;
                    vec2f const leftLegTraverseVector = leftLegDir.to_perpendicular() * halfLegW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftLegJointPosition - leftLegTraverseVector,
                        leftLegJointPosition + leftLegTraverseVector,
                        leftLegJointPosition + leftLegVector - leftLegTraverseVector,
                        leftLegJointPosition + leftLegVector + leftLegTraverseVector,
                        humanNpcState.TextureFrames.LegFront,
                        npc.Highlight);

                    // Right leg (on right side of the screen)
                    vec2f const rightLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightLeg, animationState.LimbAnglesSin.RightLeg);
                    vec2f const rightLegVector = rightLegDir * rightLegLength;
                    vec2f const rightLegTraverseVector = rightLegDir.to_perpendicular() * halfLegW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightLegJointPosition - rightLegTraverseVector,
                        rightLegJointPosition + rightLegTraverseVector,
                        rightLegJointPosition + rightLegVector - rightLegTraverseVector,
                        rightLegJointPosition + rightLegVector + rightLegTraverseVector,
                        humanNpcState.TextureFrames.LegFront.FlipH(),
                        npc.Highlight);
                }
                else
                {
                    // Back

                    // Left arm (on right side of screen)
                    vec2f const leftArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftArm, -animationState.LimbAnglesSin.LeftArm);
                    vec2f const leftArmVector = leftArmDir * leftArmLength;
                    vec2f const leftArmTraverseVector = leftArmDir.to_perpendicular() * halfArmW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightArmJointPosition - leftArmTraverseVector,
                        rightArmJointPosition + leftArmTraverseVector,
                        rightArmJointPosition + leftArmVector - leftArmTraverseVector,
                        rightArmJointPosition + leftArmVector + leftArmTraverseVector,
                        humanNpcState.TextureFrames.ArmBack,
                        npc.Highlight);

                    // Right arm (on left side of the screen)
                    vec2f const rightArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightArm, -animationState.LimbAnglesSin.RightArm);
                    vec2f const rightArmVector = rightArmDir * rightArmLength;
                    vec2f const rightArmTraverseVector = rightArmDir.to_perpendicular() * halfArmW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftArmJointPosition - rightArmTraverseVector,
                        leftArmJointPosition + rightArmTraverseVector,
                        leftArmJointPosition + rightArmVector - rightArmTraverseVector,
                        leftArmJointPosition + rightArmVector + rightArmTraverseVector,
                        humanNpcState.TextureFrames.ArmBack.FlipH(),
                        npc.Highlight);

                    // Left leg (on right side of the screen)
                    vec2f const leftLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftLeg, -animationState.LimbAnglesSin.LeftLeg);
                    vec2f const leftLegVector = leftLegDir * leftLegLength;
                    vec2f const leftLegTraverseVector = leftLegDir.to_perpendicular() * halfLegW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightLegJointPosition - leftLegTraverseVector,
                        rightLegJointPosition + leftLegTraverseVector,
                        rightLegJointPosition + leftLegVector - leftLegTraverseVector,
                        rightLegJointPosition + leftLegVector + leftLegTraverseVector,
                        humanNpcState.TextureFrames.LegBack,
                        npc.Highlight);

                    // Right leg (on left side of the screen)
                    vec2f const rightLegDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightLeg, -animationState.LimbAnglesSin.RightLeg);
                    vec2f const rightLegVector = rightLegDir * rightLegLength;
                    vec2f const rightLegTraverseVector = rightLegDir.to_perpendicular() * halfLegW;
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftLegJointPosition - rightLegTraverseVector,
                        leftLegJointPosition + rightLegTraverseVector,
                        leftLegJointPosition + rightLegVector - rightLegTraverseVector,
                        leftLegJointPosition + rightLegVector + rightLegTraverseVector,
                        humanNpcState.TextureFrames.LegBack.FlipH(),
                        npc.Highlight);
                }

                // Torso

                shipRenderContext.UploadNpcTextureQuad(
                    planeId,
                    torsoTop - actualBodyHDir * halfTorsoW,
                    torsoTop + actualBodyHDir * halfTorsoW,
                    torsoBottom - actualBodyHDir * halfTorsoW,
                    torsoBottom + actualBodyHDir * halfTorsoW,
                    humanNpcState.CurrentFaceOrientation > 0.0f ? humanNpcState.TextureFrames.TorsoFront : humanNpcState.TextureFrames.TorsoBack,
                    npc.Highlight);
            }
            else
            {
                //
                // Left-Right
                //

                struct TextureQuad
                {
                    vec2f TopLeftPosition;
                    vec2f TopRightPosition;
                    vec2f BottomLeftPosition;
                    vec2f BottomRightPosition;
                    Render::TextureCoordinatesQuad TextureCoords;

                    TextureQuad() = default;
                };

                // Note: angles are with body vertical, regardless of L/R

                vec2f const leftArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.LeftArm, animationState.LimbAnglesSin.LeftArm);
                vec2f const leftArmVector = leftArmDir * leftArmLength;
                vec2f const leftArmTraverseVector = leftArmDir.to_perpendicular() * halfArmW;
                TextureQuad leftArmQuad{
                    armTop - leftArmTraverseVector,
                    armTop + leftArmTraverseVector,
                    armTop + leftArmVector - leftArmTraverseVector,
                    armTop + leftArmVector + leftArmTraverseVector,
                    humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.ArmSide : humanNpcState.TextureFrames.ArmSide.FlipH() };

                vec2f const rightArmDir = actualBodyVDir.rotate(animationState.LimbAnglesCos.RightArm, animationState.LimbAnglesSin.RightArm);
                vec2f const rightArmVector = rightArmDir * rightArmLength;
                vec2f const rightArmTraverseVector = rightArmDir.to_perpendicular() * halfArmW;
                TextureQuad rightArmQuad{
                    armTop - rightArmTraverseVector,
                    armTop + rightArmTraverseVector,
                    armTop + rightArmVector - rightArmTraverseVector,
                    armTop + rightArmVector + rightArmTraverseVector,
                    humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.ArmSide : humanNpcState.TextureFrames.ArmSide.FlipH() };

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
                    float const kneeTextureY =
                        humanNpcState.TextureFrames.LegSide.TopY
                        - animationState.UpperLegLengthFraction * (humanNpcState.TextureFrames.LegSide.TopY - humanNpcState.TextureFrames.LegSide.BottomY);

                    Render::TextureCoordinatesQuad const upperLegTextureQuad = Render::TextureCoordinatesQuad({
                        humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.LeftX : humanNpcState.TextureFrames.LegSide.RightX,
                        humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.RightX : humanNpcState.TextureFrames.LegSide.LeftX,
                        kneeTextureY,
                        humanNpcState.TextureFrames.LegSide.TopY });

                    Render::TextureCoordinatesQuad const lowerLegTextureQuad = Render::TextureCoordinatesQuad({
                        humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.LeftX : humanNpcState.TextureFrames.LegSide.RightX,
                        humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide.RightX : humanNpcState.TextureFrames.LegSide.LeftX,
                        humanNpcState.TextureFrames.LegSide.BottomY,
                        kneeTextureY });

                    // We extrude the corners to make them join nicely to the previous
                    // and next segments. The calculation of the extrusion (J) between two
                    // segments is based on these observations:
                    //  * The direction of the extrusion is along the resultant of the normals
                    //    to the two segments
                    //  * The magnitude of the extrusion is (W/2) / cos(alpha), where alpha is
                    //    the angle between a normal and the direction of the extrusion

                    float constexpr MinJ = 0.9f;

                    vec2f const leftLowerLegDir = (feetPosition - leftKneeOrFootPosition).normalise_approx();
                    vec2f const leftLowerLegVector = leftLowerLegDir * leftLegLength * lowerLegLengthFraction;
                    vec2f const leftLowerLegTraverseDir = leftLowerLegDir.to_perpendicular();
                    vec2f const leftLegResultantNormal = leftUpperLegTraverseDir + leftLowerLegTraverseDir;
                    vec2f const leftLegJ = leftLegResultantNormal / std::max(MinJ, leftUpperLegTraverseDir.dot(leftLegResultantNormal)) * halfLegW;

                    leftUpperLegQuad = {
                        legTop - leftUpperLegTraverseDir * halfLegW,
                        legTop + leftUpperLegTraverseDir * halfLegW,
                        leftKneeOrFootPosition - leftLegJ,
                        leftKneeOrFootPosition + leftLegJ,
                        upperLegTextureQuad };

                    leftLowerLegQuad = {
                        leftKneeOrFootPosition - leftLegJ,
                        leftKneeOrFootPosition + leftLegJ,
                        leftKneeOrFootPosition + leftLowerLegVector - leftLowerLegTraverseDir * halfLegW,
                        leftKneeOrFootPosition + leftLowerLegVector + leftLowerLegTraverseDir * halfLegW,
                        lowerLegTextureQuad };

                    vec2f const rightLowerLegDir = (feetPosition - rightKneeOrFootPosition).normalise_approx();
                    vec2f const rightLowerLegVector = rightLowerLegDir * rightLegLength * lowerLegLengthFraction;
                    vec2f const rightLowerLegTraverseDir = rightLowerLegDir.to_perpendicular();
                    vec2f const rightLegResultantNormal = rightUpperLegTraverseDir + rightLowerLegTraverseDir;
                    vec2f const rightLegJ = rightLegResultantNormal / std::max(MinJ, rightUpperLegTraverseDir.dot(rightLegResultantNormal)) * halfLegW;

                    rightUpperLegQuad = {
                        legTop - rightUpperLegTraverseDir * halfLegW,
                        legTop + rightUpperLegTraverseDir * halfLegW,
                        rightKneeOrFootPosition - rightLegJ,
                        rightKneeOrFootPosition + rightLegJ,
                        upperLegTextureQuad };

                    rightLowerLegQuad = {
                        rightKneeOrFootPosition - rightLegJ,
                        rightKneeOrFootPosition + rightLegJ,
                        rightKneeOrFootPosition + rightLowerLegVector - rightLowerLegTraverseDir * halfLegW,
                        rightKneeOrFootPosition + rightLowerLegVector + rightLowerLegTraverseDir * halfLegW,
                        lowerLegTextureQuad };
                }
                else
                {
                    // Just upper leg

                    leftUpperLegQuad = {
                        legTop - leftUpperLegTraverseDir * halfLegW,
                        legTop + leftUpperLegTraverseDir * halfLegW,
                        leftKneeOrFootPosition - leftUpperLegTraverseDir * halfLegW,
                        leftKneeOrFootPosition + leftUpperLegTraverseDir * halfLegW,
                        humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide : humanNpcState.TextureFrames.LegSide.FlipH() };

                    rightUpperLegQuad = {
                        legTop - rightUpperLegTraverseDir * halfLegW,
                        legTop + rightUpperLegTraverseDir * halfLegW,
                        rightKneeOrFootPosition - rightUpperLegTraverseDir * halfLegW,
                        rightKneeOrFootPosition + rightUpperLegTraverseDir * halfLegW,
                        humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.LegSide : humanNpcState.TextureFrames.LegSide.FlipH() };
                }

                // Arm and leg far

                if (humanNpcState.CurrentFaceDirectionX > 0.0f)
                {
                    // Left leg
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftUpperLegQuad.TopLeftPosition,
                        leftUpperLegQuad.TopRightPosition,
                        leftUpperLegQuad.BottomLeftPosition,
                        leftUpperLegQuad.BottomRightPosition,
                        leftUpperLegQuad.TextureCoords,
                        npc.Highlight);
                    if (leftLowerLegQuad.has_value())
                    {
                        shipRenderContext.UploadNpcTextureQuad(
                            planeId,
                            leftLowerLegQuad->TopLeftPosition,
                            leftLowerLegQuad->TopRightPosition,
                            leftLowerLegQuad->BottomLeftPosition,
                            leftLowerLegQuad->BottomRightPosition,
                            leftLowerLegQuad->TextureCoords,
                            npc.Highlight);
                    }

                    // Left arm
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftArmQuad.TopLeftPosition,
                        leftArmQuad.TopRightPosition,
                        leftArmQuad.BottomLeftPosition,
                        leftArmQuad.BottomRightPosition,
                        leftArmQuad.TextureCoords,
                        npc.Highlight);
                }
                else
                {
                    // Right leg
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightUpperLegQuad.TopLeftPosition,
                        rightUpperLegQuad.TopRightPosition,
                        rightUpperLegQuad.BottomLeftPosition,
                        rightUpperLegQuad.BottomRightPosition,
                        rightUpperLegQuad.TextureCoords,
                        npc.Highlight);
                    if (rightLowerLegQuad.has_value())
                    {
                        shipRenderContext.UploadNpcTextureQuad(
                            planeId,
                            rightLowerLegQuad->TopLeftPosition,
                            rightLowerLegQuad->TopRightPosition,
                            rightLowerLegQuad->BottomLeftPosition,
                            rightLowerLegQuad->BottomRightPosition,
                            rightLowerLegQuad->TextureCoords,
                            npc.Highlight);
                    }

                    // Right arm
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightArmQuad.TopLeftPosition,
                        rightArmQuad.TopRightPosition,
                        rightArmQuad.BottomLeftPosition,
                        rightArmQuad.BottomRightPosition,
                        rightArmQuad.TextureCoords,
                        npc.Highlight);
                }

                // Head

                shipRenderContext.UploadNpcTextureQuad(
                    planeId,
                    headTop - actualBodyHDir * halfHeadW,
                    headTop + actualBodyHDir * halfHeadW,
                    headBottom - actualBodyHDir * halfHeadW,
                    headBottom + actualBodyHDir * halfHeadW,
                    humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.HeadSide : humanNpcState.TextureFrames.HeadSide.FlipH(),
                    npc.Highlight);

                // Torso

                shipRenderContext.UploadNpcTextureQuad(
                    planeId,
                    torsoTop - actualBodyHDir * halfTorsoW,
                    torsoTop + actualBodyHDir * halfTorsoW,
                    torsoBottom - actualBodyHDir * halfTorsoW,
                    torsoBottom + actualBodyHDir * halfTorsoW,
                    humanNpcState.CurrentFaceDirectionX > 0.0f ? humanNpcState.TextureFrames.TorsoSide : humanNpcState.TextureFrames.TorsoSide.FlipH(),
                    npc.Highlight);

                // Arm and leg near

                if (humanNpcState.CurrentFaceDirectionX > 0.0f)
                {
                    // Right leg
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightUpperLegQuad.TopLeftPosition,
                        rightUpperLegQuad.TopRightPosition,
                        rightUpperLegQuad.BottomLeftPosition,
                        rightUpperLegQuad.BottomRightPosition,
                        rightUpperLegQuad.TextureCoords,
                        npc.Highlight);
                    if (rightLowerLegQuad.has_value())
                    {
                        shipRenderContext.UploadNpcTextureQuad(
                            planeId,
                            rightLowerLegQuad->TopLeftPosition,
                            rightLowerLegQuad->TopRightPosition,
                            rightLowerLegQuad->BottomLeftPosition,
                            rightLowerLegQuad->BottomRightPosition,
                            rightLowerLegQuad->TextureCoords,
                            npc.Highlight);
                    }

                    // Right arm
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        rightArmQuad.TopLeftPosition,
                        rightArmQuad.TopRightPosition,
                        rightArmQuad.BottomLeftPosition,
                        rightArmQuad.BottomRightPosition,
                        rightArmQuad.TextureCoords,
                        npc.Highlight);

                }
                else
                {
                    // Left leg
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftUpperLegQuad.TopLeftPosition,
                        leftUpperLegQuad.TopRightPosition,
                        leftUpperLegQuad.BottomLeftPosition,
                        leftUpperLegQuad.BottomRightPosition,
                        leftUpperLegQuad.TextureCoords,
                        npc.Highlight);
                    if (leftLowerLegQuad.has_value())
                    {
                        shipRenderContext.UploadNpcTextureQuad(
                            planeId,
                            leftLowerLegQuad->TopLeftPosition,
                            leftLowerLegQuad->TopRightPosition,
                            leftLowerLegQuad->BottomLeftPosition,
                            leftLowerLegQuad->BottomRightPosition,
                            leftLowerLegQuad->TextureCoords,
                            npc.Highlight);
                    }

                    // Left arm
                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        leftArmQuad.TopLeftPosition,
                        leftArmQuad.TopRightPosition,
                        leftArmQuad.BottomLeftPosition,
                        leftArmQuad.BottomRightPosition,
                        leftArmQuad.TextureCoords,
                        npc.Highlight);
                }
            }

            break;
        }

        case NpcKindType::Furniture:
        {
            if (npc.ParticleMesh.Particles.size() == 4)
            {
                // Quad
                shipRenderContext.UploadNpcTextureQuad(
                    planeId,
                    mParticles.GetPosition(npc.ParticleMesh.Particles[0].ParticleIndex),
                    mParticles.GetPosition(npc.ParticleMesh.Particles[1].ParticleIndex),
                    mParticles.GetPosition(npc.ParticleMesh.Particles[3].ParticleIndex),
                    mParticles.GetPosition(npc.ParticleMesh.Particles[2].ParticleIndex),
                    npc.KindSpecificState.FurnitureNpcState.TextureCoordinatesQuad,
                    npc.Highlight);
            }
            else
            {
                // Bunch-of-particles (each a quad)

                for (auto const & particle : npc.ParticleMesh.Particles)
                {
                    float constexpr ParticleHalfWidth = 0.15f;
                    vec2f const position = mParticles.GetPosition(particle.ParticleIndex);

                    shipRenderContext.UploadNpcTextureQuad(
                        planeId,
                        vec2f(position.x - ParticleHalfWidth, position.y + ParticleHalfWidth),
                        vec2f(position.x + ParticleHalfWidth, position.y + ParticleHalfWidth),
                        vec2f(position.x - ParticleHalfWidth, position.y - ParticleHalfWidth),
                        vec2f(position.x + ParticleHalfWidth, position.y - ParticleHalfWidth),
                        npc.KindSpecificState.FurnitureNpcState.TextureCoordinatesQuad,
                        npc.Highlight);
                }
            }

            break;
        }
    }
}

void Npcs::UpdateNpcAnimation(
    StateType & npc,
    float currentSimulationTime,
    Ship const & homeShip)
{
    if (npc.Kind == NpcKindType::Human) // Take the primary as the only representative of a human
    {
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
        FS_ALIGN16_BEG LimbVector targetAngles(animationState.LimbAngles) FS_ALIGN16_END;

        float convergenceRate = 0.0f;

        // Stuff we calc in some cases and which we need again later for lengths
        float humanEdgeAngle = 0.0f;
        float adjustedStandardHumanHeight = 0.0f;
        vec2f edg1, edg2, edgVector, edgDir;
        vec2f feetPosition, actualBodyVector, actualBodyDir;
        float periodicValue = 0.0f;

        float targetUpperLegLengthFraction = 1.0f;

        // Angle of human wrt edge until which arm is angled to the max
        // (extent of early stage during rising)
        float constexpr MaxHumanEdgeAngleForArms = 0.40489178628508342331207292900944f;
        //static_assert(MaxHumanEdgeAngleForArms == std::atan(GameParameters::HumanNpcGeometry::ArmLengthFraction / (1.0f - GameParameters::HumanNpcGeometry::HeadLengthFraction)));

        switch (humanNpcState.CurrentBehavior)
        {
            case HumanNpcStateType::BehaviorType::BeingPlaced:
            {
                float const arg =
                    (
                        (currentSimulationTime - humanNpcState.CurrentStateTransitionSimulationTimestamp) * 1.0f
                        + humanNpcState.TotalDistanceTraveledOffEdgeSinceStateTransition * 0.2f
                        ) * (1.0f + humanNpcState.ResultantPanicLevel * 0.2f)
                    * (Pi<float> *2.0f + npc.RandomNormalizedUniformSeed * 4.0f);

                float const yArms = std::sin(arg);
                targetAngles.RightArm = Pi<float> / 2.0f + Pi<float> / 2.0f * 0.7f * yArms;
                targetAngles.LeftArm = -targetAngles.RightArm;

                float const yLegs = std::sin(arg + npc.RandomNormalizedUniformSeed * Pi<float> *2.0f);
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

                // Legs stay

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
                    float constexpr OtherLegAlphaAngle = 0.7f;

                    // Shortening of angle path for legs becoming straight when closing knees
                    float const AnglePathShorteningForLegsInLateStage = 0.9f;

                    //

                    //  *  0 --> maxHumanEdgeAngle (which is PI/2 when edge is flat)
                    //   \
                    //   |\
                    // -----

                    // Arm: at MaxArmAngle until MaxHumanEdgeAngleForArms, then goes down to rest

                    float targetArm;
                    float targetLeg = 0.0f; // Start with legs closed - we'll change if we're in the early stage of rising and we're L/R

                    if (humanEdgeAngle <= MaxHumanEdgeAngleForArms)
                    {
                        // Early stage

                        // Arms: leave them where they are (MaxArmAngle)
                        targetArm = MaxArmAngle;

                        // Legs: we want a knee (iff we're facing L/R)

                        if (humanNpcState.CurrentFaceOrientation == 0.0f)
                        {
                            targetLeg = MaxArmAngle;
                            targetUpperLegLengthFraction = humanEdgeAngle / MaxHumanEdgeAngleForArms * 0.5f; // 0.0 @ 0.0 -> 0.5 @ MaxHumanEdgeAngleForArms
                        }
                    }
                    else
                    {
                        // Late stage: -> towards maxHumanEdgeAngle

                        // Arms: MaxArmAngle -> RestArmAngle

                        targetArm = MaxArmAngle + (MaxHumanEdgeAngleForArms - humanEdgeAngle) / (MaxHumanEdgeAngleForArms - maxHumanEdgeAngle) * (RestArmAngle - MaxArmAngle); // MaxArmAngle @ MaxHumanEdgeAngleForArms -> RestArmAngle @ maxHumanEdgeAngle

                        // Legs: towards zero

                        if (humanNpcState.CurrentFaceOrientation == 0.0f)
                        {
                            targetLeg = std::max(
                                MaxArmAngle - (MaxHumanEdgeAngleForArms - humanEdgeAngle) / (MaxHumanEdgeAngleForArms - maxHumanEdgeAngle * AnglePathShorteningForLegsInLateStage) * MaxArmAngle, // MaxArmAngle @ MaxHumanEdgeAngleForArms -> 0 @ maxHumanEdgeAngle-e
                                0.0f);
                            targetUpperLegLengthFraction = 0.5f;
                        }
                    }

                    // Knees cannot bend backwards!
                    if ((humanNpcState.CurrentFaceDirectionX > 0.0f && isOnLeftSide)
                        || (humanNpcState.CurrentFaceDirectionX < 0.0f && !isOnLeftSide))
                    {
                        targetLeg *= -1.0f;
                    }

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
                    0.41f // std::atan((GameParameters::HumanNpcGeometry::StepLengthFraction / 2.0f) / GameParameters::HumanNpcGeometry::LegLengthFraction)
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
                    float const halfPeriod = 1.0f - 0.6f * std::min(humanNpcState.ResultantPanicLevel, 4.0f) / 4.0f;
                    float const inPeriod = FastMod(elapsed, halfPeriod * 2.0f);

                    float constexpr MaxAngle = Pi<float> / 2.0f;
                    float const angle = std::abs(halfPeriod - inPeriod) / halfPeriod * 2.0f * MaxAngle - MaxAngle;

                    // PanicMultiplier: p=0.0 => 0.7 p=2.0 => 0.4
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

                    ElementIndex const edgeElementIndex =
                        homeShip.GetTriangles().GetSubSprings(primaryContrainedState->CurrentVirtualFloor->TriangleElementIndex)
                        .SpringIndices[primaryContrainedState->CurrentVirtualFloor->EdgeOrdinal];
                    // Note: we do not care if not in CW order
                    edg1 = homeShip.GetSprings().GetEndpointAPosition(edgeElementIndex, homeShip.GetPoints());
                    edg2 = homeShip.GetSprings().GetEndpointBPosition(edgeElementIndex, homeShip.GetPoints());
                    edgVector = edg2 - edg1;
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
                    targetAngles.LeftArm = Pi<float>;
                }

                // Legs: 0

                targetAngles.RightLeg = 0.0f;
                targetAngles.LeftLeg = 0.0f;

                convergenceRate = 0.2f;

                // Upper length fraction: when we transition from Rising (which has UpperLengthFraction < 1.0) to KnockedOut,
                // changing UpperLengthFraction immediately to 0.0 causes a "kick" (because leg angles are 90 degrees at that moment);
                // smooth that kick here
                targetUpperLegLengthFraction = animationState.UpperLegLengthFraction + (1.0f - animationState.UpperLegLengthFraction) * 0.3f;

                // But converge to one definitely
                if (targetUpperLegLengthFraction >= 0.98f)
                {
                    targetUpperLegLengthFraction = 1.0f;
                }

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

                vec2f const resultantVelocity = (mParticles.GetVelocity(primaryParticleIndex) + mParticles.GetVelocity(secondaryParticleIndex)) / 2.0f;
                float const resVelPerpToBody = resultantVelocity.dot(actualBodyDir.to_perpendicular()); // Positive when pointing towards right
                float const legAngle = SmoothStep(0.0f, 4.0f, std::abs(resVelPerpToBody)) * 0.5f;
                if (resVelPerpToBody >= 0.0f)
                {
                    // Res vel to the right - legs to the left
                    targetAngles.RightLeg = -legAngle;
                    targetAngles.LeftLeg = targetAngles.RightLeg - 0.6f;
                }
                else
                {
                    // Res vel to the left - legs to the right
                    targetAngles.LeftLeg = legAngle;
                    targetAngles.RightLeg = targetAngles.LeftLeg + 0.6f;
                }

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
        }

        // Converge
        animationState.LimbAngles.ConvergeTo(targetAngles, convergenceRate);
        animationState.UpperLegLengthFraction = targetUpperLegLengthFraction;

        // Calculate sins and coss
        SinCos4(animationState.LimbAngles.fptr(), animationState.LimbAnglesSin.fptr(), animationState.LimbAnglesCos.fptr());

        //
        // Length Multipliers
        //

        FS_ALIGN16_BEG LimbVector targetLengthMultipliers({ 1.0f, 1.0f, 1.0f, 1.0f }) FS_ALIGN16_END;
        float limbLengthConvergenceRate = convergenceRate;

        float targetLowerExtremityLengthMultiplier = 1.0f;

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
                // Take into account that crotch is lower
                targetLowerExtremityLengthMultiplier = animationState.LimbAnglesCos.RightLeg;

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
                    vec2f const crotchPosition = feetPosition - actualBodyVector * (GameParameters::HumanNpcGeometry::LegLengthFraction * targetLowerExtremityLengthMultiplier);

                    // leg*1 is crotchPosition
                    float const numerator = (edg1.y - crotchPosition.y) * (edg2.x - edg1.x) + (crotchPosition.x - edg1.x) * (edg2.y - edg1.y);

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

            case HumanNpcStateType::BehaviorType::BeingPlaced:
            case HumanNpcStateType::BehaviorType::Constrained_Equilibrium:
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
            {
                // Nop
                break;
            }
        }

        // Converge
        animationState.LimbLengthMultipliers.ConvergeTo(targetLengthMultipliers, limbLengthConvergenceRate);
        animationState.LowerExtremityLengthMultiplier += (targetLowerExtremityLengthMultiplier - animationState.LowerExtremityLengthMultiplier) * convergenceRate;
    }
}

}