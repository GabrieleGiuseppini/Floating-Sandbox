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

float constexpr PositionXVarianceFactor = 1.0f / 4.0f;
float constexpr PositionYVariance = 10.0f;

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

Fishes::Fishes(
    FishSpeciesDatabase const & fishSpeciesDatabase,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
    : mFishSpeciesDatabase(fishSpeciesDatabase)
    , mGameEventHandler(std::move(gameEventDispatcher))
    , mShoalBatchSize(GetShoalBatchSize(mFishSpeciesDatabase))
    , mFishShoals()
    , mFishes()
    , mCurrentFishSizeMultiplier(0.0f)
    , mCurrentFishSpeedAdjustment(0.0f)
    , mCurrentDoFishShoaling(false)
{
}

void Fishes::Update(
    float currentSimulationTime,
    OceanSurface & oceanSurface,
    OceanFloor const & oceanFloor,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld,
    Geometry::AABBSet const & aabbSet)
{
    //
    // Update parameters that changed, if any
    //

    if (mCurrentFishSizeMultiplier != gameParameters.FishSizeMultiplier
        || mCurrentFishSpeedAdjustment != gameParameters.FishSpeedAdjustment)
    {
        // Update fish parameters that depends on these parameters

        float const speedFactor = (mCurrentFishSpeedAdjustment != 0.0f)
            ? gameParameters.FishSpeedAdjustment / mCurrentFishSpeedAdjustment
            : 1.0f;

        float const sizeFactor = (mCurrentFishSizeMultiplier != 0.0f)
            ? gameParameters.FishSizeMultiplier / mCurrentFishSizeMultiplier
            : 1.0f;

        for (auto & fish : mFishes)
        {
            fish.CurrentVelocity *= speedFactor * sizeFactor;
            fish.TargetVelocity *= speedFactor * sizeFactor;
            fish.ShoalingVelocity *= speedFactor * sizeFactor;
            // No need to change render direction, velocity hasn't changed direction

            fish.HeadOffset *= sizeFactor;
        }

        // Update parameters
        mCurrentFishSizeMultiplier = gameParameters.FishSizeMultiplier;
        mCurrentFishSpeedAdjustment = gameParameters.FishSpeedAdjustment;
    }

    if (mCurrentDoFishShoaling != gameParameters.DoFishShoaling)
    {
        // Update shoaling velocity if we're turning off shoaling
        if (!gameParameters.DoFishShoaling)
        {
            for (auto & fish : mFishes)
            {
                fish.ShoalingVelocity = vec2f::zero();
            }
        }

        // Update parameters
        mCurrentDoFishShoaling = gameParameters.DoFishShoaling;
    }

    //
    // Update number of fishes
    //

    UpdateNumberOfFishes(
        currentSimulationTime,
        oceanSurface,
        oceanFloor,
        gameParameters,
        visibleWorld,
        aabbSet);

    //
    // Update dynamics
    //

    UpdateDynamics(
        currentSimulationTime,
        oceanSurface,
        oceanFloor,
        gameParameters,
        visibleWorld,
        aabbSet);

    //
    // Update shoaling
    //

    if (gameParameters.DoFishShoaling)
    {
        UpdateShoaling(
            currentSimulationTime,
            gameParameters,
            visibleWorld);
    }
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
            angleCw = Pi<float> +angleCw;
            horizontalScale *= -1.0f;
        }
        else if (angleCw > Pi<float> / 2.0f)
        {
            angleCw = -Pi<float> +angleCw;
            horizontalScale *= -1.0f;
        }

        auto const & species = mFishShoals[fish.ShoalId].Species;

        renderContext.UploadFish(
            fish.RenderTextureFrameId,
            fish.CurrentPosition,
            species.WorldSize * mCurrentFishSizeMultiplier,
            angleCw,
            horizontalScale,
            species.TailX,
            species.TailSwingWidth,
            std::sin(fish.CurrentTailProgressPhase));
    }

    renderContext.UploadFishesEnd();
}

void Fishes::DisturbAt(
    vec2f const & worldCoordinates,
    float worldRadius,
    GameParameters const & gameParameters)
{
    float const effectiveRadius =
        worldRadius
        * gameParameters.FishSizeMultiplier
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto & fish : mFishes)
    {
        if (!fish.IsInFreefall)
        {
            FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

            // Calculate position of head
            vec2f const fishHeadPosition =
                fish.CurrentPosition
                + fish.CurrentRenderVector.normalise() * species.WorldSize.x * gameParameters.FishSizeMultiplier * (species.HeadOffsetX - 0.5f);

            // Calculate distance from disturbance
            float const distance = (fishHeadPosition - worldCoordinates).length();

            // Check whether the fish has been disturbed
            if (distance < effectiveRadius) // Within radius
            {
                // Enter panic mode with a charge decreasing with distance, and a
                // tiny bit being random
                float constexpr MinPanic = 0.25f;
                fish.PanicCharge = std::max(
                    MinPanic
                    + (0.8f - MinPanic) * (1.0f - SmoothStep(0.0f, effectiveRadius, distance))
                    + 0.2f * fish.PersonalitySeed,
                    fish.PanicCharge);

                // Don't change target position, we'll return to it when panic is over

                // Calculate new direction, away from disturbance
                vec2f panicDirection = (fishHeadPosition - worldCoordinates).normalise(distance);

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
                fish.TargetVelocity = MakeCuisingVelocity(panicDirection, species, fish.PersonalitySeed, gameParameters);

                // Converge directions really fast
                fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                    0.5f,
                    fish.CurrentDirectionSmoothingConvergenceRate);

                // Stop u-turn, if any
                fish.CruiseSteeringState.reset();
            }
        }
    }
}

