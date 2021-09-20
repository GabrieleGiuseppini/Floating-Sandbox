/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipTexturizer.h"

#include "ImageFileTools.h"

#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <chrono>

size_t constexpr MaterialTextureCacheSizeHighWatermark = 40;
size_t constexpr MaterialTextureCacheSizeLowWatermark = 25;

std::string const MaterialTextureNameNone = "none";

namespace /*anonymous*/ {

    inline float BidirMultiplyBlend(float x1, float x2)
    {
        return (x2 <= 0.5f)
            ? x1 * 2.0f * x2                        // Damper: x1 * [0.0, 1.0]
            : x1 + (x2 - x1) * 2.0f * (x2 - 0.5f);  // Amplifier:x1 + (x2 - x1) * [0.0, 1.0]
    }
}

ShipTexturizer::ShipTexturizer(
    MaterialDatabase const & materialDatabase,
    ResourceLocator const & resourceLocator)
    // Here is where default settings live
    : mSharedSettings(
        ShipAutoTexturizationModeType::MaterialTextures,
        1.0f, // MaterialTextureMagnification
        0.0f) // MaterialTextureTransparency
    , mDoForceSharedSettingsOntoShipSettings(false)
    , mMaterialTextureNameToTextureFilePathMap(
        MakeMaterialTextureNameToTextureFilePathMap(materialDatabase, resourceLocator))
    , mMaterialTextureCache()
{
}

