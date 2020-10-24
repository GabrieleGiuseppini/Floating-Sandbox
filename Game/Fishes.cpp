/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>
#include <GameCore/Utils.h>

#include <picojson.h>

namespace Physics {

Fishes::Fishes(FishSpeciesDatabase const & fishSpeciesDatabase)
    : mFishSpeciesDatabase(fishSpeciesDatabase)
    , mFishes()
{
}

void Fishes::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld)
{
    //
    // 1) Update number of fish
    //

    if (mFishes.size() > gameParameters.NumberOfFishes)
    {
        // Remove extra fish
        mFishes.erase(
            mFishes.begin() + gameParameters.NumberOfFishes,
            mFishes.end());
    }
    else
    {
        // Add missing fish
        for (size_t f = mFishes.size(); f < gameParameters.NumberOfFishes; ++f)
        {
            // Choose species
            size_t const speciesIndex = GameRandomEngine::GetInstance().Choose(mFishSpeciesDatabase.GetFishSpecies().size());
            FishSpecies const & species = mFishSpeciesDatabase.GetFishSpecies()[speciesIndex];

            //
            // Choose initial and target position
            //

            vec2f const initialPosition = ChooseTargetPosition(species, visibleWorld, species.BasalDepth);
            vec2f const targetPosition = ChooseTargetPosition(species, visibleWorld, initialPosition.y);

            mFishes.emplace_back(
                &species,
                static_cast<TextureFrameIndex>(speciesIndex),
                GameRandomEngine::GetInstance().GenerateNormalizedUniformReal(),
                StateType::Cruising,
                initialPosition,
                targetPosition,
                GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>));
        }
    }

    //
    // 2) Update fish
    //

    float constexpr BasalSpeedToProgressPhaseSpeedFactor =
        40.0f // Magic, from observation
        * GameParameters::SimulationStepTimeDuration<float>;

    for (auto & fish : mFishes)
    {
        switch (fish.CurrentState)
        {
            case StateType::Cruising:
            {
                // Check whether we should start fleeing
                // TODO

                // Check whether we should choose a new target position
                if ((fish.CurrentPosition - fish.TargetPosition).length() < 1.0f)
                    fish.TargetPosition = ChooseTargetPosition(*fish.Species, visibleWorld, fish.CurrentPosition.y);

                // Update progress phase: basal speed
                fish.CurrentProgressPhase += fish.Species->BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;

                // Update position: basal speed along current->target direction
                fish.CurrentPosition +=
                    (fish.TargetPosition - fish.CurrentPosition).normalise()
                    * (fish.Species->BasalSpeed * (0.7f + fish.PersonalitySeed * 0.3f) + (1.0f + std::sin(2.0f * fish.CurrentProgressPhase + Pi<float>)) / 80.0f);


                break;
            }

            case StateType::Fleeing:
            {
                // Check whether we should start cruising again
                if ((fish.CurrentPosition - fish.TargetPosition).length() < 1.0f)
                {
                    fish.CurrentState = StateType::Cruising;
                    fish.TargetPosition = ChooseTargetPosition(*fish.Species, visibleWorld, fish.CurrentPosition.y);
                }
                else
                {
                    // Update progress phase: fleeing speed
                    fish.CurrentProgressPhase += 4.0f * fish.Species->BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;

                    // Update position: fleeing speed along current->target direction
                    fish.CurrentPosition +=
                        (fish.TargetPosition - fish.CurrentPosition).normalise()
                        * 4.0f * fish.Species->BasalSpeed;


                }

                break;
            }
        }

        //
        // Update current progress
        //

        fish.CurrentProgress = std::sin(fish.CurrentProgressPhase);
    }
}

void Fishes::Upload(Render::RenderContext & renderContext) const
{
    renderContext.UploadFishesStart(mFishes.size());

    for (auto const & fish : mFishes)
    {
        renderContext.UploadFish(
            TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, fish.RenderFrameIndex),
            fish.CurrentPosition,
            (fish.TargetPosition - fish.CurrentPosition).angleCw(),
            fish.TargetPosition.x < fish.CurrentPosition.x ? -1.0f : 1.0f,
            fish.Species->TailX,
            fish.CurrentProgress);
    }

    renderContext.UploadFishesEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////

vec2f Fishes::ChooseTargetPosition(
    FishSpecies const & fishSpecies,
    VisibleWorld const & visibleWorld,
    float currentY) const
{
    float const x = GameRandomEngine::GetInstance().GenerateNormalReal(visibleWorld.Center.x, visibleWorld.Width);

    float const y =
        -5.0f // Min depth
        - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(currentY, 15.0f));

    return vec2f(x, y);
}

}