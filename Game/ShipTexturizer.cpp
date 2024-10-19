/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipTexturizer.h"

#include "ImageFileTools.h"

#include <GameCore/GameChronometer.h>
#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <chrono>

size_t constexpr MaterialTextureCacheSizeHighWatermark = 40;
size_t constexpr MaterialTextureCacheSizeLowWatermark = 25;

std::string const MaterialTextureNameNone = "none";

namespace /*anonymous*/ {

    inline vec3f BidirMultiplyBlend(
        vec3f const & inputColor,
        vec2f const & bumpMapSample)
    {
        if (bumpMapSample.x <= 0.5f)
        {
            // Damper: x1 * [0.0, 1.0]
            return inputColor * 2.0f * bumpMapSample.x;
        }
        else
        {
            // Amplifier: x1 + (x2 - x1) * [0.0, 1.0]
            float const factor = 2.0f * (bumpMapSample.x - 0.5f);
            return vec3f(
                inputColor.x + (bumpMapSample.x - inputColor.x) * factor,
                inputColor.y + (bumpMapSample.x - inputColor.y) * factor,
                inputColor.z + (bumpMapSample.x - inputColor.z) * factor);
        }
    }
}

ShipTexturizer::ShipTexturizer(
    MaterialDatabase const & materialDatabase,
    ResourceLocator const & resourceLocator)
    : mSharedSettings() // Default settings
    , mDoForceSharedSettingsOntoShipSettings(false)
    , mMaterialTextureNameToTextureFilePathMap(
        MakeMaterialTextureNameToTextureFilePathMap(materialDatabase, resourceLocator))
    , mMaterialTextureCache()
{
}

int ShipTexturizer::CalculateHighDefinitionTextureMagnificationFactor(
    ShipSpaceSize const & shipSize,
    int maxTextureSize)
{
    //
    // Calculate target texture size: integral multiple of structure size, but without
    // exceeding maxTextureSize (magic number, also max texture size for low-end gfx cards),
    // and no more than 32 times the original size
    //

    int const maxDimension = std::max(shipSize.width, shipSize.height);
    assert(maxDimension > 0);

    int const magnificationFactor = std::min(32, std::max(1, maxTextureSize / maxDimension));

    return magnificationFactor;
}

RgbaImageData ShipTexturizer::MakeAutoTexture(
    StructuralLayerData const & structuralLayer,
    std::optional<ShipAutoTexturizationSettings> const & settings,
    int maxTextureSize) const
{
    auto const startTime = GameChronometer::now();

    // Zero-out cache usage counts
    ResetMaterialTextureCacheUseCounts();

    // Calculate texture size
    ShipSpaceSize const shipSize = structuralLayer.Buffer.Size;
    int magnificationFactor = CalculateHighDefinitionTextureMagnificationFactor(shipSize, maxTextureSize);
    ImageSize const textureSize = ImageSize(
        shipSize.width * magnificationFactor,
        shipSize.height * magnificationFactor);

    // Allocate texture image
    RgbaImageData texture = RgbaImageData(textureSize);

    // Nail down settings
    ShipAutoTexturizationSettings const & actualSettings = (mDoForceSharedSettingsOntoShipSettings || !settings.has_value())
        ? mSharedSettings
        : *settings;

    // Texturize
    AutoTexturizeInto(
        structuralLayer,
        ShipSpaceRect({ 0, 0 }, shipSize), // Whole quad
        texture,
        magnificationFactor,
        actualSettings);

    LogMessage("ShipTexturizer: completed auto-texturization:",
        " shipSize=", shipSize, " textureSize=", textureSize,
        " time=", std::chrono::duration_cast<std::chrono::microseconds>(GameChronometer::now() - startTime).count(), "us");

    return texture;
}