void Fishes::AttractAt(
    vec2f const & worldCoordinates,
    float worldRadius,
    GameParameters const & gameParameters)
{
    float const effectiveRadius =
        worldRadius
        * gameParameters.FishSizeMultiplier
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto & fish : mFishes)
    {
        if (!fish.IsInFreefall
            && fish.PanicCharge < 0.65f) // Don't attract fish in much panic
        {
            FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

            // Calculate position of head
            vec2f const fishHeadPosition =
                fish.CurrentPosition
                + fish.CurrentRenderVector.normalise() * species.WorldSize.x * gameParameters.FishSizeMultiplier * (species.HeadOffsetX - 0.5f);

            // Calculate distance from attraction
            float const distance = (worldCoordinates - fishHeadPosition).length();

            // Check whether the fish has been attracted
            if (distance < effectiveRadius
                && fish.AttractionDecayTimer < 0.05f) // Free to begin a new attraction cycle
            {
                // Enter panic mode with a charge decreasing with distance
                fish.PanicCharge = std::max(
                    0.3f + 0.7f * (1.0f - SmoothStep(0.0f, effectiveRadius, distance)), // At least 0.3 immediate panic once in radius
                    fish.PanicCharge);

                // Calculate new direction, randomly in the area of food
                float constexpr RandomnessWidth = 3.0f;
                vec2f const randomDelta(
                    GameRandomEngine::GetInstance().GenerateUniformReal(-RandomnessWidth, RandomnessWidth),
                    GameRandomEngine::GetInstance().GenerateUniformReal(-RandomnessWidth, RandomnessWidth));
                vec2f panicDirection = ((worldCoordinates + randomDelta) - fishHeadPosition).normalise();

                // Make sure direction is not too steep
                float constexpr MinXComponent = 0.3f;
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

                // Don't change target position, we'll return to it when panic is over

                // Calculate new target velocity - towards food, and will be panic velocity
                fish.TargetVelocity = MakeCuisingVelocity(panicDirection, species, fish.PersonalitySeed, gameParameters);

                // Converge directions at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                    0.1f,
                    fish.CurrentDirectionSmoothingConvergenceRate);

                // Stop u-turn, if any
                fish.CruiseSteeringState.reset();

                // Begin attraction cycle
                fish.AttractionDecayTimer = 1.0f;
            }
        }
    }
}

