/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/GameMath.h>
#include <Core/GameRandomEngine.h>
#include <Core/Utils.h>

#include <picojson.h>

#include <algorithm>
#include <chrono>

namespace Physics {

namespace /*anonymous*/ {

float constexpr PositionXVarianceFactor = 1.0f / 4.0f;
float constexpr PositionYVariance = 10.0f;

float constexpr AABBMargin = 4.0f;

}

Fishes::Fishes(
    FishSpeciesDatabase const & fishSpeciesDatabase,
    SimulationEventDispatcher & simulationEventDispatcher)
    : mFishSpeciesDatabase(fishSpeciesDatabase)
    , mSimulationEventHandler(simulationEventDispatcher)
    , mFishShoals()
    , mFishes()
    , mInteractions()
    , mCurrentFishSizeMultiplier(0.0f)
    , mCurrentFishSpeedAdjustment(0.0f)
    , mCurrentDoFishShoaling(false)
{
}

void Fishes::Update(
    float currentSimulationTime,
    OceanSurface & oceanSurface,
    OceanFloor const & oceanFloor,
    SimulationParameters const & simulationParameters,
    VisibleWorld const & visibleWorld,
    Geometry::AABBSet const & aabbSet)
{
    //
    // Update parameters that changed, if any
    //

    if (mCurrentFishSizeMultiplier != simulationParameters.FishSizeMultiplier
        || mCurrentFishSpeedAdjustment != simulationParameters.FishSpeedAdjustment)
    {
        // Update fish parameters that depends on these parameters

        float const speedFactor = (mCurrentFishSpeedAdjustment != 0.0f)
            ? simulationParameters.FishSpeedAdjustment / mCurrentFishSpeedAdjustment
            : 1.0f;

        float const sizeFactor = (mCurrentFishSizeMultiplier != 0.0f)
            ? simulationParameters.FishSizeMultiplier / mCurrentFishSizeMultiplier
            : 1.0f;

        for (auto & fish : mFishes)
        {
            fish.CurrentVelocity *= speedFactor * sizeFactor;
            fish.TargetVelocity *= speedFactor * sizeFactor;
            fish.ShoalingVelocity *= speedFactor * sizeFactor;
            // No need to change render direction, velocity hasn't changed direction

            fish.HeadOffset *= sizeFactor;
        }

        for (auto & fishShoal : mFishShoals)
        {
            fishShoal.MaxWorldDimension *= sizeFactor;
        }

        // Update parameters
        mCurrentFishSizeMultiplier = simulationParameters.FishSizeMultiplier;
        mCurrentFishSpeedAdjustment = simulationParameters.FishSpeedAdjustment;
    }

    if (mCurrentDoFishShoaling != simulationParameters.DoFishShoaling)
    {
        // Update shoaling velocity if we're turning off shoaling
        if (!simulationParameters.DoFishShoaling)
        {
            for (auto & fish : mFishes)
            {
                fish.ShoalingVelocity = vec2f::zero();
            }
        }

        // Update parameters
        mCurrentDoFishShoaling = simulationParameters.DoFishShoaling;
    }

    //
    // Update number of fishes
    //

    UpdateNumberOfFishes(
        currentSimulationTime,
        oceanSurface,
        oceanFloor,
        aabbSet,
        simulationParameters,
        visibleWorld);

    //
    // Update interactions
    //

    UpdateInteractions(simulationParameters);

    //
    // Update dynamics
    //

    UpdateDynamics(
        currentSimulationTime,
        oceanSurface,
        oceanFloor,
        aabbSet,
        simulationParameters,
        visibleWorld);

    //
    // Update shoaling
    //

    if (simulationParameters.DoFishShoaling)
    {
        UpdateShoaling(
            currentSimulationTime,
            simulationParameters,
            visibleWorld);
    }
}

void Fishes::Upload(RenderContext & renderContext) const
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
            std::sinf(fish.CurrentTailProgressPhase));
    }

    renderContext.UploadFishesEnd();
}

void Fishes::DisturbAt(
    vec2f const & worldCoordinates,
    float worldRadius,
    std::chrono::milliseconds delay)
{
    mInteractions.emplace_back(
        Interaction::InteractionType::Disturbance,
        Interaction::AreaSpecification(
            worldCoordinates,
            worldRadius),
        GameWallClock::GetInstance().Now() + delay);
}

