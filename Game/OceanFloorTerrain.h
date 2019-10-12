/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "GameParameters.h"

#include <GameCore/UniqueBuffer.h>

#include <filesystem>

namespace Physics
{

class OceanFloorTerrain
{
public:

    static OceanFloorTerrain LoadFromImage(std::filesystem::path const & imageFilePath);

public:

    explicit OceanFloorTerrain(unique_buffer<float> && terrainBuffer)
        : mTerrainBuffer(std::move(terrainBuffer))
    {
        assert(terrainBuffer.size() == GameParameters::OceanFloorTerrainSamples);
    }

    OceanFloorTerrain(OceanFloorTerrain const & other) = default;
    OceanFloorTerrain(OceanFloorTerrain && other) = default;

    OceanFloorTerrain & operator=(OceanFloorTerrain const & other) = default;
    OceanFloorTerrain & operator=(OceanFloorTerrain && other) = default;

    bool operator==(OceanFloorTerrain const & other) const
    {
        assert(other.mTerrainBuffer.size() == GameParameters::OceanFloorTerrainSamples);

        return (mTerrainBuffer == other.mTerrainBuffer);
    }

    float const * data() const
    {
        return mTerrainBuffer.get();
    }

    size_t size() const
    {
        return mTerrainBuffer.size();
    }

private:

    unique_buffer<float> mTerrainBuffer;
};

}
