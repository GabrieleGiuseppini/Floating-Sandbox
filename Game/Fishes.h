/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FishSpeciesDatabase.h"
#include "GameParameters.h"
#include "RenderContext.h"
#include "RenderTypes.h"
#include "VisibleWorld.h"

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <optional>
#include <vector>

namespace Physics
{

class Fishes
{

public:

    explicit Fishes(FishSpeciesDatabase const & fishSpeciesDatabase);

    void ApplyDisturbanceAt(vec2f const & worldCoordinates)
    {
        mCurrentInteractiveDisturbance = worldCoordinates;
    }

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void Upload(Render::RenderContext & renderContext) const;

private:

    struct FishShoal
    {
        FishSpecies const & Species;
        size_t CurrentMemberCount;

        vec2f InitialPosition;
        vec2f InitialDirection;

        FishShoal(FishSpecies const & species)
            : Species(species)
            , CurrentMemberCount(0)
            , InitialPosition(vec2f::zero())
            , InitialDirection(vec2f::zero())
        {}
    };

    // Shoal ID is index in Shoals vector
    using FishShoalId = size_t;

    enum class SteeringType
    {
        CruiseWithTurn,
        CruiseWithoutTurn
    };

    struct Fish
    {
        FishShoalId ShoalId;

        float PersonalitySeed;

        vec2f CurrentPosition;
        vec2f TargetPosition;

        vec2f StartVelocity;
        vec2f CurrentVelocity;
        vec2f TargetVelocity;

        vec2f StartDirection;
        vec2f CurrentDirection;
        vec2f TargetDirection;

        float CurrentProgressPhase;
        float CurrentProgress; // Calcd off CurrentProgressPhase

        // Steering state machine
        std::optional<SteeringType> CurrentSteeringState;
        float SteeringSimulationTimeStart;

        Fish(
            FishShoalId shoalId,
            float personalitySeed,
            vec2f const & initialPosition,
            vec2f const & targetPosition,
            vec2f const & targetVelocity,
            float initialProgressPhase)
            : ShoalId(shoalId)
            , PersonalitySeed(personalitySeed)
            , CurrentPosition(initialPosition)
            , TargetPosition(targetPosition)
            , StartVelocity(targetVelocity) // We start up with current velocity
            , CurrentVelocity(targetVelocity)
            , TargetVelocity(targetVelocity)
            , StartDirection(targetVelocity.normalise())
            , CurrentDirection(StartDirection)
            , TargetDirection(StartDirection)
            , CurrentProgressPhase(initialProgressPhase)
            , CurrentProgress(0.0f) // Assumption: progress==0 @ phase==0
            , CurrentSteeringState()
            , SteeringSimulationTimeStart(0.0f) // Arbitrary
        {}
    };

private:

    void CreateNewFishShoalBatch();

    inline static vec2f FindPosition(
        vec2f const & averagePosition,
        float xVariance,
        float yVariance);

    inline static vec2f CalculateNewCruisingTargetPosition(
        vec2f const & currentPosition,
        vec2f const & newDirection,
        VisibleWorld const & visibleWorld);

    inline static vec2f CalculateVelocity(
        vec2f const & startPosition,
        vec2f const & endPosition,
        FishSpecies const & species,
        float velocityMultiplier,
        float personalitySeed);

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;

    // A shoal batch is a set of shoals, one for each species
    size_t const mShoalBatchSize;

    // Shoals never move around in the vector
    std::vector<FishShoal> mFishShoals;

    std::vector<Fish> mFishes;

    // The world position at which there's been an interactive disturbance, if any
    std::optional<vec2f> mCurrentInteractiveDisturbance;
};

}
