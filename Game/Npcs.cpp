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
	// Destroy all NPCs of this ship
	//

	for (auto const & state : mStateByShip[oldShipOrdinal])
	{
		OnNpcDestroyed(state);
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

void Npcs::Update(
	float currentSimulationTime,
	GameParameters const & gameParameters)
{
	// TODOHERE
	(void)currentSimulationTime;
	(void)gameParameters;
}

void Npcs::Upload(Render::RenderContext & renderContext) const
{
	// Upload all ships
	for (auto const & npcShip : mShipIdToShipIndex)
	{
		if (npcShip)
		{
			Render::ShipRenderContext & shipRenderContext = renderContext.GetShipRenderContext(npcShip->ShipRef.GetId());

			auto const & npcStates = mStateByShip[npcShip->Ordinal];

			if (mAreStaticRenderAttributesDirty)
			{
				shipRenderContext.UploadNpcStaticAttributesStart(npcStates.size());

				for (auto const & npcState : npcStates)
				{
					static vec4f constexpr HightlightColors[] = {
						vec4f(0.760f, 0.114f, 0.114f, 1.0f),	// Error
						vec4f(0.208f, 0.590f, 0.0177f, 1.0f),	// Selected
						vec4f::zero()							// None
					};

					shipRenderContext.UploadNpcStaticAttributes(HightlightColors[static_cast<size_t>(npcState.Highlight)]);
				}

				shipRenderContext.UploadNpcStaticAttributesEnd();
			}

			shipRenderContext.UploadNpcQuadsStart(npcStates.size());

			for (auto const & npcState : npcStates)
			{
				shipRenderContext.UploadNpcQuad(
					npcState.TriangleIndex
						? npcShip->ShipRef.GetPoints().GetPlaneId(npcShip->ShipRef.GetTriangles().GetPointAIndex(*npcState.TriangleIndex))
						: npcShip->ShipRef.GetMaxPlaneId(),
					mParticles.GetPosition(npcState.PrimaryParticleIndex),
					vec2f(0, HumanNpcSize),
					HumanNpcSize);
			}

			shipRenderContext.UploadNpcQuadsEnd();
		}
	}

	mAreStaticRenderAttributesDirty;
}

// Interactions

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

	LogMessage("TODOTEST: BeginMove: triangleId=", triangleId ? triangleId->ToString() : "<NONE>");

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

	auto const shipId = id.GetShipId();
	auto const localNpcId = id.GetLocalObjectId();
	auto const & npcState = GetNpcState(shipId, localNpcId);

	assert(npcState.Regime == RegimeType::Placement);

	// Calculate new position
	vec2f const newPosition = mParticles.GetPosition(npcState.PrimaryParticleIndex) + offset;
	mParticles.SetPosition(npcState.PrimaryParticleIndex, newPosition);

	// Calculate new triangle index
	auto const newTriangleId = FindContainingTriangle(newPosition);

	// Figure out if the NPC needs to move ships
	// TODOHERE

	// Now tell caller if this is a suitable position
	return IsTriangleSuitableForNpc(npcState.Type, newTriangleId);
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
	auto const shipId = id.GetShipId();
	auto const localNpcId = id.GetLocalObjectId();

	auto & npcState = GetNpcState(shipId, localNpcId);

	npcState.Highlight = highlight;

	// Remember to re-upload this static attribute
	mAreStaticRenderAttributesDirty = true;
}

void Npcs::RemoveNpc(NpcId const & id)
{
	auto const shipId = id.GetShipId();
	auto const localNpcId = id.GetLocalObjectId();

	auto const & npcState = GetNpcState(shipId, localNpcId);

	ElementIndex const shipOrdinal = mShipIdToShipIndex[static_cast<size_t>(shipId)]->Ordinal;
	ElementIndex const oldNpcOrdinal = mNpcIdToNpcOrdinalIndex[static_cast<size_t>(shipId)][static_cast<size_t>(localNpcId)];

	//
	// Destroy NPC
	//

	OnNpcDestroyed(npcState);

	mGameEventHandler->OnNpcCountsUpdated(
		mNpcCount,
		mConstrainedRegimeHumanNpcCount,
		mFreeRegimeHumanNpcCount,
		GameParameters::MaxNpcs - mNpcCount);

	//
	// Remove State buffers
	//

	// Remove NPC state
	mStateByShip[shipOrdinal].erase(mStateByShip[shipOrdinal].begin() + oldNpcOrdinal);

	//
	// Maintain indices
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
	vec2f const & initialPosition,
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
		initialPosition,
		mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Human));

	// Add NPC state
	auto const & state = mStateByShip[shipOrdinal].emplace_back(
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

	return NpcId(shipId, newNpcId);
}

void Npcs::OnNpcDestroyed(NpcState const & state)
{
	// Free particle

	mParticles.Remove(state.PrimaryParticleIndex);

	// Update stats

	if (state.Regime == RegimeType::Constrained)
	{
		--mConstrainedRegimeHumanNpcCount;
	}
	else if (state.Regime == RegimeType::Free)
	{
		--mFreeRegimeHumanNpcCount;
	}

	--mNpcCount;
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
	// Visit all ships in reverse ship ID order (i.e. from topmost to bottommost)
	for (auto it = mShipIdToShipIndex.rbegin(); it != mShipIdToShipIndex.rend(); ++it)
	{		
		if ((*it).has_value())
		{
			// Find the triangle in this ship containing this position and having the highest plane ID

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
	std::optional<ElementId> const & triangleId) const
{
	if (!triangleId || type != NpcType::Human)
	{
		// Outside of a shipL always good
		// Not a human: always good
		
		return true;
	}

	// TODOHERE: check conditions (need new Springs getter, see plan)
	return true;
}

ShipId Npcs::GetTopmostShipId() const
{
	assert(mShipIdToShipIndex.size() >= 1);

	for (auto it = mShipIdToShipIndex.rbegin(); it != mShipIdToShipIndex.rend(); ++it)
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