/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cassert>

namespace Physics {

void Npcs::OnShipAdded(Ship const & ship)
{
	//
	// State buffer
	//

	// Make new state buffer
	ElementIndex const newShipOrdinal = static_cast<ElementIndex>(mStateByShip.size());
	mStateByShip.emplace_back();

	//
	// Indices
	//

	// Make room in indices
	while (static_cast<size_t>(ship.GetId()) >= mShipIdToShipIndex.size())
	{
		mShipIdToShipIndex.emplace_back();
		mNpcIdToNpcOrdinalIndex.emplace_back();
	}

	// We do not know about this ship yet
	assert(!mShipIdToShipIndex[static_cast<size_t>(ship.GetId())].has_value());

	// Store Ship
	mShipIdToShipIndex[static_cast<size_t>(ship.GetId())].emplace(ship, newShipOrdinal);
}

void Npcs::OnShipRemoved(ShipId shipId)
{
	// We know about this ship
	assert(static_cast<size_t>(shipId) < mShipIdToShipIndex.size());
	assert(static_cast<size_t>(shipId) < mNpcIdToNpcOrdinalIndex.size());
	assert(mShipIdToShipIndex[static_cast<size_t>(shipId)].has_value());
	ElementIndex const oldShipOrdinal = mShipIdToShipIndex[static_cast<size_t>(shipId)]->Ordinal;
	assert(oldShipOrdinal != NoneElementIndex);

	//
	// Update NPCs
	//

	for (auto const & state : mStateByShip[oldShipOrdinal])
	{
		--mNpcCount;

		switch (state.Type)
		{
			case NpcType::Human:
			{
				// Free particle
				mParticles.Remove(state.TypeSpecificState.Human.FeetParticleIndex);

				// Update stats

				if (state.IsInConstrainedRegime())
				{
					--mConstrainedRegimeHumanNpcCount;
				}

				if (state.IsInFreeRegime())
				{
					--mFreeRegimeHumanNpcCount;
				}

				break;
			}
		}
	}

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	//
	// State buffer
	//

	// Remove state buffer
	mStateByShip.erase(mStateByShip.begin() + oldShipOrdinal);

	//
	// Indices
	//

	// Forget about this ship
	mShipIdToShipIndex[static_cast<size_t>(shipId)].reset();

	// Update subsequent ship ordinals
	for (auto & s : mShipIdToShipIndex)
	{
		if (s.has_value() && s->Ordinal > oldShipOrdinal)
		{
			--(s->Ordinal);
		}
	}

	// Forget about the NPCs of this ship
	mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].clear();

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;
}

std::optional<NpcId> Npcs::PickNpc(vec2f const & position) const
{
	// TODO
	(void)position;
	return std::nullopt;
}

void Npcs::BeginMoveNpc(NpcId const & id)
{
	NpcState & state = GetNpcState(id.GetShipId(), id.GetLocalObjectId());

	// This NPC is not in placement already
	assert(state.Regime != RegimeType::Placement);

	// Transition to placement
	state.Regime = RegimeType::Placement;

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;
}

NpcId Npcs::BeginMoveNewHumanNpc(
	HumanNpcRoleType role,
	vec2f const & initialPosition)
{
	// Find triangle that this NPC belongs to
	auto const triangleId = FindContainingTriangle(initialPosition);

	// Create NPC in placement regime
	return AddHumanNpc(
		role,
		initialPosition,
		RegimeType::Placement,
		NpcHighlightType::None,
		triangleId ? triangleId->GetShipId() : GetTopmostShipId(),
		triangleId ? triangleId->GetLocalObjectId() : NoneElementIndex);
}

bool Npcs::IsSuitableNpcPosition(
	NpcId const & id,
	vec2f const & position) const
{
	// Find triangle ID; check conditions

	// TODO
	(void)id;
	(void)position;
	return true;
}

bool Npcs::MoveNpcBy(
	NpcId const & id,
	vec2f const & offset)
{
	// Eventually moves around ships, but no regime change

	// TODO: assert it's in Placement regime

	// TODO
	(void)id;
	(void)offset;
	return true;
}

void Npcs::EndMoveNpc(
	NpcId const & id,
	vec2f const & finalOffset)
{
	// Find triangle ID; exit placement regime

	// TODO: assert it's in Placement regime
	// TODO: remember to update stats for exiting placement

	// TODO
	(void)id;
	(void)finalOffset;
}

void Npcs::AbortNewNpc(NpcId const & id)
{
	// Remove NPC
	RemoveNpc(id);
}

void Npcs::HighlightNpc(
	NpcId const & id,
	NpcHighlightType highlight)
{
	// TODO: find NPC and set highlight
	// TODO: remember elements dirty

	// TODO
	(void)id;
	(void)highlight;
}