void Fishes::AttractAt(
    vec2f const & worldCoordinates,
    float worldRadius,
    std::chrono::milliseconds delay)
{
    mInteractions.emplace_back(
        Interaction::InteractionType::Attraction,
        Interaction::AreaSpecification(
            worldCoordinates,
            worldRadius),
        GameWallClock::GetInstance().Now() + delay);
}

void Fishes::TriggerWidespreadPanic(std::chrono::milliseconds delay)
{
    mInteractions.emplace_back(
        Interaction::InteractionType::Disturbance,
        std::nullopt,
        GameWallClock::GetInstance().Now() + delay);
}

////////////////////////////////////////////////////////////////////////////////////////////////

void Fishes::UpdateNumberOfFishes(
    float /*currentSimulationTime*/,
    OceanSurface & /*oceanSurface*/,
    OceanFloor const & oceanFloor,
    Geometry::AABBSet const & aabbSet,
    SimulationParameters const & simulationParameters,
    VisibleWorld const & visibleWorld)
{
    bool isDirty = false;

    if (mFishes.size() > simulationParameters.NumberOfFishes)
    {
        //
        // Remove extra fishes
        //

        // Visit all fishes that will be removed
        for (auto fishIt = mFishes.cbegin() + simulationParameters.NumberOfFishes; fishIt != mFishes.cend(); ++fishIt)
        {
            // Update shoal
            assert(mFishShoals[fishIt->ShoalId].CurrentMemberCount > 0);
            --mFishShoals[fishIt->ShoalId].CurrentMemberCount;
        }

        // Remove fishes
        mFishes.erase(
            mFishes.begin() + simulationParameters.NumberOfFishes,
            mFishes.end());

        // Trim empty shoals
        while (!mFishShoals.empty())
        {
            if (mFishShoals.back().CurrentMemberCount == 0)
                mFishShoals.pop_back();
            else
                break;
        }

        isDirty = true;
    }
    else if (mFishes.size() < simulationParameters.NumberOfFishes)
    {
        //
        // Add new fishes
        //

        size_t freeShoalIndex = 0;

        for (size_t f = mFishes.size(); f < simulationParameters.NumberOfFishes; ++f)
        {
            //
            // 1) Find a shoal for this new fish
            //

            // Find next incomplete shoal
            for (; freeShoalIndex < mFishShoals.size(); ++freeShoalIndex)
            {
                if (mFishShoals[freeShoalIndex].CurrentMemberCount < mFishShoals[freeShoalIndex].Species.ShoalSize)
                {
                    // Found a free spot!
                    break;
                }
            }

            if (freeShoalIndex == mFishShoals.size())
            {
                // Make a new shoal altogether

                // Pick a new species - start from the last one used
                assert(mFishSpeciesDatabase.GetFishSpeciesCount() > 0);
                size_t fishSpeciesCandidateIndex = (mFishShoals.empty())
                    ? 0
                    : mFishSpeciesDatabase.GetFishSpeciesIndex(mFishShoals.back().Species) + 1;

                // Loop around the species database until a species is found
                for (;; ++fishSpeciesCandidateIndex)
                {
                    fishSpeciesCandidateIndex = fishSpeciesCandidateIndex % mFishSpeciesDatabase.GetFishSpeciesCount();

                    // Make sure ocean depth is good
                    auto const speciesDepth = mFishSpeciesDatabase.GetFishSpecies()[fishSpeciesCandidateIndex].OceanDepth;
                    if (speciesDepth <= 300
                        || simulationParameters.SeaDepth >= speciesDepth + PositionYVariance)
                    {
                        // Found!
                        break;
                    }
                }

                //
                // Create new shoal
                //

                auto const & species = mFishSpeciesDatabase.GetFishSpecies()[fishSpeciesCandidateIndex];
                auto & newShoal = mFishShoals.emplace_back(
                    species,
                    static_cast<ElementIndex>(f), // startFishIndex
                    std::max(species.WorldSize.x, species.WorldSize.y) * mCurrentFishSizeMultiplier);

                assert(freeShoalIndex == mFishShoals.size() - 1);

                // Decide an initial direction for the shoal

                if (freeShoalIndex > 0)
                    newShoal.InitialDirection = -mFishShoals[freeShoalIndex - 1].InitialDirection; // Opposite of previous shoal's
                else
                    newShoal.InitialDirection = vec2f(
                        GameRandomEngine::GetInstance().Choose(2) == 1 ? -1.0f : 1.0f, // Random left or right
                        0.0f);

                // Decide an initial position for the shoal

                // The x variance grows with the number of shoals
                float const xVariance =
                    visibleWorld.Width * PositionXVarianceFactor
                    * 3.0f * (1.0f + static_cast<float>(mFishShoals.size()) / static_cast<float>(mFishSpeciesDatabase.GetFishSpeciesCount()));

                newShoal.InitialPosition = FindPosition(
                    vec2f(visibleWorld.Center.x, species.OceanDepth),
                    xVariance,
                    PositionYVariance * 0.5f,
                    oceanFloor,
                    aabbSet);

                newShoal.InitialPosition.x = newShoal.InitialDirection.x < 0.0f
                    ? std::abs(newShoal.InitialPosition.x)
                    : -std::abs(newShoal.InitialPosition.x);
            }

            assert(mFishShoals.size() > 0 && mFishShoals.back().CurrentMemberCount < mFishShoals.back().Species.ShoalSize);

            //
            // 2) Create fish in this shoal
            //

            auto & shoal = mFishShoals[freeShoalIndex];
            auto const & species = shoal.Species;

            float const shoalSizeVarianceFactor = species.ShoalRadius * simulationParameters.FishShoalRadiusAdjustment * mCurrentFishSizeMultiplier / 2.0f;

            vec2f const initialPosition = FindPosition(
                shoal.InitialPosition,
                species.WorldSize.x * shoalSizeVarianceFactor,
                species.WorldSize.y * shoalSizeVarianceFactor,
                oceanFloor,
                aabbSet);

            vec2f const targetPosition = FindNewCruisingTargetPosition(
                initialPosition,
                shoal.InitialDirection,
                species,
                visibleWorld);

            float const headOffset = species.WorldSize.x * mCurrentFishSizeMultiplier * (species.HeadOffsetX - 0.5f);

            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            TextureFrameIndex const renderTextureFrameIndex = static_cast<TextureFrameIndex>(
                GameRandomEngine::GetInstance().Choose(species.RenderTextureFrameIndices.size()));

            mFishes.emplace_back(
                freeShoalIndex,
                personalitySeed,
                initialPosition,
                targetPosition,
                MakeCruisingVelocity((targetPosition - initialPosition).normalise(), species, personalitySeed, simulationParameters),
                headOffset,
                GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 2.0f * Pi<float>), // initial progress phase
                TextureFrameId<GameTextureDatabases::FishTextureGroups>(
                    GameTextureDatabases::FishTextureGroups::Fish,
                    species.RenderTextureFrameIndices[renderTextureFrameIndex]));

            // Update shoal
            ++(shoal.CurrentMemberCount);
        }

        isDirty = true;
    }

    if (isDirty)
    {
        // Notify new count
        mSimulationEventHandler.OnFishCountUpdated(mFishes.size());
    }
}

