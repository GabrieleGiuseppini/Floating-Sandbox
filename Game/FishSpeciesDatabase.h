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

    float BasalSpeed;
    float TailX; // In texture (normalized) coordinates

    TextureFrameIndex RenderTextureFrameIndex;

    FishSpecies(
        std::string const & name,
        float basalSpeed,
        float tailX,
        TextureFrameIndex renderTextureFrameIndex)
        : Name(name)
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

private:

    explicit FishSpeciesDatabase(std::vector<FishSpecies> && fishSpecies)
        : mFishSpecies(std::move(fishSpecies))
    {}

    std::vector<FishSpecies> mFishSpecies;
};
