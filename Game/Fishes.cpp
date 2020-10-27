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

float constexpr TurningThreshold = 7.0f;
float constexpr SteeringWithTurnDurationSeconds = 1.5f;
float constexpr SteeringWithoutTurnDurationSeconds = 1.0f;

Fishes::Fishes(FishSpeciesDatabase const & fishSpeciesDatabase)
    : mFishSpeciesDatabase(fishSpeciesDatabase)
    , mShoalBatchSize(GetShoalBatchSize(mFishSpeciesDatabase))
    , mFishShoals()
    , mFishes()
    , mCurrentDisturbance()
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
            while (mFishShoals[currentShoalSearchIndex].CurrentMemberCount == mFishShoals[currentShoalSearchIndex].Species->ShoalSize)
            {
                ++currentShoalSearchIndex;
                if (currentShoalSearchIndex == mFishShoals.size())
                    currentShoalSearchIndex = shoalSearchStartIndex;
            }

            assert(mFishShoals[currentShoalSearchIndex].CurrentMemberCount < mFishShoals[currentShoalSearchIndex].Species->ShoalSize);

            LogMessage("TODOTEST: creating fish in shoal ", currentShoalSearchIndex);

            FishSpecies const & species = *(mFishShoals[currentShoalSearchIndex].Species);

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
                StateType::Cruising,
                initialPosition,
                targetPosition,
                CalculateVelocity(initialPosition, targetPosition, species, 1.0f, personalitySeed),
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
        assert(mFishShoals[fish.ShoalId].Species != nullptr);

        FishSpecies const & species = *(mFishShoals[fish.ShoalId].Species);

        // TODOHERE: new algo:
        // 1) Turn (& return)
        // 2) Shoal magic
        // 3) Pos update
        //      - With X sort maintenance in separate vector
        // 4) Panic check X 5

        switch (fish.CurrentState)
        {
            case StateType::Cruising:
            {
                //
                // Check whether we should start turning
                //

                if (std::abs(fish.CurrentPosition.x - fish.TargetPosition.x) < TurningThreshold)
                {
                    //
                    // Transition to Steering
                    //

                    // Choose new target position
                    fish.TargetPosition = CalculateNewCruisingTargetPosition(
                        fish.CurrentPosition,
                        -fish.CurrentDirection,
                        visibleWorld);

                    // Calculate new target velocity and direction
                    fish.StartVelocity = fish.CurrentVelocity;
                    fish.TargetVelocity = CalculateVelocity(fish.CurrentPosition, fish.TargetPosition, species, 1.0f, fish.PersonalitySeed);
                    fish.StartDirection = fish.CurrentDirection;
                    fish.TargetDirection = fish.TargetVelocity.normalise();

                    // Change state, depending on whether we're turning or not
                    if (fish.TargetDirection.x * fish.StartDirection.x <= 0.0f)
                        fish.CurrentState = StateType::CruiseSteering_WithTurn;
                    else
                        fish.CurrentState = StateType::CruiseSteering_WithoutTurn;

                    // Remember steering starting time
                    fish.SteeringSimulationTimeStart = currentSimulationTime;
                }
                else
                {
                    //
                    // Normal dynamics
                    //

                    // Update position: add velocity, with superimposed sin component
                    fish.CurrentPosition +=
                        fish.CurrentVelocity
                        + fish.CurrentVelocity.normalise() * (1.0f + std::sin(2.0f * fish.CurrentProgressPhase + Pi<float> / 2.0f)) / 120.0f;

                    // Update progress phase: add basal speed
                    fish.CurrentProgressPhase += species.BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;
                }

                break;
            }

            case StateType::CruiseSteering_WithTurn:
            {
                //
                // Check whether we should stop steering
                //

                if (currentSimulationTime - fish.SteeringSimulationTimeStart >= SteeringWithTurnDurationSeconds)
                {
                    //
                    // Transition to Cruising
                    //

                    // Change state
                    fish.CurrentState = StateType::Cruising;

                    // Reach all target quantities
                    fish.CurrentVelocity = fish.TargetVelocity;
                    fish.CurrentDirection = fish.TargetDirection;
                }
                else
                {
                    //
                    // Turning dynamics
                    //
                    // |      Velocity -> 0        |      Velocity -> Target      |
                    // |  DirY -> 0  |                          |  DirY -> Target |
                    // |        |            DirX -> Target             |         |
                    //

                    float const elapsedFraction = (currentSimulationTime - fish.SteeringSimulationTimeStart) / SteeringWithTurnDurationSeconds;

                    // Velocity:
                    // - smooth towards zero during first half
                    // - smooth towards target during second half
                    if (elapsedFraction <= 0.5f)
                    {
                        fish.CurrentVelocity =
                            fish.StartVelocity * (1.0f - SmoothStep(0.0f, 0.5f, elapsedFraction));
                    }
                    else
                    {
                        fish.CurrentVelocity =
                            fish.TargetVelocity * SmoothStep(0.5f, 1.0f, elapsedFraction);
                    }

                    // Direction Y:
                    // - smooth towards zero during an initial interval
                    // - smooth towards target during a second interval
                    if (elapsedFraction <= 0.30f)
                    {
                        fish.CurrentDirection.y =
                            fish.StartDirection.y * (1.0f - SmoothStep(0.0f, 0.30f, elapsedFraction));
                    }
                    else if (elapsedFraction >= 0.70f)
                    {
                        fish.CurrentDirection.y =
                            fish.TargetDirection.y * SmoothStep(0.70f, 1.0f, elapsedFraction);
                    }

                    // Direction X:
                    // - smooth towards target during a central interval (actual turning around),
                    //   without crossing zero
                    float constexpr TurnLimit = 0.2f;
                    if (elapsedFraction >= 0.15f && elapsedFraction <= 0.5f)
                    {
                        fish.CurrentDirection.x =
                            fish.StartDirection.x * (1.0f - (1.0f - TurnLimit) * SmoothStep(0.15f, 0.5f, elapsedFraction));
                    }
                    else if (elapsedFraction > 0.50f && elapsedFraction <= 0.85f)
                    {
                        fish.CurrentDirection.x =
                            fish.TargetDirection.x * (TurnLimit + (1.0f - TurnLimit) * SmoothStep(0.5f, 0.85f, elapsedFraction));
                    }

                    //
                    // Normal dynamics
                    //

                    // Update position: add velocity
                    fish.CurrentPosition += fish.CurrentVelocity;

                    // Update progress phase: add basal speed
                    fish.CurrentProgressPhase += species.BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;
                }

                break;
            }

            case StateType::CruiseSteering_WithoutTurn:
            {
                //
                // Check whether we should stop steering
                //

                if (currentSimulationTime - fish.SteeringSimulationTimeStart >= SteeringWithoutTurnDurationSeconds)
                {
                    //
                    // Transition to Cruising
                    //

                    // Change state
                    fish.CurrentState = StateType::Cruising;

                    // Reach all target quantities
                    fish.CurrentVelocity = fish.TargetVelocity;
                    fish.CurrentDirection = fish.TargetDirection;
                }
                else
                {
                    //
                    // Turning dynamics
                    //

                    float const elapsedFraction = (currentSimulationTime - fish.SteeringSimulationTimeStart) / SteeringWithoutTurnDurationSeconds;

                    // Velocity: smooth towards target
                    fish.CurrentVelocity =
                        fish.StartVelocity
                        + (fish.TargetVelocity - fish.StartVelocity) * SmoothStep(0.0f, 1.0f, elapsedFraction);

                    // Direction: smooth towards target
                    fish.CurrentDirection =
                        fish.StartDirection
                        + (fish.TargetDirection - fish.StartDirection) * SmoothStep(0.0f, 1.0f, elapsedFraction);

                    //
                    // Normal dynamics
                    //

                    // Update position: add velocity
                    fish.CurrentPosition += fish.CurrentVelocity;

                    // Update progress phase: add basal speed
                    fish.CurrentProgressPhase += species.BasalSpeed * BasalSpeedToProgressPhaseSpeedFactor;
                }

                break;
            }
        }

        //
        // Update current progress
        //

        fish.CurrentProgress = std::sin(fish.CurrentProgressPhase);
    }

    //
    // 3) Nuke disturbance, now that we've consumed it
    //

    mCurrentDisturbance.reset();
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
            TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, mFishShoals[fish.ShoalId].Species->RenderTextureFrameIndex),
            fish.CurrentPosition,
            angleCw,
            horizontalScale,
            mFishShoals[fish.ShoalId].Species->TailX,
            fish.CurrentProgress);
    }

    renderContext.UploadFishesEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////

void Fishes::CreateNewFishShoalBatch()
{
    for (auto const & species : mFishSpeciesDatabase.GetFishSpecies())
    {
        mFishShoals.emplace_back(&species);
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
    return FindPosition(
        currentPosition + newDirection * visibleWorld.Width,
        visibleWorld.Width / 4.0f, // x variance
        5.0f); // y variance
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