void ShipTexturizer::AutoTexturizeInto(
    StructuralLayerData const & structuralLayer,
    ShipSpaceRect const & structuralLayerRegion,
    RgbaImageData & targetTextureImage,
    int magnificationFactor,
    ShipAutoTexturizationSettings const & settings) const
{
    //
    // Prepare constants
    //

    assert((targetTextureImage.Size.width % structuralLayer.Buffer.Size.width) == 0
        && (targetTextureImage.Size.height % structuralLayer.Buffer.Size.height) == 0);
    assert(magnificationFactor == targetTextureImage.Size.width / structuralLayer.Buffer.Size.width);
    assert(magnificationFactor == targetTextureImage.Size.height / structuralLayer.Buffer.Size.height);

    int const targetTextureWidth = targetTextureImage.Size.width;

    float const magnificationFactorInvF = 1.0f / static_cast<float>(magnificationFactor);

    float const worldToMaterialTexturePixelConversionFactor =
        MaterialTextureMagnificationToPixelConversionFactor(settings.MaterialTextureMagnification);

    float const materialTextureAlpha = 1.0f - settings.MaterialTextureTransparency;

    struct XInterpolationData
    {
        register_int pixelXI;
        float pixelDx;
        register_int nextPixelXI;
    };

    std::vector<XInterpolationData> xInterpolationData;
    xInterpolationData.resize(magnificationFactor);

    //
    // Populate texture
    //

    auto targetImageData = targetTextureImage.Data.get();
    auto const & structuralBuffer = structuralLayer.Buffer;

    int const startY = structuralLayerRegion.origin.y;
    int const endY = structuralLayerRegion.origin.y + structuralLayerRegion.size.height;

    int const startX = structuralLayerRegion.origin.x;
    int const endX = structuralLayerRegion.origin.x + structuralLayerRegion.size.width;

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            ShipSpaceCoordinates const coords = ShipSpaceCoordinates(x, y);

            // Get structure pixel color
            StructuralMaterial const * const structuralMaterial = structuralBuffer[coords].Material;
            rgbaColor const structurePixelColor = structuralMaterial != nullptr
                ? structuralMaterial->RenderColor
                : rgbaColor::zero(); // Fully transparent

            if (settings.Mode == ShipAutoTexturizationModeType::FlatStructure
                || structuralMaterial == nullptr)
            {
                //
                // Flat structure/transparent
                //

                // Fill quad with color
                for (int yy = 0; yy < magnificationFactor; ++yy)
                {
                    int const quadOffset =
                        x * magnificationFactor
                        + (y * magnificationFactor + yy) * targetTextureWidth;

                    for (int xx = 0; xx < magnificationFactor; ++xx)
                    {
                        targetImageData[quadOffset + xx] = structurePixelColor;
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
                assert(structuralMaterial != nullptr);
                Vec2fImageData const & materialTexture = GetMaterialTexture(structuralMaterial->MaterialTextureName);

                //
                // Prepare bilinear interpolation along X
                //

                float pixelX = static_cast<float>(x) * worldToMaterialTexturePixelConversionFactor;
                for (int xx = 0; xx < magnificationFactor; ++xx, pixelX += magnificationFactorInvF * worldToMaterialTexturePixelConversionFactor)
                {
                    // Integral part
                    xInterpolationData[xx].pixelXI = FastTruncateToArchInt(pixelX);

                    // Fractional part between index and next index
                    xInterpolationData[xx].pixelDx = pixelX - xInterpolationData[xx].pixelXI;

                    // Wrap integral coordinates
                    xInterpolationData[xx].pixelXI %= static_cast<register_int>(materialTexture.Size.width);

                    // Next X
                    xInterpolationData[xx].nextPixelXI = (xInterpolationData[xx].pixelXI + 1) % static_cast<register_int>(materialTexture.Size.width);

                    assert(xInterpolationData[xx].pixelXI >= 0 && xInterpolationData[xx].pixelXI < materialTexture.Size.width);
                    assert(xInterpolationData[xx].pixelDx >= 0.0f && xInterpolationData[xx].pixelDx < 1.0f);
                    assert(xInterpolationData[xx].nextPixelXI >= 0 && xInterpolationData[xx].nextPixelXI < materialTexture.Size.width);
                }

                //
                // Fill quad with color multiply-blended with "bump map" texture
                //

                int const baseTargetQuadOffset = (x + y * targetTextureWidth) * magnificationFactor;

                float worldY = static_cast<float>(y);
                for (int yy = 0; yy < magnificationFactor; ++yy, worldY += magnificationFactorInvF)
                {
                    int const targetQuadOffset = baseTargetQuadOffset + yy * targetTextureWidth;

                    //
                    // Prepare bilinear interpolation for Y
                    //

                    float const pixelY = worldY * worldToMaterialTexturePixelConversionFactor;

                    // Integral part
                    auto pixelYI = FastTruncateToArchInt(pixelY);

                    // Fractional part between index and next index
                    float const pixelDy = pixelY - pixelYI;

                    // Wrap integral coordinates
                    pixelYI %= static_cast<decltype(pixelYI)>(materialTexture.Size.height);
                    auto const pixelYIOffset = pixelYI * materialTexture.Size.width;

                    // Next Y
                    auto const nextPixelYI = (pixelYI + 1) % static_cast<decltype(pixelYI)>(materialTexture.Size.height);
                    auto const nextPixelYIOffset = nextPixelYI * materialTexture.Size.width;;

                    assert(pixelYI >= 0 && pixelYI < materialTexture.Size.height);
                    assert(pixelDy >= 0.0f && pixelDy < 1.0f);
                    assert(nextPixelYI >= 0 && nextPixelYI < materialTexture.Size.height);

                    //
                    // Loop for all Xs
                    //

                    for (int xx = 0; xx < magnificationFactor; ++xx)
                    {
                        //
                        // Bilinear interpolation for X
                        //

                        // Linear interpolation between x samples at bottom
                        vec2f const interpolatedXColorBottom = Mix(
                            materialTexture.Data[xInterpolationData[xx].pixelXI + pixelYIOffset],
                            materialTexture.Data[xInterpolationData[xx].nextPixelXI + pixelYIOffset],
                            xInterpolationData[xx].pixelDx);

                        // Linear interpolation between x samples at top
                        vec2f const interpolatedXColorTop = Mix(
                            materialTexture.Data[xInterpolationData[xx].pixelXI + nextPixelYIOffset],
                            materialTexture.Data[xInterpolationData[xx].nextPixelXI + nextPixelYIOffset],
                            xInterpolationData[xx].pixelDx);

                        // Linear interpolation between two vertical samples
                        vec2f const bumpMapSample = Mix(
                            interpolatedXColorBottom,
                            interpolatedXColorTop,
                            pixelDy);

                        //
                        // Bi-directional multiply blending between structural color and bumpmap sample "value" (just r),
                        // blended again with structural color via material transparency
                        //

                        float const whateverFactor = (2.0f * bumpMapSample.x - 1.0f) * materialTextureAlpha;

                        vec3f resultantColor;
                        if (bumpMapSample.x <= 0.5f)
                        {
                            // Damper: input * [0.0, 1.0]
                            // Then: mix of input and of result of multiply-blend, via materialTextureAlpha
                            resultantColor = structurePixelColorF * (1.0f + whateverFactor);
                        }
                        else
                        {
                            // Amplifier: input + (bump - input) * [0.0, 1.0]
                            // Then: mix of input and of result of multiply-blend, via materialTextureAlpha
                            float const bFactor = bumpMapSample.x * whateverFactor;
                            resultantColor = structurePixelColorF * (1.0f - whateverFactor) + vec3f(bFactor, bFactor, bFactor);
                        }

                        // Store resultant color, using structure's alpha channel value as the final alpha
                        targetImageData[targetQuadOffset + xx] = rgbaColor(
                            resultantColor,
                            structurePixelColor.a);
                    }
                }
            }
        }
    }
}

