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

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <memory.h>
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
        VisibleWorld const & visibleWorld);

    void Upload(Render::RenderContext & renderContext) const;

public:

    void DisturbAt(
        vec2f const & worldCoordinates,
        float worldRadius,
        GameParameters const & gameParameters);

    void AttractAt(
        vec2f const & worldCoordinates,
        float worldRadius,
        GameParameters const & gameParameters);

    void TriggerWidespreadPanic(GameParameters const & gameParameters);

private:

    struct FishShoal
    {
        FishSpecies const & Species;

        size_t CurrentMemberCount;
        ElementIndex StartFishIndex;

        vec2f InitialPosition;
        vec2f InitialDirection;

        float MaxWorldDimension;

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

        float CurrentTailProgressPhase;

        // Panic mode state machine
        float PanicCharge; // When not zero, fish is panic mode; decays towards zero

        // Provides a heartbeat for attractions
        float AttractionDecayTimer;

        // Provides a heartbeat for shoaling
        float ShoalingDecayTimer;

        // Steering state machine
        std::optional<CruiseSteering> CruiseSteeringState; // When set, fish is turning around during cruise
        float LastSteeringSimulationTime;

        // Freefall state machine
        bool IsInFreefall;

        // Index in vector of fish indices sorted by X
        ElementIndex FishesByXIndex;

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
            , ShoalingVelocity(vec2f::zero())
            , TargetVelocity(CurrentVelocity)
            , CurrentRenderVector(targetVelocity.normalise())
            , CurrentDirectionSmoothingConvergenceRate(0.0f) // Arbitrary, will be set as needed
            , CurrentTailProgressPhase(initialTailProgressPhase)
            , PanicCharge(0.0f)
            , AttractionDecayTimer(0.0f)
            , ShoalingDecayTimer(0.0f)
            , CruiseSteeringState()
            , LastSteeringSimulationTime(0.0f)
            , IsInFreefall(false)
            , FishesByXIndex(0) // Arbitrary, will be set as needed
        {}
    };

private:

    void UpdateNumberOfFishes(
        float currentSimulationTime,
        OceanSurface & oceanSurface,
        OceanFloor const & oceanFloor,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void UpdateDynamics(
        float currentSimulationTime,
        OceanSurface & oceanSurface,
        OceanFloor const & oceanFloor,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void UpdateShoaling(
        float currentSimulationTime,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void CreateNewFishShoalBatch(ElementIndex startFishIndex);

    inline static vec2f ChoosePosition(
        vec2f const & averagePosition,
        float xVariance,
        float yVariance);

    inline static vec2f FindNewCruisingTargetPosition(
        vec2f const & currentPosition,
        vec2f const & newDirection,
        VisibleWorld const & visibleWorld);

    inline static vec2f MakeCuisingVelocity(
        vec2f const & direction,
        FishSpecies const & species,
        float personalitySeed,
        GameParameters const & gameParameters);

    static std::vector<ElementIndex> SortByX(std::vector<Fish> & fishes);

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // A shoal batch is a set of shoals, one for each species
    size_t const mShoalBatchSize;

    // Shoals never move around in the vector
    std::vector<FishShoal> mFishShoals;

    // The...fishes
    std::vector<Fish> mFishes;

    // The indices of fishes in their vector, sorted by X
    std::vector<ElementIndex> mFishesByX;

    // Parameters that the calculated values are current with
    float mCurrentFishSizeMultiplier;
    float mCurrentFishSpeedAdjustment;
    bool mCurrentDoFishShoaling;
};

}
