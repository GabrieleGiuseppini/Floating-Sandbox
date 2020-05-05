/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipTexturizer.h"

#include "ImageFileTools.h"

#include <GameCore/GameMath.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <chrono>

ShipTexturizer::ShipTexturizer(ResourceLocator const & resourceLocator)
    : mAutoTexturizationMode(ShipAutoTexturizationMode::MaterialTextures)
    , mMaterialTextureMagnification(0.05f)
    , mMaterialTextureWorldToPixelConversionFactor(1.0f / mMaterialTextureMagnification)
    , mMaterialTexturesFolderPath(resourceLocator.GetMaterialTexturesFolderPath())
    , mMaterialTextureCache()
{
}

void ShipTexturizer::VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const
{
    // TODO
}

RgbaImageData ShipTexturizer::Texturize(
    ImageSize const & structureSize,
    ShipBuildPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
    std::vector<ShipBuildPoint> const & points) const
{
    //
    // Calculate target texture size: integral multiple of structure size, but without
    // exceeding 4096 (magic number, also max texture size for low-end gfx cards)
    //

    int const maxDimension = std::max(structureSize.Width, structureSize.Height);
    assert(maxDimension > 0);

    int const magnificationFactor = std::max(1, 4096 / maxDimension);
    float const magnificationFactorInvF = 1.0f / static_cast<float>(magnificationFactor);

    ImageSize const textureSize = structureSize * magnificationFactor;

    //
    // Create texture
    //

    auto newImageData = std::make_unique<rgbaColor[]>(textureSize.GetPixelCount());

    auto const startTime = std::chrono::steady_clock::now();

    for (int y = 1; y <= structureSize.Height; ++y)
    {
        for (int x = 1; x <= structureSize.Width; ++x)
        {
            // Get structure pixel color
            rgbaColor const structurePixelColor = pointMatrix[x][y].has_value()
                ? rgbaColor(points[*pointMatrix[x][y]].StructuralMtl.RenderColor)
                : rgbaColor::zero(); // Fully transparent

            if (mAutoTexturizationMode == ShipAutoTexturizationMode::FlatStructure
                || !pointMatrix[x][y].has_value())
            {
                //
                // Flat structure
                //

                // Fill quad with color
                for (int yy = 0; yy < magnificationFactor; ++yy)
                {
                    int const quadOffset =
                        (x - 1) * magnificationFactor
                        + ((y - 1) * magnificationFactor + yy) * textureSize.Width;

                    for (int xx = 0; xx < magnificationFactor; ++xx)
                    {
                        newImageData[quadOffset + xx] = structurePixelColor;
                    }
                }
            }
            else
            {
                //
                // Material textures
                //

                assert(mAutoTexturizationMode == ShipAutoTexturizationMode::MaterialTextures);

                vec3f const structurePixelColorF = structurePixelColor.toVec3f();

                // Get bump map texture
                assert(pointMatrix[x][y].has_value());
                Vec3fImageData const & materialTexture = GetMaterialTexture(points[*pointMatrix[x][y]].StructuralMtl.MaterialTextureName);

                //
                // Fill quad with color multiply-blended with "bump map" texture
                //

                float const baseWorldY = static_cast<float>(y - 1);

                int const baseTargetQuadOffset =
                    ((x - 1) + (y - 1) * textureSize.Width)
                    * magnificationFactor;

                for (int yy = 0; yy < magnificationFactor; ++yy)
                {
                    int const targetQuadOffset = baseTargetQuadOffset + yy * textureSize.Width;

                    float const worldY = baseWorldY + static_cast<float>(yy) * magnificationFactorInvF;

                    float const baseWorldX = static_cast<float>(x - 1);
                    for (int xx = 0; xx < magnificationFactor; ++xx)
                    {
                        float const worldX = baseWorldX + static_cast<float>(xx) * magnificationFactorInvF;

                        vec3f const bumpMapSample = SampleTexture(
                            materialTexture,
                            worldX * mMaterialTextureWorldToPixelConversionFactor,
                            worldY * mMaterialTextureWorldToPixelConversionFactor);

                        vec3f const resultantColor(
                            structurePixelColorF.x * bumpMapSample.x,
                            structurePixelColorF.y * bumpMapSample.y,
                            structurePixelColorF.z * bumpMapSample.z);

                        // Store resultant color, using structure's alpha channel value
                        newImageData[targetQuadOffset + xx] = rgbaColor(resultantColor, structurePixelColor.a);
                    }
                }
            }
        }
    }

    LogMessage("Ship Auto-Texturization: magFactor=", magnificationFactor,
        " textureSize=", textureSize,
        " time=", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count(), "us");

    return RgbaImageData(textureSize, std::move(newImageData));
}

///////////////////////////////////////////////////////////////////////////////////

Vec3fImageData const & ShipTexturizer::GetMaterialTexture(std::string const & textureName) const
{
    std::string const TODOtextureName = "wood_1";

    auto const & it = mMaterialTextureCache.find(TODOtextureName);
    if (it != mMaterialTextureCache.end())
    {
        // Texture is cached
        ++it->second.UseCount;
        return it->second.Texture;
    }
    else
    {
        // Have to load texture

        // Check whether need to purge
        // TODOHERE

        // Load and cache texture
        // TODO: use name->path map built at cctor
        RgbImageData texture = ImageFileTools::LoadImageRgb(mMaterialTexturesFolderPath / (TODOtextureName + ".png"));
        auto const inserted = mMaterialTextureCache.emplace(
            TODOtextureName,
            ImageTools::ToVec3f(texture));

        assert(inserted.second);

        return inserted.first->second.Texture;
    }
}

vec3f ShipTexturizer::SampleTexture(
    Vec3fImageData const & texture,
    float pixelX,
    float pixelY) const
{
    // Wrap coordinates
    float const wrappedPixelX = std::fmodf(pixelX, static_cast<float>(texture.Size.Width));
    float const wrappedPixelY = std::fmodf(pixelY, static_cast<float>(texture.Size.Height));

    // Integral part
    int32_t const pixelXI = FastTruncateInt32(wrappedPixelX);
    int32_t const pixelYI = FastTruncateInt32(wrappedPixelY);

    // Fractional part between index and next index
    float const pixelDx = wrappedPixelX - pixelXI;
    float const pixelDy = wrappedPixelY - pixelYI;

    assert(pixelXI >= 0 && pixelXI < texture.Size.Width);
    assert(pixelDx >= 0.0f && pixelDx < 1.0f);
    assert(pixelYI >= 0 && pixelYI < texture.Size.Height);
    assert(pixelDy >= 0.0f && pixelDy < 1.0f);

    //
    // Bilinear
    //

    int const nextPixelXI = (pixelXI + 1) % texture.Size.Width;
    int const nextPixelYI = (pixelYI + 1) % texture.Size.Height;

    // Linear interpolation between x samples at bottom
    vec3f const interpolatedXColorBottom = Mix(
        texture.Data[pixelXI + pixelYI * texture.Size.Width],
        texture.Data[nextPixelXI + pixelYI * texture.Size.Width],
        pixelDx);

    // Linear interpolation between x samples at top
    vec3f const interpolatedXColorTop = Mix(
        texture.Data[pixelXI + nextPixelYI * texture.Size.Width],
        texture.Data[nextPixelXI + nextPixelYI * texture.Size.Width],
        pixelDx);

    // Linear interpolation between two vertical samples
    return Mix(
        interpolatedXColorBottom,
        interpolatedXColorTop,
        pixelDy);
}