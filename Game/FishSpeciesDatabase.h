/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-10-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "ResourceLocator.h"

#include <GameCore/GameTypes.h>

#include <string>
#include <vector>

struct FishSpecies
{
    std::string Name;

    size_t ShoalSize;
    float BasalDepth;
    float BasalSpeed;
    float TailX; // In texture (normalized) coordinates

    TextureFrameIndex RenderTextureFrameIndex;

    FishSpecies(
        std::string const & name,
        size_t shoalSize,
        float basalDepth,
        float basalSpeed,
        float tailX,
        TextureFrameIndex renderTextureFrameIndex)
        : Name(name)
        , ShoalSize(shoalSize)
        , BasalDepth(basalDepth)
        , BasalSpeed(basalSpeed)
        , TailX(tailX)
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
