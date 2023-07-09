/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cassert>

namespace Physics {

void Npcs::OnShipAdded(ShipId shipId)
{
	//
	// Buffers
	//

	// Add buffers
	ElementIndex const newShipOrdinal = static_cast<ElementIndex>(mStateByShip.size());
	mStateByShip.emplace_back();

	//
	// Indices
	//

	// Make room in indices
	while (static_cast<size_t>(shipId) >= mShipIdToShipOrdinalIndex.size())
	{
		mShipIdToShipOrdinalIndex.emplace_back(NoneElementIndex);
		mNpcIdToNpcOrdinalIndex.emplace_back();
	}

	// We do not know about this ship yet
	assert(mShipIdToShipOrdinalIndex[static_cast<size_t>(shipId)] == NoneElementIndex);

	// Store ShipId->ShipOrdinal
	mShipIdToShipOrdinalIndex[static_cast<size_t>(shipId)] = newShipOrdinal;
}

void Npcs::OnShipRemoved(ShipId shipId)
{
	// We know about this ship
	assert(static_cast<size_t>(shipId) < mShipIdToShipOrdinalIndex.size());
	assert(static_cast<size_t>(shipId) < mNpcIdToNpcOrdinalIndex.size());
	ElementIndex const oldShipOrdinal = mShipIdToShipOrdinalIndex[static_cast<size_t>(shipId)];
	assert(oldShipOrdinal != NoneElementIndex);

	//
	// Buffers
	//

	mStateByShip.erase(mStateByShip.begin() + oldShipOrdinal);

	//
	// Indices
	//

	// Forget about this ship
	mShipIdToShipOrdinalIndex[static_cast<size_t>(shipId)] = NoneElementIndex;

	// Update subsequent ship ordinals
	for (auto & o : mShipIdToShipOrdinalIndex)
	{
		if (o != NoneElementIndex
			&& o > oldShipOrdinal)
		{
			--o;
		}
	}

	// Forget about the NPCs of this ship
	mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].clear();

	// Remember we're dirty
	mAreElementsDirtyForRendering = true;
}

NpcId Npcs::AddHumanNpc(
	HumanNpcRoleType role,
	Ship const & ship,
	std::optional<ElementIndex> triangleIndex)
{
	auto const shipId = ship.GetId();

	// We know about this ship
	assert(static_cast<size_t>(shipId) < mShipIdToShipOrdinalIndex.size());
	assert(static_cast<size_t>(shipId) < mNpcIdToNpcOrdinalIndex.size());
	ElementIndex const shipOrdinal = mShipIdToShipOrdinalIndex[static_cast<size_t>(shipId)];
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
	// Buffers
	//

	// Add buffers	
	auto const & state = mStateByShip[shipOrdinal].emplace_back(
		triangleIndex,
		TypeSpecificNpcState::HumanState(role));

	//
	// Update state
	//

	++mNpcCount;

	if (state.IsInConstrainedRegime())
	{
		++mConstrainedRegimeHumanNpcCount;
	}

	if (state.IsInFreeRegime())
	{
		++mFreeRegimeHumanNpcCount;
	}

	mGameEventHandler->OnNpcStatisticsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	// Remember we're dirty
	mAreElementsDirtyForRendering = true;

	return NpcId(shipId, newNpcId);
}

void Npcs::RemoveNpc(NpcId const id)
{
	auto const shipId = id.GetShipId();
	auto const localNpcId = id.GetLocalObjectId();

	// We know about this ship
	assert(static_cast<size_t>(shipId) < mShipIdToShipOrdinalIndex.size());
	assert(static_cast<size_t>(shipId) < mNpcIdToNpcOrdinalIndex.size());
	ElementIndex const shipOrdinal = mShipIdToShipOrdinalIndex[static_cast<size_t>(shipId)];
	assert(shipOrdinal != NoneElementIndex);

	// We know about this NPC
	assert(static_cast<size_t>(localNpcId) < mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)].size());
	ElementIndex const oldNpcOrdinal = mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][static_cast<size_t>(localNpcId)];
	assert(oldNpcOrdinal != NoneElementIndex);

	//
	// Update state
	//

	auto const & state = mStateByShip[shipOrdinal][oldNpcOrdinal];

	--mNpcCount;

	if (state.IsInConstrainedRegime())
	{
		--mConstrainedRegimeHumanNpcCount;
	}

	if (state.IsInFreeRegime())
	{
		--mFreeRegimeHumanNpcCount;
	}

	mGameEventHandler->OnNpcStatisticsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	//
	// Buffers
	//

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

	// Remember we're dirty
	mAreElementsDirtyForRendering = true;
}

}