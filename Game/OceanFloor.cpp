/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

// Bump map bitmaps smaller than this width are resized up (interpolating) to this width
static constexpr int IdealBumpMapWidth = 5000;

// The number of slices we want to render the ocean floor as;
// this is the graphical resolution
static constexpr int RenderSlices = 512;

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

void OceanFloor::Upload(
    GameParameters const & /*gameParameters*/,
    Render::RenderContext & renderContext) const
{
    //
    // We want to upload at most RenderSlices slices
    //

    // Find index of leftmost sample, and its corresponding world X
    auto sampleIndex = FastTruncateInt64((renderContext.GetVisibleWorldLeft() + GameParameters::HalfMaxWorldWidth) / Dx);
    float sampleIndexX = -GameParameters::HalfMaxWorldWidth + (Dx * sampleIndex);

    // Calculate number of slices required to cover up to the visible world right (included)
    float const coverageWidth = renderContext.GetVisibleWorldRight() - sampleIndexX;
    auto const numberOfSlicesToRender = static_cast<int64_t>(ceil(coverageWidth / Dx));

    if (numberOfSlicesToRender >= RenderSlices)
    {
        //
        // Have to take 1 sample every NSkip
        //

        // TODOHERE: this is the old one
        size_t constexpr SlicesCount = 500;

        float const sliceWidth = renderContext.GetVisibleWorldWidth() / static_cast<float>(SlicesCount);

        renderContext.UploadLandStart(SlicesCount);

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        float sliceX = renderContext.GetVisibleWorldLeft();
        for (size_t i = 0; i <= SlicesCount; ++i, sliceX += sliceWidth)
        {
            renderContext.UploadLand(
                sliceX,
                GetFloorHeightAt(sliceX));
        }
    }
    else
    {
        //
        // We just upload the required number of samples, which is less than
        // the max number of slices we're prepared to upload, and we let OpenGL
        // interpolate on our behalf
        //

        renderContext.UploadLandStart(numberOfSlicesToRender);

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (std::int64_t s = 0; s <= numberOfSlicesToRender; ++s, sampleIndexX += Dx)
        {
            renderContext.UploadLand(
                sampleIndexX,
                mSamples[s + sampleIndex].SampleValue);
        }
    }

    renderContext.UploadLandEnd();

    // TODOOLD

    ////size_t constexpr SlicesCount = 500;

    ////float const visibleWorldWidth = renderContext.GetVisibleWorldWidth();
    ////float const sliceWidth = visibleWorldWidth / static_cast<float>(SlicesCount);
    ////float sliceX = renderContext.GetCameraWorldPosition().x - (visibleWorldWidth / 2.0f);

    ////renderContext.UploadLandStart(SlicesCount);

    ////// We do one extra iteration as the number of slices is the number of quads, and the last vertical
    ////// quad side must be at the end of the width
    ////for (size_t i = 0; i <= SlicesCount; ++i, sliceX += sliceWidth)
    ////{
    ////    renderContext.UploadLand(
    ////        sliceX,
    ////        GetFloorHeightAt(sliceX));
    ////}

    ////renderContext.UploadLandEnd();
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

    float const sampleIndexF = (leftX + GameParameters::HalfMaxWorldWidth) / Dx;

    int64_t sampleIndex = FastTruncateInt64(sampleIndexF + 0.5f);

    assert(sampleIndex >= 0 && sampleIndex <= SamplesCount);


    //
    // Update values for all samples in the trajectory
    //

    bool hasAdjusted = false;
    float x = leftX;
    for (int64_t s = sampleIndex; x <= rightX; ++s, x += Dx)
    {
        // Update sample value
        float newSampleValue = leftTargetY + slopeY * (x - leftX);
        hasAdjusted |= abs(newSampleValue - mSamples[s].SampleValue) > 0.2f;
        mSamples[s].SampleValue = newSampleValue;

        // Update previous sample's delta
        if (s > 0) // No point in updating sample[-1]
            mSamples[s - 1].SampleValuePlusOneMinusSampleValue = newSampleValue - mSamples[s - 1].SampleValue;

        // Update this sample's delta
        if (s < SamplesCount) // No point in updating delta of extra sample
            mSamples[s].SampleValuePlusOneMinusSampleValue = mSamples[s + 1].SampleValue - newSampleValue;
    }

    return hasAdjusted;
}

}