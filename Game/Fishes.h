/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FishSpeciesDatabase.h"
#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "RenderContext.h"
#include "RenderTypes.h"
#include "VisibleWorld.h"

#include <GameCore/AABBSet.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Vectors.h>

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

namespace Physics
{

class Fishes
{

public:

    explicit Fishes(
        FishSpeciesDatabase const & fishSpeciesDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher);

    void Update(
        float currentSimulationTime,
        OceanSurface & oceanSurface,
        OceanFloor const & oceanFloor,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld,
        Geometry::AABBSet const & aabbSet);

    void Upload(Render::RenderContext & renderContext) const;

public:

    void DisturbAt(
        vec2f const & worldCoordinates,
        float worldRadius,
        std::chrono::milliseconds delay);

    void AttractAt(
        vec2f const & worldCoordinates,
        float worldRadius,
        std::chrono::milliseconds delay);

    void TriggerWidespreadPanic(std::chrono::milliseconds delay);

private:

    struct FishShoal
    {
        FishSpecies const & Species;

        ElementCount CurrentMemberCount;
        ElementIndex StartFishIndex;

        vec2f InitialPosition;
        vec2f InitialDirection;

        float MaxWorldDimension; // Inclusive of SizeMultiplier

        FishShoal(
            FishSpecies const & species,
            ElementIndex startFishIndex,
            float maxWorldDimension)
            : Species(species)
            , CurrentMemberCount(0)
            , StartFishIndex(startFishIndex)
            , InitialPosition(vec2f::zero())
            , InitialDirection(vec2f::zero())
            , MaxWorldDimension(maxWorldDimension)
        {}
    };

    // Shoal ID is index in Shoals vector
    using FishShoalId = size_t;

    struct Fish
    {
    public:

        struct CruiseSteering
        {
            vec2f StartVelocity;
            vec2f StartRenderVector;
            float SimulationTimeStart;
            float SimulationTimeDuration;

            CruiseSteering(
                vec2f startVelocity,
                vec2f startRenderVector,
                float simulationTimeStart,
                float simulationTimeDuration)
                : StartVelocity(startVelocity)
                , StartRenderVector(startRenderVector)
                , SimulationTimeStart(simulationTimeStart)
                , SimulationTimeDuration(simulationTimeDuration)
            {}
        };

    public:

        FishShoalId ShoalId;

        float PersonalitySeed;

        vec2f CurrentPosition;
        vec2f TargetPosition;

        vec2f CurrentVelocity;
        vec2f TargetVelocity;

        vec2f ShoalingVelocity;

        vec2f CurrentRenderVector;

        float CurrentDirectionSmoothingConvergenceRate; // Rate of converge of velocity and direction
        static float constexpr IdealDirectionSmoothingConvergenceRate = 0.016f;

        float HeadOffset; // Offset of head from position along fish direction
        float CurrentTailProgressPhase;

        // Panic mode state machine
        float PanicCharge; // When not zero, fish is panic mode; decays towards zero

        // Provides a heartbeat for attractions
        float AttractionDecayTimer;

        // Provides a heartbeat for shoaling
        float ShoalingTimer;
        static float constexpr ShoalingTimerCycleDuration = 2.421875f; // Simulation time

        // Steering state machine
        std::optional<CruiseSteering> CruiseSteeringState; // When set, fish is turning around during cruise
        float LastSteeringSimulationTime;

        // Freefall state machine
        bool IsInFreefall;

        // The texture frame for this fish
        TextureFrameId<Render::FishTextureGroups> RenderTextureFrameId;

        Fish(
            FishShoalId shoalId,
            float personalitySeed,
            vec2f const & initialPosition,
            vec2f const & targetPosition,
            vec2f const & targetVelocity,
            float headOffset,
            float initialTailProgressPhase,
            TextureFrameId<Render::FishTextureGroups> renderTextureFrameId)
            : ShoalId(shoalId)
            , PersonalitySeed(personalitySeed)
            , CurrentPosition(initialPosition)
            , TargetPosition(targetPosition)
            , CurrentVelocity(targetVelocity)
            , TargetVelocity(CurrentVelocity)
            , ShoalingVelocity(vec2f::zero())
            , CurrentRenderVector(targetVelocity.normalise())
            , CurrentDirectionSmoothingConvergenceRate(IdealDirectionSmoothingConvergenceRate)
            , HeadOffset(headOffset)
            , CurrentTailProgressPhase(initialTailProgressPhase)
            , PanicCharge(0.0f)
            , AttractionDecayTimer(0.0f)
            , ShoalingTimer(personalitySeed * ShoalingTimerCycleDuration) // Randomize a bit the shoaling cycles
            , CruiseSteeringState()
            , LastSteeringSimulationTime(0.0f)
            , IsInFreefall(false)
            , RenderTextureFrameId(renderTextureFrameId)
        {}
    };

    struct Interaction
    {
        enum class InteractionType
        {
            Attraction,
            Disturbance
        };

        struct AreaSpecification
        {
            vec2f Position;
            float Radius;

            AreaSpecification(
                vec2f const & position,
                float radius)
                : Position(position)
                , Radius(radius)
            {}
        };

        InteractionType Type;
        std::optional<AreaSpecification> Area;
        typename GameWallClock::time_point StartTime;

        Interaction(
            InteractionType type,
            std::optional<AreaSpecification> area,
            typename GameWallClock::time_point startTime)
            : Type(type)
            , Area(area)
            , StartTime(startTime)
        {}
    };

private:

    void UpdateNumberOfFishes(
        float currentSimulationTime,
        OceanSurface & oceanSurface,
        OceanFloor const & oceanFloor,
        Geometry::AABBSet const & aabbSet,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void UpdateInteractions(GameParameters const & gameParameters);

    void UpdateDynamics(
        float currentSimulationTime,
        OceanSurface & oceanSurface,
        OceanFloor const & oceanFloor,
        Geometry::AABBSet const & aabbSet,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void UpdateShoaling(
        float currentSimulationTime,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void EnactDisturbance(
        vec2f const & worldCoordinates,
        float worldRadius,
        GameParameters const & gameParameters);

    void EnactAttraction(
        vec2f const & worldCoordinates,
        float worldRadius,
        GameParameters const & gameParameters);

    void EnactWidespreadPanic(GameParameters const & gameParameters);

    inline static vec2f ChoosePosition(
        vec2f const & averagePosition,
        float xVariance,
        float yVariance);

    inline static vec2f FindPosition(
        vec2f const & averagePosition,
        float xVariance,
        float yVariance,
        OceanFloor const & oceanFloor,
        Geometry::AABBSet const & aabbSet);

    inline static vec2f FindNewCruisingTargetPosition(
        vec2f const & currentPosition,
        vec2f const & newDirection,
        FishSpecies const & species,
        VisibleWorld const & visibleWorld);

    inline static vec2f MakeCruisingVelocity(
        vec2f const & direction,
        FishSpecies const & species,
        float personalitySeed,
        GameParameters const & gameParameters);

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // Shoals never move around in this vector
    std::vector<FishShoal> mFishShoals;

    // The...fishes
    std::vector<Fish> mFishes;

    // Delayed interactions
    std::vector<Interaction> mInteractions;

    // Parameters that the calculated values are current with
    float mCurrentFishSizeMultiplier;
    float mCurrentFishSpeedAdjustment;
    bool mCurrentDoFishShoaling;
};

}
