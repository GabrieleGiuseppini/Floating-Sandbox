/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "GameParameters.h"
#include "OceanFloorTerrain.h"

#include <GameCore/GameMath.h>
#include <GameCore/UniqueBuffer.h>

#include <memory>
#include <optional>

namespace Physics
{

class OceanFloor
{
public:

    OceanFloor(OceanFloorTerrain && terrain);

    OceanFloorTerrain const & GetTerrain() const
    {
        return mTerrain;
    }

    void SetTerrain(OceanFloorTerrain const & terrain);

    void Update(GameParameters const & gameParameters);

    void Upload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    std::optional<bool> AdjustTo(
        float x1,
        float targetY1,
        float x2,
        float targetY2);

    void DisplaceAt(
        float x,
        float yOffset);

    /*
     * Assumption: x is in world boundaries.
     */
    inline float GetHeightAt(float x) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth && x <= GameParameters::HalfMaxWorldWidth);

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

    /*
     * Assumption: x is in world boundaries.
     */
    inline vec2f GetNormalAt(float x) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth && x <= GameParameters::HalfMaxWorldWidth);

        //
        // Find sample index and use delta from next sample
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return vec2f(
            -mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue,
            Dx).normalise();
    }

    /*
     * Assumption: x is in world boundaries.
     */
    inline bool IsUnderOceanFloor(float x, float y) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth && x <= GameParameters::HalfMaxWorldWidth);

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);
        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        // Rough check (we allocate an extra sample just for this)
        if (y > mSamples[sampleIndexI].SampleValue && y > mSamples[sampleIndexI + 1].SampleValue)
        {
            return false;
        }

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        float const sampleValue =
            mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;

        return y < sampleValue;
    }

private:

    void SetTerrainHeight(
        size_t sampleIndex,
        float terrainHeight);

    void CalculateBumpProfile();

    void CalculateResultantSampleValues();

    inline float CalculateResultantSampleValue(size_t sampleIndex) const
    {
        assert(sampleIndex < SamplesCount);

        return
            -mCurrentSeaDepth
            + mBumpProfile[sampleIndex]
            + mTerrain[sampleIndex] * mCurrentOceanFloorDetailAmplification;
    }

private:

    // The bump profile (ondulating component seafloor);
    // one value for each sample
    unique_buffer<float> mBumpProfile;

    // The terrain (user-provided component of seafloor);
    // one value for each sample
    OceanFloorTerrain mTerrain;

    //
    // Pre-calculated samples, i.e. world y of ocean floor at the sample's x
    //

    // The number of samples
    static constexpr size_t SamplesCount = GameParameters::OceanFloorTerrainSamples<size_t>;

    // The x step of the samples
    static constexpr float Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount - 1);

    // What we store for each sample
    struct Sample
    {
        float SampleValue;
        float SampleValuePlusOneMinusSampleValue; // Delta w/next
    };

    // The current samples, calculated from the components
    std::unique_ptr<Sample[]> mSamples;

    //
    // The game parameters for which we're current
    //

    float mCurrentSeaDepth;
    float mCurrentOceanFloorBumpiness;
    float mCurrentOceanFloorDetailAmplification;
};

}
