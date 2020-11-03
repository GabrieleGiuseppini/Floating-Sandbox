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

    void Update(
        float currentSimulationTime,
        OceanSurface & oceanSurface,
        OceanFloor const & oceanFloor,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void Upload(Render::RenderContext & renderContext) const;

public:

    void ApplyDisturbanceAt(vec2f const & worldCoordinates)
    {
        mCurrentInteractiveDisturbance = worldCoordinates;

        // TODOTEST
        /*
        auto & fish = mFishes[0];
        fish.TargetPosition = worldCoordinates;

        // Calculate new target velocity and direction
        fish.StartVelocity = fish.CurrentVelocity;
        fish.TargetVelocity = MakeBasalVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), mFishShoals[fish.ShoalId].Species, fish.PersonalitySeed);
        fish.StartDirection = fish.CurrentDirection;
        fish.TargetDirection = fish.TargetVelocity.normalise();
        */
    }

    void ApplyAttractionAt(vec2f const & worldCoordinates)
    {
        mCurrentInteractiveAttraction = worldCoordinates;
    }

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

        vec2f CurrentRenderVector;
        vec2f TargetRenderVector;

        float CurrentDirectionSmoothingConvergenceRate; // Rate of converge of velocity and direction

        float CurrentTailProgressPhase;

        // Panic mode state machine
        float PanicCharge; // When not zero, fish is panic mode; decays towards zero

        // Steering state machine
        std::optional<CruiseSteering> CruiseSteeringState; // When set, fish is turning around during cruise

        // Freefall state machine
        bool IsInFreefall;

        Fish(
            FishShoalId shoalId,
            float personalitySeed,
            vec2f const & initialPosition,
            vec2f const & targetPosition,
            vec2f const & targetVelocity,
            float initialTailProgressPhase)
            : ShoalId(shoalId)
            , PersonalitySeed(personalitySeed)
            , CurrentPosition(initialPosition)
            , TargetPosition(targetPosition)
            , CurrentVelocity(targetVelocity)
            , TargetVelocity(CurrentVelocity)
            , CurrentRenderVector(targetVelocity.normalise())
            , TargetRenderVector(CurrentRenderVector)
            , CurrentDirectionSmoothingConvergenceRate(0.0f) // Arbitrary, will be set as needed
            , CurrentTailProgressPhase(initialTailProgressPhase)
            , PanicCharge(0.0f)
            , CruiseSteeringState()
            , IsInFreefall(false)
        {}
    };

private:

    void CreateNewFishShoalBatch();

    inline static vec2f FindPosition(
        vec2f const & averagePosition,
        float xVariance,
        float yVariance);

    inline static vec2f FindNewCruisingTargetPosition(
        vec2f const & currentPosition,
        vec2f const & newDirection,
        VisibleWorld const & visibleWorld);

    inline static vec2f MakeBasalVelocity(
        vec2f const & direction,
        FishSpecies const & species,
        float personalitySeed,
        GameParameters const & gameParameters);

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;

    // A shoal batch is a set of shoals, one for each species
    size_t const mShoalBatchSize;

    // Shoals never move around in the vector
    std::vector<FishShoal> mFishShoals;

    std::vector<Fish> mFishes;

    // The world position at which there's been an interactive disturbance/attraction, if any
    std::optional<vec2f> mCurrentInteractiveDisturbance;
    std::optional<vec2f> mCurrentInteractiveAttraction;

    // Parameters that the calculated values are current with
    float mCurrentFishSizeAdjustment;
};

}
