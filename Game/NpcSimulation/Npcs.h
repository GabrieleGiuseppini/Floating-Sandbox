/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-10-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../GameEventDispatcher.h"
#include "../GameParameters.h"
#include "../NpcDatabase.h"
#include "../Physics.h"
#include "../RenderContext.h"
#include "../RenderTypes.h"
#include "../VisibleWorld.h"

#include <GameCore/BarycentricCoords.h>
#include <GameCore/ElementIndexRangeIterator.h>
#include <GameCore/EnumFlags.h>
#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameGeometry.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Log.h>
#include <GameCore/StrongTypeDef.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

#include <memory>
#include <optional>
#include <vector>

#ifdef IN_BARYLAB
#ifdef _DEBUG
#define IN_BARYLAB_DEBUG
#define BARYLAB_PROBING
#endif
#endif

#ifdef IN_BARYLAB_DEBUG
#define BARYLAB_LOG_DEBUG
#endif

#ifdef BARYLAB_LOG_DEBUG
#define LogNpcDebug(...) LogDebug(__VA_ARGS__);
#else
#define LogNpcDebug(...)
#endif

namespace Physics {

class Npcs final
{
private:

#pragma pack(push)

	struct LimbVector final
	{
		float RightLeg;
		float LeftLeg;
		float RightArm;
		float LeftArm;

		float const * fptr() const
		{
			return &(RightLeg);
		}

		float * fptr()
		{
			return &(RightLeg);
		}

		void ConvergeTo(LimbVector const & target, float convergenceRate)
		{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
			__m128 v = _mm_loadu_ps(fptr());
			__m128 t = _mm_loadu_ps(target.fptr());
			__m128 r = _mm_set_ps1(convergenceRate);
			_mm_storeu_ps(
				fptr(),
				_mm_add_ps(
					v,
					_mm_mul_ps(
						_mm_sub_ps(t, v),
						r)));
#else
			RightLeg += (target.RightLeg - RightLeg) * convergenceRate;
			LeftLeg += (target.LeftLeg - LeftLeg) * convergenceRate;
			RightArm += (target.RightArm - RightArm) * convergenceRate;
			LeftArm += (target.LeftArm - LeftArm) * convergenceRate;
#endif
		}
	};

#pragma pack(pop)

	//
	// The NPC state.
	//

	struct StateType final
	{
		enum class RegimeType
		{
			BeingPlaced,
			Constrained,
			Free
		};

		struct NpcParticleStateType final
		{
			ElementIndex ParticleIndex;

			struct ConstrainedStateType final
			{
				AbsoluteTriangleBCoords CurrentBCoords;

				// The edge on which we're currently non-inertial;
				// when set, we are "conceptually" along this edge - might not be really the
				// case e.g. if during non-inertial we've reached a vertex, have navigated
				// through it, and bumped against a wall
				std::optional<TriangleAndEdge> CurrentVirtualFloor;

				// Velocity of particle (as in velocity buffer), but relative to mesh
				// (ship) at the moment velocity was calculated
				vec2f MeshRelativeVelocity;

				// When true, no floor is a floor to this particle
				bool GhostParticlePulse;

				ConstrainedStateType(
					ElementIndex currentTriangle,
					bcoords3f const & currentTriangleBarycentricCoords)
					: CurrentBCoords(currentTriangle, currentTriangleBarycentricCoords)
					, CurrentVirtualFloor()
					, MeshRelativeVelocity(vec2f::zero())
					, GhostParticlePulse(false)
				{}
			};

			std::optional<ConstrainedStateType> ConstrainedState;

			NpcParticleStateType() // Default cctor only needed for array
				: ParticleIndex(NoneElementIndex)
				, ConstrainedState()
			{}

			NpcParticleStateType(
				ElementIndex particleIndex,
				std::optional<ConstrainedStateType> && constrainedState)
				: ParticleIndex(particleIndex)
				, ConstrainedState(std::move(constrainedState))
			{}

			vec2f const & GetApplicableVelocity(NpcParticles const & particles) const
			{
				if (ConstrainedState.has_value())
				{
					return ConstrainedState->MeshRelativeVelocity;
				}
				else
				{
					return particles.GetVelocity(ParticleIndex);
				}
			}
		};

		struct NpcSpringStateType final
		{
			ElementIndex EndpointAIndex; // Index in NpcParticles
			ElementIndex EndpointBIndex; // Index in NpcParticles

			// Constants
			float BaseRestLength;
			float BaseSpringReductionFraction;
			float BaseSpringDampingCoefficient;

			// Calculated
			float RestLength; // Adjusted
			float SpringStiffnessFactor;
			float SpringDampingFactor;

			NpcSpringStateType() = default; // For array

			NpcSpringStateType(
				ElementIndex endpointAIndex,
				ElementIndex endpointBIndex,
				float baseRestLength,
				float baseSpringReductionFraction,
				float baseSpringDampingCoefficient)
				: EndpointAIndex(endpointAIndex)
				, EndpointBIndex(endpointBIndex)
				, BaseRestLength(baseRestLength)
				, BaseSpringReductionFraction(baseSpringReductionFraction)
				, BaseSpringDampingCoefficient(baseSpringDampingCoefficient)
				, RestLength(0.0f) // Calculated later
				, SpringStiffnessFactor(0.0f) // Calculated later
				, SpringDampingFactor(0.0f) // Calculated later
			{}
		};

		struct ParticleMeshType final
		{
			FixedSizeVector<NpcParticleStateType, GameParameters::MaxParticlesPerNpc> Particles;

			FixedSizeVector<NpcSpringStateType, GameParameters::MaxSpringsPerNpc> Springs;
		};

		union KindSpecificStateType
		{
			struct FurnitureNpcStateType final
			{
				NpcSubKindIdType const SubKindId;
				NpcFurnitureRoleType const Role;

				Render::TextureCoordinatesQuad const TextureCoordinatesQuad;
				float CurrentFaceDirectionX; // [-1.0f, 0.0f, 1.0f]

				FurnitureNpcStateType(
					NpcSubKindIdType subKindId,
					NpcFurnitureRoleType role,
					Render::TextureCoordinatesQuad const & textureCoordinatesQuad)
					: SubKindId(subKindId)
					, Role(role)
					, TextureCoordinatesQuad(textureCoordinatesQuad)
					, CurrentFaceDirectionX(1.0f)
				{}
			} FurnitureNpcState;

			struct HumanNpcStateType final
			{
				NpcSubKindIdType const SubKindId;
				NpcHumanRoleType const Role;
				float const WidthMultipier; // Randomization
				float const WalkingSpeedBase;

				NpcDatabase::HumanTextureFramesType const & TextureFrames;
				NpcDatabase::HumanTextureGeometryType const & TextureGeometry;

				enum class BehaviorType
				{
					BeingPlaced, // Initial state, just monkeying

					Constrained_Falling, // Clueless, does nothing; with feet on edge
					Constrained_Aerial, // Clueless, does nothing; with feet in air
					Constrained_KnockedOut, // Clueless, does nothing, not relevant where; waits until can rise

					Constrained_PreRising, // Prepares to stand up
					Constrained_Rising, // Tries to stand up (appliying torque)
					Constrained_Equilibrium, // Stands up; continues to adjust alignment with torque
					Constrained_Walking, // Walks; continues to adjust alignment with torque

