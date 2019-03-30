/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

static constexpr int IdealBumpMapWidth = 5000;

namespace /* anonymous */ {

    int GetTopmostY(
        RgbImageData const & imageData,
        int x)
    {
        int imageY;
        for (imageY = 0 /*top*/; imageY < imageData.Size.Height; ++imageY)
        {
            int pointIndex = imageY * imageData.Size.Width + x;
            if (imageData.Data[pointIndex] != rgbColor::zero())
            {
                // Found it!
                break;
            }
        }

        return imageY;
    }
}

OceanFloor::OceanFloor(ResourceLoader & resourceLoader)
    : mSamples(new Sample[SamplesCount + 1])
    , mBumpMapSamples(new float[SamplesCount + 1])
    , mCurrentSeaDepth(std::numeric_limits<float>::lowest())
    , mCurrentOceanFloorBumpiness(std::numeric_limits<float>::lowest())
    , mCurrentOceanFloorDetailAmplification(std::numeric_limits<float>::lowest())
{
    //
    // Initialize bump map
    // - Load bump map image
    // - Implicitly resample it to 5000m, if width < 5000
    // - Convert each (topmost) y of the map into a Y coordinate, between H/2 (top) and -H/2 (bottom)
    //

    RgbImageData bumpMapImage = ImageFileTools::LoadImageRgbUpperLeft(resourceLoader.GetOceanFloorBumpMapFilepath());

    float const worldXToImageX = bumpMapImage.Size.Width < IdealBumpMapWidth
        ? static_cast<float>(bumpMapImage.Size.Width) / static_cast<float>(IdealBumpMapWidth)
        : 1.0f;

    float const halfHeight = static_cast<float>(bumpMapImage.Size.Height / 2);

    for (size_t s = 0; s < SamplesCount; ++s)
    {
        // Calculate pixel X
        float const imageX = static_cast<float>(s)
            * Dx
            * worldXToImageX;

        // Calculate the left and right x's, repeating bump map as necessary
        int x1 = static_cast<int>(floor(imageX)) % bumpMapImage.Size.Width;
        int x2 = static_cast<int>(ceil(imageX)) % bumpMapImage.Size.Width;

        // Find topmost Y's
        float sampleValue1 = static_cast<float>(bumpMapImage.Size.Height - GetTopmostY(bumpMapImage, x1)) - halfHeight;
        float sampleValue2 = static_cast<float>(bumpMapImage.Size.Height - GetTopmostY(bumpMapImage, x2)) - halfHeight;

        // Store sample
        mBumpMapSamples[s] =
            sampleValue1
            + (sampleValue2 - sampleValue1) / Dx;
    }

    // Populate extra sample
    mBumpMapSamples[SamplesCount] = mBumpMapSamples[SamplesCount - 1];
}

bool OceanFloor::AdjustTo(
    float x1,
    float targetY1,
    float x2,
    float targetY2)
{
    float leftX, leftTargetY;
    float rightX, rightTargetY;
    if (x1 <= x2)
    {
        leftX = x1;
        leftTargetY = targetY1;
        rightX = x2;
        rightTargetY = targetY2;
    }
    else
    {
        leftX = x2;
        leftTargetY = targetY2;
        rightX = x1;
        rightTargetY = targetY1;
    }

    float slopeY = (leftX != rightX)
        ? (rightTargetY - leftTargetY) / (rightX - leftX)
        : 1.0f;

    //
    // Calculate left first sample index, minimizing error
    //

    float const sampleIndexF = (leftX + GameParameters::MaxWorldWidth / 2.0f) / Dx;

    int64_t sampleIndex = FastFloorInt64(sampleIndexF + 0.5f);

    assert(sampleIndex >= 0 && sampleIndex <= SamplesCount);


    //
    // Update values for all samples in the trajectory
    //

    bool hasAdjusted = false;
    float x = leftX;
    for (int64_t s = sampleIndex; x <= rightX; x += Dx)
    {
        // Update sample value
        float newSampleValue = leftTargetY + slopeY * (x - leftX);
        hasAdjusted |= abs(newSampleValue - mSamples[s].SampleValue) > 0.2f;
        mSamples[s].SampleValue = newSampleValue;

        // Update previous sample's delta
        if (sampleIndex > 0) // No point in updating sample[-1]
            mSamples[sampleIndex - 1].SampleValuePlusOneMinusSampleValue = newSampleValue - mSamples[sampleIndex - 1].SampleValue;

        // Update this sample's delta
        if (sampleIndex < SamplesCount) // No point in updating delta of extra sample
            mSamples[sampleIndex].SampleValuePlusOneMinusSampleValue = mSamples[sampleIndex + 1].SampleValue - newSampleValue;
    }

    return hasAdjusted;
}

void OceanFloor::Update(GameParameters const & gameParameters)
{
    if (gameParameters.SeaDepth != mCurrentSeaDepth
        || gameParameters.OceanFloorBumpiness != mCurrentOceanFloorBumpiness
        || gameParameters.OceanFloorDetailAmplification != mCurrentOceanFloorDetailAmplification)
    {
        // Frequencies of bump sine components
        static constexpr float BumpFrequency1 = 0.005f;
        static constexpr float BumpFrequency2 = 0.015f;
        static constexpr float BumpFrequency3 = 0.001f;

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
            float const c1 = sinf(x * BumpFrequency1) * 10.f;
            float const c2 = sinf(x * BumpFrequency2) * 6.f;
            float const c3 = sinf(x * BumpFrequency3) * 45.f;
            float const sampleValue =
                -seaDepth
                + (c1 + c2 - c3) * oceanFloorBumpiness
                + mBumpMapSamples[i] * oceanFloorDetailAmplification;

            mSamples[i].SampleValue = sampleValue;
            mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

            previousSampleValue = sampleValue;
        }

        // Populate last delta (extra sample has same value as this sample)
        mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = 0.0f;

        // Populate extra sample - same value as last sample
        mSamples[SamplesCount].SampleValue = mSamples[SamplesCount - 1].SampleValue;
        mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f; // Never used

        // Remember current game parameters
        mCurrentSeaDepth = gameParameters.SeaDepth;
        mCurrentOceanFloorBumpiness = gameParameters.OceanFloorBumpiness;
        mCurrentOceanFloorDetailAmplification = gameParameters.OceanFloorDetailAmplification;
    }
}

}