void Fishes::UpdateInteractions(SimulationParameters const & simulationParameters)
{
    auto const now = GameWallClock::GetInstance().Now();

    for (auto it = mInteractions.begin(); it != mInteractions.end(); /* incremented in loop */)
    {
        if (now >= it->StartTime)
        {
            //
            // Enact this disturbance
            //

            switch (it->Type)
            {
                case Interaction::InteractionType::Attraction:
                {
                    assert(it->Area.has_value());

                    EnactAttraction(
                        it->Area->Position,
                        it->Area->Radius,
                        simulationParameters);

                    break;
                }

                case Interaction::InteractionType::Disturbance:
                {
                    if (it->Area.has_value())
                    {
                        EnactDisturbance(
                            it->Area->Position,
                            it->Area->Radius,
                            simulationParameters);
                    }
                    else
                    {
                        EnactWidespreadPanic(simulationParameters);
                    }

                    break;
                }
            }

            it = mInteractions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Fishes::UpdateDynamics(
    float currentSimulationTime,
    OceanSurface & oceanSurface,
    OceanFloor const & oceanFloor,
    Geometry::AABBSet const & aabbSet,
    SimulationParameters const & simulationParameters,
    VisibleWorld const & visibleWorld)
{
    float constexpr OceanSurfaceLowWatermark = 3.0f;

    float const outOfWaterVelocityAmplification = (1.0f + std::max(5.0f - mCurrentFishSpeedAdjustment, 0.0f)); // 5 at adj==1

    ElementCount const fishCount = static_cast<ElementCount>(mFishes.size());
    for (ElementIndex f = 0; f < fishCount; ++f)
    {
        Fish & fish = mFishes[f];
        FishShoal const & fishShoal = mFishShoals[fish.ShoalId];
        FishSpecies const & fishSpecies = fishShoal.Species;

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
                if (elapsedSteeringDurationFraction <= 0.5f)
                {
                    fish.CurrentRenderVector.y =
                        fish.CruiseSteeringState->StartRenderVector.y
                        * (1.0f - 2.0f * SmoothStep(0.0f, 1.0f, elapsedSteeringDurationFraction));
                }
                else
                {

                    fish.CurrentRenderVector.y =
                        targetRenderVector.y
                        * (1.0f - 2.0f * SmoothStep(0.0f, 1.0f, 1.0f - elapsedSteeringDurationFraction));
                }

                // RenderVector X:
                // - smooth towards target during a central interval (actual turning around),
                //   without crossing zero
                float constexpr TimeMargin = 0.15f; // Time of start of the turn
                float constexpr TurnLimit = 0.05f; // Minimum multiplier of render vector X - not going to zero
                if (elapsedSteeringDurationFraction <= 0.5f)
                {
                    fish.CurrentRenderVector.x =
                        fish.CruiseSteeringState->StartRenderVector.x
                        * (1.0f - (1.0f - TurnLimit) * 2.0f * SmoothStep(TimeMargin, 1.0f - TimeMargin, elapsedSteeringDurationFraction));
                }
                else
                {
                    fish.CurrentRenderVector.x =
                        targetRenderVector.x
                        * (1.0f - (1.0f - TurnLimit) * 2.0f * SmoothStep(TimeMargin, 1.0f - TimeMargin, 1.0f - elapsedSteeringDurationFraction));
                }
            }
        }
        else
        {
            //
            // Automated direction smoothing
            //

            if (!fish.IsInFreefall) // If we're free-falling, current velocity has already converged towards target velocity
            {
                // Smooth velocity towards target + shoaling
                fish.CurrentVelocity +=
                    ((fish.TargetVelocity + fish.ShoalingVelocity) - fish.CurrentVelocity) * fish.CurrentDirectionSmoothingConvergenceRate;
            }

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

        float constexpr OceanSurfaceDisturbanceMagnitude = 8.0f; // Magic number

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
            oceanSurface.DisplaceAt(fish.CurrentPosition.x, OceanSurfaceDisturbanceMagnitude);
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
            oceanSurface.DisplaceAt(fish.CurrentPosition.x, OceanSurfaceDisturbanceMagnitude);
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
                * SimulationParameters::SimulationStepTimeDuration<float>
                * speedMultiplier;

            // Update tail progress phase: add basal speed
            fish.CurrentTailProgressPhase += fishSpecies.TailSpeed * speedMultiplier * simulationParameters.FishSpeedAdjustment;

            // Update position: superimpose a small sin component, unless we're steering
            if (!fish.CruiseSteeringState.has_value())
            {
                fish.CurrentPosition +=
                    fish.CurrentRenderVector
                    * (1.0f + std::sinf(2.0f * fish.CurrentTailProgressPhase))
                    * (1.0f + fish.PanicCharge) // Grow incisiveness with panic
                    / 150.0f; // Magic number
            }
        }
        else
        {
            //
            // Free-falling
            //

            // Update velocity with gravity
            float const newVelocityY = fish.CurrentVelocity.y
                - 2.0f // Magnification factor
                * SimulationParameters::GravityMagnitude
                * SimulationParameters::SimulationStepTimeDuration<float>;

            fish.TargetVelocity = vec2f(
                fish.CurrentVelocity.x,
                newVelocityY);

            fish.CurrentVelocity = fish.TargetVelocity; // Converge immediately

            // Converge direction at this rate, overriding current convergence rate
            fish.CurrentDirectionSmoothingConvergenceRate = 0.06f;

            // Update position: add velocity
            fish.CurrentPosition +=
                fish.CurrentVelocity
                * SimulationParameters::SimulationStepTimeDuration<float>
                * outOfWaterVelocityAmplification;

            // Update tail progress phase: add extra speed (fish flapping its tail)
            fish.CurrentTailProgressPhase += fishSpecies.TailSpeed * 20.0f;
        }

        // Decay panic charge
        fish.PanicCharge *= 0.985f;

        // Decay attraction timer
        fish.AttractionDecayTimer *= 0.75f;

        ///////////////////////////////////////////////////////////////////
        // 3) World boundaries check
        ///////////////////////////////////////////////////////////////////

        bool hasBouncedAgainstWorldBoundaries = false;

        if (fish.CurrentPosition.x < -SimulationParameters::HalfMaxWorldWidth)
        {
            // Bounce position
            fish.CurrentPosition.x = -SimulationParameters::HalfMaxWorldWidth + (-SimulationParameters::HalfMaxWorldWidth - fish.CurrentPosition.x);

            // Bounce both current and target velocity
            fish.CurrentVelocity.x = std::abs(fish.CurrentVelocity.x);
            fish.TargetVelocity.x = std::abs(fish.TargetVelocity.x);

            // Adjust other fish properties
            hasBouncedAgainstWorldBoundaries = true;
        }
        else if (fish.CurrentPosition.x > SimulationParameters::HalfMaxWorldWidth)
        {
            // Bounce position
            fish.CurrentPosition.x = SimulationParameters::HalfMaxWorldWidth - (fish.CurrentPosition.x - SimulationParameters::HalfMaxWorldWidth);

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
                fishSpecies,
                visibleWorld);

            // Stop cruising, in case we were cruising
            fish.CruiseSteeringState.reset();

            // Skip everything else
            continue;
        }

        assert(fish.CurrentPosition.x >= -SimulationParameters::HalfMaxWorldWidth
            && fish.CurrentPosition.x <= SimulationParameters::HalfMaxWorldWidth);

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
                fishSpecies,
                visibleWorld);

            // Calculate new target velocity
            fish.TargetVelocity = MakeCruisingVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), fishSpecies, fish.PersonalitySeed, simulationParameters);

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
            fish.TargetVelocity = MakeCruisingVelocity((fish.TargetPosition - fish.CurrentPosition).normalise(), fishSpecies, fish.PersonalitySeed, simulationParameters);

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

        // Calculate depth of fish head
        float const fishHeadDepth = oceanY - fishHeadPosition.y;

        // Check whether we're too close to the water surface (idealized as being horizontal) - but only if fish is not in too much panic
        if (fishHeadDepth < 2.0f + OceanSurfaceLowWatermark
            && fish.PanicCharge <= 0.3f // Not too much panic
            && fish.TargetVelocity.y >= 0.0f) // Bounce away only if we're really going into it
        {
            //
            // OceanSurface Bounce
            //

            // Bounce direction, opposite of target
            vec2f const bounceDirection = vec2f(fish.TargetVelocity.x, -fish.TargetVelocity.y).normalise();

            // Calculate new target velocity - along bounce direction
            fish.TargetVelocity = MakeCruisingVelocity(bounceDirection, fishSpecies, fish.PersonalitySeed, simulationParameters);

            // Converge direction change at this rate
            fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                0.05f * (1.0f + fish.PanicCharge),
                fish.CurrentDirectionSmoothingConvergenceRate);
        }

        // Check ocean floor collision
        float const clampedX = Clamp(fishHeadPosition.x, -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth);
        if (auto const [isUnderneathFloor, oceanFloorHeight, oceanFloorIndexI] = oceanFloor.GetHeightIfUnderneathAt(clampedX, fishHeadPosition.y);
            isUnderneathFloor // fishHeadPosition.y < oceanFloorHeight
            && fishHeadDepth > fishShoal.MaxWorldDimension * 2.0f)
        {
            //
            // Ocean floor collision
            //

            // Calculate sea floor normal (positive points up, out)
            vec2f const seaFloorNormal = oceanFloor.GetNormalAt(oceanFloorIndexI);

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

        //if (fish.PanicCharge <= 0.3f) // Only if we're not in panic
        if (fish.PanicCharge <= 0.1f) // Only if we're not in panic
        {
            for (auto const & aabb : aabbSet.GetItems())
            {
                float const lMargin = fishHeadPosition.x - (aabb.BottomLeft.x - AABBMargin);
                float const rMargin = (aabb.TopRight.x + AABBMargin) - fishHeadPosition.x;
                float const tMargin = (aabb.TopRight.y + AABBMargin) - fishHeadPosition.y;
                float const bMargin = fishHeadPosition.y - (aabb.BottomLeft.y - AABBMargin);

                if (lMargin >= 0.0f && rMargin >= 0.0f && tMargin >= 0.0f && bMargin >= 0.0f)
                {
                    // Fish head is in AABB (plus margin)...
                    // ...find to which side of the AABB it's closest

                    vec2f outwardNormal;
                    if (std::min(lMargin, rMargin) < std::min(bMargin, tMargin))
                    {
                        // Vertical axes
                        outwardNormal = vec2f(
                            lMargin < rMargin ? -1.0f : 1.0f,
                            0.0f);
                    }
                    else
                    {
                        // Horizontal axes
                        outwardNormal = vec2f(
                            0.0f,
                            bMargin < tMargin ? -1.0f : 1.0f);
                    }

                    // Rotate target velocity towards normal
                    float const targetVelocityMagnitude = fish.TargetVelocity.length();
                    fish.TargetVelocity =
                        (fish.TargetVelocity.normalise(targetVelocityMagnitude) + outwardNormal * 2.0f).normalise()
                        * targetVelocityMagnitude;

                    // Converge direction change at a fast rate
                    fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                        0.15f,
                        fish.CurrentDirectionSmoothingConvergenceRate);

                    // Panic a bit
                    fish.PanicCharge = std::max(
                        0.5f,
                        fish.PanicCharge);

                    // Stop steering, if we're steering
                    fish.CruiseSteeringState.reset();
                }
            }
        }
    }
}