					Constrained_InWater, // Does nothing (like Constrained_Aerial), but waits to swim
					Constrained_Swimming_Style1, // Swims
					Constrained_Swimming_Style2, // Swims

					Constrained_Electrified, // Doing electrification dance, assuming being vertical

					Free_Aerial, // Does nothing, stays here as long as it's moving
					Free_KnockedOut, // Does nothing, stays here as long as it's still

					Free_InWater, // Does nothing, but waits to swim
					Free_Swimming_Style1, // Swims
					Free_Swimming_Style2, // Swims
					Free_Swimming_Style3, // Swims

					ConstrainedOrFree_Smashed // Plat
				};

				BehaviorType CurrentBehavior;

				union BehaviorStateType
				{
					struct BeingPlacedStateType
					{
						void Reset()
						{
						}
					} BeingPlaced;

					struct Constrained_FallingStateType
					{
						float ProgressToAerial;
						float ProgressToPreRising;

						void Reset()
						{
							ProgressToAerial = 0.0f;
							ProgressToPreRising = 0.0f;
						}
					} Constrained_Falling;

					struct Constrained_AerialStateType
					{
						float ProgressToFalling;
						float ProgressToRising;

						void Reset()
						{
							ProgressToFalling = 0.0f;
							ProgressToRising = 0.0f;
						}
					} Constrained_Aerial;

					struct Constrained_KnockedOutStateType
					{
						float ProgressToPreRising;
						float ProgressToAerial;

						void Reset()
						{
							ProgressToPreRising = 0.0f;
							ProgressToAerial = 0.0f;
						}
					} Constrained_KnockedOut;

					struct Constrained_PreRisingType
					{
						float ProgressToRising;
						float ProgressToAerial;

						void Reset()
						{
							ProgressToRising = 0.0f;
							ProgressToAerial = 0.0f;
						}
					} Constrained_PreRising;

					struct Constrained_RisingStateType
					{
						// The virtual edge we're rising against, remembered in order to survive small bursts
						// of being off the edge
						TriangleAndEdge VirtualEdgeRisingAgainst;

						float CurrentSoftTerminationDecision; // [0.0f, 1.0f]

						void Reset()
						{
							VirtualEdgeRisingAgainst.TriangleElementIndex = NoneElementIndex; // Can't use optional here as it does not have a trivial cctor
							CurrentSoftTerminationDecision = 0.0f;
						}
					} Constrained_Rising;

					struct Constrained_EquilibriumStateType
					{
						float ProgressToWalking;

						void Reset()
						{
							ProgressToWalking = 0.0f;
						}
					} Constrained_Equilibrium;

					struct Constrained_WalkingStateType
					{
						float CurrentWalkMagnitude; // [0.0f, 1.0f]

						float CurrentFlipDecision; // [0.0f, 1.0f]
						float TargetFlipDecision; // [0.0f, 1.0f]

						void Reset()
						{
							CurrentWalkMagnitude = 0.0f;
							CurrentFlipDecision = 0.0f;
							TargetFlipDecision = 0.0f;
						}
					} Constrained_Walking;

					struct Constrained_InWaterType
					{
						float ProgressToSwimming;

						void Reset()
						{
							ProgressToSwimming = 0.0f;
						}
					} Constrained_InWater;

					struct Constrained_SwimmingType
					{
						void Reset()
						{
						}
					} Constrained_Swimming;

					struct Constrained_ElectrifiedStateType
					{
						float ProgressToLeaving;

						void Reset()
						{
							ProgressToLeaving = 0.0f;
						}
					} Constrained_Electrified;

					struct Free_AerialStateType
					{
						float ProgressToKnockedOut;

						void Reset()
						{
							ProgressToKnockedOut = 0.0f;
						}
					} Free_Aerial;

					struct Free_KnockedOutStateType
					{
						float ProgressToAerial;
						float ProgressToInWater;

						void Reset()
						{
							ProgressToAerial = 0.0f;
							ProgressToInWater = 0.0f;
						}
					} Free_KnockedOut;

					struct Free_InWaterType
					{
						float NextBubbleEmissionSimulationTimestamp;
						float ProgressToSwimming;

						void Reset()
						{
							NextBubbleEmissionSimulationTimestamp = 0.0f;
							ProgressToSwimming = 0.0f;
						}
					} Free_InWater;

					struct Free_SwimmingType
					{
						float NextBubbleEmissionSimulationTimestamp;
						float ProgressToLeavingSwimming;

						void Reset()
						{
							NextBubbleEmissionSimulationTimestamp = 0.0f;
							ProgressToLeavingSwimming = 0.0f;
						}
					} Free_Swimming;

					struct ConstrainedOrFree_SmashedStateType
					{
						float ProgressToLeaving;

						void Reset()
						{
							ProgressToLeaving = 0.0f;
						}
					} ConstrainedOrFree_Smashed;

				} CurrentBehaviorState;

				float CurrentStateTransitionSimulationTimestamp;
				float TotalDistanceTraveledOnEdgeSinceStateTransition; // [0.0f, +INF] - when we're constrained on an edge (e.g. walking)
				float TotalDistanceTraveledOffEdgeSinceStateTransition; // [0.0f, +INF] - when we're constrained off an edge or free

				float EquilibriumTorque; // Reset at beginning of each human update state

				float CurrentEquilibriumSoftTerminationDecision; // Cross-state

				float CurrentFaceOrientation; // [-1.0f, 0.0f, 1.0f]
				float CurrentFaceDirectionX; // [-1.0f, 0.0f, 1.0f]

				// Panic levels
				float OnFirePanicLevel; // [0.0f ... +1.0f], auto-decayed
				float BombProximityPanicLevel; // [0.0f ... +1.0f], auto-decayed
				float MiscPanicLevel; // [0.0f ... +1.0f], auto-decayed; includes triangle break
				float ResultantPanicLevel; // [0.0f ... +INF)

				// Animation

				struct AnimationStateType
				{
					// Angles are CCW relative to vertical, regardless of where the NPC is looking towards (L/R)
					// (when we flip we pretend immediate mirroring of limbs from the point of view of the human,
					// so angles are independent from direction, and animation is smoother)

					// "Left" and "Right" are relative to screen when the NPC is looking at us
					// (so "right arm" is really its left arm)
					FS_ALIGN16_BEG LimbVector LimbAngles FS_ALIGN16_END;
					FS_ALIGN16_BEG LimbVector LimbAnglesCos FS_ALIGN16_END;
					FS_ALIGN16_BEG LimbVector LimbAnglesSin FS_ALIGN16_END;

					static float constexpr InitialArmAngle = Pi<float> / 2.0f * 0.3f;
					static float constexpr InitialLegAngle = 0.2f;

					FS_ALIGN16_BEG LimbVector LimbLengthMultipliers FS_ALIGN16_END;
					float UpperLegLengthFraction; // When less than 1.0, we have a knee
					float CrotchHeightMultiplier; // Multiplier for the part of the body from the crotch down to the feet

					AnimationStateType()
						: LimbAngles({ InitialLegAngle, -InitialLegAngle, InitialArmAngle, -InitialArmAngle })
						, LimbAnglesCos({std::cosf(LimbAngles.RightLeg), std::cosf(LimbAngles.LeftLeg), std::cosf(LimbAngles.RightArm), std::cosf(LimbAngles.LeftArm)})
						, LimbAnglesSin({ std::sinf(LimbAngles.RightLeg), std::sinf(LimbAngles.LeftLeg), std::sinf(LimbAngles.RightArm), std::sinf(LimbAngles.LeftArm) })
						, LimbLengthMultipliers({1.0f, 1.0f, 1.0f, 1.0f})
						, UpperLegLengthFraction(1.0f)
						, CrotchHeightMultiplier(1.0f)
					{}
				} AnimationState;

