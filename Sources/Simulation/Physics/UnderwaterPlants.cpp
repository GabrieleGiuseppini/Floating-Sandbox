/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/GameRandomEngine.h>
#include <Core/SysSpecifics.h>

#include <algorithm>
#include <cassert>

namespace Physics {

size_t constexpr MinPatchCount = 170;
size_t constexpr MaxPatchCount = 200;
static float constexpr PatchRadius = SimulationParameters::MaxWorldWidth / 250.0f; // Also serves as canary: if one day world size becomes a runtime property, this implementation will have to change
static float constexpr PatchStdDev = PatchRadius / 16.0f;
size_t constexpr UniformlyDistributedPercentage = 5;

UnderwaterPlants::UnderwaterPlants(size_t speciesCount)
    : mSpeciesCount(speciesCount)
    , mPatchLocii(GeneratePatchLocii())
    , mArePlantsDirtyForRendering(true)
    , mCurrentRotationAngle(0.0f)
    , mIsCurrentRotationAngleDirtyForRendering(true)
    , mCurrentDensity(0.0f)
    , mCurrentSizeMultiplier(0.0f)
    , mCurrentWindBaseSpeedMagnitude(0.0f)
{
}

void UnderwaterPlants::Update(
    float /*currentSimulationTime*/,
    Wind const & wind,
    OceanSurface const & oceanSurface,
    OceanFloor const & oceanFloor,
    SimulationParameters const & simulationParameters)
{
    //
    // React to parameter changes, if any
    //

    if (mCurrentDensity != simulationParameters.UnderwaterPlantsDensity)
    {
        RepopulatePlants(oceanSurface, oceanFloor, simulationParameters);

        mCurrentDensity = simulationParameters.UnderwaterPlantsDensity;
        mCurrentSizeMultiplier = simulationParameters.UnderwaterPlantSizeMultiplier;

        mArePlantsDirtyForRendering = true;
    }
    else
    {
        if (oceanFloor.IsDirty()) // Only do this if we haven't done it with plant re-population
        {
            RecalculateBottomYs(oceanFloor);

            mArePlantsDirtyForRendering = true;
        }

        if (mCurrentSizeMultiplier != simulationParameters.UnderwaterPlantSizeMultiplier)
        {
            RecalculateScales(simulationParameters.UnderwaterPlantSizeMultiplier);

            mCurrentSizeMultiplier = simulationParameters.UnderwaterPlantSizeMultiplier;

            mArePlantsDirtyForRendering = true;
        }
    }

    assert(mOceanSurfaceCoordinatesProxies.size() == mPlants.size());
    assert(mOceanDepths.size() == mPlants.size());

    //
    // Update ocean depths
    //

    size_t const n = mPlants.size();
    for (size_t i = 0; i < n; ++i)
    {
        mOceanDepths[i] = oceanSurface.GetHeightAt(mOceanSurfaceCoordinatesProxies[i]);
    }

    //
    // Update rotation angle
    //

    if (wind.GetBaseSpeedMagnitude() != mCurrentWindBaseSpeedMagnitude)
    {
        float const absWindSpeed = std::abs(wind.GetBaseSpeedMagnitude());

        mCurrentRotationAngle =
            Pi<float> / 2.0f
            * (0.05f + 0.0055f * absWindSpeed - 0.000025f * absWindSpeed * absWindSpeed);

        mCurrentWindBaseSpeedMagnitude = wind.GetBaseSpeedMagnitude();

        mIsCurrentRotationAngleDirtyForRendering = true;
    }
}

void UnderwaterPlants::Upload(RenderContext & renderContext)
{
    // Upload plant structures - only if they've changed

    if (mArePlantsDirtyForRendering)
    {
        renderContext.UploadUnderwaterPlantStaticVertexAttributesStart(mPlants.size());

        for (auto const & plant : mPlants)
        {
            renderContext.UploadUnderwaterPlantStaticVertexAttributes(
                vec2f(plant.CenterX, plant.BottomY),
                plant.SpeciesIndex,
                plant.Scale,
                plant.PersonalitySeed,
                plant.IsSpecular);
        }

        renderContext.UploadUnderwaterPlantStaticVertexAttributesEnd();

        mArePlantsDirtyForRendering = false;
    }

    // Upload plants' depths - always

    renderContext.UploadUnderwaterPlantOceanDepths(mOceanDepths);

    // Upload rotation angle - only if it has changed

    if (mIsCurrentRotationAngleDirtyForRendering)
    {
        renderContext.UploadUnderwaterPlantRotationAngle(mCurrentRotationAngle);

        mIsCurrentRotationAngleDirtyForRendering = false;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

std::vector<float> UnderwaterPlants::GeneratePatchLocii()
{
    // Choose number of patches

    size_t const nPatches = GameRandomEngine::GetInstance().GenerateUniformInteger(MinPatchCount, MaxPatchCount);

    // Generate patch locii

    float const maxWorldX = SimulationParameters::HalfMaxWorldWidth - PatchRadius; // Make sure patch locii are not too close to borders

    std::vector<float> locii;

    for (size_t n = 0; n < nPatches; ++n)
    {
        locii.emplace_back(GameRandomEngine::GetInstance().GenerateUniformReal(-maxWorldX, maxWorldX));
    }

    return locii;
}

void UnderwaterPlants::RepopulatePlants(
    OceanSurface const & oceanSurface,
    OceanFloor const & oceanFloor,
    SimulationParameters const & simulationParameters)
{
    assert(mSpeciesCount > 0);

    mPlants.clear();
    mOceanSurfaceCoordinatesProxies.clear();
    mOceanDepths.clear();

    //
    // Calculate plant count
    //

    size_t plantCount = static_cast<size_t>(simulationParameters.UnderwaterPlantsDensity * SimulationParameters::MaxWorldWidth / 1000.0f);
    if (plantCount == 0)
    {
        return;
    }

    // Round to ceil power of two, so we can use all species with our subdivision algorithm
    plantCount = std::max(plantCount, ceil_power_of_two(mSpeciesCount));

    LogMessage("Number of underwater plants: ", plantCount);

    mPlants.reserve(plantCount);
    mOceanSurfaceCoordinatesProxies.reserve(plantCount);
    mOceanDepths.reserve(plantCount);

    //
    // 1. Populate plants
    //
    // For each species, we generate a number of plants equal to alpha*remaining_plants.
    // Alpha is dimensioned so that the last species is guaranteed a fixed number of plants.
    //

    float const lastSpeciesPlantsCount = std::min(200.0f, static_cast<float>(plantCount));
    float const alpha = 1.0f - std::powf(lastSpeciesPlantsCount / static_cast<float>(plantCount), 1.0f / static_cast<float>(mSpeciesCount - 1));

    for (size_t iSpecies = 0; iSpecies < mSpeciesCount; ++iSpecies)
    {
        // Calculate number of plants that we want to create for this species
        size_t nPlants;
        if (iSpecies < mSpeciesCount - 1)
        {
            nPlants = static_cast<size_t>(static_cast<float>(plantCount - mPlants.size()) * alpha);
        }
        else
        {
            // Last species: do all remaining
            nPlants = plantCount - mPlants.size();
        }

        // X's are gaussian-centered on patches, but last x% of plants is uniformly distributed
        size_t firstUniformlyDistributedPlant = (nPlants * (100 - UniformlyDistributedPercentage)) / 100;

        for (size_t p = 0; p < nPlants; ++p)
        {
            // Choose X
            float x;
            if (p >= firstUniformlyDistributedPlant)
            {
                x = GameRandomEngine::GetInstance().GenerateUniformReal(
                    -SimulationParameters::HalfMaxWorldWidth,
                    SimulationParameters::HalfMaxWorldWidth);
            }
            else
            {
                x = GameRandomEngine::GetInstance().GenerateNormalReal(
                    mPatchLocii[mPlants.size() % mPatchLocii.size()],
                    PatchStdDev);

                if (x < -SimulationParameters::HalfMaxWorldWidth)
                {
                    x = -(std::fmod(-x, SimulationParameters::MaxWorldWidth) - SimulationParameters::MaxWorldWidth / 2.0f);
                }
                else if (x > SimulationParameters::HalfMaxWorldWidth)
                {
                    x = std::fmod(x, SimulationParameters::MaxWorldWidth) - SimulationParameters::MaxWorldWidth / 2.0f;
                }
            }

            // Choose basis scale
            float const basisScale = Clamp(
                GameRandomEngine::GetInstance().GenerateNormalReal(
                    1.0f, // Mean
                    0.5f), // StdDev
                0.5f,
                4.0f);

            // Choose personality seed
            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            // Create plant
            mPlants.emplace_back(
                x,
                CalculateBottomY(x, oceanFloor),
                iSpecies,
                basisScale,
                CalculateScale(basisScale, simulationParameters.UnderwaterPlantSizeMultiplier),
                personalitySeed,
                (mPlants.size() % 2) == 1); // IsSpecular
        }
    }

    assert(mPlants.size() == plantCount);

    // Sort plants so to achieve better cache locality when calculating ocean depths
    std::sort(
        mPlants.begin(),
        mPlants.end(),
        [](Plant const & p1, Plant const & p2) -> bool
        {
            return p1.CenterX < p2.CenterX;
        });

    //
    // 2. Populate auxiliary data structures
    //

    for (Plant const & plant : mPlants)
    {
        // Calculate ocean surface proxy
        mOceanSurfaceCoordinatesProxies.emplace_back(oceanSurface.GetCoordinatesProxyAt(plant.CenterX));

        // Make room for underwater depths
        mOceanDepths.emplace_back(0.0f);
    }

    assert(mOceanSurfaceCoordinatesProxies.size() == plantCount);
    assert(mOceanDepths.size() == plantCount);
}

void UnderwaterPlants::RecalculateBottomYs(OceanFloor const & oceanFloor)
{
    for (auto & plant : mPlants)
    {
        plant.BottomY = CalculateBottomY(plant.CenterX, oceanFloor);
    }
}

float UnderwaterPlants::CalculateBottomY(float x, OceanFloor const & oceanFloor)
{
    return oceanFloor.GetMinHeightAt(x) - 0.2f; // Cover roots underneath semi-transparent ocean floor
}

void UnderwaterPlants::RecalculateScales(float sizeMultiplier)
{
    for (auto & plant : mPlants)
    {
        plant.Scale = CalculateScale(plant.BasisScale, sizeMultiplier);
    }
}

float UnderwaterPlants::CalculateScale(float basisScale, float sizeMultiplier)
{
    return basisScale * sizeMultiplier;
}

}