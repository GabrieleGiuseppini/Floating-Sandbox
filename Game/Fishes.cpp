/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameMath.h>
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
    , mCurrentInteractiveAttraction()
    , mCurrentFishSizeAdjustment(0.0f)
{
}

void Fishes::Update(
    float currentSimulationTime,
    OceanSurface & oceanSurface,
    OceanFloor const & oceanFloor,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld)
{
    //
    // 1) Update parameters that changed, if any
    //

    if (gameParameters.FishSizeAdjustment != mCurrentFishSizeAdjustment)
    {
        mCurrentFishSizeAdjustment = gameParameters.FishSizeAdjustment;
    }

    //
    // 2) Update number of fishes
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
                    - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(species.OceanDepth, 15.0f));

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

            vec2f const targetPosition = FindNewCruisingTargetPosition(
                initialPosition,
                mFishShoals[currentShoalSearchIndex].InitialDirection,
                visibleWorld);

            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            mFishes.emplace_back(
                currentShoalSearchIndex,
                personalitySeed,
                initialPosition,
                targetPosition,
                MakeBasalVelocity((targetPosition - initialPosition).normalise(), species, personalitySeed),
                GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>)); // initial progress phase

            // Update shoal
            ++(mFishShoals[currentShoalSearchIndex].CurrentMemberCount);
        }
    }

    //
    // 3) Update fishes
    //

    for (auto & fish : mFishes)
    {
        FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

        //
        // 1) Steer or auto-smooth direction
        //

        if (fish.CruiseSteeringState.has_value())
        {
            //
            // Cruise steering
            //

            //LogMessage("TODOHERE: 1: CruiseSteering");

            float const elapsedSteeringDurationFraction = (currentSimulationTime - fish.CruiseSteeringState->SimulationTimeStart) / fish.CruiseSteeringState->SimulationTimeDuration;

            // Check whether we should stop steering
            if (elapsedSteeringDurationFraction >= 1.0f)
            {
                // Stop steering

                // Change state
                fish.CruiseSteeringState.reset();

                // Reach all targets
                fish.CurrentVelocity = fish.TargetVelocity;
                fish.CurrentRenderVector = fish.TargetRenderVector;
            }
            else
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
                        fish.CruiseSteeringState->StartVelocity * (1.0f - SmoothStep(0.0f, 0.5f, elapsedSteeringDurationFraction));
                }
                else
                {
                    fish.CurrentVelocity =
                        fish.TargetVelocity * SmoothStep(0.5f, 1.0f, elapsedSteeringDurationFraction);
                }

                // RenderVector Y:
                // - smooth towards zero during an initial interval
                // - smooth towards target during a second interval
                if (elapsedSteeringDurationFraction <= 0.40f)
                {
                    fish.CurrentRenderVector.y =
                        fish.CruiseSteeringState->StartRenderVector.y * (1.0f - SmoothStep(0.0f, 0.40f, elapsedSteeringDurationFraction));
                }
                else if (elapsedSteeringDurationFraction >= 0.60f)
                {
                    fish.CurrentRenderVector.y =
                        fish.TargetRenderVector.y * SmoothStep(0.60f, 1.0f, elapsedSteeringDurationFraction);
                }

                // RenderVector X:
                // - smooth towards target during a central interval (actual turning around),
                //   without crossing zero
                float constexpr TurnLimit = 0.2f;
                if (elapsedSteeringDurationFraction >= 0.15f && elapsedSteeringDurationFraction <= 0.5f)
                {
                    fish.CurrentRenderVector.x =
                        fish.CruiseSteeringState->StartRenderVector.x * (1.0f - (1.0f - TurnLimit) * SmoothStep(0.15f, 0.5f, elapsedSteeringDurationFraction));
                }
                else if (elapsedSteeringDurationFraction > 0.50f && elapsedSteeringDurationFraction <= 0.85f)
                {
                    fish.CurrentRenderVector.x =
                        fish.TargetRenderVector.x * (TurnLimit + (1.0f - TurnLimit) * SmoothStep(0.5f, 0.85f, elapsedSteeringDurationFraction));
                }
            }
        }
        else
        {
            //
            // Automated direction smoothing
            //

            //LogMessage("TODOHERE: 1: AutoSmoothing");

            // Smooth velocity towards target
            fish.CurrentVelocity +=
                (fish.TargetVelocity - fish.CurrentVelocity) * fish.CurrentDirectionSmoothingConvergenceRate;

            // Smooth render vector towards target
            fish.CurrentRenderVector +=
                (fish.TargetRenderVector - fish.CurrentRenderVector) * fish.CurrentDirectionSmoothingConvergenceRate;
        }

        //
        // 2) Do shoal magic
        //

        // TODO

        //
        // 3) Update dynamics
        //

        float constexpr OceanSurfaceDisturbance = 1.0f; // Magic number

        // Get water surface level at this fish
        float const oceanY = oceanSurface.GetHeightAt(fish.CurrentPosition.x);

        // Run freefall state machine
        if (!fish.IsInFreefall
            && fish.CurrentPosition.y > oceanY + 4.0f) // Higher watermark, so that jump is more pronounced
        {
            // Enter freefall
            fish.IsInFreefall = true;

            // Create a little disturbance in the ocean surface
            oceanSurface.DisplaceAt(fish.CurrentPosition.x, OceanSurfaceDisturbance);
        }
        else if (fish.IsInFreefall
            && fish.CurrentPosition.y <= oceanY)
        {
            // Leave freefall
            fish.IsInFreefall = false;

            LogMessage("TODOHERE: 3: LeavingFreefall");

            // Drag velocity down
            float const currentVelocityMagnitude = fish.CurrentVelocity.length();
            float constexpr MaxVelocityMagnitude = 0.5f;
            fish.TargetVelocity =
                fish.CurrentVelocity.normalise(currentVelocityMagnitude)
                * MaxVelocityMagnitude * SmoothStep(0.0f, MaxVelocityMagnitude, currentVelocityMagnitude);
            fish.CurrentDirectionSmoothingConvergenceRate = 0.15f; // Converge to dragged velocity at this rate

            // Note: no need to change render vector, velocity direction has not changed

            // Enter "a bit of" panic mode - after exhausting this panic charge,
            // the fish will resume swimming towards it current target
            // position
            fish.PanicCharge = 0.03f;

            // Create a little disturbance in the ocean surface
            oceanSurface.DisplaceAt(fish.CurrentPosition.x, OceanSurfaceDisturbance);
        }

        // Dynamics update
        if (!fish.IsInFreefall)
        {
            //
            // Swimming
            //

            //LogMessage("TODOHERE: 3: Swimming");

            float const speedMultiplier = fish.PanicCharge * 8.5f + 1.0f;

            // Update position: add current velocity
            fish.CurrentPosition += fish.CurrentVelocity * speedMultiplier;

            // Update tail progress phase: add basal speed
            fish.CurrentTailProgressPhase += species.TailSpeed * speedMultiplier;

            // Update position: superimpose a small sin component, unless we're steering
            if (!fish.CruiseSteeringState.has_value())
                fish.CurrentPosition += fish.CurrentVelocity.normalise() * (1.0f + std::sin(2.0f * fish.CurrentTailProgressPhase + Pi<float> / 2.0f)) / 200.0f;
        }
        else
        {
            //
            // Free-falling
            //

            //LogMessage("TODOHERE: 3: Free-falling");

            // Update velocity with gravity, amplified for better scenics
            float const newVelocityY = fish.CurrentVelocity.y
                - 10.0f // Amplification factor
                * GameParameters::GravityMagnitude
                * GameParameters::SimulationStepTimeDuration<float> * GameParameters::SimulationStepTimeDuration<float>;
            fish.TargetVelocity = vec2f(
                fish.CurrentVelocity.x,
                newVelocityY);
            fish.CurrentVelocity = fish.TargetVelocity; // Converge immediately

            // Update render vector to match velocity
            fish.TargetRenderVector = fish.TargetVelocity.normalise();
            fish.CurrentDirectionSmoothingConvergenceRate = 0.06f; // Converge at this rate

            // Update position: add velocity
            fish.CurrentPosition += fish.CurrentVelocity;

            // Update tail progress phase: add extra speed (fish flapping its tail)
            fish.CurrentTailProgressPhase += species.TailSpeed * 20.0f;

            // Cut short state machine now, this fish can't swim
            continue;
        }

        // Decay panic charge
        fish.PanicCharge *= 0.985f;

        //
        // 4) Disturbances check
        //

        // Calculate position of head
        vec2f const fishHeadPosition =
            fish.CurrentPosition
            + fish.CurrentRenderVector.normalise() * species.WorldSize.x * mCurrentFishSizeAdjustment * (species.HeadOffsetX - 0.5f);

        // TODO: x6:
        // + Current disturbance
        // - Current attraction
        // - AABB
        // + Water level
        // - Ocean floor
        // + Reached target

        // Check whether the fish has been interactively disturbed
        if (float const distance = (fishHeadPosition - mCurrentInteractiveDisturbance.value_or(vec2f::zero())).length();
            mCurrentInteractiveDisturbance.has_value()
            && distance < 7.5f) // Within radius
        {
            //LogMessage("TODOHERE: 4: InteractiveDisturbancePanic");

            //
            // Interactive disturbance, enter panic mode
            //

            fish.PanicCharge = 1.0f;

            // Don't change target position, we'll return to it when panic is over

            // Calculate new direction, away from disturbance
            vec2f panicDirection = (fishHeadPosition - *mCurrentInteractiveDisturbance).normalise(distance);

            LogMessage("TODOHERE: Fish @ ", fish.CurrentPosition.toString(), " Head @ ", fishHeadPosition.toString(), " Dist @ ", mCurrentInteractiveDisturbance->toString(), " Dir=", panicDirection.toString(), " Radius=", distance);

            // Make sure direction is not too steep
            float constexpr MinXComponent = 0.4f;
            if (panicDirection.x >= 0.0f && panicDirection.x < MinXComponent)
            {
                panicDirection.x = MinXComponent;
                panicDirection = panicDirection.normalise();
            }
            else if (panicDirection.x < 0.0f && panicDirection.x > -MinXComponent)
            {
                panicDirection.x = -MinXComponent;
                panicDirection = panicDirection.normalise();
            }

            // Calculate new target velocity - away from disturbance point, and will be panic velocity
            fish.TargetVelocity = MakeBasalVelocity(panicDirection, species, fish.PersonalitySeed);

            // Update render vector to match velocity
            fish.TargetRenderVector = fish.TargetVelocity.normalise();

            // Converge directions really fast
            fish.CurrentDirectionSmoothingConvergenceRate = 0.5f;
        }
        // Check whether we're too close to the water surface (idealized as being horizontal) - but only if fish is not in too much panic
        else if (float const depth = oceanY - fish.CurrentPosition.y;
            depth < 5.0f
            && fish.PanicCharge <= 0.7f)
        {
            // Bounce away only if we're really going into it
            if (fish.TargetVelocity.y >= 0.0f)
            {
                LogMessage("TODOHERE: 4: OceanSurface - Bounce");

                // Bounce direction, opposite of target
                vec2f const bounceDirection = vec2f(fish.TargetVelocity.x, -fish.TargetVelocity.y).normalise();

                // Calculate new target velocity - away from disturbance point
                fish.TargetVelocity = MakeBasalVelocity(bounceDirection, species, fish.PersonalitySeed);

                // Update render vector to match velocity
                fish.TargetRenderVector = fish.TargetVelocity.normalise();

                // Converge direction change at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = 0.05f * (1.0f + fish.PanicCharge);
            }
        }
        // Check whether this fish has reached its target, while not in panic mode
        else if (fish.PanicCharge == 0.0f &&  std::abs(fish.CurrentPosition.x - fish.TargetPosition.x) < 7.0f) // Reached target when not in panic
        {
            //LogMessage("TODOHERE: 4: TargetReached");

            //
            // Transition to Steering
            //

            // Choose new target position
            fish.TargetPosition = FindNewCruisingTargetPosition(
                fish.CurrentPosition,
                -fish.CurrentVelocity.normalise(),
                visibleWorld);

            // Calculate new target velocity
            fish.TargetVelocity = MakeBasalVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), species, fish.PersonalitySeed);

            // Update render vector to match velocity
            fish.TargetRenderVector = fish.TargetVelocity.normalise();

            // Setup steering, depending on whether we're turning or not
            if (fish.TargetRenderVector.x * fish.CurrentRenderVector.x <= 0.0f)
            {
                // Perform a cruise steering
                fish.CruiseSteeringState.emplace(
                    fish.CurrentVelocity,
                    fish.CurrentRenderVector,
                    currentSimulationTime,
                    1.5f); // Slow turn
            }
            else
            {    // Converge direction change at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = 0.15f;
            }
        }
        // Check whether this fish has reached the end of panic mode
        else if (fish.PanicCharge != 0.0f && fish.PanicCharge < 0.02f) // Reached end of panic
        {
            LogMessage("TODOHERE: 4: EndOfPanic");

            //
            // Continue to current target
            //

            fish.PanicCharge = 0.0f;

            // Calculate new target velocity
            fish.TargetVelocity = MakeBasalVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), species, fish.PersonalitySeed);

            // Update render vector to match velocity
            fish.TargetRenderVector = fish.TargetVelocity.normalise();

            // Setup steering, depending on whether we're turning or not
            if (fish.TargetRenderVector.x * fish.CurrentRenderVector.x < 0.0f)
            {
                // Perform a cruise steering
                fish.CruiseSteeringState.emplace(
                    fish.CurrentVelocity,
                    fish.CurrentRenderVector,
                    currentSimulationTime,
                    1.5f); // Slow turn
            }
            else
            {    // Converge direction change at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = 0.03f;
            }
        }
    }

    //
    // 4) Nuke disturbances, now that we've consumed it
    //

    mCurrentInteractiveDisturbance.reset();
    mCurrentInteractiveAttraction.reset();
}

