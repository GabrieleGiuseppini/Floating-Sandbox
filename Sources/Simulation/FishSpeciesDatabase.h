/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-10-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/Vectors.h>

#include <picojson.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

struct FishSpecies
{
    std::string Name;

    vec2f WorldSize; // World coordinate units

    size_t ShoalSize;
    float ShoalRadius; // In "bodies"
    float OceanDepth;
    float BasalSpeed;

    float TailX; // Normalized coordinates (bottom-left origin) - fraction of WorldSize
    float TailSpeed; // Radians
    float TailSwingWidth; // Radians

    float HeadOffsetX; // Normalized coordinates (bottom-left origin) - fraction of WorldSize

    std::vector<TextureFrameIndex> RenderTextureFrameIndices;

    FishSpecies(
        std::string const & name,
        vec2f const & worldSize,
        size_t shoalSize,
        float shoalRadius,
        float oceanDepth,
        float basalSpeed,
        float tailX,
        float tailSpeed,
        float tailSwingWidth,
        float headOffsetX,
        std::vector<TextureFrameIndex> const & renderTextureFrameIndices)
        : Name(name)
        , WorldSize(worldSize)
        , ShoalSize(shoalSize)
        , ShoalRadius(shoalRadius)
        , OceanDepth(oceanDepth)
        , BasalSpeed(basalSpeed)
        , TailX(tailX)
        , TailSpeed(tailSpeed)
        , TailSwingWidth(tailSwingWidth)
        , HeadOffsetX(headOffsetX)
        , RenderTextureFrameIndices(renderTextureFrameIndices)
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

    static FishSpeciesDatabase Load(IAssetManager const & assetManager);

    size_t GetFishSpeciesCount() const
    {
        return mFishSpecies.size();
    }

    std::vector<FishSpecies> const & GetFishSpecies() const
    {
        return mFishSpecies;
    }

    size_t GetFishSpeciesIndex(FishSpecies const & species) const
    {
        auto const findIt = std::find_if(
            mFishSpecies.cbegin(),
            mFishSpecies.cend(),
            [&species](auto const & fs)
            {
                return fs.Name == species.Name;
            });

        assert(findIt != mFishSpecies.cend());

        return std::distance(mFishSpecies.cbegin(), findIt);
    }

private:

    explicit FishSpeciesDatabase(std::vector<FishSpecies> && fishSpecies)
        : mFishSpecies(std::move(fishSpecies))
    {}

    std::vector<FishSpecies> mFishSpecies;
};
