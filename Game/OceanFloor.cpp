/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

OceanFloor::OceanFloor(ResourceLoader & resourceLoader)
    : mSamples(new Sample[SamplesCount])
    , mBumpMapSamples(new float[SamplesCount])
    , mCurrentSeaDepth(std::numeric_limits<float>::lowest())
    , mCurrentOceanFloorBumpiness(std::numeric_limits<float>::lowest())
    , mCurrentOceanFloorDetailAmplification(std::numeric_limits<float>::lowest())
{
    //
    // Pre-process bump map:
    // - Load bump map image
    // - Convert each (topmost) y of the map into a Y coordinate, between H/2 (top) and -H/2 (bottom)
    //

    ImageData bumpMapImage = ImageFileTools::LoadImageRgbUpperLeft(resourceLoader.GetOceanFloorBumpMapFilepath());

    float const sampleIndexToX = static_cast<float>(bumpMapImage.Size.Width) / static_cast<float>(SamplesCount);
    float const halfHeight = static_cast<float>(bumpMapImage.Size.Height / 2);

    for (size_t s = 0; s < SamplesCount; ++s)
    {
        // Worst case, sample is zero
        float bumpMapSampleValue = 0.0f;

        // Calculate image X for this sample
        int imageX = static_cast<int>(floor(sampleIndexToX * s));

        // Find topmost Y
        for (int imageY = 0 /*top*/; imageY < bumpMapImage.Size.Height; ++imageY)
        {
            int pointIndex = 3 * (imageY * bumpMapImage.Size.Width + imageX);
            if (bumpMapImage.Data[pointIndex] != 0x00
                || bumpMapImage.Data[pointIndex + 1] != 0x00
                || bumpMapImage.Data[pointIndex + 2] != 0x00)
            {
                // Found it!
                bumpMapSampleValue =
                    static_cast<float>(bumpMapImage.Size.Height - imageY)
                    - halfHeight;

                break;
            }
        }

        // Store sample
        mBumpMapSamples[s] = bumpMapSampleValue;
    }
}

bool OceanFloor::AdjustTo(
    float x,
    float targetY)
{
    //
    // Calculate sample index, minimizing error
    //

    float const absoluteSampleIndexF = x / Dx;
    int64_t absoluteSampleIndexI =
        (absoluteSampleIndexF >= 0.0f)
        ? FastFloorInt64(absoluteSampleIndexF + 0.5f)
        : FastFloorInt64(absoluteSampleIndexF - 0.5f);
    int64_t sampleIndex = absoluteSampleIndexI % SamplesCount;
    if (sampleIndex < 0)
    {
        sampleIndex += SamplesCount;
    }

    assert(sampleIndex >= 0 && sampleIndex < SamplesCount);


    //
    // Update values
    //

    float const oldValue = mSamples[sampleIndex].SampleValue;

    // Update sample value
    mSamples[sampleIndex].SampleValue = targetY;

    // Update previous sample's delta
    int64_t previousSampleIndex = (sampleIndex == 0) ? SamplesCount - 1 : sampleIndex - 1;
    mSamples[previousSampleIndex].SampleValuePlusOneMinusSampleValue = targetY - mSamples[previousSampleIndex].SampleValue;

    // Update this sample's delta
    int64_t nextSampleIndex = (sampleIndex == SamplesCount - 1) ? 0 : sampleIndex + 1;
    mSamples[sampleIndex].SampleValuePlusOneMinusSampleValue = mSamples[nextSampleIndex].SampleValue - targetY;

    return abs(targetY - oldValue) > 0.2f;
}

void OceanFloor::Update(GameParameters const & gameParameters)
{
    if (gameParameters.SeaDepth != mCurrentSeaDepth
        || gameParameters.OceanFloorBumpiness != mCurrentOceanFloorBumpiness
        || gameParameters.OceanFloorDetailAmplification != mCurrentOceanFloorDetailAmplification)
    {
        float const seaDepth = gameParameters.SeaDepth;
        float const oceanFloorBumpiness = gameParameters.OceanFloorBumpiness;
        float const oceanFloorDetailAmplification = gameParameters.OceanFloorDetailAmplification;

        // sample index = 0
        float previousSampleValue;
        {
            previousSampleValue =
                -seaDepth
                + 0.0f
                + mBumpMapSamples[0] * oceanFloorDetailAmplification;

            mSamples[0].SampleValue = previousSampleValue;
        }

        // Calulate samples = world y of ocean floor at the sample's x
        // sample index = 1...SamplesCount-1
        float x = Dx;
        for (int64_t i = 1; i < SamplesCount; i++, x += Dx)
        {
            float const c1 = sinf(x * Frequency1) * 10.f;
            float const c2 = sinf(x * Frequency2) * 6.f;
            float const c3 = sinf(x * Frequency3) * 45.f;
            float const sampleValue =
                -seaDepth
                + (c1 + c2 - c3) * oceanFloorBumpiness
                + mBumpMapSamples[i] * oceanFloorDetailAmplification;

            mSamples[i].SampleValue = sampleValue;
            mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

            previousSampleValue = sampleValue;
        }

        // sample index = SamplesCount
        mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = mSamples[0].SampleValue - previousSampleValue;

        // Remember current game parameters
        mCurrentSeaDepth = gameParameters.SeaDepth;
        mCurrentOceanFloorBumpiness = gameParameters.OceanFloorBumpiness;
        mCurrentOceanFloorDetailAmplification = gameParameters.OceanFloorDetailAmplification;
    }
}

}