				HumanNpcStateType(
					NpcSubKindIdType subKindId,
					NpcHumanRoleType role,
					float widthMultipier,
					float walkingSpeedBase,
					NpcDatabase::HumanTextureFramesType const & textureFrames,
					NpcDatabase::HumanTextureGeometryType const & textureGeometry,
					BehaviorType initialBehavior,
					float currentSimulationTime)
					: SubKindId(subKindId)
					, Role(role)
					, WidthMultipier(widthMultipier)
					, WalkingSpeedBase(walkingSpeedBase)
					, TextureFrames(textureFrames)
					, TextureGeometry(textureGeometry)
					, EquilibriumTorque(0.0f)
					, CurrentEquilibriumSoftTerminationDecision(0.0f)
					, CurrentFaceOrientation(1.0f)
					, CurrentFaceDirectionX(0.0f)
					, OnFirePanicLevel(0.0f)
					, BombProximityPanicLevel(0.0f)
					, MiscPanicLevel(0.0f)
					, ResultantPanicLevel(0.0f)
					// Animation
					, AnimationState()
				{
					TransitionToState(initialBehavior, currentSimulationTime);
				}

				void TransitionToState(
					BehaviorType behavior,
					float currentSimulationTime)
				{
					LogNpcDebug("  HumanBehaviorTransition: ", int(CurrentBehavior), " -> ", int(behavior));

					CurrentBehavior = behavior;

					switch (behavior)
					{
						case BehaviorType::BeingPlaced:
						{
							CurrentBehaviorState.BeingPlaced.Reset();
							break;
						}

						case BehaviorType::Constrained_Aerial:
						{
							CurrentBehaviorState.Constrained_Aerial.Reset();
							break;
						}

						case BehaviorType::Constrained_Electrified:
						{
							CurrentBehaviorState.Constrained_Electrified.Reset();
							break;
						}

						case BehaviorType::Constrained_Equilibrium:
						{
							CurrentBehaviorState.Constrained_Equilibrium.Reset();
							break;
						}

						case BehaviorType::Constrained_Falling:
						{
							CurrentBehaviorState.Constrained_Falling.Reset();
							break;
						}

						case BehaviorType::Constrained_InWater:
						{
							CurrentBehaviorState.Constrained_InWater.Reset();
							break;
						}

						case BehaviorType::Constrained_KnockedOut:
						{
							CurrentBehaviorState.Constrained_KnockedOut.Reset();
							break;
						}

						case BehaviorType::Constrained_PreRising:
						{
							CurrentBehaviorState.Constrained_PreRising.Reset();
							break;
						}

						case BehaviorType::Constrained_Rising:
						{
							CurrentBehaviorState.Constrained_Rising.Reset();
							CurrentEquilibriumSoftTerminationDecision = 0.0f; // Start clean
							break;
						}

						case BehaviorType::Constrained_Swimming_Style1:
						case BehaviorType::Constrained_Swimming_Style2:
						{
							CurrentBehaviorState.Constrained_Swimming.Reset();
							break;
						}

						case BehaviorType::Constrained_Walking:
						{
							CurrentBehaviorState.Constrained_Walking.Reset();
							break;
						}

						case BehaviorType::Free_Aerial:
						{
							CurrentBehaviorState.Free_Aerial.Reset();
							break;
						}

						case BehaviorType::Free_InWater:
						{
							CurrentBehaviorState.Free_InWater.Reset();
							break;
						}

						case BehaviorType::Free_KnockedOut:
						{
							CurrentBehaviorState.Free_KnockedOut.Reset();
							break;
						}

						case BehaviorType::Free_Swimming_Style1:
						case BehaviorType::Free_Swimming_Style2:
						case BehaviorType::Free_Swimming_Style3:
						{
							CurrentBehaviorState.Free_Swimming.Reset();
							break;
						}

						case BehaviorType::ConstrainedOrFree_Smashed:
						{
							CurrentBehaviorState.ConstrainedOrFree_Smashed.Reset();
							break;
						}
					}

					CurrentStateTransitionSimulationTimestamp = currentSimulationTime;
					TotalDistanceTraveledOnEdgeSinceStateTransition = 0.0f;
					TotalDistanceTraveledOffEdgeSinceStateTransition = 0.0f;
				}
			} HumanNpcState;

			explicit KindSpecificStateType(FurnitureNpcStateType && furnitureState)
				: FurnitureNpcState(std::move(furnitureState))
			{}

			explicit KindSpecificStateType(HumanNpcStateType && humanState)
				: HumanNpcState(std::move(humanState))
			{}
		};

		struct CombustionStateType
		{
			vec2f FlameVector;
			float FlameWindRotationAngle;

			CombustionStateType(
				vec2f const & flameVector,
				float flameWindRotationAngle)
				: FlameVector(flameVector)
				, FlameWindRotationAngle(flameWindRotationAngle)
			{}
		};

		struct BeingPlacedStateType
		{
			int AnchorParticleOrdinal; // In NPC's mesh
			bool DoMoveWholeMesh;
			std::optional<RegimeType> PreviousRegime; // If any (i.e. if this is not an initial placement)
		};

		//
		// Members
		//

		// The ID of this NPC.
		NpcId const Id;

		// The type of this NPC.
		NpcKindType const Kind;

		// The render color for this NPC.
		vec3f const RenderColor;

		// The current ship that this NPC belongs to.
		// NPCs always belong to a ship, and can change ships during the
		// course of their lives.
		ShipId CurrentShipId;

		// The current plane ID. Since NPCs always belong to a ship, they also always
		// are on a plane.
		PlaneId CurrentPlaneId;

		// The current connected component of the NPC, when it's constrained; particles
		// are always constrained to belong to this connected component.
		// Its presence is correlated with the NPC being contrained.
		std::optional<ConnectedComponentId> CurrentConnectedComponentId;

		// The current regime.
		RegimeType CurrentRegime;

		// The mesh
		ParticleMeshType ParticleMesh;

		// The additional state specific to the type of this NPC.
		KindSpecificStateType KindSpecificState;

		// How much this NPC is on fire
		float CombustionProgress; // [-1.0f, 1.0f], "on fire" if > 0.0f

		// The state of combustion, if this NPC is "on fire"
		std::optional<CombustionStateType> CombustionState;

		// Randomness specific to this NPC.
		float RandomNormalizedUniformSeed; // [-1.0f ... +1.0f]

		// Info for placing (presence is correlated with regime==BeingPlaced)
		std::optional<BeingPlacedStateType> BeingPlacedState;

		StateType(
			NpcId id,
			NpcKindType kind,
			vec3f const & renderColor,
			ShipId initialShipId,
			PlaneId initialPlaneId,
			std::optional<ConnectedComponentId> currentConnectedComponentId,
			RegimeType initialRegime,
			ParticleMeshType && particleMesh,
			KindSpecificStateType && kindSpecificState,
			BeingPlacedStateType beingPlacedState)
			: Id(id)
			, Kind(kind)
			, RenderColor(renderColor)
			, CurrentShipId(initialShipId)
			, CurrentPlaneId(initialPlaneId)
			, CurrentConnectedComponentId(currentConnectedComponentId)
			, CurrentRegime(initialRegime)
			, ParticleMesh(std::move(particleMesh))
			, KindSpecificState(std::move(kindSpecificState))
			, CombustionProgress(-1.0f)
			, CombustionState()
			, RandomNormalizedUniformSeed(GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, 1.0f))
			, BeingPlacedState(beingPlacedState)
		{}
	};