void Fishes::TriggerWidespreadPanic(GameParameters const & gameParameters)
{
    for (auto & fish : mFishes)
    {
        if (!fish.IsInFreefall)
        {
            FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

            // Enter panic mode
            fish.PanicCharge = std::max(
                1.6f,
                fish.PanicCharge);

            // Calculate new direction - opposite of current
            float constexpr RandomnessWidth = 5.0f;
            vec2f const randomDelta(
                GameRandomEngine::GetInstance().GenerateUniformReal(-RandomnessWidth, RandomnessWidth),
                GameRandomEngine::GetInstance().GenerateUniformReal(-RandomnessWidth, RandomnessWidth));
            vec2f panicDirection = (-fish.CurrentVelocity + randomDelta).normalise();

            // Don't change target position, we'll return to it when panic is over

            // Calculate new target velocity in this direction - and will be panic velocity
            fish.TargetVelocity = MakeCuisingVelocity(panicDirection, species, fish.PersonalitySeed, gameParameters);

            // Converge directions at this rate
            fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                0.15f,
                fish.CurrentDirectionSmoothingConvergenceRate);

            // Stop u-turn, if any
            fish.CruiseSteeringState.reset();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

void Fishes::UpdateNumberOfFishes(
    float /*currentSimulationTime*/,
    OceanSurface & /*oceanSurface*/,
    OceanFloor const & /*oceanFloor*/,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld,
    Geometry::AABBSet const & aabbSet)
{
    bool isDirty = false;

    if (mFishes.size() > gameParameters.NumberOfFishes)
    {
        //
        // Remove extra fishes
        //

        // Visit all fishes that will be removed
        for (auto fishIt = mFishes.cbegin() + gameParameters.NumberOfFishes; fishIt != mFishes.cend(); ++fishIt)
        {
            // Update shoal
            assert(mFishShoals[fishIt->ShoalId].CurrentMemberCount > 0);
            --mFishShoals[fishIt->ShoalId].CurrentMemberCount;
        }

        // Remove fishes
        mFishes.erase(
            mFishes.begin() + gameParameters.NumberOfFishes,
            mFishes.end());

        isDirty = true;
    }
    else if (mFishes.size() < gameParameters.NumberOfFishes)
    {
        //
        // Add new fishes
        //

        // The index in the shoals at which we start searching for free shoals; this
        // points to the beginning of the first incomplete shoal batch
        size_t shoalSearchStartIndex = (mFishes.size() / mShoalBatchSize) * mFishSpeciesDatabase.GetFishSpeciesCount();
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
                CreateNewFishShoalBatch(static_cast<ElementIndex>(f));

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

            // Initialize shoal, if it's still empty
            if (mFishShoals[currentShoalSearchIndex].CurrentMemberCount == 0)
            {
                //
                // Decide an initial direction for the shoal
                //

                if (currentShoalSearchIndex > 0)
                    mFishShoals[currentShoalSearchIndex].InitialDirection = -mFishShoals[currentShoalSearchIndex - 1].InitialDirection; // Opposite of previous shoal's
                else
                    mFishShoals[currentShoalSearchIndex].InitialDirection = vec2f(
                        GameRandomEngine::GetInstance().Choose(2) == 1 ? -1.0f : 1.0f, // Random left or right
                        0.0f);

                //
                // Decide an initial position for the shoal
                //

                // The x variance grows with the number of fishes
                float const xVariance =
                    visibleWorld.Width * PositionXVarianceFactor
                    * 3.0f * static_cast<float>(1 + mFishes.size() / mShoalBatchSize);

                mFishShoals[currentShoalSearchIndex].InitialPosition = ChoosePosition(
                    vec2f(visibleWorld.Center.x, species.OceanDepth),
                    xVariance,
                    PositionYVariance * 0.5f);

                mFishShoals[currentShoalSearchIndex].InitialPosition.x = mFishShoals[currentShoalSearchIndex].InitialDirection.x < 0.0f
                    ? std::abs(mFishShoals[currentShoalSearchIndex].InitialPosition.x)
                    : -std::abs(mFishShoals[currentShoalSearchIndex].InitialPosition.x);
            }

            //
            // 2) Create fish in this shoal
            //

            vec2f const initialPosition = ChoosePosition(
                mFishShoals[currentShoalSearchIndex].InitialPosition,
                10.0f,
                4.0f);

            vec2f const targetPosition = FindNewCruisingTargetPosition(
                initialPosition,
                mFishShoals[currentShoalSearchIndex].InitialDirection,
                species,
                visibleWorld);

            float const headOffset = species.WorldSize.x * mCurrentFishSizeMultiplier * (species.HeadOffsetX - 0.5f);

            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            TextureFrameIndex const renderTextureFrameIndex = static_cast<TextureFrameIndex>(
                GameRandomEngine::GetInstance().Choose(species.RenderTextureFrameIndices.size()));

            mFishes.emplace_back(
                currentShoalSearchIndex,
                personalitySeed,
                initialPosition,
                targetPosition,
                MakeCuisingVelocity((targetPosition - initialPosition).normalise(), species, personalitySeed, gameParameters),
                headOffset,
                GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>), // initial progress phase
                TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, species.RenderTextureFrameIndices[renderTextureFrameIndex]));

            // Update shoal
            ++(mFishShoals[currentShoalSearchIndex].CurrentMemberCount);
        }

        isDirty = true;
    }

    if (isDirty)
    {
        // Notify new count
        mGameEventHandler->OnFishCountUpdated(mFishes.size());
    }
}

