/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "Materials.h"
#include "MaterialDatabase.h"
#include "NpcParticles.h"

#include <GameCore/Buffer.h>
#include <GameCore/BufferAllocator.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

namespace Physics
{

/*
 * Container of NPCs.
 */
class Npcs final
{
private:

    /*
     * The physical regimes that a given NPC may be in at any given moment.
     */
    enum class RegimeType
    {
        Constrained,
        Free
    };

    /*
     * The type-specific part of an NPC state.
     */
    union TypeSpecificNpcState
    {
        struct HumanState
        {
            HumanNpcRoleType Role;
            ElementIndex FeetParticleIndex;

            HumanState(
                HumanNpcRoleType role,
                ElementIndex feetParticleIndex)
                : Role(role)
                , FeetParticleIndex(feetParticleIndex)
            {}
        };

        HumanState Human;

        TypeSpecificNpcState(HumanState human)
            : Human(human)
        {}
    };

    /*
     * The NPC state.
     */
    struct NpcState
    {
        NpcType Type;

        // When not set it's in free regime
        std::optional<ElementIndex> TriangleIndex;

        // Randomness specific to this NPC
        float RandomNormalizedUniformSeed;

        // The type-specific state
        TypeSpecificNpcState TypeSpecificState;

        NpcState(
            std::optional<ElementIndex> const & triangleIndex,
            TypeSpecificNpcState::HumanState const & humanState)
            : Type(NpcType::Human)
            , TriangleIndex(triangleIndex)
            , RandomNormalizedUniformSeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
            , TypeSpecificState(humanState)

        {}

        bool IsInFreeRegime() const
        {
            return !TriangleIndex.has_value();
        }

        bool IsInConstrainedRegime() const
        {
            return TriangleIndex.has_value();
        }
    };

public:

    Npcs(
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
        : mMaterialDatabase(materialDatabase)
        , mGameEventHandler(std::move(gameEventDispatcher))
        // Storage
        , mStateByShip()
        , mShipIdToShipOrdinalIndex()
        , mNpcIdToNpcOrdinalIndex()
        , mParticles(GameParameters::MaxNpcs) // TODO: multiply accordingly when moving on to non-single-particle NPCs
        // State
        , mNpcCount(0)
        , mFreeRegimeHumanNpcCount(0)
        , mConstrainedRegimeHumanNpcCount(0)
        , mAreElementsDirtyForRendering(true)
    {
    }

    Npcs(Npcs && other) = default;

    void OnShipAdded(ShipId shipId);

    void OnShipRemoved(ShipId shipId);

    NpcId AddHumanNpc(
        HumanNpcRoleType role,
        Ship const & ship,
        vec2f const & position,
        std::optional<ElementIndex> triangleIndex);

    void RemoveNpc(NpcId const id);

private:

private:

    MaterialDatabase const & mMaterialDatabase;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;

    //
    // Container
    //    

    // State: organized by ship index (not ship ID), compacted at addition/removal
    std::vector<std::vector<NpcState>> mStateByShip;

    // Indices mapping global IDs to our ordinals
    std::vector<ElementIndex> mShipIdToShipOrdinalIndex;
    std::vector<std::vector<ElementIndex>> mNpcIdToNpcOrdinalIndex; // Indexed by ship ID

    // All particles
    NpcParticles mParticles;

    //
    // State
    //

    // Stats
    ElementCount mNpcCount;
    ElementCount mFreeRegimeHumanNpcCount;
    ElementCount mConstrainedRegimeHumanNpcCount;

    // Flag remembering whether the set of NPCs is dirty
    // (i.e. whether there are more or less NPCs than previously
    // reported to the rendering engine)
    bool mutable mAreElementsDirtyForRendering;
};

}
