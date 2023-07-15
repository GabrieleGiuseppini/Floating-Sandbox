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

            HumanState(
                HumanNpcRoleType role)
                : Role(role)
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
        NpcId Id;

        NpcType Type;

        // Current regime
        RegimeType Regime;

        // (Primary) particle for this NPC
        ElementIndex PrimaryParticleIndex;

        // Current hightlight
        NpcHighlightType Highlight;

        // Only set in constrained regime
        std::optional<ElementIndex> TriangleIndex;

        // Randomness specific to this NPC
        float RandomNormalizedUniformSeed;

        // The type-specific additional state
        TypeSpecificNpcState TypeSpecificState;

        NpcState(
            NpcId const & id,
            RegimeType regime,
            ElementIndex primaryParticleIndex,
            NpcHighlightType highlight,
            std::optional<ElementIndex> const & triangleIndex,
            TypeSpecificNpcState::HumanState const & humanState)
            : Id(id)
            , Type(NpcType::Human)
            , Regime(regime)
            , PrimaryParticleIndex(primaryParticleIndex)
            , Highlight(highlight)
            , TriangleIndex(triangleIndex)
            , RandomNormalizedUniformSeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
            , TypeSpecificState(humanState)

        {}
    };

public:

    Npcs(
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
        : mMaterialDatabase(materialDatabase)
        , mGameEventHandler(std::move(gameEventDispatcher))
        // Storage
        , mNpcShipsByShipId()
        , mNpcEntriesByNpcId()
        , mParticles(GameParameters::MaxNpcs) // FUTUREWORK: multiply accordingly when moving on to non-single-particle NPCs
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

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

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
        vec2f const & initialPosition,
        RegimeType initialRegime,      
        NpcHighlightType initialHighlight,
        ShipId const & shipId,
        std::optional<ElementIndex> triangleIndex);

    NpcState & InternalMoveNpcBy(
        NpcId const & id,
        vec2f const & offset);

    void OnNpcDestroyed(NpcState const & state);
    
    inline NpcState & GetNpcState(NpcId const & id);
    inline NpcState & GetNpcState(ShipId const & shipId, LocalNpcId const & localNpcId);

    inline std::optional<ElementId> FindTopmostContainingTriangle(vec2f const & position) const;

    inline bool IsTriangleSuitableForNpc(
        NpcType type,
        ShipId shipId,
        std::optional<ElementIndex> const & triangleIndex) const;

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
        std::vector<NpcState> NpcStates;

        NpcShip(Ship const & shipRef)
            : ShipRef(shipRef)
            , NpcStates()
        {}
    };

    std::vector<std::optional<NpcShip>> mNpcShipsByShipId; // Indexed by ship ID

    struct NpcEntry
    {
        ShipId Ship;
        ElementIndex StateOrdinal;

        NpcEntry(
            ShipId ship,
            ElementIndex stateOrdinal)
            : Ship(ship)
            , StateOrdinal(stateOrdinal)
        {}
    };

    // Here we lie a bit: the local NPC ID is actually *global*, or else we can't guarantee its stability
    // when an NPC changes ships
    std::vector<std::optional<NpcEntry>> mNpcEntriesByNpcId; // Indexed by local (actually global) NPC ID

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
