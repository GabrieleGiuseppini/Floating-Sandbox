/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-10-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "ResourceLocator.h"

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <string>
#include <vector>

struct FishSpecies
{
    std::string Name;

    vec2f WorldSize; // World coordinate units

    size_t ShoalSize;
    float OceanDepth;
    float BasalSpeed;

    float TailX; // Normalized coordinates (bottom-left origin) - fraction of WorldSize
    float TailSpeed; // Radians
    float TailSwingWidth; // Radians

    float HeadOffsetX; // Normalized coordinates (bottom-left origin) - fraction of WorldSize

    TextureFrameIndex RenderTextureFrameIndex;

    FishSpecies(
        std::string const & name,
        vec2f const & worldSize,
        size_t shoalSize,
        float oceanDepth,
        float basalSpeed,
        float tailX,
        float tailSpeed,
        float tailSwingWidth,
        float headOffsetX,
        TextureFrameIndex renderTextureFrameIndex)
        : Name(name)
        , WorldSize(worldSize)
        , ShoalSize(shoalSize)
        , OceanDepth(oceanDepth)
        , BasalSpeed(basalSpeed)
        , TailX(tailX)
        , TailSpeed(tailSpeed)
        , TailSwingWidth(tailSwingWidth)
        , HeadOffsetX(headOffsetX)
        , RenderTextureFrameIndex(renderTextureFrameIndex)
    {}
};

class FishSpeciesDatabase
{
public:

    // Only movable
    FishSpeciesDatabase(FishSpeciesDatabase const & other) = delete;
    FishSpeciesDatabase(FishSpeciesDatabase && other) = default;
    FishSpeciesDatabase & operator=(FishSpeciesDatabase const & other) = delete;
    FishSpeciesDatabase & operator=(FishSpeciesDatabase && other) = default;

    static FishSpeciesDatabase Load(ResourceLocator const & resourceLocator)
    {
        return Load(resourceLocator.GetFishSpeciesDatabaseFilePath());
    }

    static FishSpeciesDatabase Load(std::filesystem::path fishSpeciesDatabaseFilePath);

    size_t GetFishSpeciesCount() const
    {
        return mFishSpecies.size();
    }

    std::vector<FishSpecies> const & GetFishSpecies() const
    {
        return mFishSpecies;
    }

private:

    explicit FishSpeciesDatabase(std::vector<FishSpecies> && fishSpecies)
        : mFishSpecies(std::move(fishSpecies))
    {}

    std::vector<FishSpecies> mFishSpecies;
};
