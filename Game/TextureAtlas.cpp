/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureAtlas.h"

#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>

#include <algorithm>
#include <cstring>

namespace Render {

TextureAtlas TextureAtlasBuilder::BuildAtlas(
    TextureGroup const & group,
    ProgressCallback const & progressCallback)
{
    // Build TextureInfo's
    std::vector<TextureInfo> textureInfos;
    AddTextureInfos(group, textureInfos);

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
        AddTextureInfos(group, textureInfos);
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

TextureAtlas TextureAtlasBuilder::BuildAtlas(ProgressCallback const & progressCallback)
{
    // Build TextureInfo's
    std::vector<TextureInfo> textureInfos;
    for (auto const & frameSpecification : mTextureFrameSpecifications)
    {
        textureInfos.emplace_back(
            frameSpecification.second.Metadata.FrameId,
            frameSpecification.second.Metadata.Size);
    }

    // Build specification
    AtlasSpecification specification = BuildAtlasSpecification(textureInfos);

    // Build atlas
    return BuildAtlas(
        specification,
        [this](TextureFrameId const & frameId)
        {
            return this->mTextureFrameSpecifications.at(frameId).LoadFrame();
        },
        progressCallback);
}

/////////////////////////////////////////////////////////////////////////////////////

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
            return a.Size.Height > b.Size.Height
                || (a.Size.Height == b.Size.Height && a.Size.Width > b.Size.Width);
        });


    //
    // Calculate size of atlas
    //

    uint64_t totalArea = 0;
    for (auto const & ti : sortedTextureInfos)
    {
        // Verify tile dimensions are powers of two
        if (ti.Size.Width != CeilPowerOfTwo(ti.Size.Width)
            || ti.Size.Height != CeilPowerOfTwo(ti.Size.Height))
        {
            throw GameException("Dimensions of texture frame \"" + ti.FrameId.ToString() + "\" are not a power of two");
        }

        totalArea += ti.Size.Width * ti.Size.Height;
    }

    // Square root of area, floor'd to next power of two, minimized
    int const atlasSide = CeilPowerOfTwo(static_cast<int>(std::floor(std::sqrt(static_cast<float>(totalArea))))) / 2;
    int atlasWidth = atlasSide;
    int atlasHeight = atlasSide;


    //
    // Place tiles
    //

    std::vector<AtlasSpecification::TexturePosition> texturePositions;
    texturePositions.reserve(inputTextureInfos.size());

    struct Position
    {
        int x;
        int y;

        Position(
            int _x,
            int _y)
            : x(_x)
            , y(_y)
        {}
    };

    std::vector<Position> positionStack;
    positionStack.emplace_back(0, 0);

    for (TextureInfo const & t : sortedTextureInfos)
    {
        while (true)
        {
            Position const currentPosition = positionStack.back();

            if (currentPosition.x + t.Size.Width < atlasWidth   // Fits at current position
                || positionStack.size() == 1                    // We can't backtrack
                || (CeilPowerOfTwo(currentPosition.x + t.Size.Width) - atlasWidth) <= (CeilPowerOfTwo(positionStack.front().y + t.Size.Height) - atlasHeight)) // Extra W <= Extra H
            {
                // Put it at the current location
                texturePositions.emplace_back(
                    t.FrameId,
                    currentPosition.x,
                    currentPosition.y);

                if (positionStack.size() == 1
                    || currentPosition.y + t.Size.Height < std::next(positionStack.rbegin())->y)
                {
                    // Move current location up to top
                    positionStack.back().y += t.Size.Height;
                }
                else
                {
                    // Current location is completed
                    assert(currentPosition.y + t.Size.Height == std::next(positionStack.rbegin())->y);

                    // Pop it from stack
                    positionStack.pop_back();
                }

                // Add new location to the right of this tile
                positionStack.emplace_back(currentPosition.x + t.Size.Width, currentPosition.y);

                // Adjust atlas dimensions
                atlasWidth = CeilPowerOfTwo(std::max(atlasWidth, currentPosition.x + t.Size.Width));
                atlasHeight = CeilPowerOfTwo(std::max(atlasHeight, currentPosition.y + t.Size.Height));

                // We are done with this tile
                break;
            }
            else
            {
                // Backtrack
                positionStack.pop_back();
                assert(!positionStack.empty());
            }
        }
    }


    //
    // Round final size
    //

    atlasWidth = CeilPowerOfTwo(atlasWidth);
    atlasHeight = CeilPowerOfTwo(atlasHeight);


    //
    // Return atlas
    //

    return AtlasSpecification(
        std::move(texturePositions),
        ImageSize(atlasWidth, atlasHeight));
}

TextureAtlas TextureAtlasBuilder::BuildAtlas(
    AtlasSpecification const & specification,
    std::function<TextureFrame(TextureFrameId const &)> frameLoader,
    ProgressCallback const & progressCallback)
{
    float const dx = 0.5f / static_cast<float>(specification.AtlasSize.Width);
    float const dy = 0.5f / static_cast<float>(specification.AtlasSize.Height);

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
            texturePosition.FrameBottomY);

        // Store texture coordinates
        metadata.emplace_back(
            // Bottom-left
            vec2f(
                dx + static_cast<float>(texturePosition.FrameLeftX) / static_cast<float>(specification.AtlasSize.Width),
                dy + static_cast<float>(texturePosition.FrameBottomY) / static_cast<float>(specification.AtlasSize.Height)),
            // Top-right
            vec2f(
                static_cast<float>(texturePosition.FrameLeftX + textureFrame.TextureData.Size.Width) / static_cast<float>(specification.AtlasSize.Width) - dx,
                static_cast<float>(texturePosition.FrameBottomY + textureFrame.TextureData.Size.Height) / static_cast<float>(specification.AtlasSize.Height) - dy),
            texturePosition.FrameLeftX,
            texturePosition.FrameBottomY,
            textureFrame.Metadata);
    }

    ImageData atlasImageData(
        specification.AtlasSize,
        std::move(atlasImage));

    progressCallback(
        1.0f,
        "Building texture atlas...");

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
    int destinationBottomY)
{
    // From bottom to top
    for (int y = 0; y < sourceImageSize.Height; ++y)
    {
        // From left to right
        size_t const srcIndex = y * sourceImageSize.Width;
        size_t const dstIndex = (destinationBottomY + y) * destImageSize.Width + destinationLeftX;
        std::copy_n(
            &(sourceImage[srcIndex * 4]),
            sourceImageSize.Width * 4,
            &(destImage[dstIndex * 4]));
    }
}

}