RgbaImageData ShipTexturizer::Texturize(
    std::optional<ShipAutoTexturizationSettings> const & shipDefinitionSettings,
    ImageSize const & structureSize,
    ShipFactoryPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
    std::vector<ShipFactoryPoint> const & points) const
{
    auto const startTime = std::chrono::steady_clock::now();

    // Zero-out cache usage counts
    ResetMaterialTextureCacheUseCounts();

    //
    // Calculate target texture size: integral multiple of structure size, but without
    // exceeding 4096 (magic number, also max texture size for low-end gfx cards),
    // and no more than 32 times the original size
    //

    int const maxDimension = std::max(structureSize.width, structureSize.height);
    assert(maxDimension > 0);

    int const magnificationFactor = std::min(32, std::max(1, 4096 / maxDimension));
    float const magnificationFactorInvF = 1.0f / static_cast<float>(magnificationFactor);

    ImageSize const textureSize = structureSize * magnificationFactor;

    //
    // Prepare constants
    //

    ShipAutoTexturizationSettings const & settings = (mDoForceSharedSettingsOntoShipSettings || !shipDefinitionSettings.has_value())
        ? mSharedSettings
        : *shipDefinitionSettings;

    float const worldToMaterialTexturePixelConversionFactor =
        MaterialTextureMagnificationToPixelConversionFactor(settings.MaterialTextureMagnification);

    float const materialTextureAlpha = 1.0f - settings.MaterialTextureTransparency;

    //
    // Create texture
    //

    auto newImageData = std::make_unique<rgbaColor[]>(textureSize.GetLinearSize());

    for (int y = 1; y <= structureSize.height; ++y)
    {
        for (int x = 1; x <= structureSize.width; ++x)
        {
            vec2i const pixelCoords(x, y);

            // Get structure pixel color
            rgbaColor const structurePixelColor = pointMatrix[pixelCoords].has_value()
                ? rgbaColor(points[*pointMatrix[pixelCoords]].StructuralMtl.RenderColor)
                : rgbaColor::zero(); // Fully transparent

            if (settings.Mode == ShipAutoTexturizationModeType::FlatStructure
                || !pointMatrix[pixelCoords].has_value())
            {
                //
                // Flat structure
                //

                // Fill quad with color
                for (int yy = 0; yy < magnificationFactor; ++yy)
                {
                    int const quadOffset =
                        (x - 1) * magnificationFactor
                        + ((y - 1) * magnificationFactor + yy) * textureSize.width;

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

                assert(settings.Mode == ShipAutoTexturizationModeType::MaterialTextures);

                vec3f const structurePixelColorF = structurePixelColor.toVec3f();

                // Get bump map texture
                assert(pointMatrix[pixelCoords].has_value());
                Vec3fImageData const & materialTexture = GetMaterialTexture(points[*pointMatrix[pixelCoords]].StructuralMtl.MaterialTextureName);

                //
                // Fill quad with color multiply-blended with "bump map" texture
                //

                int const baseTargetQuadOffset = ((x - 1) + (y - 1) * textureSize.width) * magnificationFactor;

                float worldY = static_cast<float>(y - 1);
                for (int yy = 0; yy < magnificationFactor; ++yy, worldY += magnificationFactorInvF)
                {
                    int const targetQuadOffset = baseTargetQuadOffset + yy * textureSize.width;

                    float worldX = static_cast<float>(x - 1);
                    for (int xx = 0; xx < magnificationFactor; ++xx, worldX += magnificationFactorInvF)
                    {
                        vec3f const bumpMapSample = SampleTexture(
                            materialTexture,
                            worldX * worldToMaterialTexturePixelConversionFactor,
                            worldY * worldToMaterialTexturePixelConversionFactor);

                        ////// Vanilla multiply blending
                        ////vec3f const resultantColor(
                        ////    structurePixelColorF.x * bumpMapSample.x,
                        ////    structurePixelColorF.y * bumpMapSample.y,
                        ////    structurePixelColorF.z * bumpMapSample.z);

                        // Bi-directional multiply blending
                        vec3f const resultantColorF(
                            BidirMultiplyBlend(structurePixelColorF.x, bumpMapSample.x),
                            BidirMultiplyBlend(structurePixelColorF.y, bumpMapSample.y),
                            BidirMultiplyBlend(structurePixelColorF.z, bumpMapSample.z));

                        // Store resultant color, using structure's alpha channel value,
                        // and blended with transparency
                        newImageData[targetQuadOffset + xx] = rgbaColor(
                            Mix(structurePixelColorF,
                                resultantColorF,
                                materialTextureAlpha),
                            structurePixelColor.a);
                    }
                }
            }
        }
    }

    LogMessage("ShipTexturizer: completed auto-texturization:",
        " materialTextureMagnification=", settings.MaterialTextureMagnification,
        " structureSize=", structureSize, " textureSize=", textureSize, " magFactor=", magnificationFactor,
        " time=", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count(), "us");

    return RgbaImageData(textureSize, std::move(newImageData));
}

///////////////////////////////////////////////////////////////////////////////////

std::unordered_map<std::string, std::filesystem::path> ShipTexturizer::MakeMaterialTextureNameToTextureFilePathMap(
    MaterialDatabase const & materialDatabase,
    ResourceLocator const & resourceLocator)
{
    std::unordered_map<std::string, std::filesystem::path> materialTextureNameToTextureFilePath;

    // Add "none" entry
    {
        std::filesystem::path const materialTextureFilePath = resourceLocator.GetMaterialTextureFilePath(MaterialTextureNameNone);

        // Make sure file exists
        if (!std::filesystem::exists(materialTextureFilePath)
            || !std::filesystem::is_regular_file(materialTextureFilePath))
        {
            throw GameException(
                "Cannot find material texture file for texture name \"" + MaterialTextureNameNone + "\"");
        }

        // Store mapping
        materialTextureNameToTextureFilePath[MaterialTextureNameNone] = materialTextureFilePath;
    }

    // Add entries for all materials
    for (auto const & category : materialDatabase.GetStructuralMaterialPalette().Categories)
    {
        for (auto const & subCategory : category.SubCategories)
        {
            for (StructuralMaterial const & material : subCategory.Materials)
            {
                if (material.MaterialTextureName.has_value())
                {
                    std::string const & materialTextureName = *material.MaterialTextureName;
                    if (materialTextureNameToTextureFilePath.count(materialTextureName) == 0)
                    {
                        std::filesystem::path const materialTextureFilePath = resourceLocator.GetMaterialTextureFilePath(materialTextureName);

                        // Make sure file exists
                        if (!std::filesystem::exists(materialTextureFilePath)
                            || !std::filesystem::is_regular_file(materialTextureFilePath))
                        {
                            throw GameException(
                                "Cannot find material texture file for texture name \"" + materialTextureName + "\""
                                + " specified for material \"" + material.Name + "\"");
                        }

                        // Store mapping
                        materialTextureNameToTextureFilePath[materialTextureName] = materialTextureFilePath;
                    }
                }
            }
        }
    }

    return materialTextureNameToTextureFilePath;
}

float ShipTexturizer::MaterialTextureMagnificationToPixelConversionFactor(float magnification)
{
    // Magic number
    return 1.0f / (0.08f * magnification);
}

RgbaImageData ShipTexturizer::MakeTextureSample(
    std::optional<ShipAutoTexturizationSettings> const & settings,
    ImageSize const & sampleSize,
    rgbaColor const & renderColor,
    std::optional<std::string> const & textureName) const
{
    assert(sampleSize.width >= 2); // We'll split the width in half

    // Use shared settings if no settings have been provided
    ShipAutoTexturizationSettings const & effectiveSettings = !settings.has_value()
        ? mSharedSettings
        : *settings;

    // Create output image
    auto sampleData = std::make_unique<rgbaColor[]>(sampleSize.GetLinearSize());

    // Get bump map texture and render color
    Vec3fImageData const & materialTexture = GetMaterialTexture(textureName);
    vec3f const renderPixelColorF = renderColor.toVec3f();

    // Calculate constants
    float const sampleToMaterialTexturePixelConversionFactor = 1.0f / effectiveSettings.MaterialTextureMagnification;
    float const materialTextureAlpha = 1.0f - effectiveSettings.MaterialTextureTransparency;

    //
    // Fill quad with color multiply-blended with "bump map" texture
    //

    for (int y = 0; y < sampleSize.height; ++y)
    {
        int const targetQuadOffset = y * sampleSize.width;

        for (int x = 0; x < sampleSize.width / 2; ++x)
        {
            vec3f const bumpMapSample = SampleTexture(
                materialTexture,
                static_cast<float>(x) * sampleToMaterialTexturePixelConversionFactor,
                static_cast<float>(y) * sampleToMaterialTexturePixelConversionFactor);

            // Bi-directional multiply blending
            vec3f const resultantColorF(
                BidirMultiplyBlend(renderPixelColorF.x, bumpMapSample.x),
                BidirMultiplyBlend(renderPixelColorF.y, bumpMapSample.y),
                BidirMultiplyBlend(renderPixelColorF.z, bumpMapSample.z));

            // Store resultant color to the left side, using structure's alpha channel value,
            // and blended with transparency
            sampleData[targetQuadOffset + x] = rgbaColor(
                Mix(renderPixelColorF,
                    resultantColorF,
                    materialTextureAlpha),
                renderColor.a);

            // Store instead raw render color to the right side
            sampleData[targetQuadOffset + x + sampleSize.width / 2] = renderColor;
        }
    }

    return RgbaImageData(sampleSize, std::move(sampleData));
}

Vec3fImageData const & ShipTexturizer::GetMaterialTexture(std::optional<std::string> const & textureName) const
{
    std::string const actualTextureName = textureName.value_or(MaterialTextureNameNone);

    auto const & it = mMaterialTextureCache.find(actualTextureName);
    if (it != mMaterialTextureCache.end())
    {
        // Texture is cached
        ++it->second.UseCount;
        return it->second.Texture;
    }
    else
    {
        // Have to load texture

        // Check whether need to make room in the cache, first
        if (mMaterialTextureCache.size() + 1 >= MaterialTextureCacheSizeHighWatermark)
        {
            PurgeMaterialTextureCache(MaterialTextureCacheSizeLowWatermark);
        }

        // Load and cache texture
        assert(mMaterialTextureNameToTextureFilePathMap.count(actualTextureName) > 0);
        RgbImageData texture = ImageFileTools::LoadImageRgb(mMaterialTextureNameToTextureFilePathMap.at(actualTextureName));
        auto const inserted = mMaterialTextureCache.emplace(
            actualTextureName,
            ImageTools::ToVec3f(texture));

        assert(inserted.second);

        return inserted.first->second.Texture;
    }
}

void ShipTexturizer::ResetMaterialTextureCacheUseCounts() const
{
    std::for_each(
        mMaterialTextureCache.begin(),
        mMaterialTextureCache.end(),
        [](auto & kv)
        {
            kv.second.UseCount = 0;
        });
}

void ShipTexturizer::PurgeMaterialTextureCache(size_t maxSize) const
{
    LogMessage("ShipTexturizer: purging ", maxSize, " material texture cache elements");

    // Create vector with keys and usage counts
    std::vector<std::pair<std::string, size_t>> keyUsages;
    std::transform(
        mMaterialTextureCache.cbegin(),
        mMaterialTextureCache.cend(),
        std::back_inserter(keyUsages),
        [](auto const & it) ->std::pair<std::string, size_t>
        {
            return std::make_pair(it.first, it.second.UseCount);
        });

    // Sorty by usage count, ascending
    std::sort(
        keyUsages.begin(),
        keyUsages.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.second < rhs.second;
        });

    // Trim the top elements
    for (size_t i = 0; i < maxSize && i < keyUsages.size(); ++i)
    {
        mMaterialTextureCache.erase(keyUsages[i].first);
    }
}

