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

namespace /*anonymous*/ {

    inline float BidirMultiplyBlend(float x1, float x2)
    {
        return (x2 <= 0.5f)
            ? x1 * 2.0f * x2                        // Damper: x1 * [0.0, 1.0]
            : x1 + (x2 - x1) * 2.0f * (x2 - 0.5f);  // Amplifier:x1 + (x2 - x1) * [0.0, 1.0]
    }
}

ShipTexturizer::ShipTexturizer(ResourceLocator const & resourceLocator)
    : mDefaultSettings(
        ShipAutoTexturizationModeType::MaterialTextures,
        1.0f, // MaterialTextureMagnification
        0.0f) // MaterialTextureTransparency
    , mDoForceDefaultSettingsOntoShipSettings(false)
    , mMaterialTexturesFolderPath(resourceLocator.GetMaterialTexturesFolderPath())
    , mMaterialTextureNameToTextureFilePathMap(MakeMaterialTextureNameToTextureFilePathMap(mMaterialTexturesFolderPath))
    , mMaterialTextureCache()
{
}

void ShipTexturizer::VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const
{
    for (auto const & entry : materialDatabase.GetStructuralMaterialsByColorKeys())
    {
        if (entry.second.MaterialTextureName.has_value())
        {
            if (mMaterialTextureNameToTextureFilePathMap.count(*entry.second.MaterialTextureName) == 0)
            {
                throw GameException(
                    "Material texture name \"" + *entry.second.MaterialTextureName + "\""
                    + " specified for material \"" + entry.second.Name + "\" is unknown");
            }
        }
    }
}

RgbaImageData ShipTexturizer::Texturize(
    std::optional<ShipAutoTexturizationSettings> const & shipDefinitionSettings,
    ImageSize const & structureSize,
    ShipBuildPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
    std::vector<ShipBuildPoint> const & points) const
{
    // Zero-out cache usage counts
    ResetMaterialTextureCacheUseCounts();

    //
    // Calculate target texture size: integral multiple of structure size, but without
    // exceeding 4096 (magic number, also max texture size for low-end gfx cards),
    // and no more than 32 times the original size
    //

    int const maxDimension = std::max(structureSize.Width, structureSize.Height);
    assert(maxDimension > 0);

    int const magnificationFactor = std::min(32, std::max(1, 4096 / maxDimension));
    float const magnificationFactorInvF = 1.0f / static_cast<float>(magnificationFactor);

    ImageSize const textureSize = structureSize * magnificationFactor;

    //
    // Prepare constants
    //

    ShipAutoTexturizationSettings const & settings = (mDoForceDefaultSettingsOntoShipSettings || !shipDefinitionSettings.has_value())
        ? mDefaultSettings
        : *shipDefinitionSettings;

    float const materialTextureWorldToPixelConversionFactor =
        MaterialTextureMagnificationToPixelConversionFactor(settings.MaterialTextureMagnification);

    float const materialTextureAlpha = 1.0f - settings.MaterialTextureTransparency;

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

            if (settings.Mode == ShipAutoTexturizationModeType::FlatStructure
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

                assert(settings.Mode == ShipAutoTexturizationMode::MaterialTextures);

                vec3f const structurePixelColorF = structurePixelColor.toVec3f();

                // Get bump map texture
                assert(pointMatrix[x][y].has_value());
                Vec3fImageData const & materialTexture = GetMaterialTexture(points[*pointMatrix[x][y]].StructuralMtl.MaterialTextureName);

                //
                // Fill quad with color multiply-blended with "bump map" texture
                //

                int const baseTargetQuadOffset = ((x - 1) + (y - 1) * textureSize.Width) * magnificationFactor;

                float worldY = static_cast<float>(y - 1);
                for (int yy = 0; yy < magnificationFactor; ++yy, worldY += magnificationFactorInvF)
                {
                    int const targetQuadOffset = baseTargetQuadOffset + yy * textureSize.Width;

                    float worldX = static_cast<float>(x - 1);
                    for (int xx = 0; xx < magnificationFactor; ++xx, worldX += magnificationFactorInvF)
                    {
                        vec3f const bumpMapSample = SampleTexture(
                            materialTexture,
                            worldX * materialTextureWorldToPixelConversionFactor,
                            worldY * materialTextureWorldToPixelConversionFactor);

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

std::unordered_map<std::string, std::filesystem::path> ShipTexturizer::MakeMaterialTextureNameToTextureFilePathMap(std::filesystem::path const materialTexturesFolderPath)
{
    std::unordered_map<std::string, std::filesystem::path> materialTextureNameToTextureFilePath;

    for (auto const & entryIt : std::filesystem::directory_iterator(materialTexturesFolderPath))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            std::string const textureName = entryIt.path().stem().string();

            assert(materialTextureNameToTextureFilePath.count(textureName) == 0);
            materialTextureNameToTextureFilePath[textureName] = entryIt.path();
        }
    }

    return materialTextureNameToTextureFilePath;
}

float ShipTexturizer::MaterialTextureMagnificationToPixelConversionFactor(float magnification)
{
    // Magic number
    return 1.0f / (0.08f * magnification);
}

Vec3fImageData const & ShipTexturizer::GetMaterialTexture(std::optional<std::string> const & textureName) const
{
    std::string const actualTextureName = textureName.value_or("none");

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
    int32_t pixelXI = FastTruncateInt32(pixelX);
    int32_t pixelYI = FastTruncateInt32(pixelY);

    // Fractional part between index and next index
    float const pixelDx = pixelX - pixelXI;
    float const pixelDy = pixelY - pixelYI;

    // Wrap integral coordinates
    pixelXI %= texture.Size.Width;
    pixelYI %= texture.Size.Height;

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