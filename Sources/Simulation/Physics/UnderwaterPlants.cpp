/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/GameRandomEngine.h>
#include <Core/Log.h>
#include <Core/SysSpecifics.h>

#include <cassert>

namespace Physics {

UnderwaterPlants::UnderwaterPlants(size_t speciesCount)
    : mSpeciesCount(speciesCount)
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
    assert(mUnderwaterDepths.size() == mPlants.size());

    //
    // Update underwater depths
    //

    size_t const n = mPlants.size();
    for (size_t i = 0; i < n; ++i)
    {
        mUnderwaterDepths[i] = oceanSurface.GetHeightAt(mOceanSurfaceCoordinatesProxies[i]);
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

    if (mIsCurrentRotationAngleDirtyForRendering)
    {
        renderContext.UploadUnderwaterPlantRotationAngle(mCurrentRotationAngle);

        mIsCurrentRotationAngleDirtyForRendering = false;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

void UnderwaterPlants::RepopulatePlants(
    OceanSurface const & oceanSurface,
    OceanFloor const & oceanFloor,
    SimulationParameters const & simulationParameters)
{
    mPlants.clear();
    mOceanSurfaceCoordinatesProxies.clear();
    mUnderwaterDepths.clear();

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
    mUnderwaterDepths.reserve(plantCount);

    //
    // Populate
    //

    for (size_t iSpecies = 0; iSpecies < mSpeciesCount; ++iSpecies)
    {
        // Calculate number of plants that we want to create for this species
        size_t nPlants;
        if (iSpecies < mSpeciesCount - 1)
        {
            nPlants = (plantCount - mPlants.size()) / 2;
        }
        else
        {
            // Last species: do all remaining
            nPlants = plantCount - mPlants.size();
        }

        for (size_t p = 0; p < nPlants; ++p)
        {
            // Choose X
            float const x = GameRandomEngine::GetInstance().GenerateUniformReal(-SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth);

            // Choose basis scale
            float const basisScale = Clamp(
                GameRandomEngine::GetInstance().GenerateNormalReal(
                    1.0f, // Mean
                    0.5f), // StdDev
                0.25f,
                10.0f);

            // Choose personality seed
            float const personalitySeed = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();

            // Create plant
            mPlants.emplace_back(
                x,
                iSpecies,
                basisScale,
                personalitySeed,
                (mPlants.size() % 2) == 1); // IsSpecular

            // Calculate ocean surface proxy
            mOceanSurfaceCoordinatesProxies.emplace_back(oceanSurface.GetCoordinatesProxyAt(x));

            // Make room for underwater depths
            mUnderwaterDepths.emplace_back(0.0f);
        }
    }

    assert(mPlants.size() == plantCount);
    assert(mOceanSurfaceCoordinatesProxies.size() == plantCount);
    assert(mUnderwaterDepths.size() == plantCount);

    // Populate bottom Y's
    RecalculateBottomYs(oceanFloor);

    // Calculate sizes
    RecalculateScales(simulationParameters.UnderwaterPlantSizeMultiplier);
}

void UnderwaterPlants::RecalculateBottomYs(OceanFloor const & oceanFloor)
{
    for (auto & plant : mPlants)
    {
        plant.BottomY = oceanFloor.GetMinHeightAt(plant.CenterX) - 0.2f; // Cover roots underneath semi-transparent ocean floor
    }
}

void UnderwaterPlants::RecalculateScales(float sizeMultiplier)
{
    for (auto & plant : mPlants)
    {
        plant.Scale = plant.BasisScale * sizeMultiplier;
    }
}

}