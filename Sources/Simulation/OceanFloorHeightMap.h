/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/ImageData.h>
#include <Core/Streams.h>
#include <Core/UniqueBuffer.h>

/*
 * This class represents the user-modifiable component of the ocean floor.
 *
 * The class is a value (data) object.
 */
class OceanFloorHeightMap
{
public:

    static OceanFloorHeightMap LoadFromImage(RgbImageData const & imageData);

    static OceanFloorHeightMap LoadFromStream(BinaryReadStream & inputStream);

public:

    OceanFloorHeightMap()
        : mTerrainBuffer(Size)
    {
        mTerrainBuffer.fill(0.0f);
    }

    explicit OceanFloorHeightMap(unique_buffer<float> const & terrainBuffer)
        : mTerrainBuffer(terrainBuffer)
    {
        assert(mTerrainBuffer.size() == Size);
    }

    explicit OceanFloorHeightMap(unique_buffer<float> && terrainBuffer)
        : mTerrainBuffer(std::move(terrainBuffer))
    {
        assert(mTerrainBuffer.size() == Size);
    }

    OceanFloorHeightMap(OceanFloorHeightMap const & other) = default;
    OceanFloorHeightMap(OceanFloorHeightMap && other) = default;

    OceanFloorHeightMap & operator=(OceanFloorHeightMap const & other) = default;
    OceanFloorHeightMap & operator=(OceanFloorHeightMap && other) = default;

    bool operator==(OceanFloorHeightMap const & other) const
    {
        return mTerrainBuffer == other.mTerrainBuffer;
    }

	friend OceanFloorHeightMap operator+(
        OceanFloorHeightMap lhs,
        OceanFloorHeightMap const & rhs)
	{
		lhs.mTerrainBuffer += rhs.mTerrainBuffer;

		return lhs;
	}

	friend OceanFloorHeightMap operator-(
        OceanFloorHeightMap lhs,
        OceanFloorHeightMap const & rhs)
	{
		lhs.mTerrainBuffer -= rhs.mTerrainBuffer;

		return lhs;
	}

	friend OceanFloorHeightMap operator*(
        OceanFloorHeightMap lhs,
		float rhs)
	{
		lhs.mTerrainBuffer *= rhs;

		return lhs;
	}

	friend OceanFloorHeightMap operator/(
        OceanFloorHeightMap lhs,
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

    void SaveToStream(BinaryWriteStream & outputStream) const;

private:

    static size_t Size;

    unique_buffer<float> mTerrainBuffer;
};
