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

    bool IsDirty() const
    {
        return mIsDirty;
    }

    void Update(SimulationParameters const & simulationParameters);

    void UpdateEnd();

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
     * Gets the height of the ocean floor, measured from the silt layer. Guaranteed to be above or equal to the bedrock layer.
     * Assumption: x is in world boundaries.
     */
    inline float GetSiltHeightAt(float x) const noexcept
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

        return mSamples[sampleIndexI].SiltSampleValue
            + mSamples[sampleIndexI].SiltSampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

    /*
     * Assumption: x is within world boundaries.
     */
    inline std::tuple<bool, float, register_int> GetSiltHeightIfUnderneathAt(float x, float y) const noexcept
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
        if (y > mSamples[sampleIndexI].SiltSampleValue && y > mSamples[sampleIndexI + 1].SiltSampleValue)
        {
            return std::make_tuple(false, 0.0f, sampleIndexI);
        }

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;
        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        float const sampleValue =
            mSamples[sampleIndexI].SiltSampleValue
            + mSamples[sampleIndexI].SiltSampleValuePlusOneMinusSampleValue * sampleIndexDx;

        return std::make_tuple(y < sampleValue, sampleValue, sampleIndexI);
    }

    /*
     * Assumption: x is within world boundaries.
     */
    inline std::tuple<float, float, register_int> GetHeightsIfUnderneathAt(float x, float y) const noexcept
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

        // Rough check against silt (we allocate an extra sample just for this): if we're above silt,
        // we are above bedrock as well
        if (y > mSamples[sampleIndexI].SiltSampleValue && y > mSamples[sampleIndexI + 1].SiltSampleValue)
        {
            return std::make_tuple(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), sampleIndexI);
        }

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;
        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        float const siltSampleValue =
            mSamples[sampleIndexI].SiltSampleValue
            + mSamples[sampleIndexI].SiltSampleValuePlusOneMinusSampleValue * sampleIndexDx;

        float const bedrockSampleValue =
            mSamples[sampleIndexI].BedrockSampleValue
            + mSamples[sampleIndexI].BedrockSampleValuePlusOneMinusSampleValue * sampleIndexDx;

        return std::make_tuple(siltSampleValue, bedrockSampleValue, sampleIndexI);
    }

    /*
     * Assumption: x is within world boundaries.
     */
    inline vec2f GetSiltNormalAt(register_int sampleIndexI) const noexcept
    {
        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return vec2f(
            -mSamples[sampleIndexI].SiltSampleValuePlusOneMinusSampleValue,
            Dx).normalise_approx();
    }

    /*
     * Assumption: x is within world boundaries.
     */
    inline vec2f GetBedrockNormalAt(register_int sampleIndexI) const noexcept
    {
        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return vec2f(
            -mSamples[sampleIndexI].BedrockSampleValuePlusOneMinusSampleValue,
            Dx).normalise_approx();
    }

private:

    void SetTerrainHeight(
        size_t sampleIndex,
        float terrainHeight);

    void CalculateBumpProfile();

    void CalculateResultantSampleValues();

    inline float CalculateResultantBedrockSampleValue(size_t sampleIndex) const
    {
        assert(sampleIndex < SamplesCount);

        return
            -mCurrentSeaDepth
            + mBumpProfile[sampleIndex]
            + mHeightMap[sampleIndex] * mCurrentOceanFloorDetailAmplification;
    }

    void CalculateSiltSampleValues(size_t startIndex, size_t endIndex); // end included

    enum SegmentDirection
    {
        Upward = 0,
        Downward = 1,
        Horizontal = 2
    };

    inline SegmentDirection GetSegmentDirection(size_t sampleIndex) const;

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
    static size_t constexpr SamplesCount = SimulationParameters::OceanFloorTerrainSamples<size_t>;

    // The x step of the samples
    static float constexpr Dx = SimulationParameters::MaxWorldWidth / static_cast<float>(SamplesCount - 1);

    // What we store for each sample
    struct Sample
    {
        float SiltSampleValue;
        float SiltSampleValuePlusOneMinusSampleValue; // Delta w/next
        float BedrockSampleValue;
        float BedrockSampleValuePlusOneMinusSampleValue; // Delta w/next
    };

    // The current samples, calculated from the components
    std::unique_ptr<Sample[]> mSamples;

    // Whether the floor samples have changed in the current simulation step;
    // use for communication with other subsystems.
    // Cleared at end of each simulation step
    bool mIsDirty;

    //
    // Rendering
    //

    bool mutable mIsDirtyForRendering; // We only upload to the GPU when dirty

    //
    // The game parameters for which we're current
    //

    float mCurrentSeaDepth;
    float mCurrentOceanFloorSiltThickness;
    float mCurrentOceanFloorBumpiness;
    float mCurrentOceanFloorDetailAmplification;
};

}