void Npcs::RemoveNpc(NpcId const & id)
{
	auto const shipId = id.GetShipId();
	auto const localNpcId = id.GetLocalObjectId();

	auto const & npcState = GetNpcState(shipId, localNpcId);

	ElementIndex const shipOrdinal = mShipIdToShipIndex[static_cast<size_t>(shipId)]->Ordinal;
	ElementIndex const oldNpcOrdinal = mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][static_cast<size_t>(localNpcId)];

	//
	// Update NPC
	//

	--mNpcCount;

	switch (npcState.Type)
	{
		case NpcType::Human:
		{
			// Free particle
			mParticles.Remove(npcState.TypeSpecificState.Human.FeetParticleIndex);

			// Update stats
			if (npcState.Regime != RegimeType::Placement)
			{
				if (npcState.IsInConstrainedRegime())
				{
					--mConstrainedRegimeHumanNpcCount;
				}

				if (npcState.IsInFreeRegime())
				{
					--mFreeRegimeHumanNpcCount;
				}
			}

			break;
		}
	}

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	//
	// State buffers
	//

	// Remove NPC state
	mStateByShip[shipOrdinal].erase(mStateByShip[shipOrdinal].begin() + oldNpcOrdinal);

	//
	// Indices
	//

	// Forget about this NPC
	mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][static_cast<size_t>(localNpcId)] = NoneElementIndex;

	// Update subsequent NPC ordinals
	for (auto & o : mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)])
	{
		if (o != NoneElementIndex
			&& o > oldNpcOrdinal)
		{
			--o;
		}
	}

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

NpcId Npcs::AddHumanNpc(
	HumanNpcRoleType role,
	vec2f const & position,
	RegimeType initialRegime,
	NpcHighlightType initialHighlight,
	ShipId const & shipId,
	std::optional<ElementIndex> triangleIndex)
{
	assert(mNpcCount < GameParameters::MaxNpcs); // We still have room for an extra NPC

	// We know about this ship
	assert(static_cast<size_t>(shipId) < mShipIdToShipIndex.size());
	assert(static_cast<size_t>(shipId) < mNpcIdToNpcOrdinalIndex.size());
	assert(mShipIdToShipIndex[static_cast<size_t>(shipId)].has_value());
	ElementIndex const shipOrdinal = mShipIdToShipIndex[static_cast<size_t>(shipId)]->Ordinal;
	assert(shipOrdinal != NoneElementIndex);

	// Make ordinal for this NPC
	ElementIndex const newNpcOrdinal = static_cast<ElementIndex>(mStateByShip[shipOrdinal].size());

	//
	// Make (ship-local) stable ID for this NPC, and update indices
	//

	LocalNpcId newNpcId = NoneLocalNpcId;
	for (size_t npcId = 0; npcId < mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].size(); ++npcId)
	{
		if (mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][npcId] == NoneElementIndex)
		{
			// Found!
			newNpcId = static_cast<LocalNpcId>(npcId);
			mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][npcId] = newNpcOrdinal;
			break;
		}
	}

	if (newNpcId == NoneLocalNpcId)
	{
		// Add to the end
		newNpcId = static_cast<LocalNpcId>(mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].size());
		mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].emplace_back(newNpcOrdinal);
	}

	//
	// State buffer
	//

	// Take particle
	ElementIndex const feetParticleIndex = mParticles.Add(
		position,
		mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Human));

	// Add NPC state
	auto const & state = mStateByShip[shipOrdinal].emplace_back(
		initialRegime,
		initialHighlight,
		triangleIndex,
		TypeSpecificNpcState::HumanState(role, feetParticleIndex));

	//
	// Update state
	//

	++mNpcCount;

	if (initialRegime != RegimeType::Placement)
	{
		if (state.IsInConstrainedRegime())
		{
			++mConstrainedRegimeHumanNpcCount;
		}

		if (state.IsInFreeRegime())
		{
			++mFreeRegimeHumanNpcCount;
		}
	}

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;

	return NpcId(shipId, newNpcId);
}

Npcs::NpcState & Npcs::GetNpcState(ShipId const & shipId, LocalNpcId const & localNpcId)
{
	// We know about this ship
	assert(static_cast<size_t>(shipId) < mShipIdToShipIndex.size());
	assert(static_cast<size_t>(shipId) < mNpcIdToNpcOrdinalIndex.size());
	assert(mShipIdToShipIndex[static_cast<size_t>(shipId)].has_value());
	ElementIndex const shipOrdinal = mShipIdToShipIndex[static_cast<size_t>(shipId)]->Ordinal;
	assert(shipOrdinal != NoneElementIndex);

	// We know about this NPC
	assert(static_cast<size_t>(localNpcId) < mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].size());
	ElementIndex const npcOrdinal = mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][static_cast<size_t>(localNpcId)];
	assert(npcOrdinal != NoneElementIndex);

	return mStateByShip[shipOrdinal][npcOrdinal];
}

std::optional<ElementId> Npcs::FindContainingTriangle(vec2f const & position) const
{
	// TODO
	(void)position;
	return std::nullopt;
}

ShipId Npcs::GetTopmostShipId() const
{
	assert(mShipIdToShipIndex.size() >= 1);

	for (ShipId shipId = static_cast<ShipId>(mShipIdToShipIndex.size()) - 1; ; )
	{
		if (mShipIdToShipIndex[shipId].has_value())
		{
			return shipId;
		}

		assert(shipId > 0);
		--shipId;
	}
}

}