void Fishes::UpdateDynamics(
    float currentSimulationTime,
    OceanSurface & oceanSurface,
    OceanFloor const & oceanFloor,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld,
    Geometry::AABBSet const & aabbSet)
{
    float constexpr OceanSurfaceLowWatermark = 4.0f;

    ElementCount const fishCount = static_cast<ElementCount>(mFishes.size());

    for (ElementIndex f = 0; f < fishCount; ++f)
    {
        Fish & fish = mFishes[f];

        FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

        ///////////////////////////////////////////////////////////////////
        // 1) Steer or auto-smooth direction
        ///////////////////////////////////////////////////////////////////

        if (fish.CruiseSteeringState.has_value())
        {
            //
            // Cruise steering
            //

            float const elapsedSteeringDurationFraction = (currentSimulationTime - fish.CruiseSteeringState->SimulationTimeStart) / fish.CruiseSteeringState->SimulationTimeDuration;

            // Check whether we should stop steering
            if (elapsedSteeringDurationFraction >= 1.0f)
            {
                // Stop steering

                // Change state
                fish.CruiseSteeringState.reset();

                // Reach all targets
                fish.CurrentVelocity = fish.TargetVelocity;
                fish.CurrentRenderVector = fish.TargetVelocity.normalise();
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

                vec2f const targetRenderVector = fish.TargetVelocity.normalise();

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
                        targetRenderVector.y * SmoothStep(0.60f, 1.0f, elapsedSteeringDurationFraction);
                }

                // RenderVector X:
                // - smooth towards target during a central interval (actual turning around),
                //   without crossing zero
                float constexpr TurnLimit = 0.05f;
                if (elapsedSteeringDurationFraction <= 0.5f)
                {
                    fish.CurrentRenderVector.x =
                        fish.CruiseSteeringState->StartRenderVector.x * (1.0f - (1.0f - TurnLimit) * SmoothStep(0.15f, 0.5f, elapsedSteeringDurationFraction));
                }
                else
                {
                    fish.CurrentRenderVector.x =
                        targetRenderVector.x * (TurnLimit + (1.0f - TurnLimit) * SmoothStep(0.5f, 0.85f, elapsedSteeringDurationFraction));
                }
            }
        }
        else
        {
            //
            // Automated direction smoothing
            //

            // Smooth velocity towards target + shoaling
            fish.CurrentVelocity +=
                ((fish.TargetVelocity + fish.ShoalingVelocity) - fish.CurrentVelocity) * fish.CurrentDirectionSmoothingConvergenceRate;

            // Make RenderVector match current velocity
            fish.CurrentRenderVector = fish.CurrentVelocity.normalise();

            // Converge smoothing convergence rate to its ideal value
            fish.CurrentDirectionSmoothingConvergenceRate =
                Fish::IdealDirectionSmoothingConvergenceRate
                + (fish.CurrentDirectionSmoothingConvergenceRate - Fish::IdealDirectionSmoothingConvergenceRate) * 0.98f;
        }

        ///////////////////////////////////////////////////////////////////
        // 2) Update dynamics
        ///////////////////////////////////////////////////////////////////

        float constexpr OceanSurfaceDisturbance = 1.0f; // Magic number

        // Get water surface level at this fish
        float const oceanY = oceanSurface.GetHeightAt(fish.CurrentPosition.x);

        //
        // Run freefall state machine
        //

        if (!fish.IsInFreefall
            && fish.CurrentPosition.y > oceanY)
        {
            //
            // Enter freefall
            //

            fish.IsInFreefall = true;

            // Stop u-turn, in case we were across it
            fish.CruiseSteeringState.reset();

            // Create a little disturbance in the ocean surface
            oceanSurface.DisplaceAt(fish.CurrentPosition.x, OceanSurfaceDisturbance);
        }
        else if (fish.IsInFreefall
            && fish.CurrentPosition.y <= oceanY - OceanSurfaceLowWatermark)  // Lower level for re-entry, so that jump is more pronounced
        {
            //
            // Leave freefall (re-entry!)
            //

            fish.IsInFreefall = false;

            // Drag velocity down
            float const currentVelocityMagnitude = fish.CurrentVelocity.length();
            float constexpr MaxVelocityMagnitude = 1.3f; // Magic number
            fish.TargetVelocity =
                fish.CurrentVelocity.normalise(currentVelocityMagnitude)
                * Clamp(currentVelocityMagnitude, 0.0f, MaxVelocityMagnitude);

            // Converge to dragged velocity at this rate, overriding current rate
            fish.CurrentDirectionSmoothingConvergenceRate = 0.05f;

            // Note: no need to change render vector, velocity direction has not changed

            // Enter "a bit of" panic mode (overriding current panic);
            // after exhausting this panic charge, the fish will resume
            // swimming towards it current target position
            fish.PanicCharge = 0.03f;

            // Create a little disturbance in the ocean surface
            oceanSurface.DisplaceAt(fish.CurrentPosition.x, OceanSurfaceDisturbance);
        }

        //
        // Dynamics update
        //

        if (!fish.IsInFreefall)
        {
            //
            // Swimming
            //

            float const speedMultiplier = (fish.PanicCharge * 8.5f + 1.0f);

            // Update position: add current velocity
            fish.CurrentPosition +=
                fish.CurrentVelocity
                * GameParameters::SimulationStepTimeDuration<float>
                * speedMultiplier;

            // Update tail progress phase: add basal speed
            fish.CurrentTailProgressPhase += species.TailSpeed * speedMultiplier * gameParameters.FishSpeedAdjustment;

            // Update position: superimpose a small sin component, unless we're steering
            if (!fish.CruiseSteeringState.has_value())
            {
                fish.CurrentPosition +=
                    fish.CurrentRenderVector
                    * (1.0f + std::sin(2.0f * fish.CurrentTailProgressPhase))
                    * (1.0f + fish.PanicCharge) // Grow incisiveness with panic
                    / 150.0f; // Magic number
            }
        }
        else
        {
            //
            // Free-falling
            //

            // Update velocity with gravity, amplified for better scenics
            float const newVelocityY = fish.CurrentVelocity.y
                - 10.0f // Amplification factor
                * GameParameters::GravityMagnitude
                * GameParameters::SimulationStepTimeDuration<float>;
            fish.TargetVelocity = vec2f(
                fish.CurrentVelocity.x,
                newVelocityY);
            fish.CurrentVelocity = fish.TargetVelocity; // Converge immediately

            // Converge at this rate, overriding current convergence rate
            fish.CurrentDirectionSmoothingConvergenceRate = 0.06f;

            // Update position: add velocity
            fish.CurrentPosition +=
                fish.CurrentVelocity
                * GameParameters::SimulationStepTimeDuration<float>;

            // Update tail progress phase: add extra speed (fish flapping its tail)
            fish.CurrentTailProgressPhase += species.TailSpeed * 20.0f;
        }

        // Decay panic charge
        fish.PanicCharge *= 0.985f;

        // Decay attraction timer
        fish.AttractionDecayTimer *= 0.75f;

        ///////////////////////////////////////////////////////////////////
        // 3) World boundaries check
        ///////////////////////////////////////////////////////////////////

        bool hasBouncedAgainstWorldBoundaries = false;

        if (fish.CurrentPosition.x < -GameParameters::HalfMaxWorldWidth)
        {
            // Bounce position
            fish.CurrentPosition.x = -GameParameters::HalfMaxWorldWidth + (-GameParameters::HalfMaxWorldWidth - fish.CurrentPosition.x);

            // Bounce both current and target velocity
            fish.CurrentVelocity.x = std::abs(fish.CurrentVelocity.x);
            fish.TargetVelocity.x = std::abs(fish.TargetVelocity.x);

            // Adjust other fish properties
            hasBouncedAgainstWorldBoundaries = true;
        }
        else if (fish.CurrentPosition.x > GameParameters::HalfMaxWorldWidth)
        {
            // Bounce position
            fish.CurrentPosition.x = GameParameters::HalfMaxWorldWidth - (fish.CurrentPosition.x - GameParameters::HalfMaxWorldWidth);

            // Bounce both current and target velocity
            fish.CurrentVelocity.x = -std::abs(fish.CurrentVelocity.x);
            fish.TargetVelocity.x = -std::abs(fish.TargetVelocity.x);

            // Adjust other fish properties
            hasBouncedAgainstWorldBoundaries = true;
        }

        if (hasBouncedAgainstWorldBoundaries)
        {
            // Find a new target position away
            fish.TargetPosition = FindNewCruisingTargetPosition(
                fish.CurrentPosition,
                fish.TargetVelocity.normalise(),
                species,
                visibleWorld);

            // Stop cruising, in case we were cruising
            fish.CruiseSteeringState.reset();

            // Skip everything else
            continue;
        }

        assert(fish.CurrentPosition.x >= -GameParameters::HalfMaxWorldWidth
            && fish.CurrentPosition.x <= GameParameters::HalfMaxWorldWidth);

        // Stop now if we're free-falling
        if (fish.IsInFreefall)
        {
            // Cut short state machine now, this fish can't swim
            continue;
        }

        ///////////////////////////////////////////////////////////////////
        // 4) Check state machine transitions
        ///////////////////////////////////////////////////////////////////

        // Check whether this fish has reached its target
        if (std::abs(fish.CurrentPosition.x - fish.TargetPosition.x) < 7.0f
            && fish.PanicCharge == 0.0f) // Not in panic
        {
            //
            // Target Reached
            //

            // Choose new target position
            fish.TargetPosition = FindNewCruisingTargetPosition(
                fish.CurrentPosition,
                -fish.CurrentVelocity.normalise(),
                species,
                visibleWorld);

            // Calculate new target velocity
            fish.TargetVelocity = MakeCuisingVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), species, fish.PersonalitySeed, gameParameters);

            // Setup steering, depending on whether we're turning or not
            if (fish.TargetVelocity.x * fish.CurrentVelocity.x < 0.0f
                && !fish.CruiseSteeringState.has_value()) // Not steering already
            {
                // Perform a cruise steering
                fish.CruiseSteeringState.emplace(
                    fish.CurrentVelocity,
                    fish.CurrentRenderVector,
                    currentSimulationTime,
                    1.5f); // Slow turn

                // Remember the time at which we did the last steering
                fish.LastSteeringSimulationTime = currentSimulationTime;
            }
            else
            {    // Converge direction change at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                    0.15f,
                    fish.CurrentDirectionSmoothingConvergenceRate);
            }
        }
        // Check whether this fish has reached the end of panic mode
        else if (fish.PanicCharge != 0.0f && fish.PanicCharge < 0.02f) // Reached end of panic
        {
            //
            // End of Panic
            //

            fish.PanicCharge = 0.0f;

            // Continue to current target

            // Calculate new target velocity
            fish.TargetVelocity = MakeCuisingVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), species, fish.PersonalitySeed, gameParameters);

            // Setup steering, depending on whether we're turning or not
            if (fish.TargetVelocity.x * fish.CurrentVelocity.x < 0.0f
                && !fish.CruiseSteeringState.has_value()) // Not steering already
            {
                // Perform a cruise steering
                fish.CruiseSteeringState.emplace(
                    fish.CurrentVelocity,
                    fish.CurrentRenderVector,
                    currentSimulationTime,
                    1.5f); // Slow turn

                // Remember the time at which we did the last steering
                fish.LastSteeringSimulationTime = currentSimulationTime;
            }
            else
            {    // Converge direction change at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                    0.08f,
                    fish.CurrentDirectionSmoothingConvergenceRate);
            }
        }

        ///////////////////////////////////////////////////////////////////
        // 5) Check ocean boundaries
        ///////////////////////////////////////////////////////////////////

        // Calculate position of head
        vec2f const fishHeadPosition =
            fish.CurrentPosition
            + fish.CurrentRenderVector * fish.HeadOffset;

        // Check whether we're too close to the water surface (idealized as being horizontal) - but only if fish is not in too much panic
        if (float const depth = oceanY - fish.CurrentPosition.y;
            depth < 2.0f + OceanSurfaceLowWatermark
            && fish.PanicCharge <= 0.3f // Not too much panig
            && fish.TargetVelocity.y >= 0.0f) // Bounce away only if we're really going into it
        {
            //
            // OceanSurface Bounce
            //

            // Bounce direction, opposite of target
            vec2f const bounceDirection = vec2f(fish.TargetVelocity.x, -fish.TargetVelocity.y).normalise();

            // Calculate new target velocity - away from disturbance point
            fish.TargetVelocity = MakeCuisingVelocity(bounceDirection, species, fish.PersonalitySeed, gameParameters);

            // Converge direction change at this rate
            fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                0.05f * (1.0f + fish.PanicCharge),
                fish.CurrentDirectionSmoothingConvergenceRate);
        }

        // Check ocean floor collision
        float const clampedX = Clamp(fishHeadPosition.x, -GameParameters::HalfMaxWorldWidth, GameParameters::HalfMaxWorldWidth);
        float const oceanFloorHeight = oceanFloor.GetHeightAt(clampedX);
        if (fishHeadPosition.y <= oceanFloorHeight)
        {
            //
            // Ocean floor collision
            //

            // Calculate sea floor normal (positive points out), via
            // the derivative of the ocean floor at this X
            float constexpr Dx = 0.01f;
            vec2f const seaFloorNormal = vec2f(
                oceanFloorHeight - oceanFloor.GetHeightAt(clampedX + 0.01f),
                Dx).normalise(); // Points below

            // Calculate the component of the fish's target velocity along the normal,
            // i.e. towards the outside of the floor...
            float const targetVelocityAlongNormal = fish.TargetVelocity.dot(seaFloorNormal);

            // ...if positive, it will soon be going already outside of the floor, hence we leave it as-is
            if (targetVelocityAlongNormal <= 0.0f)
            {
                // Set target velocity to reflection of fish's target velocity around normal:
                // R = V − 2(V⋅N^)N^
                fish.TargetVelocity =
                    fish.TargetVelocity
                    - seaFloorNormal * 2.0f * targetVelocityAlongNormal;

                // Converge direction change at this rate
                fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                    0.15f,
                    fish.CurrentDirectionSmoothingConvergenceRate);
            }
        }

        ///////////////////////////////////////////////////////////////////
        // 6) Check AABB boundaries
        ///////////////////////////////////////////////////////////////////

        if (fish.PanicCharge <= 0.3f) // Only if we're not in panic
        {
            float constexpr AABBMargin = 4.0f;

            for (auto const & aabb : aabbSet.GetItems())
            {
                // TODO: optimize: move boundary calc's here and use it instead of Contains
                if (aabb.Contains(fishHeadPosition, AABBMargin))
                {
                    float const lBoundary = aabb.BottomLeft.x - AABBMargin;
                    float const rBoundary = aabb.TopRight.x + AABBMargin;
                    float const tBoundary = aabb.TopRight.y + AABBMargin;
                    float const bBoundary = aabb.BottomLeft.y - AABBMargin;

                    // Find which axes we are currently closest with, and bounce on it
                    vec2f normal = vec2f::zero();
                    bool hasBounced = false;
                    if (std::min(fishHeadPosition.x - lBoundary, rBoundary - fishHeadPosition.x)
                        < std::min(fishHeadPosition.y - bBoundary, tBoundary - fishHeadPosition.y))
                    {
                        // We are closest to the vertical axes
                        if (fishHeadPosition.x - lBoundary < rBoundary - fishHeadPosition.x)
                        {
                            // Left
                            //if (fish.TargetVelocity.x > 0.0f)
                            {
                                normal = vec2f(-1.0f, 0.0f);
                                //fish.TargetVelocity.x *= -1.0f;
                                hasBounced = true;
                            }
                        }
                        else
                        {
                            // Right
                            //if (fish.TargetVelocity.x < 0.0f)
                            {
                                normal = vec2f(1.0f, 0.0f);
                                //fish.TargetVelocity.x *= -1.0f;
                                hasBounced = true;
                            }
                        }
                    }
                    else
                    {
                        // We are closest to the horizontal axes
                        if (fishHeadPosition.y - bBoundary < tBoundary - fishHeadPosition.y)
                        {
                            // Bottom
                            //if (fish.TargetVelocity.y > 0.0f)
                            {
                                normal = vec2f(0.0f, -1.0f);
                                //fish.TargetVelocity.y *= -1.0f;
                                hasBounced = true;
                            }
                        }
                        else
                        {
                            // Top
                            //if (fish.TargetVelocity.y < 0.0f)
                            {
                                normal = vec2f(0.0f, 1.0f);
                                //fish.TargetVelocity.y *= -1.0f;
                                hasBounced = true;
                            }
                        }
                    }

                    // Panic a bit
                    fish.PanicCharge = std::max(
                        0.5f,
                        fish.PanicCharge);

                    fish.TargetVelocity =
                        (fish.TargetVelocity.normalise() + normal * 2.0f).normalise()
                        * fish.TargetVelocity.length();

                    // Converge direction change at a fast rate
                    fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                        0.15f, // TODOHERE
                        fish.CurrentDirectionSmoothingConvergenceRate);

                    // Stop steering, if we're steering
                    fish.CruiseSteeringState.reset();
                }
            }
        }
    }
}