RgbaImageData ShipTexturizer::MakeInteriorViewTexture(
    Physics::Triangles const & triangles,
    Physics::Points const & points,
    ShipSpaceSize const & shipSize,
    RgbaImageData const & backgroundTexture) const
{
    auto const startTime = GameChronometer::now();

    //
    // Start with a copy of the background
    //

    RgbaImageData interiorView = backgroundTexture.Clone();

    //
    // Visit all triangles and render their floors
    //

    vec2f const shipSizeF = shipSize.ToFloat();
    vec2f const textureSizeF = interiorView.Size.ToFloat();

    // Size of the quad occupied by two triangles adjoined along
    // their diagonals, in pixels
    ImageSize const quadSize = ImageSize::FromFloatFloor(textureSizeF / shipSizeF);

    // Thickness of a floor, in pixels
    //
    // Futurework: should incorporate ship's scale, as now we calculate thickness assuming
    // width and height are 1:1 with meters
    int const floorThickness = std::max(
        std::max(
            quadSize.width / 10,
            quadSize.height / 10),
        2);

    for (auto const t : triangles)
    {
        DrawTriangleFloorInto(
            t,
            points,
            triangles,
            textureSizeF,
            floorThickness,
            interiorView);
    }

    LogMessage("ShipTexturizer: completed interior view:",
        " shipSize=", shipSize, " textureSize=", interiorView.Size,
        " time=", std::chrono::duration_cast<std::chrono::microseconds>(GameChronometer::now() - startTime).count(), "us");

    return interiorView;
}

