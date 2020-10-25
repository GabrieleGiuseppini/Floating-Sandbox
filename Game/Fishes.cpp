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

float constexpr TurningRadius = 7.0f;

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

            vec2f const initialPosition = ChooseTargetPosition(species, visibleWorld);
            vec2f const targetPosition = CalculateNewCruisingTargetPosition(initialPosition, species, visibleWorld);

            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            mFishes.emplace_back(
                &species,
                static_cast<TextureFrameIndex>(speciesIndex),
                personalitySeed,
                StateType::Cruising,
                initialPosition,
                targetPosition,
                CalculateVelocity(initialPosition, targetPosition, species, 1.0f, personalitySeed),
                GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>)); // initial progress phase
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
                //
                // Check whether we should start turning
                //

                if (std::abs(fish.CurrentPosition.x - fish.TargetPosition.x) < TurningRadius)
                {
                    //
                    // Transition to Turning
                    //

                    // Change state
                    // TODOTEST
                    //fish.CurrentState = StateType::Turning;

                    // Choose new target position
                    fish.TargetPosition = CalculateNewCruisingTargetPosition(fish.CurrentPosition, *(fish.Species), visibleWorld);

                    // Calculate new target velocity
                    fish.TargetVelocity = CalculateVelocity(fish.CurrentPosition, fish.TargetPosition, *(fish.Species), 1.0f, fish.PersonalitySeed);
                }
                else
                {
                    // TODOTEST
                    // Update velocity: smooth it towards target
                    fish.CurrentVelocity +=
                        (fish.TargetVelocity - fish.CurrentVelocity)
                        * 0.014f; // Converge rate

                    // Update position: add velocity, with superimposed sin component
                    fish.CurrentPosition +=
                        fish.CurrentVelocity
                        + fish.CurrentVelocity.normalise() * (1.0f + std::sin(2.0f * fish.CurrentProgressPhase + Pi<float> / 2.0f)) / 100.0f;

                    // Update progress phase: add basal speed
                    fish.CurrentProgressPhase += fish.Species->BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;
                }

                break;
            }

            case StateType::Turning:
            {
                // Check whether we should start cruising again
                // TODOTEST
                // if (std::abs(fish.CurrentPosition.x - fish.TargetPosition.x) > TurningRadius)
                if ((fish.TargetVelocity - fish.CurrentVelocity).length() < .1f)
                {
                    //
                    // Transition to Cruising
                    //

                    // Change state
                    fish.CurrentState = StateType::Cruising;
                }
                else
                {
                    // Update velocity: smooth it towards target
                    fish.CurrentVelocity +=
                        (fish.TargetVelocity - fish.CurrentVelocity)
                        // TODOTEST
                        //* 0.02f; // Converge rate
                        * 0.2f;

                    // TODO: avoid this repetition
                    // Update position: add velocity, with superimposed sin component
                    fish.CurrentPosition +=
                        fish.CurrentVelocity
                        + fish.CurrentVelocity.normalise() * (1.0f + std::sin(2.0f * fish.CurrentProgressPhase + Pi<float> / 2.0f)) / 100.0f;

                    // Update progress phase: add basal speed
                    fish.CurrentProgressPhase += fish.Species->BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;
                }

                break;
            }

            case StateType::Fleeing:
            {
                // TODO
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
        // Horizontal scale: absolute magnitude is
        // the fraction of the X component of the current velocity
        // over the X component of the target velocity
        assert(fish.TargetVelocity.x != 0.0f);
        // TODOHERE: don't exceed one
        float horizontalScale =
            std::min(1.0f, std::abs(fish.CurrentVelocity.x / fish.TargetVelocity.x));

        // TODO: some comments here
        float angleCw = fish.CurrentVelocity.angleCw();

        if (angleCw < -Pi<float> / 2.0f)
        {
            angleCw = -Pi<float> - angleCw;
            horizontalScale *= -1.0f;
            angleCw *= -1.0f;
        }
        else if (angleCw > Pi<float> / 2.0f)
        {
            angleCw = Pi<float> - angleCw;
            horizontalScale *= -1.0f;
            angleCw *= -1.0f;
        }

        renderContext.UploadFish(
            TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, fish.RenderFrameIndex),
            fish.CurrentPosition,
            angleCw,
            horizontalScale,
            fish.Species->TailX,
            fish.CurrentProgress);
    }

    renderContext.UploadFishesEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////

vec2f Fishes::ChooseTargetPosition(
    FishSpecies const & fishSpecies,
    VisibleWorld const & visibleWorld)
{
    //static vec2f todo = vec2f(0.0f, 0.0f);
    static vec2f todo = vec2f(-20.0f, -20.0f);

    if (todo == vec2f(0.0f, 0.0f))
    {
        todo = vec2f(-20.0f, -20.0f);
    }
    else if (todo == vec2f(-20.0f, -20.0f))
    {
        todo = vec2f(0.0f, -40.0f);
    }
    else if (todo == vec2f(0.0f, -40.0f))
    {
        todo = vec2f(20.0f, -20.0f);
    }
    else if (todo == vec2f(20.0f, -20.0f))
    {
        todo = vec2f(0.0f, 0.0f);
    }

    return todo;
    // TODOTEST
    /*
    float const x = GameRandomEngine::GetInstance().GenerateNormalReal(visibleWorld.Center.x, visibleWorld.Width);

    float const y =
        -5.0f // Min depth
        - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(fishSpecies.BasalDepth, 15.0f));

    return vec2f(x, y);
    */
}

vec2f Fishes::CalculateNewCruisingTargetPosition(
    vec2f const & currentPosition,
    FishSpecies const & species,
    VisibleWorld const & visibleWorld)
{
    // TODOTEST
    return ChooseTargetPosition(species, visibleWorld);

    // TODO: opposite side of screen, at least at minimum distance

    float const x = GameRandomEngine::GetInstance().GenerateNormalReal(visibleWorld.Center.x, visibleWorld.Width);

    float const y =
        -5.0f // Min depth
        - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(species.BasalDepth, 15.0f));

    return vec2f(x, y);
}

vec2f Fishes::CalculateVelocity(
    vec2f const & startPosition,
    vec2f const & endPosition,
    FishSpecies const & species,
    float velocityMultiplier,
    float personalitySeed)
{
    return (endPosition - startPosition).normalise()
        * (species.BasalSpeed * velocityMultiplier * (0.7f + personalitySeed * 0.3f));
}

}