/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "../OceanFloorHeightMap.h"
#include "../SimulationParameters.h"

#include <Core/GameMath.h>
#include <Core/UniqueBuffer.h>

#include <memory>
#include <optional>

namespace Physics
{

class OceanFloor
{
public:

    OceanFloor(OceanFloorHeightMap && heightMap);

    OceanFloorHeightMap const & GetHeightMap() const
    {
        return mHeightMap;
    }

    void SetHeightMap(OceanFloorHeightMap const & heightMap);

    void Update(SimulationParameters const & simulationParameters);

    void Upload(
        SimulationParameters const & simulationParameters,
        RenderContext & renderContext) const;

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
        assert(x >= -SimulationParameters::HalfMaxWorldWidth && x <= SimulationParameters::HalfMaxWorldWidth);

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

    /*
     * Assumption: x is within world boundaries.
     */
    inline std::tuple<bool, float, register_int> GetHeightIfUnderneathAt(float x, float y) const noexcept
    {
        assert(x >= -SimulationParameters::HalfMaxWorldWidth && x <= SimulationParameters::HalfMaxWorldWidth);

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF);
        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        // Rough check (we allocate an extra sample just for this)
        if (y > mSamples[sampleIndexI].SampleValue && y > mSamples[sampleIndexI + 1].SampleValue)
        {
            return std::make_tuple(false, 0.0f, sampleIndexI);
        }

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;
        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        float const sampleValue =
            mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;

        return std::make_tuple(y < sampleValue, sampleValue, sampleIndexI);
    }

    /*
     * Assumption: x is within world boundaries.
     */
    inline vec2f GetNormalAt(register_int sampleIndexI) const noexcept
    {
        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return vec2f(
            -mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue,
            Dx).normalise_approx();
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
            + mHeightMap[sampleIndex] * mCurrentOceanFloorDetailAmplification;
    }

private:

    // The bump profile (ondulating component seafloor);
    // one value for each sample
    unique_buffer<float> mBumpProfile;

    // The terrain (user-provided component of seafloor);
    // one value for each sample
    OceanFloorHeightMap mHeightMap;

    //
    // Pre-calculated samples, i.e. world y of ocean floor at the sample's x
    //

    // The number of samples
    static constexpr size_t SamplesCount = SimulationParameters::OceanFloorTerrainSamples<size_t>;

    // The x step of the samples
    static constexpr float Dx = SimulationParameters::MaxWorldWidth / static_cast<float>(SamplesCount - 1);

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