	//
	// The information heading the list of NPCs in a ship.
	//

	struct ShipNpcsType final
	{
		Ship & HomeShip; // Non-const as we forward interactions to ships
		std::vector<NpcId> Npcs;

		std::vector<NpcId> BurningNpcs; // Maintained as a set

		// Stats
		size_t FurnitureNpcCount;
		size_t HumanNpcCount;
		size_t HumanNpcCaptainCount;

		ShipNpcsType(Ship & homeShip)
			: HomeShip(homeShip)
			, Npcs()
			, BurningNpcs()
			//
			, FurnitureNpcCount(0)
			, HumanNpcCount(0)
			, HumanNpcCaptainCount(0)
		{}
	};

public:

	Npcs(
		Physics::World & parentWorld,
		NpcDatabase const & npcDatabase,
		std::shared_ptr<GameEventDispatcher> gameEventHandler,
		GameParameters const & gameParameters)
		: mParentWorld(parentWorld)
		, mNpcDatabase(npcDatabase)
		, mGameEventHandler(std::move(gameEventHandler))
		, mMaxNpcs(gameParameters.MaxNpcs)
		// Container
		, mStateBuffer()
		, mShips()
		, mParticles(static_cast<ElementCount>(mMaxNpcs * GameParameters::MaxParticlesPerNpc))
		// State
		, mCurrentSimulationSequenceNumber()
		, mCurrentlySelectedNpc()
		, mCurrentlySelectedNpcWallClockTimestamp()
		, mCurrentlyHighlightedNpc()
		, mGeneralizedPanicLevel(0.0f)
		// Stats
		, mFreeRegimeHumanNpcCount(0)
		, mConstrainedRegimeHumanNpcCount(0)
		// Simulation parameters
		, mGlobalDampingFactor(0.0f) // Will be calculated
		, mCurrentGlobalDampingAdjustment(1.0f)
		, mCurrentSizeMultiplier(1.0f)
		, mCurrentHumanNpcWalkingSpeedAdjustment(1.0f)
		, mCurrentSpringReductionFractionAdjustment(1.0f)
		, mCurrentSpringDampingCoefficientAdjustment(1.0f)
		, mCurrentStaticFrictionAdjustment(1.0f)
		, mCurrentKineticFrictionAdjustment(1.0f)
		, mCurrentNpcFrictionAdjustment(1.0f)
	{
		RecalculateGlobalDampingFactor();
	}

	void Update(
		float currentSimulationTime,
		Storm::Parameters const & stormParameters,
		GameParameters const & gameParameters);

	void UpdateEnd();

	void Upload(Render::RenderContext & renderContext) const;

	void UploadFlames(
		ShipId shipId,
		Render::ShipRenderContext & shipRenderContext) const;

	///////////////////////////////

	bool HasNpcs() const;

	bool HasNpc(NpcId npcId) const;

	Geometry::AABB GetNpcAABB(NpcId npcId) const;

	size_t GetFlameCount(ShipId shipId) const
	{
		assert(shipId < mShips.size());
		assert(mShips[shipId].has_value());
		return mShips[shipId]->BurningNpcs.size();
	}

	///////////////////////////////

	void OnShipAdded(Ship & ship);

	void OnShipRemoved(ShipId shipId);

	void OnShipConnectivityChanged(ShipId shipId);

	NpcKindType GetNpcKind(NpcId id);

	std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> BeginPlaceNewFurnitureNpc(
		std::optional<NpcSubKindIdType> subKind,
		vec2f const & worldCoordinates,
		bool doMoveWholeMesh,
		float currentSimulationTime);

	std::tuple<std::optional<PickedNpc>, NpcCreationFailureReasonType> BeginPlaceNewHumanNpc(
		std::optional<NpcSubKindIdType> subKind,
		vec2f const & worldCoordinates,
		bool doMoveWholeMesh,
		float currentSimulationTime);

	std::optional<PickedNpc> ProbeNpcAt(
		vec2f const & position,
		float radius,
		GameParameters const & gameParameters) const;

	void BeginMoveNpc(
		NpcId id,
		int particleOrdinal,
		float currentSimulationTime,
		bool doMoveWholeMesh);

	void MoveNpcTo(
		NpcId id,
		vec2f const & position,
		vec2f const & offset,
		bool doMoveWholeMesh);

	void EndMoveNpc(
		NpcId id,
		float currentSimulationTime);

	void CompleteNewNpc(
		NpcId id,
		float currentSimulationTime);

	void RemoveNpc(NpcId id);

	void AbortNewNpc(NpcId id);

	std::tuple<std::optional<NpcId>, NpcCreationFailureReasonType> AddNpcGroup(
		NpcKindType kind,
		VisibleWorld const & visibleWorld,
		float currentSimulationTime,
		GameParameters const & gameParameters);

	void TurnaroundNpc(NpcId id);

	std::optional<NpcId> GetCurrentlySelectedNpc() const;

	void SelectFirstNpc();

	void SelectNextNpc();

	void SelectNpc(std::optional<NpcId> id);

	void HighlightNpc(std::optional<NpcId> id);

	void Announce();

public:

	//
	// Interactions
	//

	void MoveBy(
		ShipId shipId,
		std::optional<ConnectedComponentId> connectedComponent,
		vec2f const & offset,
		vec2f const & inertialVelocity,
		GameParameters const & gameParameters);

	void RotateBy(
		ShipId shipId,
		std::optional<ConnectedComponentId> connectedComponent,
		float angle,
		vec2f const & center,
		float inertialAngle,
		GameParameters const & gameParameters);

	void SmashAt(
		vec2f const & targetPos,
		float radius,
		float currentSimulationTime);

	void DrawTo(
		vec2f const & targetPos,
		float strength);

	void SwirlAt(
		vec2f const & targetPos,
		float strength);

	void ApplyBlast(
		ShipId shipId,
		vec2f const & centerPosition,
		float blastRadius,
		float blastForce, // N
		GameParameters const & gameParameters);

	void ApplyAntiMatterBombPreimplosion(
		ShipId shipId,
		vec2f const & centerPosition,
		float radius,
		float radiusThickness,
		GameParameters const & gameParameters);

	void ApplyAntiMatterBombImplosion(
		ShipId shipId,
		vec2f const & centerPosition,
		float sequenceProgress,
		GameParameters const & gameParameters);

	void ApplyAntiMatterBombExplosion(
		ShipId shipId,
		vec2f const & centerPosition,
		GameParameters const & gameParameters);

	void OnShipTriangleDestroyed(
		ShipId shipId,
		ElementIndex triangleElementIndex);

	void SetGeneralizedPanicLevel(float panicLevel)
	{
		mGeneralizedPanicLevel = panicLevel;
	}

public:

#ifdef IN_BARYLAB

	//
	// Barylab-specific
	//

	NpcParticles const & GetParticles() const
	{
		return mParticles;
	}

	bool AddHumanNpc(
		NpcSubKindIdType subKind,
		vec2f const & worldCoordinates,
		float currentSimulationTime);

