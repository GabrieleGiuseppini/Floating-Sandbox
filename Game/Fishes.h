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

    vec2f ChooseTargetPosition(
        FishSpecies const & fishSpecies,
        VisibleWorld const & visibleWorld,
        float currentY) const;

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;

    enum class StateType
    {
        Cruising,
        Fleeing
    };

    struct Fish
    {
        FishSpecies const * Species;
        TextureFrameIndex RenderFrameIndex;
        float PersonalitySeed;

        StateType CurrentState;
        vec2f CurrentPosition;
        vec2f TargetPosition;
        float CurrentProgressPhase;
        float CurrentProgress; // Calcd off CurrentProgressPhase

        Fish(
            FishSpecies const * species,
            TextureFrameIndex const & renderFrameIndex,
            float personalitySeed,
            StateType initialState,
            vec2f const & initialPosition,
            vec2f const & targetPosition)
            : Species(species)
            , RenderFrameIndex(renderFrameIndex)
            , PersonalitySeed(personalitySeed)
            , CurrentState(initialState)
            , CurrentPosition(initialPosition)
            , TargetPosition(targetPosition)
            , CurrentProgressPhase(0.0f)
            , CurrentProgress(0.0f) // Assumption: progress==0 @ phase==0
        {}
    };

    std::vector<Fish> mFishes;
};

}
