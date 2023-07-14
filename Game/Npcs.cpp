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

	// Make room in indices
	assert(mNpcShipsByShipId.size() == mNpcOrdinalsByNpcId.size());
	while (s >= mNpcShipsByShipId.size())
	{
		mNpcShipsByShipId.emplace_back();
		mNpcOrdinalsByNpcId.emplace_back();
	}

	// We do not know about this ship yet
	assert(!mNpcShipsByShipId[s].has_value());
	assert(mNpcOrdinalsByNpcId[s].empty());

	// Initialize NPC Ship
	mNpcShipsByShipId[s].emplace(ship);
}

void Npcs::OnShipRemoved(ShipId shipId)
{
	size_t const s = static_cast<size_t>(shipId);

	// We know about this ship
	assert(s < mNpcShipsByShipId.size());
	assert(s < mNpcOrdinalsByNpcId.size());
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

	// Forget about the NPCs of this ship
	mNpcOrdinalsByNpcId[s].clear();

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

				shipRenderContext.UploadNpcQuad(
					npcState.TriangleIndex
					? ship.GetPoints().GetPlaneId(ship.GetTriangles().GetPointAIndex(*npcState.TriangleIndex))
					: ship.GetMaxPlaneId(),
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
	// TODO
	(void)position;
	return std::nullopt;
}

void Npcs::BeginMoveNpc(NpcId const & id)
{
	LogMessage("Npcs: BeginMove: id=", id);

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
	LogMessage("Npcs: EndMoveNpc: id=", id);

	// Find triangle ID; exit placement regime

	// TODO: assert it's in Placement regime
	// TODO: remember to update stats for exiting placement

	// TODO
	(void)id;
	(void)finalOffset;
}

void Npcs::AbortNewNpc(NpcId const & id)
{
	LogMessage("Npcs: AbortNewNpc: id=", id);

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

	size_t const s = static_cast<size_t>(shipId);
	size_t const ln = static_cast<size_t>(localNpcId);

	assert(s < mNpcOrdinalsByNpcId.size());
	assert(ln < mNpcOrdinalsByNpcId[s].size());
	assert(mNpcOrdinalsByNpcId[s][ln].has_value());
	ElementIndex const npcOrdinal = *(mNpcOrdinalsByNpcId[s][ln]);

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
	assert(ln < mNpcShipsByShipId[s]->NpcStates.size());
	mNpcShipsByShipId[s]->NpcStates.erase(mNpcShipsByShipId[s]->NpcStates.begin() + npcOrdinal);

	// Forget about this NPC's ordinal
	mNpcOrdinalsByNpcId[s][ln] = NoneElementIndex;

	// Update subsequent NPC ordinals
	for (auto & o : mNpcOrdinalsByNpcId[s])
	{
		if (o.has_value()
			&& *o > npcOrdinal)
		{
			--(*o);
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
	assert(s < mNpcOrdinalsByNpcId.size());
	assert(mNpcShipsByShipId[s].has_value());

	// Make ordinal for this NPC
	ElementIndex const newNpcOrdinal = static_cast<ElementIndex>(mNpcShipsByShipId[s]->NpcStates.size());

	//
	// Make (ship-local) stable ID for this NPC, and update indices
	//

	LocalNpcId newNpcId = NoneLocalNpcId;

	// Find an unused ID
	for (size_t ln = 0; ln < mNpcOrdinalsByNpcId[s].size(); ++ln)
	{
		if (!mNpcOrdinalsByNpcId[s][ln].has_value())
		{
			// Found!
			newNpcId = static_cast<LocalNpcId>(ln);
			mNpcOrdinalsByNpcId[s][ln].emplace(newNpcOrdinal);
			break;
		}
	}

	if (newNpcId == NoneLocalNpcId)
	{
		// Add to the end
		newNpcId = static_cast<LocalNpcId>(mNpcOrdinalsByNpcId[s].size());
		mNpcOrdinalsByNpcId[s].emplace_back(newNpcOrdinal);
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
	size_t const s = static_cast<size_t>(shipId);
	size_t const ln = static_cast<size_t>(localNpcId);

	// We know about this ship
	assert(s < mNpcShipsByShipId.size());
	assert(s < mNpcOrdinalsByNpcId.size());
	assert(mNpcShipsByShipId[s].has_value());

	// We know about this NPC
	assert(ln < mNpcShipsByShipId[s]->NpcStates.size());
	assert(ln < mNpcOrdinalsByNpcId[s].size());
	assert(mNpcOrdinalsByNpcId[s][ln].has_value());

	return mNpcShipsByShipId[s]->NpcStates[*mNpcOrdinalsByNpcId[s][ln]];
}

std::optional<ElementId> Npcs::FindContainingTriangle(vec2f const & position) const
{
	// Visit all ships in reverse ship ID order (i.e. from topmost to bottommost)
	for (auto it = mNpcShipsByShipId.rbegin(); it != mNpcShipsByShipId.rend(); ++it)
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