void Fishes::UpdateShoaling(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld)
{
    auto const fishCount = mFishes.size();

    for (ElementIndex f = 0; f < fishCount; ++f)
    {
        Fish & fish = mFishes[f];
        FishShoal const & fishShoal = mFishShoals[fish.ShoalId];

        if (fishShoal.CurrentMemberCount > 1 // A shoal contains at least one fish
            && fish.ShoalingTimer <= 0.0f // Wait for this fish's shoaling cycle
            && fish.PanicCharge < 0.02f) // Skip fishes even in little panic
        {
            if (!fish.CruiseSteeringState.has_value() // Fish is not u-turning
                && !fish.IsInFreefall) // Fish is swimming
            {
                // Calculate shoal radius for this fish in world coordinates
                float const fishShoalRadius =
                    fishShoal.Species.ShoalRadius
                    * gameParameters.FishShoalRadiusAdjustment
                    * fishShoal.MaxWorldDimension
                    * gameParameters.FishSizeMultiplier
                    + fish.PersonalitySeed; // Add some randomness to prevent regular patterns

                // Calculate shoal spacing as fraction of shoal radius
                float const fishShoalSpacing = 0.7f * fishShoalRadius;

                //
                // Visit all fishes in same shoal
                //

                ElementIndex closestFishIndex = NoneElementIndex; // Closest neighbour among those that are closer to fish than spacing
                float closestFishDistance = std::numeric_limits<float>::max();
                ElementIndex furthestFishIndex = NoneElementIndex; // Furthest neighbour among those that are further from fish than spacing
                float furthestFishDistance = std::numeric_limits<float>::lowest();

                for (ElementIndex n = 0; n < fishCount; ++n)
                {
                    if (mFishes[n].ShoalId == fish.ShoalId // Same shoal
                        && n != f) // Not same fish
                    {
                        Fish const & neighbor = mFishes[n];

                        if (float const distance = (neighbor.CurrentPosition - fish.CurrentPosition).length();
                            distance < fishShoalRadius) // Neighbor is in the neighborhood (...hence a neighbor)
                        {
                            // Update closest and furthest
                            if (distance < fishShoalSpacing)
                            {
                                // Too close wrt spacing
                                if (distance < closestFishDistance)
                                {
                                    closestFishIndex = n;
                                    closestFishDistance = distance;
                                }
                            }
                            else
                            {
                                // Too far wrt spacing
                                if (distance > furthestFishDistance)
                                {
                                    furthestFishIndex = n;
                                    furthestFishDistance = distance;
                                }
                            }

                            // Check if should do a u-turn based on this neighbor
                            float constexpr UTurnSpeed = 2.5f;
                            if (neighbor.TargetVelocity.x * fish.TargetVelocity.x < 0.0f // Intents are opposite
                                && (currentSimulationTime - fish.LastSteeringSimulationTime) > UTurnSpeed + 3.0f // This fish hasn't u-turned recently
                                && fish.LastSteeringSimulationTime < neighbor.LastSteeringSimulationTime) // The neighbor has u-turned more recently
                            {
                                vec2f const neighborDirection = neighbor.TargetVelocity.normalise();

                                // Find a new target position along the neighbor's direction
                                fish.TargetPosition = FindNewCruisingTargetPosition(
                                    fish.CurrentPosition,
                                    neighborDirection,
                                    fishShoal.Species,
                                    visibleWorld);

                                // Change target velocity to get to target position
                                fish.TargetVelocity = MakeCuisingVelocity(neighborDirection, fishShoal.Species, fish.PersonalitySeed, gameParameters);

                                // Perform a cruise steering
                                fish.CruiseSteeringState.emplace(
                                    fish.CurrentVelocity,
                                    fish.CurrentRenderVector,
                                    currentSimulationTime,
                                    UTurnSpeed);

                                // Remember the time at which we did the last steering
                                fish.LastSteeringSimulationTime = currentSimulationTime;

                                break;
                            }
                        }
                    }
                }

                // If we've decided we're gonna u-turn, then stop here
                if (fish.CruiseSteeringState.has_value())
                    continue;

                // Make sure we've found at least one neighbor
                if (furthestFishIndex == NoneElementIndex
                    && closestFishIndex == NoneElementIndex
                    && f != fishShoal.StartFishIndex) // This fish is not the lead
                {
                    //
                    // We're too far from anyone else...
                    // ...go towards lead then!
                    //

                    // Pick lead
                    Fish const & lead = mFishes[fishShoal.StartFishIndex];

                    vec2f const fishToLeadVector = lead.CurrentPosition - fish.CurrentPosition;
                    float const distance = fishToLeadVector.length();
                    vec2f const fishToLeadDirection = fishToLeadVector.normalise(distance);

                    // Check whether we need to turn - we do if lead is currently behind us
                    if (fish.TargetVelocity.x * fishToLeadDirection.x < 0.0f)
                    {
                        // Find a new target position towards the lead
                        fish.TargetPosition = FindNewCruisingTargetPosition(
                            fish.CurrentPosition,
                            fishToLeadDirection,
                            fishShoal.Species,
                            visibleWorld);

                        // Change target velocity to get to target position
                        fish.TargetVelocity = MakeCuisingVelocity(fishToLeadDirection, fishShoal.Species, fish.PersonalitySeed, gameParameters);

                        // Perform a cruise steering
                        fish.CruiseSteeringState.emplace(
                            fish.CurrentVelocity,
                            fish.CurrentRenderVector,
                            currentSimulationTime,
                            0.5f);

                        // Do not reset last steering time, as we want to be able to re-turn when
                        // we get back into the shoal
                    }

                    // Set shoaling velocity to match
                    fish.ShoalingVelocity =
                        fishToLeadDirection
                        * 1.8f // Magic number
                        * gameParameters.FishSpeedAdjustment;

                    // Add some panic, depending on distance
                    fish.PanicCharge = std::max(
                        fish.PanicCharge,
                        0.4f * SmoothStep(0.0f, 30.0f, distance));
                }
                else
                {
                    //
                    // Apply correction vectors
                    //

                    vec2f collisionCorrectionVelocity = (closestFishIndex != NoneElementIndex)
                        ? -(mFishes[closestFishIndex].CurrentPosition - fish.CurrentPosition).normalise() * 1.2f // Go away from neighbor
                        : vec2f::zero();

                    vec2f cohesionCorrectionVelocity = (furthestFishIndex != NoneElementIndex)
                        ? (mFishes[furthestFishIndex].CurrentPosition - fish.CurrentPosition).normalise() * 1.8f // Go towards neighbor
                        : vec2f::zero();

                    fish.ShoalingVelocity =
                        (collisionCorrectionVelocity + cohesionCorrectionVelocity)
                        * gameParameters.FishSpeedAdjustment;
                }

                // Start another shoaling cycle
                fish.ShoalingTimer = Fish::ShoalingTimerCycleDuration;
            }
            else
            {
                // Zero out any residual shoaling
                fish.ShoalingVelocity = vec2f::zero();
            }
        }

        // Decay shoaling cycle
        fish.ShoalingTimer -= GameParameters::SimulationStepTimeDuration<float>;
    }
}