vec3f ShipTexturizer::SampleTexture(
    Vec3fImageData const & texture,
    float pixelX,
    float pixelY) const
{
    // Integral part
    auto pixelXI = FastTruncateToArchInt(pixelX);
    auto pixelYI = FastTruncateToArchInt(pixelY);

    // Fractional part between index and next index
    float const pixelDx = pixelX - pixelXI;
    float const pixelDy = pixelY - pixelYI;

    // Wrap integral coordinates
    pixelXI %= static_cast<decltype(pixelXI)>(texture.Size.width);
    pixelYI %= static_cast<decltype(pixelYI)>(texture.Size.height);

    assert(pixelXI >= 0 && pixelXI < texture.Size.width);
    assert(pixelDx >= 0.0f && pixelDx < 1.0f);
    assert(pixelYI >= 0 && pixelYI < texture.Size.height);
    assert(pixelDy >= 0.0f && pixelDy < 1.0f);

    //
    // Bilinear
    //

    int const nextPixelXI = (pixelXI + 1) % static_cast<decltype(pixelXI)>(texture.Size.width);
    int const nextPixelYI = (pixelYI + 1) % static_cast<decltype(pixelYI)>(texture.Size.height);

    // Linear interpolation between x samples at bottom
    vec3f const interpolatedXColorBottom = Mix(
        texture.Data[pixelXI + pixelYI * texture.Size.width],
        texture.Data[nextPixelXI + pixelYI * texture.Size.width],
        pixelDx);

    // Linear interpolation between x samples at top
    vec3f const interpolatedXColorTop = Mix(
        texture.Data[pixelXI + nextPixelYI * texture.Size.width],
        texture.Data[nextPixelXI + nextPixelYI * texture.Size.width],
        pixelDx);

    // Linear interpolation between two vertical samples
    return Mix(
        interpolatedXColorBottom,
        interpolatedXColorTop,
        pixelDy);
}