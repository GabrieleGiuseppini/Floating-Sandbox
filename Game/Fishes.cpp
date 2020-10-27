/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>
#include <GameCore/Utils.h>

#include <picojson.h>

#include <algorithm>
#include <chrono>

namespace Physics {

namespace /*anonymous*/ {

    size_t GetShoalBatchSize(FishSpeciesDatabase const & fishSpeciesDatabase)
    {
        return std::accumulate(
            fishSpeciesDatabase.GetFishSpecies().cbegin(),
            fishSpeciesDatabase.GetFishSpecies().cend(),
            size_t(0),
            [](size_t const & total, auto const & speciesIt)
            {
                return total + speciesIt.ShoalSize;
            });
    }
}

Fishes::Fishes(FishSpeciesDatabase const & fishSpeciesDatabase)
    : mFishSpeciesDatabase(fishSpeciesDatabase)
    , mShoalBatchSize(GetShoalBatchSize(mFishSpeciesDatabase))
    , mFishShoals()
    , mFishes()
    , mCurrentInteractiveDisturbance()
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
        //
        // Remove extra fishes
        //

        for (auto fishIt = mFishes.cbegin() + gameParameters.NumberOfFishes; fishIt != mFishes.cend(); ++fishIt)
        {
            assert(mFishShoals[fishIt->ShoalId].CurrentMemberCount > 0);
            --mFishShoals[fishIt->ShoalId].CurrentMemberCount;
        }