	void FlipHumanWalk(int npcIndex);

	void FlipHumanFrontBack(int npcIndex);

	void MoveParticleBy(
		ElementIndex particleIndex,
		vec2f const & offset,
		float currentSimulationTime);

	void RotateParticlesWithShip(
		vec2f const & centerPos,
		float cosAngle,
		float sinAngle);

	void RotateParticleWithShip(
		StateType::NpcParticleStateType const & npcParticleState,
		vec2f const & centerPos,
		float cosAngle,
		float sinAngle,
		Ship const & homeShip);

	void OnPointMoved(float currentSimulationTime);

	//
	// Probing
	//

	void SelectParticle(ElementIndex particleIndex)
	{
		mCurrentlySelectedParticle = particleIndex;
		Publish();
	}

	std::optional<ElementIndex> GetCurrentOriginTriangle() const
	{
		return mCurrentOriginTriangle;
	}

	void SelectOriginTriangle(ElementIndex triangleIndex)
	{
		mCurrentOriginTriangle = triangleIndex;
	}

	void ResetOriginTriangle()
	{
		mCurrentOriginTriangle.reset();
	}

	void NotifyParticleTrajectory(
		ElementIndex particleIndex,
		vec2f const & targetPosition)
	{
		mCurrentParticleTrajectoryNotification.emplace(
			particleIndex,
			targetPosition);

		mCurrentParticleTrajectory.reset();
	}

	void SetParticleTrajectory(
		ElementIndex particleIndex,
		vec2f const & targetPosition)
	{
		mCurrentParticleTrajectory.emplace(
			particleIndex,
			targetPosition);

		mCurrentParticleTrajectoryNotification.reset();
	}

	bool IsTriangleConstrainingCurrentlySelectedParticle(ElementIndex triangleIndex) const;

	bool IsSpringHostingCurrentlySelectedParticle(ElementIndex springIndex) const;

	void Publish() const;

#endif

private:

	void PublishCount();

	void PublishSelection();

	NpcId GetNewNpcId();

	NpcSubKindIdType ChooseSubKind(
		NpcKindType kind,
		std::optional<ShipId> shipId) const;

	bool CommonNpcRemoval(NpcId npcId);

	size_t CalculateTotalNpcCount() const;

	ShipId GetTopmostShipId() const;

	std::optional<GlobalElementId> FindTopmostWorkableTriangleContaining(vec2f const & position) const;

	static ElementIndex FindWorkableTriangleContaining(
		vec2f const & position,
		Ship const & homeShip,
		std::optional<ConnectedComponentId> constrainedConnectedComponentId);

	void TransferNpcToShip(
		StateType & npc,
		ShipId newShip);

	static inline int GetSpringAmongEndpoints(
		int particleEndpoint1,
		int particleEndpoint2,
		StateType::ParticleMeshType const & particleMesh)
	{
		assert(particleMesh.Particles.size() >= 2);
		ElementIndex p1 = particleMesh.Particles[particleEndpoint1].ParticleIndex;
		ElementIndex p2 = particleMesh.Particles[particleEndpoint2].ParticleIndex;
		for (size_t s = 0; s < particleMesh.Springs.size(); ++s)
		{
			auto const & spring = particleMesh.Springs[s];
			if ((spring.EndpointAIndex == p1 && spring.EndpointBIndex == p2)
				|| (spring.EndpointBIndex == p1 && spring.EndpointAIndex == p2))
			{
				return static_cast<int>(s);
			}
		}

		assert(false);
		return -1;
	}

	void PublishHumanNpcStats();

	void RenderNpc(
		StateType const & npc,
		Render::RenderContext & renderContext,
		Render::ShipRenderContext & shipRenderContext) const;

	template<NpcRenderModeType RenderMode>
	void RenderNpc(
		StateType const & npc,
		Render::RenderContext & renderContext,
		Render::ShipRenderContext & shipRenderContext) const;

	void UpdateNpcAnimation(
		StateType & npc,
		float currentSimulationTime,
		Ship const & homeShip);

private:

	//
	// Simulation
	//

	void InternalEndMoveNpc(
		NpcId id,
		float currentSimulationTime);

	void InternalCompleteNewNpc(
		NpcId id,
		float currentSimulationTime);

	void ResetNpcStateToWorld(
		StateType & npc,
		float currentSimulationTime);

	void ResetNpcStateToWorld(
		StateType & npc,
		float currentSimulationTime,
		Ship const & homeShip,
		std::optional<ElementIndex> primaryParticleTriangleIndex);

	void TransitionParticleToConstrainedState(
		StateType & npc,
		int npcParticleOrdinal,
		StateType::NpcParticleStateType::ConstrainedStateType constrainedState);

	void TransitionParticleToFreeState(
		StateType & npc,
		int npcParticleOrdinal,
		Ship const & homeShip);

	static std::optional<StateType::NpcParticleStateType::ConstrainedStateType> CalculateParticleConstrainedState(
		vec2f const & position,
		Ship const & homeShip,
		std::optional<ElementIndex> triangleIndex,
		std::optional<ConnectedComponentId> constrainedConnectedComponentId);

	void OnMayBeNpcRegimeChanged(
		StateType::RegimeType oldRegime,
		StateType const & npc);

	static StateType::RegimeType CalculateRegime(StateType const & npc);

	void UpdateNpcs(
		float currentSimulationTime,
		Storm::Parameters const & stormParameters,
		GameParameters const & gameParameters);

	void UpdateNpcsEnd();

	void UpdateNpcParticlePhysics(
		StateType & npc,
		int npcParticleOrdinal,
		Ship & homeShip,
		float currentSimulationTime,
		GameParameters const & gameParameters);

	inline void CalculateNpcParticlePreliminaryForces(
		StateType const & npc,
		int npcParticleOrdinal,
		vec2f const & globalWindForce,
		GameParameters const & gameParameters);

	inline void CalculateNpcParticleSpringForces(StateType const & npc);

	inline vec2f CalculateNpcParticleDefinitiveForces(
		StateType const & npc,
		int npcParticleOrdinal,
		GameParameters const & gameParameters) const;

	void RecalculateSizeAndMassParameters();

	static float CalculateParticleMass(
		float baseMass,
		float sizeMultiplier
#ifdef IN_BARYLAB
		, float massAdjustment
#endif
		);

	static float CalculateParticleBuoyancyFactor(
		float baseBuoyancyVolumeFill,
		float sizeMultiplier
#ifdef IN_BARYLAB
		, float buoyancyAdjustment
#endif
		);

	void RecalculateFrictionTotalAdjustments();

	static float CalculateFrictionTotalAdjustment(
		float npcSurfaceFrictionAdjustment,
		float npcAdjustment,
		float globalAdjustment);

	static void CalculateSprings(
		float sizeMultiplier,
#ifdef IN_BARYLAB
		float massAdjustment,
#endif
		float springReductionFractionAdjustment,
		float springDampingCoefficientAdjustment,
		NpcParticles const & particles,
		StateType::ParticleMeshType & mesh); // In/Out

	static float CalculateSpringLength(
		float baseLength,
		float sizeMultiplier);

	void RecalculateGlobalDampingFactor();

	void UpdateNpcParticle_BeingPlaced(
		StateType & npc,
		int npcParticleOrdinal,
		vec2f physicsDeltaPos,
		NpcParticles & particles) const;