void Fishes::UpdateShoaling(
    float currentSimulationTime,
    SimulationParameters const & simulationParameters,
    VisibleWorld const & visibleWorld)
{
    // Visit all shoals
    for (auto const & fishShoal : mFishShoals)
    {
        // Calculate shoal radius for this shoal in world coordinates
        float const shoalRadius =
            fishShoal.Species.ShoalRadius
            * simulationParameters.FishShoalRadiusAdjustment
            * fishShoal.MaxWorldDimension; // Inclusive of FishSizeMultiplier

        // Visit all fishes in this shoal
        ElementIndex const endFishIndex = fishShoal.StartFishIndex + fishShoal.CurrentMemberCount;
        for (ElementIndex f = fishShoal.StartFishIndex; f < endFishIndex; ++f)
        {
            Fish & fish = mFishes[f];

            if (fishShoal.CurrentMemberCount > 1 // A shoal contains at least one fish
                && fish.ShoalingTimer <= 0.0f // Wait for this fish's shoaling cycle
                && fish.PanicCharge < 0.02f) // Skip fishes even in little panic
            {
                if (!fish.CruiseSteeringState.has_value() // Fish is not u-turning
                    && !fish.IsInFreefall) // Fish is swimming
                {
                    // Calculate shoal radius for this fish in world coordinates
                    float const fishShoalRadius = shoalRadius + fish.PersonalitySeed; // Add some randomness to prevent regular patterns

                    // Calculate shoal spacing as fraction of shoal radius
                    float const fishShoalSpacing = 0.7f * fishShoalRadius;

                    //
                    // Visit all fishes in same shoal looking for neighbors
                    //

                    ElementIndex closestFishIndex = NoneElementIndex; // Closest neighbour among those that are closer to fish than spacing
                    float closestFishDistance = std::numeric_limits<float>::max();
                    ElementIndex furthestFishIndex = NoneElementIndex; // Furthest neighbour among those that are further from fish than spacing
                    float furthestFishDistance = std::numeric_limits<float>::lowest();

                    for (ElementIndex n = fishShoal.StartFishIndex; n < endFishIndex; ++n)
                    {
                        assert(mFishes[n].ShoalId == fish.ShoalId);
                        if (n != f) // Not same fish
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
                                    fish.TargetVelocity = MakeCruisingVelocity(neighborDirection, fishShoal.Species, fish.PersonalitySeed, simulationParameters);

                                    // Perform a cruise steering
                                    fish.CruiseSteeringState.emplace(
                                        fish.CurrentVelocity,
                                        fish.CurrentRenderVector,
                                        currentSimulationTime,
                                        UTurnSpeed);

                                    // Remember the time at which we did the last steering
                                    fish.LastSteeringSimulationTime = currentSimulationTime;

                                    // No need to look at other neighbors
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
                            fish.TargetVelocity = MakeCruisingVelocity(fishToLeadDirection, fishShoal.Species, fish.PersonalitySeed, simulationParameters);

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
                            * simulationParameters.FishSpeedAdjustment;

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
                            * simulationParameters.FishSpeedAdjustment;
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
            fish.ShoalingTimer -= SimulationParameters::SimulationStepTimeDuration<float>;
        }
    }
}

void Fishes::EnactDisturbance(
    vec2f const & worldCoordinates,
    float worldRadius,
    SimulationParameters const & simulationParameters)
{
    float const effectiveRadius =
        worldRadius
        * (simulationParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto & fish : mFishes)
    {
        if (!fish.IsInFreefall)
        {
            FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

            // Calculate position of head
            vec2f const fishHeadPosition =
                fish.CurrentPosition
                + fish.CurrentRenderVector.normalise() * fish.HeadOffset;

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
                fish.TargetVelocity = MakeCruisingVelocity(panicDirection, species, fish.PersonalitySeed, simulationParameters);

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

void Fishes::EnactAttraction(
    vec2f const & worldCoordinates,
    float worldRadius,
    SimulationParameters const & simulationParameters)
{
    float const effectiveRadius =
        worldRadius
        * (simulationParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    for (auto & fish : mFishes)
    {
        if (!fish.IsInFreefall
            && fish.PanicCharge < 0.65f) // Don't attract fish in much panic
        {
            FishSpecies const & species = mFishShoals[fish.ShoalId].Species;

            // Calculate position of head
            vec2f const fishHeadPosition =
                fish.CurrentPosition
                + fish.CurrentRenderVector.normalise() * fish.HeadOffset;

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
                fish.TargetVelocity = MakeCruisingVelocity(panicDirection, species, fish.PersonalitySeed, simulationParameters);

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

void Fishes::EnactWidespreadPanic(SimulationParameters const & simulationParameters)
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
            fish.TargetVelocity = MakeCruisingVelocity(panicDirection, species, fish.PersonalitySeed, simulationParameters);

            // Converge directions at this rate
            fish.CurrentDirectionSmoothingConvergenceRate = std::max(
                0.15f,
                fish.CurrentDirectionSmoothingConvergenceRate);

            // Stop u-turn, if any
            fish.CruiseSteeringState.reset();
        }
    }
}

vec2f Fishes::ChoosePosition(
    vec2f const & averagePosition,
    float xVariance,
    float yVariance)
{
    float const positionX = Clamp(
        GameRandomEngine::GetInstance().GenerateNormalReal(averagePosition.x, xVariance),
        -SimulationParameters::HalfMaxWorldWidth,
        SimulationParameters::HalfMaxWorldWidth);

    float const positionY =
        -5.0f // Min depth
        - std::abs(GameRandomEngine::GetInstance().GenerateNormalReal(averagePosition.y, yVariance));

    return vec2f(positionX, positionY);
}

vec2f Fishes::FindPosition(
    vec2f const & averagePosition,
    float xVariance,
    float yVariance,
    OceanFloor const & oceanFloor,
    Geometry::AABBSet const & aabbSet)
{
    vec2f position;

    // Try a few times without hitting boundaries
    for (int i = 0; i < 10; ++i)
    {
        position = ChoosePosition(averagePosition, xVariance, yVariance);

        assert(position.x >= -SimulationParameters::HalfMaxWorldWidth
                && position.x <= SimulationParameters::HalfMaxWorldWidth);

        if (!aabbSet.Contains(position, AABBMargin)
            && oceanFloor.GetHeightAt(position.x) < position.y)
        {
            // Passes all tests
            break;
        }
    }

    return position;
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

vec2f Fishes::MakeCruisingVelocity(
    vec2f const & direction,
    FishSpecies const & species,
    float personalitySeed,
    SimulationParameters const & simulationParameters)
{
    return direction
        * (species.BasalSpeed * simulationParameters.FishSpeedAdjustment * simulationParameters.FishSizeMultiplier)
        * (0.7f + personalitySeed * 0.3f);
}

}