        mFishes.erase(
            mFishes.begin() + gameParameters.NumberOfFishes,
            mFishes.end());
    }
    else
    {
        //
        // Add new fishes
        //

        // The index in the shoals at which we start searching for free shoals; this
        // points to the beginning of the latest shoal batch
        size_t shoalSearchStartIndex = (mFishShoals.size() / mFishSpeciesDatabase.GetFishSpeciesCount()) * mFishSpeciesDatabase.GetFishSpeciesCount();
        size_t currentShoalSearchIndex = shoalSearchStartIndex;

        for (size_t f = mFishes.size(); f < gameParameters.NumberOfFishes; ++f)
        {
            //
            // 1) Find the shoal for this new fish
            //

            // Make sure there are indeed free shoals available
            if ((mFishes.size() % mShoalBatchSize) == 0)
            {
                size_t const oldShoalCount = mFishShoals.size();

                // Create new batch
                CreateNewFishShoalBatch();

                // Start searching from here
                shoalSearchStartIndex = oldShoalCount;
                currentShoalSearchIndex = shoalSearchStartIndex;
            }

            // Search for the next free shoal
            assert(currentShoalSearchIndex < mFishShoals.size());
            while (mFishShoals[currentShoalSearchIndex].CurrentMemberCount == mFishShoals[currentShoalSearchIndex].Species.ShoalSize)
            {
                ++currentShoalSearchIndex;
                if (currentShoalSearchIndex == mFishShoals.size())
                    currentShoalSearchIndex = shoalSearchStartIndex;
            }

            assert(mFishShoals[currentShoalSearchIndex].CurrentMemberCount < mFishShoals[currentShoalSearchIndex].Species.ShoalSize);

            FishSpecies const & species = mFishShoals[currentShoalSearchIndex].Species;

            // Initialize shoal, if needed
            if (mFishShoals[currentShoalSearchIndex].CurrentMemberCount == 0)
            {
                //
                // Decide an initial direction
                //

                if (currentShoalSearchIndex > 0)
                    mFishShoals[currentShoalSearchIndex].InitialDirection = -mFishShoals[currentShoalSearchIndex - 1].InitialDirection;
                else
                    mFishShoals[currentShoalSearchIndex].InitialDirection = vec2f(
                        GameRandomEngine::GetInstance().Choose(2) == 1 ? -1.0f : 1.0f, // Random left or right
                        0.0f);

                //
                // Decide an initial position
                //

                float const initialX = std::abs(GameRandomEngine::GetInstance().GenerateNormalReal(visibleWorld.Center.x, visibleWorld.Width / 2.5f));

                float const initialY =
                    -5.0f // Min depth
                    - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(species.BasalDepth, 15.0f));

                mFishShoals[currentShoalSearchIndex].InitialPosition = vec2f(
                    mFishShoals[currentShoalSearchIndex].InitialDirection.x < 0.0f ? initialX : -initialX,
                    initialY);

            }

            //
            // 2) Create fish in this shoal
            //

            vec2f const initialPosition = FindPosition(
                mFishShoals[currentShoalSearchIndex].InitialPosition,
                10.0f,
                4.0f);

            vec2f const targetPosition = CalculateNewCruisingTargetPosition(
                initialPosition,
                mFishShoals[currentShoalSearchIndex].InitialDirection,
                visibleWorld);

            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            mFishes.emplace_back(
                currentShoalSearchIndex,
                personalitySeed,
                initialPosition,
                targetPosition,
                CalculateVelocity((targetPosition - initialPosition).normalise(), species, 1.0f, personalitySeed),
                GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>)); // initial progress phase

            // Update shoal
            ++(mFishShoals[currentShoalSearchIndex].CurrentMemberCount);
        }
    }

    //
    // 2) Update fishes
    //

    float constexpr BasalSpeedToProgressPhaseSpeedFactor =
        40.0f // Magic, from observation
        * GameParameters::SimulationStepTimeDuration<float>;

    for (auto & fish : mFishes)
    {
        FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

        //
        // 1) If we're turning, continue turning
        //

        if (fish.CurrentSteeringState.has_value())
        {
            float const elapsedSteeringDurationFraction = (currentSimulationTime - fish.SteeringSimulationTimeStart) / fish.SteeringSimulationTimeDuration;

            // Check whether we should stop steering
            if (elapsedSteeringDurationFraction >= 1.0f)
            {
                // Stop steering

                // Change state
                fish.CurrentSteeringState.reset();

                // Reach all target quantities
                fish.CurrentVelocity = fish.TargetVelocity;
                fish.CurrentDirection = fish.TargetDirection;
            }
            else
            {
                switch (*fish.CurrentSteeringState)
                {
                    case SteeringType::CruiseWithoutTurn:
                    {
                        // Velocity: smooth towards target
                        fish.CurrentVelocity =
                            fish.StartVelocity
                            + (fish.TargetVelocity - fish.StartVelocity) * SmoothStep(0.0f, 1.0f, elapsedSteeringDurationFraction);

                        // Direction: smooth towards target
                        fish.CurrentDirection =
                            fish.StartDirection
                            + (fish.TargetDirection - fish.StartDirection) * SmoothStep(0.0f, 1.0f, elapsedSteeringDurationFraction);

                        break;
                    }

                    case SteeringType::CruiseWithTurn:
                    {
                        //
                        // |      Velocity -> 0        |      Velocity -> Target      |
                        // |  DirY -> 0  |                          |  DirY -> Target |
                        // |        |            DirX -> Target             |         |
                        //

                        // Velocity:
                        // - smooth towards zero during first half
                        // - smooth towards target during second half
                        if (elapsedSteeringDurationFraction <= 0.5f)
                        {
                            fish.CurrentVelocity =
                                fish.StartVelocity * (1.0f - SmoothStep(0.0f, 0.5f, elapsedSteeringDurationFraction));
                        }
                        else
                        {
                            fish.CurrentVelocity =
                                fish.TargetVelocity * SmoothStep(0.5f, 1.0f, elapsedSteeringDurationFraction);
                        }

                        // Direction Y:
                        // - smooth towards zero during an initial interval
                        // - smooth towards target during a second interval
                        if (elapsedSteeringDurationFraction <= 0.30f)
                        {
                            fish.CurrentDirection.y =
                                fish.StartDirection.y * (1.0f - SmoothStep(0.0f, 0.30f, elapsedSteeringDurationFraction));
                        }
                        else if (elapsedSteeringDurationFraction >= 0.70f)
                        {
                            fish.CurrentDirection.y =
                                fish.TargetDirection.y * SmoothStep(0.70f, 1.0f, elapsedSteeringDurationFraction);
                        }

                        // Direction X:
                        // - smooth towards target during a central interval (actual turning around),
                        //   without crossing zero
                        float constexpr TurnLimit = 0.2f;
                        if (elapsedSteeringDurationFraction >= 0.15f && elapsedSteeringDurationFraction <= 0.5f)
                        {
                            fish.CurrentDirection.x =
                                fish.StartDirection.x * (1.0f - (1.0f - TurnLimit) * SmoothStep(0.15f, 0.5f, elapsedSteeringDurationFraction));
                        }
                        else if (elapsedSteeringDurationFraction > 0.50f && elapsedSteeringDurationFraction <= 0.85f)
                        {
                            fish.CurrentDirection.x =
                                fish.TargetDirection.x * (TurnLimit + (1.0f - TurnLimit) * SmoothStep(0.5f, 0.85f, elapsedSteeringDurationFraction));
                        }

                        break;
                    }
                }
            }
        }

        //
        // 2) Do shoal magic
        //

        // TODO

        //
        // 3) Update dynamics
        //

        // Update position
        {
            float const speedMultiplier = fish.PanicCharge * 8.5f + 1.0f;

            // Update position: add velocity
            fish.CurrentPosition += fish.CurrentVelocity * speedMultiplier;

            // Update progress phase: add basal speed
            fish.CurrentProgressPhase += species.BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor * speedMultiplier;

            // ...superimpose sin component unless we're steering
            if (!fish.CurrentSteeringState.has_value())
                fish.CurrentPosition += fish.CurrentVelocity.normalise() * (1.0f + std::sin(2.0f * fish.CurrentProgressPhase + Pi<float> / 2.0f)) / 200.0f;

            // Update current progress
            fish.CurrentProgress = std::sin(fish.CurrentProgressPhase);

            // TODO: do X sort maintenance in separate vector
        }

        // Update panic charge
        fish.PanicCharge *= (1.0f - 0.005f);

        //
        // 4) Disturbance check
        //

        // TODO: x5:
        // - Water level
        // - Ocean floor
        // - AABB
        // - Current disturbance
        // - Reached target

        // Check whether the fish has been interactively disturbed
        if (mCurrentInteractiveDisturbance.has_value()
            && (fish.CurrentPosition - *mCurrentInteractiveDisturbance).length() < 7.5f) // Within radius
        {
            //
            // Enter panic mode
            //

            fish.PanicCharge = 1.0f;

            // New target position: irrelevant, as we're entering panic mode and we'll
            // only exit when panic charge is exhausted
            fish.TargetPosition = vec2f(0.0f, 0.0f);

            // Calculate new target velocity and direction - away from disturbance point, and will be panic velocity
            fish.StartVelocity = fish.CurrentVelocity;
            fish.TargetVelocity = CalculateVelocity((fish.CurrentPosition - *mCurrentInteractiveDisturbance).normalise(), species, 1.0f, fish.PersonalitySeed);
            fish.StartDirection = fish.CurrentDirection;
            fish.TargetDirection = fish.TargetVelocity.normalise();

            // Start steering
            fish.SteeringSimulationTimeStart = currentSimulationTime;
            fish.SteeringSimulationTimeDuration = 0.05f;
            fish.CurrentSteeringState = SteeringType::CruiseWithoutTurn;
        }
        // Check whether this fish has reached its target
        else if (
            (fish.PanicCharge != 0.0f && fish.PanicCharge < 0.02f) // Reached end of panic
            || (fish.PanicCharge == 0.0f &&  std::abs(fish.CurrentPosition.x - fish.TargetPosition.x) < 7.0f)) // Reached target when not in panic
        {
            //
            // Transition to Steering
            //

            fish.PanicCharge = 0.0f;

            // Choose new target position
            fish.TargetPosition = CalculateNewCruisingTargetPosition(
                fish.CurrentPosition,
                -fish.CurrentDirection,
                visibleWorld);

            // Calculate new target velocity and direction
            fish.StartVelocity = fish.CurrentVelocity;
            fish.TargetVelocity = CalculateVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), species, 1.0f, fish.PersonalitySeed);
            fish.StartDirection = fish.CurrentDirection;
            fish.TargetDirection = fish.TargetVelocity.normalise();

            // Change state, depending on whether we're turning or not
            fish.SteeringSimulationTimeStart = currentSimulationTime;
            if (fish.TargetDirection.x * fish.StartDirection.x <= 0.0f)
            {
                fish.SteeringSimulationTimeDuration = 1.5f;
                fish.CurrentSteeringState = SteeringType::CruiseWithTurn;
            }
            else
            {
                fish.SteeringSimulationTimeDuration = 1.0f;
                fish.CurrentSteeringState = SteeringType::CruiseWithoutTurn;
            }
        }
    }

    //
    // 3) Nuke disturbance, now that we've consumed it
    //

    mCurrentInteractiveDisturbance.reset();
}