	void UpdateNpcParticle_Free(
		StateType::NpcParticleStateType & particle,
		vec2f const & startPosition,
		vec2f const & endPosition,
		NpcParticles & particles) const;

	struct ConstrainedNonInertialOutcome
	{
		float EdgeTraveled;				// During this single step; always valid
		bool DoStop;					// When set, we can stop
		bool HasBounced;				// If we have bounced (and then stopped)
		int FloorEdgeOrdinal;			// If we continue, this is the next edge we have chosen to walk upon; -1 if have to determine with floor normals

		static ConstrainedNonInertialOutcome MakeContinueOutcome(
			float edgeTraveled,
			int floorEdgeOrdinal)
		{
			return ConstrainedNonInertialOutcome(
				edgeTraveled,
				false,
				false,
				floorEdgeOrdinal);
		}

		static ConstrainedNonInertialOutcome MakeStopOutcome(
			float edgeTraveled,
			bool hasBounced)
		{
			return ConstrainedNonInertialOutcome(
				edgeTraveled,
				true,
				hasBounced,
				-1); // Irrelevant
		}

	private:

		ConstrainedNonInertialOutcome(
			float edgeTraveled,
			bool doStop,
			bool hasBounced,
			int floorEdgeOrdinal)
			: EdgeTraveled(edgeTraveled)
			, DoStop(doStop)
			, HasBounced(hasBounced)
			, FloorEdgeOrdinal(floorEdgeOrdinal)
		{}
	};

	// Returns total edge traveled (in step), and isStop
	ConstrainedNonInertialOutcome UpdateNpcParticle_ConstrainedNonInertial(
		StateType & npc,
		int npcParticleOrdinal,
		int edgeOrdinal,
		vec2f const & edgeDir,
		vec2f const & particleStartAbsolutePosition,
		vec2f const & trajectoryStartAbsolutePosition,
		vec2f const & flattenedTrajectoryEndAbsolutePosition,
		bcoords3f flattenedTrajectoryEndBarycentricCoords,
		vec2f const & flattenedTrajectory,
		float edgeTraveledPlanned,
		bool hasMovedInStep,
		vec2f const meshVelocity,
		float dt,
		Ship & homeShip,
		NpcParticles & particles,
		float currentSimulationTime,
		GameParameters const & gameParameters);

	float UpdateNpcParticle_ConstrainedInertial(
		StateType & npc,
		int npcParticleOrdinal,
		vec2f const & particleStartAbsolutePosition,
		vec2f const & segmentTrajectoryStartAbsolutePosition,
		vec2f const & segmentTrajectoryEndAbsolutePosition,
		bcoords3f segmentTrajectoryEndBarycentricCoords,
		bool hasMovedInStep,
		vec2f const meshVelocity,
		float segmentDt,
		Ship & homeShip,
		NpcParticles & particles,
		float currentSimulationTime,
		GameParameters const & gameParameters);

	struct NavigateVertexOutcome
	{
		enum class OutcomeType
		{
			ContinueToInterior,		// {TriangleBCoords}
			ContinueAlongFloor,		// {TriangleBCoords, FloorEdgeOrdinal}
			ImpactOnFloor,			// {TriangleBCoords, FloorEdgeOrdinal}
			BecomeFree				// {}
		};

		OutcomeType Type;

		AbsoluteTriangleBCoords TriangleBCoords;
		int FloorEdgeOrdinal; // In TriangleBCoords's triangle

		static NavigateVertexOutcome MakeContinueToInteriorOutcome(AbsoluteTriangleBCoords const & triangleBCoords)
		{
			return NavigateVertexOutcome(OutcomeType::ContinueToInterior, triangleBCoords, -1);
		}

		static NavigateVertexOutcome MakeContinueAlongFloorOutcome(
			AbsoluteTriangleBCoords const & triangleBCoords,
			int floorEdgeOrdinal)
		{
			return NavigateVertexOutcome(OutcomeType::ContinueAlongFloor, triangleBCoords, floorEdgeOrdinal);
		}

		static NavigateVertexOutcome MakeImpactOnFloorOutcome(
			AbsoluteTriangleBCoords const & triangleBCoords,
			int floorEdgeOrdinal)
		{
			return NavigateVertexOutcome(OutcomeType::ImpactOnFloor, triangleBCoords, floorEdgeOrdinal);
		}

		static NavigateVertexOutcome MakeBecomeFreeOutcome()
		{
			return NavigateVertexOutcome(OutcomeType::BecomeFree, {}, - 1);
		}

	private:

		NavigateVertexOutcome(
			OutcomeType type,
			AbsoluteTriangleBCoords const & triangleBCoords,
			int floorEdgeOrdinal)
			: Type(type)
			, TriangleBCoords(triangleBCoords)
			, FloorEdgeOrdinal(floorEdgeOrdinal)
		{}
	};

	static inline NavigateVertexOutcome NavigateVertex(
		StateType const & npc,
		int npcParticleOrdinal,
		std::optional<TriangleAndEdge> const & walkedEdge,
		int vertexOrdinal,
		vec2f const & trajectory,
		vec2f const & trajectoryEndAbsolutePosition,
		bcoords3f trajectoryEndBarycentricCoords,
		Ship const & homeShip,
		NpcParticles const & particles);

	void BounceConstrainedNpcParticle(
		StateType & npc,
		int npcParticleOrdinal,
		vec2f const & trajectory,
		bool hasMovedInStep,
		vec2f const & bouncePosition,
		int bounceEdgeOrdinal,
		vec2f const meshVelocity,
		float dt,
		Ship & homeShip,
		NpcParticles & particles,
		float currentSimulationTime,
		GameParameters const & gameParameters) const;

	void OnImpact(
		StateType & npc,
		int npcParticleOrdinal,
		vec2f const & normalResponse,
		vec2f const & bounceEdgeNormal,
		float currentSimulationTime) const;