void Fishes::CreateNewFishShoalBatch(ElementIndex startFishIndex)
{
    for (auto const & species : mFishSpeciesDatabase.GetFishSpecies())
    {
        mFishShoals.emplace_back(
            species,
            startFishIndex,
            species.WorldSize.x >= species.WorldSize.y ? species.WorldSize.x : species.WorldSize.y);

        startFishIndex += static_cast<ElementCount>(species.ShoalSize);
    }
}

vec2f Fishes::ChoosePosition(
    vec2f const & averagePosition,
    float xVariance,
    float yVariance)
{
    float const positionX = Clamp(
        GameRandomEngine::GetInstance().GenerateNormalReal(averagePosition.x, xVariance),
        -GameParameters::HalfMaxWorldWidth,
        GameParameters::HalfMaxWorldWidth);

    float const positionY =
        -5.0f // Min depth
        - std::fabs(GameRandomEngine::GetInstance().GenerateNormalReal(averagePosition.y, yVariance));

    return vec2f(positionX, positionY);
}

vec2f Fishes::FindNewCruisingTargetPosition(
    vec2f const & currentPosition,
    vec2f const & newDirection,
    FishSpecies const & species,
    VisibleWorld const & visibleWorld)
{
    // X:
    //      - if currentPosition.x with direction.x goes towards center X: go by much
    //      - else: go by less
    // Y:
    //      - obey species' band

    float const averageTargetPositionX = (visibleWorld.Center.x - currentPosition.x) * newDirection.x >= 0
        ? currentPosition.x + newDirection.x * visibleWorld.Width * 1.5f
        : currentPosition.x + newDirection.x * visibleWorld.Width / 4.0f;

    return ChoosePosition(
        vec2f(
            averageTargetPositionX,
            species.OceanDepth),
        visibleWorld.Width * PositionXVarianceFactor, // x variance
        PositionYVariance); // y variance
}

vec2f Fishes::MakeCuisingVelocity(
    vec2f const & direction,
    FishSpecies const & species,
    float personalitySeed,
    GameParameters const & gameParameters)
{
    return direction
        * (species.BasalSpeed * gameParameters.FishSpeedAdjustment * gameParameters.FishSizeMultiplier)
        * (0.7f + personalitySeed * 0.3f);
}

}