void ShipTexturizer::RenderShipInto(
    StructuralLayerData const & structuralLayer,
    ShipSpaceRect const & structuralLayerRegion,
    RgbaImageData const & sourceTextureImage,
    RgbaImageData & targetTextureImage,
    int magnificationFactor) const
{
    rgbaColor constexpr TransparentColor = rgbaColor::zero(); // Fully transparent

    //
    // Expectations:
    //
    // - The size of the target texture image is an integral multiple of the size of the structural layer
    // - The ratio of the structural layer dimensions is the same as the ratio of the source texture image
    //

    //
    // Prepare constants
    //

    assert((targetTextureImage.Size.width % structuralLayer.Buffer.Size.width) == 0
        && (targetTextureImage.Size.height % structuralLayer.Buffer.Size.height) == 0);
    assert(magnificationFactor == targetTextureImage.Size.width / structuralLayer.Buffer.Size.width);
    assert(magnificationFactor == targetTextureImage.Size.height / structuralLayer.Buffer.Size.height);

    int const targetTextureWidth = targetTextureImage.Size.width;

    float const sourcePixelsPerShipParticleX = static_cast<float>(sourceTextureImage.Size.width) / static_cast<float>(structuralLayer.Buffer.Size.width);
    float const sourcePixelsPerShipParticleY = static_cast<float>(sourceTextureImage.Size.height) / static_cast<float>(structuralLayer.Buffer.Size.height);

    //
    // Here we sample the texture with an offset of half of a "ship pixel" (which is multiple texture pixels) on both sides,
    // in the same way as we do when we build the ship at simulation time.
    // We do this so that the texture for a particle at ship coords (x, y) is sampled at the center of the
    // texture's quad for that particle.
    //

    float const sampleOffsetX = sourcePixelsPerShipParticleX / 2.0f;
    float const sampleOffsetY = sourcePixelsPerShipParticleY / 2.0f;

    float const targetTextureSpaceToShipTextureSpace = 1.0f / static_cast<float>(magnificationFactor);

    // Src = Offset + shipSpaceToSourceTextureSpace * Ship

    // At ShipX = ShipWidth - 1 (right edge) we want SrcX = SrcWidth - 1 - OffsetX
    float const shipSpaceToSourceTextureSpaceX = structuralLayer.Buffer.Size.width > 1
        ? (static_cast<float>(sourceTextureImage.Size.width) - 1.0f - sourcePixelsPerShipParticleX) / (static_cast<float>(structuralLayer.Buffer.Size.width) - 1.0f)
        : 0.0f;

    // At ShipY = ShipHeight - 1 (top edge) we want SrcY = SrcHeight - 1 - OffsetY
    float const shipSpaceToSourceTextureSpaceY = structuralLayer.Buffer.Size.height > 1
        ? (static_cast<float>(sourceTextureImage.Size.height) - 1.0f - sourcePixelsPerShipParticleY) / (static_cast<float>(structuralLayer.Buffer.Size.height) - 1.0f)
        : 0.0f;

    // Combine
    float const targetTextureSpaceToSourceTextureSpaceX =
        targetTextureSpaceToShipTextureSpace
        * shipSpaceToSourceTextureSpaceX;
    float const targetTextureSpaceToSourceTextureSpaceY =
        targetTextureSpaceToShipTextureSpace
        * shipSpaceToSourceTextureSpaceY;

    //
    // Populate texture
    //

    ShipSpaceSize const structuralSize = structuralLayer.Buffer.Size;
    auto const & structuralBuffer = structuralLayer.Buffer;
    auto targetImageData = targetTextureImage.Data.get();

    int const startY = structuralLayerRegion.origin.y;
    int const endY = structuralLayerRegion.origin.y + structuralLayerRegion.size.height;

    int const startX = structuralLayerRegion.origin.x;
    int const endX = structuralLayerRegion.origin.x + structuralLayerRegion.size.width;

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            //
            // We now populate the target texture in the quad whose corners lie at these coordinates (in the target texture):
            //
            // 3:(x * magnificationFactor, (y + 1) * magnificationFactor) ... 4:((x + 1) * magnificationFactor, (y + 1) * magnificationFactor)
            // ...
            // ...
            // ...
            // 1:[x * magnificationFactor, y * magnificationFactor] ... 2:((x + 1) * magnificationFactor, y * magnificationFactor)
            //
            // We actually populate quads or triangles (with |side|==magnificationFactor), depending on the presence of the four corners. We do so by:
            //  - Looping for all target YY's in the quad
            //  - For each YY:
            //      - Fill-in the XX segment between xxStart and xxEnd, and transparent outside (prefix and suffix)
            //      - Change xxStart and xxEnd depending on Y
            //

            //
            // Determine quad vertices
            //

            // Init with no quad - prefix only
            int xxStart = magnificationFactor, xxStartIncr = 0;
            int xxEnd = magnificationFactor, xxEndIncr = 0;

            bool const hasVertex1 = structuralBuffer[{x, y}].Material != nullptr;

            ShipSpaceCoordinates const coords2 = ShipSpaceCoordinates(x + 1, y);
            bool const hasVertex2 = coords2.IsInSize(structuralSize) && structuralBuffer[coords2].Material != nullptr;

            ShipSpaceCoordinates const coords3 = ShipSpaceCoordinates(x, y + 1);
            bool const hasVertex3 = coords3.IsInSize(structuralSize) && structuralBuffer[coords3].Material != nullptr;

            ShipSpaceCoordinates const coords4 = ShipSpaceCoordinates(x + 1, y + 1);
            bool const hasVertex4 = coords4.IsInSize(structuralSize) && structuralBuffer[coords4].Material != nullptr;

            if (hasVertex1)
            {
                if (hasVertex2)
                {
                    if (hasVertex3)
                    {
                        if (hasVertex4)
                        {
                            // Whole quad
                            xxStart = 0; xxStartIncr = 0;
                            xxEnd = magnificationFactor; xxEndIncr = 0;
                        }
                        else
                        {
                            // 3
                            // |
                            // 1---2

                            xxStart = 0; xxStartIncr = 0;
                            xxEnd = magnificationFactor; xxEndIncr = -1;
                        }
                    }
                    else if (hasVertex4)
                    {
                        //     4
                        //     |
                        // 1---2

                        xxStart = 0; xxStartIncr = 1;
                        xxEnd = magnificationFactor; xxEndIncr = 0;
                    }
                }
                else
                {
                    // No vertex 2

                    if (hasVertex3 && hasVertex4)
                    {
                        // 3---4
                        // |
                        // 1

                        xxStart = 0; xxStartIncr = 0;
                        xxEnd = 1; xxEndIncr = 1;
                    }
                }
            }
            else
            {
                // No vertex 1

                if (hasVertex2 && hasVertex3 && hasVertex4)
                {
                    // 3---4
                    //     |
                    //     2

                    xxStart = magnificationFactor - 1; xxStartIncr = -1;
                    xxEnd = magnificationFactor; xxEndIncr = 0;
                }
            }

            //
            // Fill-in quad
            //

            int targetQuadOffset =
                (y * magnificationFactor) * targetTextureWidth
                + x * magnificationFactor;

            for (int yy = 0;
                yy < magnificationFactor;
                ++yy, xxStart += xxStartIncr, xxEnd += xxEndIncr, targetQuadOffset += targetTextureWidth)
            {
                // Prefix - fill with empty
                assert(0 <= xxStart && xxStart <= magnificationFactor);
                for (int xx = 0; xx < xxStart; ++xx)
                {
                    targetImageData[targetQuadOffset + xx] = TransparentColor;
                }

                // Body - fill with source texture
                for (int xx = xxStart; xx < xxEnd; ++xx)
                {
                    rgbaColor const textureSample = SampleTextureBilinearConstrained(
                        sourceTextureImage,
                        sampleOffsetX + targetTextureSpaceToSourceTextureSpaceX * (x * magnificationFactor + xx),
                        sampleOffsetY + targetTextureSpaceToSourceTextureSpaceY * (y * magnificationFactor + yy));

                    targetImageData[targetQuadOffset + xx] = textureSample;
                }

                // Suffix - fill with empty
                assert(0 <= xxEnd && xxEnd <= magnificationFactor);
                for (int xx = xxEnd; xx < magnificationFactor; ++xx)
                {
                    targetImageData[targetQuadOffset + xx] = TransparentColor;
                }
            }
        }
    }
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

