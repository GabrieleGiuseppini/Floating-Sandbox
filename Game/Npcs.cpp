/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameGeometry.h>

#include <cassert>
#include <limits>

namespace Physics {

static float constexpr HumanNpcSize = 1.80f;

void Npcs::OnShipAdded(Ship const & ship)
{
	size_t const s = static_cast<size_t>(ship.GetId());

	// Make room for ship
	while (s >= mNpcShipsByShipId.size())
	{
		mNpcShipsByShipId.emplace_back();
	}

	// We do not know about this ship yet
	assert(!mNpcShipsByShipId[s].has_value());

	// Initialize NPC Ship
	mNpcShipsByShipId[s].emplace(ship);
}

void Npcs::OnShipRemoved(ShipId shipId)
{
	size_t const s = static_cast<size_t>(shipId);

	// We know about this ship
	assert(s < mNpcShipsByShipId.size());
	assert(mNpcShipsByShipId[s].has_value());

	//
	// Destroy all NPCs of this NPC ship
	//

	for (auto const & npcState : mNpcShipsByShipId[s]->NpcStates)
	{
		OnNpcDestroyed(npcState);
	}

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	//
	// Destroy NPC ship
	//

	mNpcShipsByShipId[s].reset();

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;
}

void Npcs::Update(
	float currentSimulationTime,
	GameParameters const & gameParameters)
{
	for (auto const & npcShip : mNpcShipsByShipId)
	{
		if (npcShip)
		{
			// TODOHERE
			(void)currentSimulationTime;
			(void)gameParameters;
		}
	}
}

void Npcs::Upload(Render::RenderContext & renderContext) const
{
	// Upload all ships
	for (auto const & npcShip : mNpcShipsByShipId)
	{
		if (npcShip)
		{
			Ship const & ship = npcShip->ShipRef;

			Render::ShipRenderContext & shipRenderContext = renderContext.GetShipRenderContext(ship.GetId());

			if (mAreStaticRenderAttributesDirty)
			{
				shipRenderContext.UploadNpcStaticAttributesStart(npcShip->NpcStates.size());
			}

			shipRenderContext.UploadNpcQuadsStart(npcShip->NpcStates.size());

			for (auto const & npcState : npcShip->NpcStates)
			{
				if (mAreStaticRenderAttributesDirty)
				{
					static vec4f constexpr HightlightColors[] = {
						vec4f(0.760f, 0.114f, 0.114f, 1.0f),	// Error
						vec4f(0.208f, 0.590f, 0.0177f, 1.0f),	// Selected
						vec4f::zero()							// None
					};

					shipRenderContext.UploadNpcStaticAttributes(HightlightColors[static_cast<size_t>(npcState.Highlight)]);
				}

				auto const planeId = npcState.TriangleIndex
					? ship.GetPoints().GetPlaneId(ship.GetTriangles().GetPointAIndex(*(npcState.TriangleIndex)))
					: ship.GetMaxPlaneId();

				shipRenderContext.UploadNpcQuad(
					planeId,
					mParticles.GetPosition(npcState.PrimaryParticleIndex),
					vec2f(0, HumanNpcSize),
					HumanNpcSize);
			}

			shipRenderContext.UploadNpcQuadsEnd();

			if (mAreStaticRenderAttributesDirty)
			{
				shipRenderContext.UploadNpcStaticAttributesEnd();
			}
		}
	}

	mAreStaticRenderAttributesDirty;
}

// Interactions

std::optional<NpcId> Npcs::PickNpc(vec2f const & position) const
{
	// TODO: FindTopmostContainingTriangle, and search on that ship; no search in other ships
	// Algo: nearest to position on same plane wins; if none on same plane. then simply nearest

	(void)position;
	return std::nullopt;
}

void Npcs::BeginMoveNpc(NpcId id)
{
	LogMessage("Npcs: BeginMove: id=", id);

	NpcState & state = GetNpcState(id);

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
	auto const triangleId = FindTopmostContainingTriangle(initialPosition);

	LogMessage("Npcs: BeginMoveNew: triangleId=", triangleId ? triangleId->ToString() : "<NONE>");

	// Create NPC in placement regime
	return AddHumanNpc(
		role,
		initialPosition,
		RegimeType::Placement,
		NpcHighlightType::None,
		triangleId ? triangleId->GetShipId() : GetTopmostShipId(),
		triangleId ? triangleId->GetLocalObjectId() : std::optional<ElementIndex>());
}

bool Npcs::IsSuitableNpcPosition(
	NpcId id,
	vec2f const & position) const
{
	// Find triangle ID; check conditions

	// TODO
	(void)id;
	(void)position;
	return true;
}

bool Npcs::MoveNpcBy(
	NpcId id,
	vec2f const & offset)
{
	//
	// Eventually moves around ships, but no regime change
	//

	assert(GetNpcState(id).Regime == RegimeType::Placement);

	// Move NPC - eventually moving around ships
	auto & npcState = InternalMoveNpcBy(id, offset);

	// Tell caller whether this is a suitable position (as a hint, not binding)
	return IsTriangleSuitableForNpc(npcState.Type, npcState.SId, npcState.TriangleIndex);
}

void Npcs::EndMoveNpc(
	NpcId id,
	vec2f const & finalOffset)
{
	LogMessage("Npcs: EndMoveNpc: id=", id);

	assert(GetNpcState(id).Regime == RegimeType::Placement);

	// Move NPC - eventually moving around ships
	auto & npcState = InternalMoveNpcBy(id, finalOffset);

	// Exit placement regime

	if (npcState.TriangleIndex.has_value())
	{
		npcState.Regime = RegimeType::Constrained;

		if (npcState.Type == NpcType::Human)
		{
			++mConstrainedRegimeHumanNpcCount;
		}
	}
	else
	{
		npcState.Regime = RegimeType::Free;

		if (npcState.Type == NpcType::Human)
		{
			++mFreeRegimeHumanNpcCount;
		}
	}
}

void Npcs::AbortNewNpc(NpcId id)
{
	LogMessage("Npcs: AbortNewNpc: id=", id);

	// Remove NPC
	RemoveNpc(id);
}

void Npcs::HighlightNpc(
	NpcId id,
	NpcHighlightType highlight)
{
	auto & npcState = GetNpcState(id);

	npcState.Highlight = highlight;

	// Remember to re-upload this static attribute
	mAreStaticRenderAttributesDirty = true;
}

void Npcs::RemoveNpc(NpcId id)
{
	auto const & npcState = GetNpcState(id);

	size_t const n = static_cast<size_t>(id);
	auto const shipId = npcState.SId;
	size_t const s = static_cast<size_t>(shipId);

	assert(n < mNpcEntriesByNpcId.size());
	assert(mNpcEntriesByNpcId[n].has_value());
	ElementIndex const npcOrdinal = mNpcEntriesByNpcId[n]->StateOrdinal;

	//
	// Destroy NPC
	//

	OnNpcDestroyed(npcState);

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	// Remove NPC state
	assert(mNpcShipsByShipId[s].has_value());
	assert(npcOrdinal < mNpcShipsByShipId[s]->NpcStates.size());
	mNpcShipsByShipId[s]->NpcStates.erase(mNpcShipsByShipId[s]->NpcStates.begin() + npcOrdinal);

	// Update subsequent NPC ordinals
	for (auto & npcEntry : mNpcEntriesByNpcId)
	{
		if (npcEntry.has_value()
			&& npcEntry->SId == shipId
			&& npcEntry->StateOrdinal > npcOrdinal)
		{
			--(npcEntry->StateOrdinal);
		}
	}

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

NpcId Npcs::AddHumanNpc(
	HumanNpcRoleType role,
	vec2f const & initialPosition,
	RegimeType initialRegime,
	NpcHighlightType initialHighlight,
	ShipId const & shipId,
	std::optional<ElementIndex> triangleIndex)
{
	assert(mNpcCount < GameParameters::MaxNpcs); // We still have room for an extra NPC

	size_t const s = static_cast<size_t>(shipId);

	// We know about this ship
	assert(s < mNpcShipsByShipId.size());
	assert(mNpcShipsByShipId[s].has_value());

	// Make ordinal for this NPC
	ElementIndex const newNpcOrdinal = static_cast<ElementIndex>(mNpcShipsByShipId[s]->NpcStates.size());

	//
	// Make (global) stable ID for this NPC, and update ordinals
	//

	NpcId newNpcId = NoneNpcId;

	// Find an unused ID
	for (size_t n = 0; n < mNpcEntriesByNpcId.size(); ++n)
	{
		if (!mNpcEntriesByNpcId[n].has_value())
		{
			// Found!
			newNpcId = static_cast<NpcId>(n);
			mNpcEntriesByNpcId[n].emplace(shipId, newNpcOrdinal);
			break;
		}
	}

	if (newNpcId == NoneNpcId)
	{
		// Add to the end
		newNpcId = static_cast<NpcId>(mNpcEntriesByNpcId.size());
		mNpcEntriesByNpcId.emplace_back();
		mNpcEntriesByNpcId.back().emplace(shipId, newNpcOrdinal);
	}

	//
	// Create NPC state
	//

	// Take particle
	ElementIndex const feetParticleIndex = mParticles.Add(
		initialPosition,
		mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Human));

	// Add NPC state
	auto const & state = mNpcShipsByShipId[s]->NpcStates.emplace_back(
		newNpcId,
		shipId,
		initialRegime,
		feetParticleIndex,
		initialHighlight,
		triangleIndex,
		TypeSpecificNpcState::HumanState(role));

	//
	// Update stats
	//

	if (state.Regime == RegimeType::Constrained)
	{
		++mConstrainedRegimeHumanNpcCount;
	}
	else if (state.Regime == RegimeType::Free)
	{
		++mFreeRegimeHumanNpcCount;
	}

	++mNpcCount;

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	// Remember to re-upload static render attributes
	mAreStaticRenderAttributesDirty = true;

	return newNpcId;
}

Npcs::NpcState & Npcs::InternalMoveNpcBy(
	NpcId id,
	vec2f const & offset)
{
	NpcState & oldState = GetNpcState(id);
	ShipId const oldShipId = oldState.SId;

	// Move NPC's particle(s)
	vec2f const newPosition = mParticles.GetPosition(oldState.PrimaryParticleIndex) + offset;
	mParticles.SetPosition(oldState.PrimaryParticleIndex, newPosition);

	// Calculate new triangle index
	auto const newTriangleId = FindTopmostContainingTriangle(newPosition);

	// Figure out if the NPC needs to move ships
	ShipId newShipId;
	if (!newTriangleId)
	{
		// * -> Outside

		if (!oldState.TriangleIndex)
		{
			// Outside -> Outside, nop
			newShipId = oldShipId;
		}
		else
		{
			// Inside -> Outside, move to topmost ship
			newShipId = GetTopmostShipId();
		}

		// Store new triangle index
		oldState.TriangleIndex.reset();
	}
	else
	{
		// * -> Inside, move to triangle's ship
		newShipId = newTriangleId->GetShipId();

		// Store new triangle index
		oldState.TriangleIndex = newTriangleId->GetLocalObjectId();
	}

	if (newShipId != oldShipId)
	{
		LogMessage("Moving NPC from ship ", oldShipId, " to ship ", newShipId);

		size_t const os = static_cast<size_t>(oldShipId);
		assert(os < mNpcShipsByShipId.size());
		assert(mNpcShipsByShipId[os].has_value());

		size_t const ns = static_cast<size_t>(newShipId);
		assert(ns < mNpcShipsByShipId.size());
		assert(mNpcShipsByShipId[ns].has_value());

		size_t const n = static_cast<size_t>(id);
		assert(n < mNpcEntriesByNpcId.size());
		assert(mNpcEntriesByNpcId[n].has_value());

		// Get old ordinal
		ElementIndex const oldNpcOrdinal = mNpcEntriesByNpcId[n]->StateOrdinal;

		// Calculate new ordinal for this NPC in new ship
		ElementIndex const newNpcOrdinal = static_cast<ElementIndex>(mNpcShipsByShipId[ns]->NpcStates.size());

		// Add state to new ship
		mNpcShipsByShipId[ns]->NpcStates.emplace_back(oldState);
		mNpcShipsByShipId[ns]->NpcStates.back().SId = newShipId;

		// Update entry of this NPC
		mNpcEntriesByNpcId[n]->SId = newShipId;
		mNpcEntriesByNpcId[n]->StateOrdinal = newNpcOrdinal;

		// Remove state from old ship
		assert(mNpcShipsByShipId[os].has_value());
		assert(oldNpcOrdinal < mNpcShipsByShipId[os]->NpcStates.size());
		mNpcShipsByShipId[os]->NpcStates.erase(mNpcShipsByShipId[os]->NpcStates.begin() + oldNpcOrdinal);

		// Update subsequent NPC ordinals
		for (auto & npcEntry : mNpcEntriesByNpcId)
		{
			if (npcEntry.has_value()
				&& npcEntry->SId == oldShipId
				&& npcEntry->StateOrdinal > oldNpcOrdinal)
			{
				--(npcEntry->StateOrdinal);
			}
		}

		return mNpcShipsByShipId[ns]->NpcStates.back();
	}
	else
	{
		return oldState;
	}
}

void Npcs::OnNpcDestroyed(NpcState const & state)
{
	size_t const n = static_cast<size_t>(state.Id);

	// We know about this NPC

	assert(n < mNpcEntriesByNpcId.size());
	assert(mNpcEntriesByNpcId[n].has_value());

	// Free NPC entry

	mNpcEntriesByNpcId[n].reset();

	// Free particle

	mParticles.Remove(state.PrimaryParticleIndex);

	// Update stats

	if (state.Regime == RegimeType::Constrained)
	{
		assert(mConstrainedRegimeHumanNpcCount > 0);
		--mConstrainedRegimeHumanNpcCount;
	}
	else if (state.Regime == RegimeType::Free)
	{
		assert(mFreeRegimeHumanNpcCount > 0);
		--mFreeRegimeHumanNpcCount;
	}

	--mNpcCount;
}

Npcs::NpcState & Npcs::GetNpcState(NpcId id)
{
	size_t const n = static_cast<size_t>(id);

	// We know about this NPC
	assert(n < mNpcEntriesByNpcId.size());
	assert(mNpcEntriesByNpcId[n].has_value());
	assert(mNpcEntriesByNpcId[n]->StateOrdinal < mNpcShipsByShipId[mNpcEntriesByNpcId[n]->SId]->NpcStates.size());

	return mNpcShipsByShipId[mNpcEntriesByNpcId[n]->SId]->NpcStates[mNpcEntriesByNpcId[n]->StateOrdinal];
}

std::optional<ElementId> Npcs::FindTopmostContainingTriangle(vec2f const & position) const
{
	// Visit all ships in reverse ship ID order (i.e. from topmost to bottommost)
	for (auto it = mNpcShipsByShipId.rbegin(); it != mNpcShipsByShipId.rend(); ++it)
	{		
		if ((*it).has_value())
		{
			// Find the triangle in this ship containing this position and having the highest plane ID

			// TODO: this might be optimized

			auto const & shipPoints = (*it)->ShipRef.GetPoints();
			auto const & shipTriangles = (*it)->ShipRef.GetTriangles();

			std::optional<ElementIndex> bestTriangleIndex;
			PlaneId bestPlaneId = std::numeric_limits<PlaneId>::lowest();
			for (auto const triangleIndex : shipTriangles)
			{
				vec2f const aPosition = shipPoints.GetPosition(shipTriangles.GetPointAIndex(triangleIndex));
				vec2f const bPosition = shipPoints.GetPosition(shipTriangles.GetPointBIndex(triangleIndex));
				vec2f const cPosition = shipPoints.GetPosition(shipTriangles.GetPointCIndex(triangleIndex));

				if (IsPointInTriangle(position, aPosition, bPosition, cPosition)
					&& (!bestTriangleIndex || shipPoints.GetPlaneId(shipTriangles.GetPointAIndex(triangleIndex)) > bestPlaneId))
				{
					bestTriangleIndex = triangleIndex;
					bestPlaneId = shipPoints.GetPlaneId(shipTriangles.GetPointAIndex(triangleIndex));
				}
			}

			if (bestTriangleIndex)
			{
				// Found a triangle on this ship
				return ElementId((*it)->ShipRef.GetId(), *bestTriangleIndex);
			}
		}
	}

	// No triangle found
	return std::nullopt;
}

bool Npcs::IsTriangleSuitableForNpc(
	NpcType type,
	ShipId shipId,
	std::optional<ElementIndex> const & triangleIndex) const
{
	if (!triangleIndex || type != NpcType::Human)
	{
		// Outside of a ship: always good
		// Not a human: always good (furniture could be tucked in a floor corner)
		return true;
	}

	// TODOHERE: check conditions (need new Springs getter, see plan)
	(void)shipId;
	return true;
}

ShipId Npcs::GetTopmostShipId() const
{
	assert(mNpcShipsByShipId.size() >= 1);

	for (auto it = mNpcShipsByShipId.rbegin(); it != mNpcShipsByShipId.rend(); ++it)
	{
		if ((*it).has_value())
		{
			return (*it)->ShipRef.GetId();
		}
	}

	assert(false);
	return NoneShip;
}

}