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

class Ship;

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
        Free,
        Placement
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

        // Current regime
        RegimeType Regime;

        // Current hightlight
        NpcHighlightType Highlight;

        // Only set in constrained regime
        std::optional<ElementIndex> TriangleIndex;

        // Randomness specific to this NPC
        float RandomNormalizedUniformSeed;

        // The type-specific state
        TypeSpecificNpcState TypeSpecificState;

        NpcState(
            RegimeType regime,
            NpcHighlightType highlight,
            std::optional<ElementIndex> const & triangleIndex,
            TypeSpecificNpcState::HumanState const & humanState)
            : Type(NpcType::Human)
            , Regime(regime)
            , Highlight(highlight)
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
        , mShipIdToShipIndex()
        , mNpcIdToNpcOrdinalIndex()
        , mParticles(GameParameters::MaxNpcs) // TODO: multiply accordingly when moving on to non-single-particle NPCs
        // State
        , mNpcCount(0)
        , mFreeRegimeHumanNpcCount(0)
        , mConstrainedRegimeHumanNpcCount(0)
        , mAreStaticRenderAttributesDirty(true)
    {
        mGameEventHandler->OnNpcCountsUpdated(
            mNpcCount,
            mConstrainedRegimeHumanNpcCount,
            mFreeRegimeHumanNpcCount,
            GameParameters::MaxNpcs - mNpcCount);
    }

    Npcs(Npcs && other) = default;

    void OnShipAdded(Ship const & ship);

    void OnShipRemoved(ShipId shipId);

    // Interactions

    std::optional<NpcId> PickNpc(vec2f const & position) const;

    void BeginMoveNpc(NpcId const & id);

    NpcId BeginMoveNewHumanNpc(
        HumanNpcRoleType role,
        vec2f const & initialPosition);

    bool IsSuitableNpcPosition(
        NpcId const & id,
        vec2f const & position) const;

    bool MoveNpcBy(
        NpcId const & id,
        vec2f const & offset);

    void EndMoveNpc(
        NpcId const & id,
        vec2f const & finalOffset);

    void AbortNewNpc(NpcId const & id);

    void HighlightNpc(
        NpcId const & id, 
        NpcHighlightType highlight);

    void RemoveNpc(NpcId const & id);

private:

    NpcId AddHumanNpc(
        HumanNpcRoleType role,
        vec2f const & position,
        RegimeType initialRegime,      
        NpcHighlightType initialHighlight,
        ShipId const & shipId,
        std::optional<ElementIndex> triangleIndex);

    inline NpcState & GetNpcState(ShipId const & shipId, LocalNpcId const & localNpcId);

    inline std::optional<ElementId> FindContainingTriangle(vec2f const & position) const;

    ShipId GetTopmostShipId() const;

private:

    MaterialDatabase const & mMaterialDatabase;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;

    //
    // Container
    //

    struct NpcShip
    {
        Ship const & ShipRef;
        ElementIndex Ordinal;

        NpcShip(
            Ship const & shipRef,
            ElementIndex ordinal)
            : ShipRef(shipRef)
            , Ordinal(ordinal)
        {}
    };

    // State: organized by ship index (not ship ID), compacted at addition/removal
    std::vector<std::vector<NpcState>> mStateByShip;

    // Indices mapping global IDs to our ordinals
    std::vector<std::optional<NpcShip>> mShipIdToShipIndex; // Indexed by ship ID
    std::vector<std::vector<ElementIndex>> mNpcIdToNpcOrdinalIndex; // Indexed by ship ID and local NPC ID

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
    bool mutable mAreStaticRenderAttributesDirty;
};

}
