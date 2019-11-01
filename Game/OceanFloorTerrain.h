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

    OceanFloorTerrain()
        : mTerrainBuffer(Size)
    {
        mTerrainBuffer.fill(0.0f);
    }

    explicit OceanFloorTerrain(unique_buffer<float> const & terrainBuffer)
        : mTerrainBuffer(terrainBuffer)
    {
        assert(mTerrainBuffer.size() == Size);
    }

    explicit OceanFloorTerrain(unique_buffer<float> && terrainBuffer)
        : mTerrainBuffer(std::move(terrainBuffer))
    {
        assert(mTerrainBuffer.size() == Size);
    }

    OceanFloorTerrain(OceanFloorTerrain const & other) = default;
    OceanFloorTerrain(OceanFloorTerrain && other) = default;

    OceanFloorTerrain & operator=(OceanFloorTerrain const & other) = default;
    OceanFloorTerrain & operator=(OceanFloorTerrain && other) = default;

    bool operator==(OceanFloorTerrain const & other) const
    {
        return mTerrainBuffer == other.mTerrainBuffer;
    }

	friend OceanFloorTerrain operator+(
		OceanFloorTerrain lhs,
		OceanFloorTerrain const & rhs)
	{
		lhs.mTerrainBuffer += rhs.mTerrainBuffer;

		return lhs;
	}

	friend OceanFloorTerrain operator-(
		OceanFloorTerrain lhs,
		OceanFloorTerrain const & rhs)
	{
		lhs.mTerrainBuffer -= rhs.mTerrainBuffer;

		return lhs;
	}

	friend OceanFloorTerrain operator*(
		OceanFloorTerrain lhs,
		float rhs)
	{
		lhs.mTerrainBuffer *= rhs;

		return lhs;
	}

	friend OceanFloorTerrain operator/(
		OceanFloorTerrain lhs,
		float rhs)
	{
		lhs.mTerrainBuffer /= rhs;

		return lhs;
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

    static size_t Size;

    unique_buffer<float> mTerrainBuffer;
};
