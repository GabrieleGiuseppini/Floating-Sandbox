/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/UniqueBuffer.h>

#include <filesystem>
#include <iostream>

/*
 * This class represents the user-modifiable component of the ocean floor.
 *
 * The class bridges between the physics and the settings infrastructure.
 */
class OceanFloorTerrain
{
public:

    static OceanFloorTerrain LoadFromImage(std::filesystem::path const & imageFilePath);

    static OceanFloorTerrain LoadFromStream(std::istream & is);

public:

    explicit OceanFloorTerrain(unique_buffer<float> const & terrainBuffer)
        : mTerrainBuffer(terrainBuffer)
    {
        assert(IsExpectedSize(mTerrainBuffer.size()));
    }

    explicit OceanFloorTerrain(unique_buffer<float> && terrainBuffer)
        : mTerrainBuffer(std::move(terrainBuffer))
    {
        assert(IsExpectedSize(mTerrainBuffer.size()));
    }

    OceanFloorTerrain(OceanFloorTerrain const & other) = default;
    OceanFloorTerrain(OceanFloorTerrain && other) = default;

    OceanFloorTerrain & operator=(OceanFloorTerrain const & other) = default;
    OceanFloorTerrain & operator=(OceanFloorTerrain && other) = default;

    bool operator==(OceanFloorTerrain const & other) const
    {
        return mTerrainBuffer == other.mTerrainBuffer;
    }

    inline float operator[](size_t index) const
    {
        return mTerrainBuffer[index];
    }

    inline float & operator[](size_t index)
    {
        return mTerrainBuffer[index];
    }

    void SaveToStream(std::ostream & os) const;

private:

    static bool IsExpectedSize(size_t size);

    unique_buffer<float> mTerrainBuffer;
};
