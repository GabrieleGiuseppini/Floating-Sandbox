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

    size_t ShoalSize;
    float OceanDepth;
    float BasalSpeed;

    float TailX; // Normalized coordinates (bottom-left origin)
    float TailSpeed; // Radians
    float TailSwingWidth; // Radians

    vec2f HeadOffset; // World coordinates (center origin)

    TextureFrameIndex RenderTextureFrameIndex;

    FishSpecies(
        std::string const & name,
        size_t shoalSize,
        float oceanDepth,
        float basalSpeed,
        float tailX,
        float tailSpeed,
        float tailSwingWidth,
        vec2f headOffset,
        TextureFrameIndex renderTextureFrameIndex)
        : Name(name)
        , ShoalSize(shoalSize)
        , OceanDepth(oceanDepth)
        , BasalSpeed(basalSpeed)
        , TailX(tailX)
        , TailSpeed(tailSpeed)
        , TailSwingWidth(tailSwingWidth)
        , HeadOffset(headOffset)
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
