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

    bool AdjustTo(
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
    float GetHeightAt(float x) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth
            && x <= GameParameters::HalfMaxWorldWidth + 0.01f); // Allow for derivative taking

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        int64_t const sampleIndexI = FastTruncateInt64(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
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
    static constexpr int64_t SamplesCount = GameParameters::OceanFloorTerrainSamples<int64_t>;

    // The x step of the samples
    static constexpr float Dx = GameParameters::MaxWorldWidth / GameParameters::OceanFloorTerrainSamples<float>;

    // What we store for each sample
    struct Sample
    {
        float SampleValue;
        float SampleValuePlusOneMinusSampleValue; // Delta w/next
    };

    // The current samples (plus 1 to account for x==MaxWorldWidth),
    // derived from the components
    std::unique_ptr<Sample[]> mSamples;

    //
    // The game parameters for which we're current
    //

    float mCurrentSeaDepth;
    float mCurrentOceanFloorBumpiness;
    float mCurrentOceanFloorDetailAmplification;
};

}
