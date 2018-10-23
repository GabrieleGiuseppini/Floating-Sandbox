/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureAtlas.h"

#include "GameException.h"
#include "GameMath.h"
#include "ResourceLoader.h"

#include <algorithm>
#include <cstring>

namespace Render {

TextureAtlas TextureAtlasBuilder::BuildAtlas(
    TextureGroup const & group,
    ProgressCallback const & progressCallback)
{
    // Build TextureInfo's
    std::vector<TextureInfo> textureInfos;
    for (auto const & frame : group.GetFrameSpecifications())
    {
        textureInfos.emplace_back(
            frame.Metadata.FrameId,
            frame.Metadata.Size);
    }

    // Build specification
    AtlasSpecification specification = BuildAtlasSpecification(textureInfos);

    // Build atlas
    return BuildAtlas(
        specification,
        [&group](TextureFrameId const & frameId)
        {
            return group.LoadFrame(frameId.FrameIndex);
        },
        progressCallback);
}

TextureAtlas TextureAtlasBuilder::BuildAtlas(
    TextureDatabase const & database,
    ProgressCallback const & progressCallback)
{
    // Build TextureInfo's
    std::vector<TextureInfo> textureInfos;
    for (auto const & group : database.GetGroups())
    {
        for (auto const & frame : group.GetFrameSpecifications())
        {
            textureInfos.emplace_back(
                frame.Metadata.FrameId,
                frame.Metadata.Size);
        }
    }

    // Build specification
    AtlasSpecification specification = BuildAtlasSpecification(textureInfos);

    // Build atlas
    return BuildAtlas(
        specification,
        [&database](TextureFrameId const & frameId)
        {
            return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex);
        },
        progressCallback);
}

TextureAtlasBuilder::AtlasSpecification TextureAtlasBuilder::BuildAtlasSpecification(std::vector<TextureInfo> const & inputTextureInfos)
{
    //
    // Sort input texture info's by height, from tallest to shortest
    //

    std::vector<TextureInfo> sortedTextureInfos = inputTextureInfos;
    std::sort(
        sortedTextureInfos.begin(),
        sortedTextureInfos.end(),
        [](TextureInfo const & a, TextureInfo const & b)
        {
            return a.Size.Height > b.Size.Height;
        });

    //
    // Calculate size of atlas
    //

    uint64_t totalArea = 0;
    for (auto const & ti : sortedTextureInfos)
    {
        totalArea += ti.Size.Width + ti.Size.Height;
    }

    // Square root of area, ceil'd to next power of two
    int const atlasSide = CeilPowerOfTwo(static_cast<int>(std::floor(std::sqrt(static_cast<float>(totalArea)))));
    int atlasWidth = atlasSide;
    int atlasHeight = atlasSide;

    // TODOHERE
    return AtlasSpecification(
        { 
            { {TextureGroupType::Cloud, 4}, 242, 243 }
        },
        ImageSize(atlasWidth, atlasHeight));
}

TextureAtlas TextureAtlasBuilder::BuildAtlas(
    AtlasSpecification const & specification,
    std::function<TextureFrame(TextureFrameId const &)> frameLoader,
    ProgressCallback const & progressCallback)
{
    // Allocate image
    size_t const imageByteSize = specification.AtlasSize.Width * specification.AtlasSize.Height * 4;
    std::unique_ptr<unsigned char[]> atlasImage(new unsigned char[imageByteSize]);

    // Fill image - transparent black
    std::fill_n(atlasImage.get(), imageByteSize, static_cast<unsigned char>(0u));

    // Copy all textures into image, building metadata at the same time
    std::vector<TextureAtlasFrameMetadata> metadata;
    for (auto const & texturePosition : specification.TexturePositions)
    {
        progressCallback(
            static_cast<float>(metadata.size()) / static_cast<float>(specification.TexturePositions.size()),
            "Building texture atlas...");
        
        // Load frame
        TextureFrame textureFrame = frameLoader(texturePosition.FrameId);

        // Copy frame
        CopyImage(
            std::move(textureFrame.TextureData.Data),
            textureFrame.TextureData.Size,
            atlasImage.get(),
            specification.AtlasSize,
            texturePosition.FrameLeftX,
            texturePosition.FrameTopY);

        // Store texture coordinates
        metadata.emplace_back(
            // Bottom-left
            vec2f(
                static_cast<float>(texturePosition.FrameLeftX) / static_cast<float>(specification.AtlasSize.Width),
                static_cast<float>(texturePosition.FrameTopY - textureFrame.TextureData.Size.Height) / static_cast<float>(specification.AtlasSize.Height)),
            // Top-right
            vec2f(
                static_cast<float>(texturePosition.FrameLeftX + textureFrame.TextureData.Size.Width) / static_cast<float>(specification.AtlasSize.Width),
                static_cast<float>(texturePosition.FrameTopY) / static_cast<float>(specification.AtlasSize.Height)),
            textureFrame.Metadata);
    }

    ImageData atlasImageData(
        specification.AtlasSize,
        std::move(atlasImage));

    progressCallback(
        1.0f,
        "Building texture atlas...");

    // TODOTEST
    ResourceLoader::SaveImage(
        "C:\\users\\neurodancer\\desktop\\texture_atlas.png",
        atlasImageData);

    // Return atlas
    return TextureAtlas(
        metadata,
        std::move(atlasImageData));
}

void TextureAtlasBuilder::CopyImage(
    std::unique_ptr<unsigned char const []> sourceImage,
    ImageSize sourceImageSize,
    unsigned char * destImage,
    ImageSize destImageSize,
    int destinationLeftX,
    int destinationTopY)
{
    // From bottom to top
    for (int y = 0; y < sourceImageSize.Height; ++y)
    {
        // From left to right
        unsigned char const * sourceStart = &(sourceImage[y * sourceImageSize.Width]);
        unsigned char * destStart = &(destImage[(destinationTopY - sourceImageSize.Height + y) * destImageSize.Width + destinationLeftX]);
        std::copy_n(sourceStart, sourceImageSize.Width * 4, destStart);
    }
}

}