RgbaImageData ShipTexturizer::MakeMaterialTextureSample(
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
    Vec2fImageData const & materialTexture = GetMaterialTexture(textureName);
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
            vec2f const bumpMapSample = SampleTextureBilinearRepeated(
                materialTexture,
                static_cast<float>(x) * sampleToMaterialTexturePixelConversionFactor,
                static_cast<float>(y) * sampleToMaterialTexturePixelConversionFactor);

            // Bi-directional multiply blending
            vec3f const resultantColorF = BidirMultiplyBlend(renderPixelColorF, bumpMapSample);

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

ShipTexturizer::Vec2fImageData const & ShipTexturizer::GetMaterialTexture(std::optional<std::string> const & textureName) const
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

        // Load texture
        assert(mMaterialTextureNameToTextureFilePathMap.count(actualTextureName) > 0);
        RgbImageData texture = ImageFileTools::LoadImageRgb(mMaterialTextureNameToTextureFilePathMap.at(actualTextureName));

        // Convert to vec2f
        auto const pixelCount = texture.Size.GetLinearSize();
        std::unique_ptr<vec2f[]> vec2fTexture = std::make_unique<vec2f[]>(pixelCount);
        for (size_t p = 0; p < pixelCount; ++p)
        {
            assert(texture.Data[p].r == texture.Data[p].g);
            assert(texture.Data[p].r == texture.Data[p].b);

            vec2fTexture[p] = vec2f(
                static_cast<float>(texture.Data[p].r) / 255.0f,
                1.0f); // Alpha: at this moment we hardcode it as opaque, we'll think whether we want to make transparent chains
        }

        // Insert texture into cache
        auto const inserted = mMaterialTextureCache.emplace(
            actualTextureName,
            Vec2fImageData(texture.Size, std::move(vec2fTexture)));

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

void ShipTexturizer::DrawTriangleFloorInto(
    ElementIndex triangleIndex,
    Physics::Points const & points,
    Physics::Triangles const & triangles,
    vec2f const & textureSizeF,
    int const floorThickness,
    RgbaImageData & targetTextureImage) const
{
    //
    // 1. Find minima and maxima
    //

    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();

    for (int v = 0; v < 3; ++v)
    {
        ElementIndex const pointIndex = triangles.GetPointIndices(triangleIndex)[v];
        vec2f const pointTextureCoords = points.GetTextureCoordinates(pointIndex);
        ImageCoordinates const endpoint = ImageCoordinates::FromFloatRound(pointTextureCoords * textureSizeF);

        minX = std::min(minX, endpoint.x);
        maxX = std::max(maxX, endpoint.x);
        minY = std::min(minY, endpoint.y);
        maxY = std::max(maxY, endpoint.y);
    }

    //
    // 2. Visit all edges
    //

    for (int e = 0; e < 3; ++e)
    {
        if (triangles.GetSubSpringNpcFloorKind(triangleIndex, e) != NpcFloorKindType::NotAFloor)
        {
            // Map edge's endpoints to pixels in texture

            ElementIndex const pointAIndex = triangles.GetPointIndices(triangleIndex)[e];
            vec2f const pointATextureCoords = points.GetTextureCoordinates(pointAIndex);
            ImageCoordinates const endpointA = ImageCoordinates::FromFloatRound(pointATextureCoords * textureSizeF);

            ElementIndex const pointBIndex = triangles.GetPointIndices(triangleIndex)[(e + 1) % 3];
            vec2f const pointBTextureCoords = points.GetTextureCoordinates(pointBIndex);
            ImageCoordinates const endpointB = ImageCoordinates::FromFloatRound(pointBTextureCoords * textureSizeF);

            assert(floorThickness >= 2);

            ImageCoordinates const & endpointBottom = (endpointA.y <= endpointB.y) ? endpointA : endpointB;
            ImageCoordinates const & endpointTop = (endpointA.y <= endpointB.y) ? endpointB : endpointA;

            // Check direction
            if (endpointA.x == endpointB.x)
            {
                // Vertical
                assert(endpointA.y != endpointB.y);

                int const yStart = endpointBottom.y - floorThickness / 2;
                int const yEnd = endpointTop.y + floorThickness / 2 - 1; // Included

                if (endpointA.x == minX)
                {
                    // Left |

                    DrawHVEdgeFloorInto(
                        minX - floorThickness / 2, // xStart
                        minX + floorThickness / 2 - 1, // xEnd, included
                        1, // xIncr,
                        0, // xLimitIncr
                        yStart,
                        yEnd,
                        1, // yIncr
                        targetTextureImage);
                }
                else
                {
                    // Right |

                    assert(endpointA.x == maxX);

                    DrawHVEdgeFloorInto(
                        maxX - floorThickness / 2, // xStart
                        maxX + floorThickness / 2 - 1, // xEnd, included
                        1, // xIncr,
                        0, // xLimitIncr
                        yStart,
                        yEnd,
                        1, // yIncr
                        targetTextureImage);
                }
            }
            else if (endpointA.y == endpointB.y)
            {
                // Horizontal

                if (endpointA.y == minY)
                {
                    // Bottom -

                    DrawHVEdgeFloorInto(
                        minX - floorThickness / 2, // xStart
                        maxX + floorThickness / 2 - 1, // xEnd, included
                        1, // xIncr
                        0, // xLimitIncr
                        minY - floorThickness / 2, // yStart
                        minY + floorThickness / 2 - 1, // yEnd, included
                        1, // yIncr
                        targetTextureImage);
                }
                else
                {
                    // Top -

                    assert(endpointA.y == maxY);

                    DrawHVEdgeFloorInto(
                        minX - floorThickness / 2, // xStart
                        maxX + floorThickness / 2 - 1, // xEnd, included
                        1, // xIncr
                        0, // xLimitIncr
                        maxY - floorThickness / 2, // yStart
                        maxY + floorThickness / 2  - 1, // yEnd, included
                        1, // yIncr
                        targetTextureImage);
                }
            }
            else
            {
                // Diagonal

                // We draw from bottom to top, and with an extra pixel on either left and right side for anti-aliasing


                int const yStart = endpointBottom.y - floorThickness / 2;
                int const yEnd = endpointTop.y + floorThickness / 2  - 1; // Included

                if (endpointBottom.x <= endpointTop.x)
                {
                    // Left-Right /

                    DrawDEdgeFloorInto(
                        (minX - floorThickness / 2 - 1) - 1, // xStart
                        (minX + floorThickness / 2 - 1) + 1, // xEnd, included
                        1, // xIncr
                        1, // xLimitIncr
                        minX - floorThickness / 2, // absoluteMinX
                        maxX + floorThickness / 2 - 1, // absoluteMaxX
                        yStart,
                        yEnd,
                        1, // yIncr
                        targetTextureImage);
                }
                else
                {
                    // Right-Left \

                    DrawDEdgeFloorInto(
                        (maxX - floorThickness / 2) - 1,
                        (maxX + floorThickness / 2) + 1,
                        1, // xIncr
                        -1, // xLimitIncr
                        minX - floorThickness / 2, // absoluteMinX
                        maxX + floorThickness / 2 - 1, // absoluteMaxX
                        yStart,
                        yEnd,
                        1, // yIncr
                        targetTextureImage);
                }
            }
        }
    }
}

void ShipTexturizer::DrawHVEdgeFloorInto(
    int xStart,
    int xEnd, // Included
    int xIncr,
    int xLimitIncr,
    int yStart,
    int yEnd, // Included
    int yIncr,
    RgbaImageData & targetTextureImage) const
{
    rgbaColor constexpr FloorColor = rgbaColor(0, 0, 0, rgbaColor::data_type_max);

    for (int y = yStart; ; y += yIncr)
    {
        for (int x = xStart; ; x += xIncr)
        {
            targetTextureImage[{x, y}] = FloorColor;

            if (x == xEnd)
            {
                break;
            }
        }

        if (y == yEnd)
        {
            break;
        }

        xStart += xLimitIncr;
        xEnd += xLimitIncr;
    }
}

void ShipTexturizer::DrawDEdgeFloorInto(
    int xStart,
    int xEnd, // Included
    int xIncr,
    int xLimitIncr,
    int absoluteMinX,
    int absoluteMaxX,
    int yStart,
    int yEnd, // Included
    int yIncr,
    RgbaImageData & targetTextureImage) const
{
    vec4f constexpr FloorColor = vec4f(0.0f, 0.0f, 0.0f, 1.0f);

    for (int y = yStart; ; y += yIncr)
    {
        for (int x = xStart; ; x += xIncr)
        {
            if (x >= absoluteMinX && x <= absoluteMaxX)
            {
                targetTextureImage[{x, y}] = rgbaColor(
                    Mix(
                        targetTextureImage[{x, y}].toVec4f(),
                        FloorColor,
                        (x == xStart || x == xEnd) ? 0.20f : 1.0f));
            }

            if (x == xEnd)
            {
                break;
            }
        }

        if (y == yEnd)
        {
            break;
        }

        xStart += xLimitIncr;
        xEnd += xLimitIncr;
    }
}

rgbaColor ShipTexturizer::SampleTextureBilinearConstrained(
    RgbaImageData const & texture,
    float pixelX,
    float pixelY) const
{
    // Integral part
    auto pixelXI = FastTruncateToArchInt(pixelX);
    auto pixelYI = FastTruncateToArchInt(pixelY);

    // Fractional part between index and next index
    float const pixelDx = pixelX - pixelXI;
    float const pixelDy = pixelY - pixelYI;

    assert(pixelXI >= 0 && pixelXI < texture.Size.width);
    assert(pixelDx >= 0.0f && pixelDx < 1.0f);
    assert(pixelYI >= 0 && pixelYI < texture.Size.height);
    assert(pixelDy >= 0.0f && pixelDy < 1.0f);

    //
    // Bilinear
    //

    auto const nextPixelXI = pixelXI + 1;
    auto const nextPixelYI = pixelYI + 1;

    assert(nextPixelXI < texture.Size.width);
    assert(nextPixelYI < texture.Size.height);

    // Linear interpolation between x samples at bottom
    vec4f const interpolatedXColorBottom = Mix(
        texture.Data[pixelXI + pixelYI * texture.Size.width].toVec4f(),
        texture.Data[nextPixelXI + pixelYI * texture.Size.width].toVec4f(),
        pixelDx);

    // Linear interpolation between x samples at top
    vec4f const interpolatedXColorTop = Mix(
        texture.Data[pixelXI + nextPixelYI * texture.Size.width].toVec4f(),
        texture.Data[nextPixelXI + nextPixelYI * texture.Size.width].toVec4f(),
        pixelDx);

    // Linear interpolation between two vertical samples
    return rgbaColor(
        Mix(
            interpolatedXColorBottom,
            interpolatedXColorTop,
            pixelDy));
}

vec2f ShipTexturizer::SampleTextureBilinearRepeated(
    Vec2fImageData const & texture,
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
    vec2f const interpolatedXColorBottom = Mix(
        texture.Data[pixelXI + pixelYI * texture.Size.width],
        texture.Data[nextPixelXI + pixelYI * texture.Size.width],
        pixelDx);

    // Linear interpolation between x samples at top
    vec2f const interpolatedXColorTop = Mix(
        texture.Data[pixelXI + nextPixelYI * texture.Size.width],
        texture.Data[nextPixelXI + nextPixelYI * texture.Size.width],
        pixelDx);

    // Linear interpolation between two vertical samples
    return Mix(
        interpolatedXColorBottom,
        interpolatedXColorTop,
        pixelDy);
}