	inline void MaintainInWorldBounds(
		StateType & npc,
		int npcParticleOrdinal,
		Ship const & homeShip,
		GameParameters const & gameParameters)
	{
		float constexpr MaxWorldLeft = -GameParameters::HalfMaxWorldWidth;
		float constexpr MaxWorldRight = GameParameters::HalfMaxWorldWidth;

		float constexpr MaxWorldTop = GameParameters::HalfMaxWorldHeight;
		float constexpr MaxWorldBottom = -GameParameters::HalfMaxWorldHeight;

		// Elasticity of the bounce against world boundaries
		//  - We use the ocean floor's elasticity for convenience
		float const elasticity = gameParameters.OceanFloorElasticityCoefficient * gameParameters.ElasticityAdjustment;

		// We clamp velocity to damp system instabilities at extreme events
		static constexpr float MaxBounceVelocity = 150.0f; // Magic number

		ElementIndex const p = npc.ParticleMesh.Particles[npcParticleOrdinal].ParticleIndex;
		auto const & pos = mParticles.GetPosition(p);
		bool hasHit = false;
		if (pos.x < MaxWorldLeft)
		{
			// Simulate bounce, bounded
			mParticles.GetPosition(p).x = std::min(MaxWorldLeft + elasticity * (MaxWorldLeft - pos.x), 0.0f);

			// Bounce bounded
			mParticles.GetVelocity(p).x = std::min(-mParticles.GetVelocity(p).x, MaxBounceVelocity);

			hasHit = true;
		}
		else if (pos.x > MaxWorldRight)
		{
			// Simulate bounce, bounded
			mParticles.GetPosition(p).x = std::max(MaxWorldRight - elasticity * (pos.x - MaxWorldRight), 0.0f);

			// Bounce bounded
			mParticles.GetVelocity(p).x = std::max(-mParticles.GetVelocity(p).x, -MaxBounceVelocity);

			hasHit = true;
		}

		if (pos.y > MaxWorldTop)
		{
			// Simulate bounce, bounded
			mParticles.GetPosition(p).y = std::max(MaxWorldTop - elasticity * (pos.y - MaxWorldTop), 0.0f);

			// Bounce bounded
			mParticles.GetVelocity(p).y = std::max(-mParticles.GetVelocity(p).y, -MaxBounceVelocity);

			hasHit = true;
		}
		else if (pos.y < MaxWorldBottom)
		{
			// Simulate bounce, bounded
			mParticles.GetPosition(p).y = std::min(MaxWorldBottom + elasticity * (MaxWorldBottom - pos.y), 0.0f);

			// Bounce bounded
			mParticles.GetVelocity(p).y = std::min(-mParticles.GetVelocity(p).y, MaxBounceVelocity);

			hasHit = true;
		}

		assert(mParticles.GetPosition(p).x >= MaxWorldLeft);
		assert(mParticles.GetPosition(p).x <= MaxWorldRight);
		assert(mParticles.GetPosition(p).y >= MaxWorldBottom);
		assert(mParticles.GetPosition(p).y <= MaxWorldTop);

		if (hasHit)
		{
			// Avoid bouncing back and forth
			TransitionParticleToFreeState(npc, npcParticleOrdinal, homeShip);
		}
	}

	inline void MaintainOverLand(
		StateType & npc,
		int npcParticleOrdinal,
		Ship const & homeShip,
		GameParameters const & gameParameters);

	static bool IsEdgeFloorToParticle(
		ElementIndex triangleElementIndex,
		int edgeOrdinal,
		StateType const & npc,
		int npcParticleOrdinal,
		NpcParticles const & npcParticles,
		Ship const & homeShip)
	{
		// First off: if this edge, regardless of its floorness, separates us from a folded triangle,
		// then we consider it as a floor, since we want to avoid folded triangles like the plague

		if (auto const & oppositeTriangleInfo = homeShip.GetTriangles().GetOppositeTriangle(triangleElementIndex, edgeOrdinal);
			oppositeTriangleInfo.TriangleElementIndex != NoneElementIndex
			&& IsTriangleFolded(oppositeTriangleInfo.TriangleElementIndex, homeShip))
		{
			return true;
		}

		// Now: if not a floor, then it's not a floor

		if (homeShip.GetTriangles().GetSubSpringNpcFloorKind(triangleElementIndex, edgeOrdinal) == NpcFloorKindType::NotAFloor)
		{
			return false;
		}

		// Ok, it's a floor

		auto const & npcParticle = npc.ParticleMesh.Particles[npcParticleOrdinal];

		// If ghost, not a floor

		if (npcParticle.ConstrainedState.has_value()
			&& npcParticle.ConstrainedState->GhostParticlePulse)
		{
			return false;
		}

		// Ok, it's a floor and we're not ghosting

		// If it's not the secondary of a dipole, then every floor is a floor

		if (npc.ParticleMesh.Particles.size() != 2
			|| npcParticleOrdinal == 0)
		{
			return true;
		}

		// Ok, it's a floor and this is a secondary particle of a dipole (e.g. head)
		//
		// Secondary particles have a ton of rules to ensure that e.g. the head
		// of a NPC doesn't behave as if it were disjoint from the feet; for
		// example we don't want the head to bang on a plane that separates it
		// from the feet, or to bang their head on a staircase above the floor
		// we're walking on.

		auto const & primaryParticle = npc.ParticleMesh.Particles[0];

		// If it's a human walking, check rules using floor depths to determine
		// which floors are seen as floors by this secondary (head)

		if (npc.Kind == NpcKindType::Human
			&& (npc.KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking
				|| (npc.KindSpecificState.HumanNpcState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Rising
					&& // During rising, do not try to ghost the edge that the secondary is resting upon
					(	!npcParticle.ConstrainedState.has_value()
						|| !npcParticle.ConstrainedState->CurrentVirtualFloor.has_value()
						|| npcParticle.ConstrainedState->CurrentVirtualFloor->TriangleElementIndex != triangleElementIndex
						|| npcParticle.ConstrainedState->CurrentVirtualFloor->EdgeOrdinal != edgeOrdinal
					)
					))
			&& primaryParticle.ConstrainedState.has_value()
			&& primaryParticle.ConstrainedState->CurrentVirtualFloor.has_value())
		{
			auto const floorGeometry = homeShip.GetTriangles().GetSubSpringNpcFloorGeometry(
				triangleElementIndex,
				edgeOrdinal);

			auto const floorGeometryDepth = NpcFloorGeometryDepth(floorGeometry);

			auto const primaryFloorGeometry = homeShip.GetTriangles().GetSubSpringNpcFloorGeometry(
				primaryParticle.ConstrainedState->CurrentVirtualFloor->TriangleElementIndex,
				primaryParticle.ConstrainedState->CurrentVirtualFloor->EdgeOrdinal);

			auto const primaryFloorDepth = NpcFloorGeometryDepth(primaryFloorGeometry);

			// Rule 1: other depth is never floor
			// - So e.g. walking up a stair doesn't make us bang our head on the floor above
			// - So e.g. walking on a floor doesn't make us bang our head on a stair
			if (floorGeometryDepth != primaryFloorDepth)
			{
				return false;
			}

			// Rule 2: when on a Sx depth, Sy is never floor
			// - So e.g. we don't bang our head at orthogonal stair intersections
			if (floorGeometryDepth == NpcFloorGeometryDepthType::Depth2
				&& primaryFloorDepth == NpcFloorGeometryDepthType::Depth2
				&& floorGeometry != primaryFloorGeometry)
			{
				return false;
			}
		}

		// If the primary is not on the other side of this edge, then every floor is a floor

		vec2f const & primaryPosition = npcParticles.GetPosition(primaryParticle.ParticleIndex);
		bcoords3f const primaryBaryCoords = homeShip.GetTriangles().ToBarycentricCoordinates(primaryPosition, triangleElementIndex, homeShip.GetPoints());

		// It's on the other side of the edge if its "edge's" b-coord is negative
		if (primaryBaryCoords[(edgeOrdinal + 2) % 3] >= -0.05f) // Some slack
		{
			return true;
		}

		// Ok, it's a floor and it's separating this secondary particle from the primary

		// Now a bit of a hack: at this moment we're hurting because of the "hanging head" problem, i.e.
		// a human NPC ending with its head on an edge and it feet hanging underneath. To prevent this,
		// we consider this as a floor only if the human is not "quite vertical"

		vec2f const & secondaryPosition = npcParticles.GetPosition(npcParticle.ParticleIndex);
		vec2f const humanDir = (primaryPosition - secondaryPosition).normalise(); // Pointing down to feet

		// It's vertical when y is -1.0 (cos of angle)
		if (humanDir.y > -0.8f)
		{
			// Not quite vertical
			return true;
		}

		return false;
	}

