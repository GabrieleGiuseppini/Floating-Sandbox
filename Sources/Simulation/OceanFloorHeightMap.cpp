/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "OceanFloorHeightMap.h"

#include "SimulationParameters.h"

#include <cmath>

namespace /* anonymous */ {

    /*
     * Y=H at topmost, Y=0 if nothing found
     */
    int GetTopmostY(
        RgbImageData const & imageData,
        int x)
    {
        int imageY;
        for (imageY = imageData.Size.height - 1; imageY >= 0; --imageY)
        {
            int pointIndex = imageY * imageData.Size.width + x;
            if (imageData.Data[pointIndex] != rgbColor::zero())
            {
                // Found it!
                break;
            }
        }

        return imageY + 1;
    }
}

size_t OceanFloorHeightMap::Size = SimulationParameters::OceanFloorTerrainSamples<size_t>;

OceanFloorHeightMap OceanFloorHeightMap::LoadFromImage(RgbImageData const & imageData)
{
    unique_buffer<float> terrainBuffer(Size);

    float const imageHalfHeight = static_cast<float>(imageData.Size.height) / 2.0f;

    // Calculate SampleI->WorldX factor, i.e. world width between two samples
    float constexpr Dx = SimulationParameters::MaxWorldWidth / SimulationParameters::OceanFloorTerrainSamples<float>;

    // Calculate WorldX->ImageX factor: we want the entire width of this image to fit the entire
    // world width (by stretching or compressing)
    float const worldXToImageX = static_cast<float>(imageData.Size.width) / SimulationParameters::MaxWorldWidth;

    for (size_t s = 0; s < Size; ++s)
    {
        // Calculate pixel X
        float const worldX = static_cast<float>(s) * Dx;

        // Calulate image X
        float const imageX = worldX * worldXToImageX;

        // Integral and fractional parts
        int const imageXI = static_cast<int>(std::floor(imageX));
        float const imageXIF = imageX - static_cast<float>(imageXI);

        assert(imageXI >= 0 && imageXI < imageData.Size.width);

        // Find topmost Y's at this image X
        //      Y=H at topmost => s=H/2, Y=0 if nothing found => s=-H/2
        float const sampleValue = static_cast<float>(GetTopmostY(imageData, imageXI)) - imageHalfHeight;

        if (imageXI < imageData.Size.width - 1)
        {
            // Interpolate with next pixel
            float const sampleValue2 = static_cast<float>(GetTopmostY(imageData, imageXI + 1)) - imageHalfHeight;

            // Store sample
            terrainBuffer[s] =
                sampleValue
                + (sampleValue2 - sampleValue) * imageXIF;
        }
        else
        {
            // Use last sample
            terrainBuffer[s] = sampleValue;
        }
    }

    return OceanFloorHeightMap(std::move(terrainBuffer));
}

OceanFloorHeightMap OceanFloorHeightMap::LoadFromStream(BinaryReadStream & inputStream)
{
    unique_buffer<float> terrainBuffer(Size);

    inputStream.Read(reinterpret_cast<std::uint8_t *>(terrainBuffer.get()), terrainBuffer.size() * sizeof(float));

    return OceanFloorHeightMap(std::move(terrainBuffer));
}

void OceanFloorHeightMap::SaveToStream(BinaryWriteStream & outputStream) const
{
    outputStream.Write(reinterpret_cast<std::uint8_t const *>(mTerrainBuffer.get()), mTerrainBuffer.size() * sizeof(float));
}