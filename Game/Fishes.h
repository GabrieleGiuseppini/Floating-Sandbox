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

#include <vector>

namespace Physics
{

class Fishes
{

public:

    explicit Fishes(FishSpeciesDatabase const & fishSpeciesDatabase);

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void Upload(Render::RenderContext & renderContext) const;

private:

    static vec2f ChooseTargetPosition(
        FishSpecies const & fishSpecies,
        VisibleWorld const & visibleWorld);

    inline static vec2f CalculateNewCruisingTargetPosition(
        vec2f const & currentPosition,
        FishSpecies const & species,
        VisibleWorld const & visibleWorld);

    inline static vec2f CalculateVelocity(
        vec2f const & startPosition,
        vec2f const & endPosition,
        FishSpecies const & species,
        float velocityMultiplier,
        float personalitySeed);

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;

    enum class StateType
    {
        Cruising,
        CruiseSteering_WithTurn,
        CruiseSteering_WithoutTurn
    };

    struct Fish
    {
        FishSpecies const * Species;
        TextureFrameIndex RenderFrameIndex;
        float PersonalitySeed;

        StateType CurrentState;

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

        float SteeringSimulationTimeStart;

        Fish(
            FishSpecies const * species,
            TextureFrameIndex renderFrameIndex,
            float personalitySeed,
            StateType initialState,
            vec2f const & initialPosition,
            vec2f const & targetPosition,
            vec2f const & targetVelocity,
            float initialProgressPhase)
            : Species(species)
            , RenderFrameIndex(renderFrameIndex)
            , PersonalitySeed(personalitySeed)
            , CurrentState(initialState)
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
            , SteeringSimulationTimeStart(0.0f) // Arbitrary
        {}
    };

    std::vector<Fish> mFishes;
};

}