	static bool IsTriangleFolded(
		ElementIndex triangleElementIndex,
		Ship const & homeShip)
	{
		return IsTriangleFolded(
			homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointAIndex(triangleElementIndex)),
			homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointBIndex(triangleElementIndex)),
			homeShip.GetPoints().GetPosition(homeShip.GetTriangles().GetPointCIndex(triangleElementIndex)));
	}

	static bool IsTriangleFolded(
		vec2f const & aPosition,
		vec2f const & bPosition,
		vec2f const & cPosition)
	{
		return !Geometry::AreVerticesInCwOrder(aPosition, bPosition, cPosition);
	}

	static bool HasBomb(
		StateType const & npc,
		Ship const & homeShip)
	{
		for (size_t p = 0; p < npc.ParticleMesh.Particles.size(); ++p)
		{
			auto const & particle = npc.ParticleMesh.Particles[p];
			if (particle.ConstrainedState.has_value())
			{
				for (auto pointElementIndex : homeShip.GetTriangles().GetPointIndices(particle.ConstrainedState->CurrentBCoords.TriangleElementIndex))
				{
					if (homeShip.AreBombsInProximity(pointElementIndex))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	static bool IsElectrified(
		StateType const & npc,
		Ship const & homeShip)
	{
		for (size_t p = 0; p < npc.ParticleMesh.Particles.size(); ++p)
		{
			auto const & particle = npc.ParticleMesh.Particles[p];
			if (particle.ConstrainedState.has_value())
			{
				for (auto pointElementIndex : homeShip.GetTriangles().GetPointIndices(particle.ConstrainedState->CurrentBCoords.TriangleElementIndex))
				{
					if (homeShip.GetPoints().GetIsElectrified(pointElementIndex))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	static vec2f CalculateSpringVector(ElementIndex primaryParticleIndex, ElementIndex secondaryParticleIndex, NpcParticles const & particles)
	{
		return particles.GetPosition(primaryParticleIndex) - particles.GetPosition(secondaryParticleIndex);
	}

	static float CalculateVerticalAlignment(vec2f const & vector)
	{
		return vector.normalise().dot(GameParameters::GravityDir);
	}

	static float CalculateSpringVerticalAlignment(ElementIndex primaryParticleIndex, ElementIndex secondaryParticleIndex, NpcParticles const & particles)
	{
		return CalculateVerticalAlignment(CalculateSpringVector(primaryParticleIndex, secondaryParticleIndex, particles));
	}

	//
	// Human simulation
	//

	static StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType CalculateHumanBehavior(StateType & npc);

	void UpdateHuman(
		StateType & npc,
		float currentSimulationTime,
		Ship & homeShip,
		GameParameters const & gameParameters);

	inline bool CheckAndMaintainHumanEquilibrium(
		ElementIndex primaryParticleIndex,
		ElementIndex secondaryParticleIndex,
		StateType::KindSpecificStateType::HumanNpcStateType & humanState,
		bool doMaintainEquilibrium,
		NpcParticles & particles,
		GameParameters const & gameParameters);

	inline void RunWalkingHumanStateMachine(
		StateType::KindSpecificStateType::HumanNpcStateType & humanState,
		StateType::NpcParticleStateType const & primaryParticleState,
		Ship const & homeShip,
		GameParameters const & gameParameters);

	void OnHumanImpact(
		StateType & npc,
		int npcParticleOrdinal,
		vec2f const & normalResponse,
		vec2f const & bounceEdgeNormal,
		float currentSimulationTime) const;

	using DoImmediate = StrongTypedBool<struct _DoImmediate>;

	void FlipHumanWalk(
		StateType::KindSpecificStateType::HumanNpcStateType & humanState,
		DoImmediate doImmediate) const;

	void TransitionHumanBehaviorToFree(
		StateType & npc,
		float currentSimulationTime);

	float CalculateActualHumanWalkingAbsoluteSpeed(StateType::KindSpecificStateType::HumanNpcStateType & humanState) const
	{
		assert(humanState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking);

		return humanState.WalkingSpeedBase * CalculateHumanWalkingSpeedAdjustment(humanState);
	}

	float CalculateHumanWalkingSpeedAdjustment(StateType::KindSpecificStateType::HumanNpcStateType & humanState) const
	{
		assert(humanState.CurrentBehavior == StateType::KindSpecificStateType::HumanNpcStateType::BehaviorType::Constrained_Walking);

		return std::min(
			humanState.CurrentBehaviorState.Constrained_Walking.CurrentWalkMagnitude // Note that this is the only one that might be zero
			* mCurrentHumanNpcWalkingSpeedAdjustment
			* (1.0f + std::min(humanState.ResultantPanicLevel, 1.0f) * 3.0f),
			GameParameters::MaxHumanNpcTotalWalkingSpeedAdjustment); // Absolute cap
	}

private:

	World & mParentWorld;
	NpcDatabase const & mNpcDatabase;
	std::shared_ptr<GameEventDispatcher> mGameEventHandler;
	size_t const mMaxNpcs;

	//
	// Container
	//
	// Use cases:
	//	1. Reaching all NPCs of a specific ship(e.g.because of ship - wide interactions, such as electrical tool, alarm, deleting ship, etc.)
	//	2. Allow an NPC to move ships(e.g.the one "being placed")
	//	3. Reaching an NPC by its ID
	//

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // std::optional of StateType gets padded because of alignment requirements
#endif

	// The actual container of NPC states, indexed by NPC ID.
	// Indices are stable; elements are null'ed when removed.
	std::vector<std::optional<StateType>> mStateBuffer;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	// All the ships - together with their NPCs - indexed by Ship ID.
	// Indices are stable; elements are null'ed when removed.
	std::vector<std::optional<ShipNpcsType>> mShips;

	// All of the NPC particles.
	NpcParticles mParticles;

	//
	// State
	//

	SequenceNumber mCurrentSimulationSequenceNumber;

	std::optional<NpcId> mCurrentlySelectedNpc;
	GameWallClock::time_point mCurrentlySelectedNpcWallClockTimestamp;
	std::optional<NpcId> mCurrentlyHighlightedNpc;

	float mGeneralizedPanicLevel; // [0.0f ... +1.0f], manually decayed

	//
	// Stats
	//

	ElementCount mFreeRegimeHumanNpcCount;
	ElementCount mConstrainedRegimeHumanNpcCount;

	//
	// Simulation parameters
	//

	float mGlobalDampingFactor; // Calculated

	// Cached from game parameters
	float mCurrentGlobalDampingAdjustment;
	float mCurrentSizeMultiplier;
	float mCurrentHumanNpcWalkingSpeedAdjustment;
	float mCurrentSpringReductionFractionAdjustment;
	float mCurrentSpringDampingCoefficientAdjustment;
	float mCurrentStaticFrictionAdjustment;
	float mCurrentKineticFrictionAdjustment;
	float mCurrentNpcFrictionAdjustment;

#ifdef IN_BARYLAB

	// Cached from LabController
	float mCurrentMassAdjustment{ 1.0f };
	float mCurrentBuoyancyAdjustment{ 1.0f };
	float mCurrentGravityAdjustment{ 1.0f };

#endif

#ifdef IN_BARYLAB

	//
	// Probing
	//

	std::optional<ElementIndex> mCurrentlySelectedParticle;
	std::optional<ElementIndex> mCurrentOriginTriangle;

	std::optional<ParticleTrajectory> mCurrentParticleTrajectory;
	std::optional<ParticleTrajectory> mCurrentParticleTrajectoryNotification;

#endif
};

}
