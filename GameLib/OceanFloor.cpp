/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

OceanFloor::OceanFloor(ResourceLoader & resourceLoader)
    : mSamples(new Sample[SamplesCount + 1])
    , mBumpMapSamples(new float[SamplesCount + 1])
    , mCurrentSeaDepth(std::numeric_limits<float>::lowest())
    , mCurrentOceanFloorBumpiness(std::numeric_limits<float>::lowest())
    , mCurrentOceanFloorDetail(std::numeric_limits<float>::lowest())
{
    //
    // Pre-process bump map:
    // - Load bump map image
    // - Convert each (topmost) y of the map into a Y coordinate, between +1.0 (top) and -1.0 (bottom)
    //

    ImageData bumpMapImage = ResourceLoader::LoadImageRgbUpperLeft(resourceLoader.GetOceanFloorBumpMapFilepath());

    float const sampleIndexToX = static_cast<float>(bumpMapImage.Size.Width) / static_cast<float>(SamplesCount);
    
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
                    2.0f 
                    * static_cast<float>((bumpMapImage.Size.Height - imageY - 1) - bumpMapImage.Size.Height / 2) 
                    / static_cast<float>(bumpMapImage.Size.Height);

                break;
            }
        }

        // Store sample
        mBumpMapSamples[s] = bumpMapSampleValue;
    }

    // Store extra sample
    mBumpMapSamples[SamplesCount] = mBumpMapSamples[SamplesCount - 1];
}

void OceanFloor::Update(GameParameters const & gameParameters)
{
    if (gameParameters.SeaDepth != mCurrentSeaDepth
        || gameParameters.OceanFloorBumpiness != mCurrentOceanFloorBumpiness
        || gameParameters.OceanFloorDetail != mCurrentOceanFloorDetail)
    {
        float const seaDepth = gameParameters.SeaDepth;
        float const oceanFloorBumpiness = gameParameters.OceanFloorBumpiness;
        float const oceanFloorDetail = gameParameters.OceanFloorDetail;
        
        // sample index = 0 
        float previousSampleValue;
        {
            previousSampleValue = 
                -seaDepth 
                + 0.0f
                + mBumpMapSamples[0] * oceanFloorDetail;

            mSamples[0].SampleValue = previousSampleValue;
        }

        // Calulate samples = world y of ocean floor at the sample's x
        // sample index = 1...SamplesCount
        // Note: we fill-in an extra sample, so we can avoid having to wrap around
        float x = Dx;
        for (int64_t i = 1; i <= SamplesCount; i++, x += Dx)
        {
            float const c1 = sinf(x * Frequency1) * 10.f;
            float const c2 = sinf(x * Frequency2) * 6.f;
            float const c3 = sinf(x * Frequency3) * 45.f;
            float const sampleValue =
                -seaDepth
                + (c1 + c2 - c3) * oceanFloorBumpiness
                + mBumpMapSamples[i] * oceanFloorDetail;

            mSamples[i].SampleValue = sampleValue;
            mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

            previousSampleValue = sampleValue;
        }

        // sample index = SamplesCount
        mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = mSamples[0].SampleValue - previousSampleValue;

        // Remember current game parameters
        mCurrentSeaDepth = gameParameters.SeaDepth;
        mCurrentOceanFloorBumpiness = gameParameters.OceanFloorBumpiness;
        mCurrentOceanFloorDetail = gameParameters.OceanFloorDetail;
    }
}

}