void Fishes::Upload(Render::RenderContext & renderContext) const
{
    renderContext.UploadFishesStart(mFishes.size());

    for (auto const & fish : mFishes)
    {
        float angleCw = fish.CurrentDirection.angleCw();
        float horizontalScale = fish.CurrentDirection.length();

        if (angleCw < -Pi<float> / 2.0f)
        {
            angleCw = Pi<float> + angleCw;
            //angleCw *= -1.0f;
            horizontalScale *= -1.0f;
        }
        else if (angleCw > Pi<float> / 2.0f)
        {
            angleCw = -Pi<float> + angleCw;
            //angleCw *= -1.0f;
            horizontalScale *= -1.0f;
        }

        renderContext.UploadFish(
            TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, mFishShoals[fish.ShoalId].Species.RenderTextureFrameIndex),
            fish.CurrentPosition,
            angleCw,
            horizontalScale,
            mFishShoals[fish.ShoalId].Species.TailX,
            fish.CurrentProgress);
    }

    renderContext.UploadFishesEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////

void Fishes::CreateNewFishShoalBatch()
{
    for (auto const & species : mFishSpeciesDatabase.GetFishSpecies())
    {
        mFishShoals.emplace_back(species);
    }
}

vec2f Fishes::FindPosition(
    vec2f const & averagePosition,
    float xVariance,
    float yVariance)
{
    // Try a few times around the average position, making
    // sure we don't hit obstacles

    vec2f position;

    for (int attempt = 0; attempt < 10; ++attempt)
    {
        position.x = GameRandomEngine::GetInstance().GenerateNormalReal(averagePosition.x, xVariance);

        position.y =
            -5.0f // Min depth
            - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(averagePosition.y, yVariance));

        // TODO: obstacle check
        break;
    }

    return position;
}

vec2f Fishes::CalculateNewCruisingTargetPosition(
    vec2f const & currentPosition,
    vec2f const & newDirection,
    VisibleWorld const & visibleWorld)
{
    // Maximize the presence of fish in the visible world:
    // if the direction is sending the fish away from the center,
    // move a little; if the direction is sending the fish towards
    // the center, move more
    float const movementMagnitude = (visibleWorld.Center.x - currentPosition.x) * newDirection.x < 0.0f
        ? visibleWorld.Width / 6.0f
        : visibleWorld.Width;

    return FindPosition(
        currentPosition + newDirection * movementMagnitude,
        visibleWorld.Width / 4.0f, // x variance
        5.0f); // y variance
}

vec2f Fishes::CalculateVelocity(
    vec2f const & direction,
    FishSpecies const & species,
    float velocityMultiplier,
    float personalitySeed)
{
    return direction
        * (species.BasalSpeed * velocityMultiplier * (0.7f + personalitySeed * 0.3f));
}

}