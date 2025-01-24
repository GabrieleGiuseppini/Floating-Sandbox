/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "OceanFloorTerrain.h"

#include "GameParameters.h"
#include "PngImageFileTools.h"

#include <cmath>

size_t OceanFloorTerrain::Size = GameParameters::OceanFloorTerrainSamples<size_t>;

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

OceanFloorTerrain OceanFloorTerrain::LoadFromImage(std::filesystem::path const & imageFilePath)
{
    unique_buffer<float> terrainBuffer(GameParameters::OceanFloorTerrainSamples<size_t>);

    // Load image
    RgbImageData oceanFloorImage = PngImageFileTools::LoadImageRgb(imageFilePath);
    float const halfHeight = static_cast<float>(oceanFloorImage.Size.height) / 2.0f;

    // Calculate SampleI->WorldX factor, i.e. world width between two samples
    float constexpr Dx = GameParameters::MaxWorldWidth / GameParameters::OceanFloorTerrainSamples<float>;

    // Calculate WorldX->ImageX factor: we want the entire width of this image to fit the entire
    // world width (by stretching or compressing)
    float const worldXToImageX = static_cast<float>(oceanFloorImage.Size.width) / GameParameters::MaxWorldWidth;

    for (size_t s = 0; s < GameParameters::OceanFloorTerrainSamples<size_t>; ++s)
    {
        // Calculate pixel X
        float const worldX = static_cast<float>(s) * Dx;

        // Calulate image X
        float const imageX = worldX * worldXToImageX;

        // Integral and fractional parts
        int const imageXI = static_cast<int>(std::floor(imageX));
        float const imageXIF = imageX - static_cast<float>(imageXI);

        assert(imageXI >= 0 && imageXI < oceanFloorImage.Size.width);

        // Find topmost Y's at this image X
        //      Y=H at topmost => s=H/2, Y=0 if nothing found => s=-H/2
        float const sampleValue = static_cast<float>(GetTopmostY(oceanFloorImage, imageXI)) - halfHeight;

        if (imageXI < oceanFloorImage.Size.width - 1)
        {
            // Interpolate with next pixel
            float const sampleValue2 = static_cast<float>(GetTopmostY(oceanFloorImage, imageXI + 1)) - halfHeight;

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

    return OceanFloorTerrain(std::move(terrainBuffer));
}

OceanFloorTerrain OceanFloorTerrain::LoadFromStream(std::istream & is)
{
    unique_buffer<float> terrainBuffer(GameParameters::OceanFloorTerrainSamples<size_t>);

    is.read(reinterpret_cast<char *>(terrainBuffer.get()), terrainBuffer.size() * sizeof(float));

    return OceanFloorTerrain(std::move(terrainBuffer));
}

void OceanFloorTerrain::SaveToStream(std::ostream & os) const
{
    os.write(reinterpret_cast<char const *>(mTerrainBuffer.get()), mTerrainBuffer.size() * sizeof(float));
}