void Fishes::Upload(Render::RenderContext & renderContext) const
{
    renderContext.UploadFishesStart(mFishes.size());

    for (auto const & fish : mFishes)
    {
        float angleCw = fish.CurrentRenderVector.angleCw();
        float horizontalScale = fish.CurrentRenderVector.length();

        if (angleCw < -Pi<float> / 2.0f)
        {
            angleCw = Pi<float> + angleCw;
            horizontalScale *= -1.0f;
        }
        else if (angleCw > Pi<float> / 2.0f)
        {
            angleCw = -Pi<float> + angleCw;
            horizontalScale *= -1.0f;
        }

        auto const & species = mFishShoals[fish.ShoalId].Species;

        renderContext.UploadFish(
            TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, species.RenderTextureFrameIndex),
            fish.CurrentPosition,
            species.WorldSize * mCurrentFishSizeAdjustment,
            angleCw,
            horizontalScale,
            species.TailX,
            species.TailSwingWidth,
            std::sin(fish.CurrentTailProgressPhase));
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

vec2f Fishes::FindNewCruisingTargetPosition(
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

vec2f Fishes::MakeBasalVelocity(
    vec2f const & direction,
    FishSpecies const & species,
    float personalitySeed)
{
    return direction
        * (species.BasalSpeed * (0.7f + personalitySeed * 0.3f));
}

}