/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureAtlas.h"

#include "ImageFileTools.h"

#include <GameCore/GameException.h>
#include <GameCore/ImageTools.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Utils.h>

#include <cmath>
#include <cstring>

namespace Render {

template <typename TextureGroups>
TextureAtlasMetadata<TextureGroups>::TextureAtlasMetadata(
    ImageSize size,
    AtlasOptions options,
    std::vector<TextureAtlasFrameMetadata<TextureGroups>> && frames)
    : mSize(size)
    , mOptions(options)
    , mFrameMetadata(std::move(frames))
    , mFrameMetadataIndices()
{
    //
    // Store frames indices in vector of vectors, indexed by group and frame index
    //

    std::sort(
        mFrameMetadata.begin(),
        mFrameMetadata.end(),
        [](TextureAtlasFrameMetadata<TextureGroups> const & f1, TextureAtlasFrameMetadata<TextureGroups> const & f2)
        {
            return f1.FrameMetadata.FrameId.Group < f2.FrameMetadata.FrameId.Group
                || (f1.FrameMetadata.FrameId.Group == f2.FrameMetadata.FrameId.Group
                    && f1.FrameMetadata.FrameId.FrameIndex < f2.FrameMetadata.FrameId.FrameIndex);
        });

    for (size_t frameIndex = 0; frameIndex < mFrameMetadata.size(); ++frameIndex)
    {
        size_t groupIndex = static_cast<size_t>(mFrameMetadata[frameIndex].FrameMetadata.FrameId.Group);
        if (groupIndex >= mFrameMetadataIndices.size())
        {
            mFrameMetadataIndices.resize(groupIndex + 1);
        }

        assert(static_cast<size_t>(mFrameMetadata[frameIndex].FrameMetadata.FrameId.FrameIndex) == mFrameMetadataIndices.back().size());
        mFrameMetadataIndices.back().emplace_back(frameIndex);
    }
}

template <typename TextureGroups>
int TextureAtlasMetadata<TextureGroups>::GetMaxDimension() const
{
    return std::accumulate(
        mFrameMetadata.cbegin(),
        mFrameMetadata.cend(),
        std::numeric_limits<int>::lowest(),
        [](int minDimension, TextureAtlasFrameMetadata<TextureGroups> const & frameMd)
        {
            return std::max(
                minDimension,
                std::max(frameMd.FrameMetadata.Size.width, frameMd.FrameMetadata.Size.height));
        });
}

template <typename TextureGroups>
void TextureAtlasFrameMetadata<TextureGroups>::Serialize(picojson::object & root) const
{
    picojson::object textureSpaceSize;
    textureSpaceSize["width"] = picojson::value(static_cast<double>(TextureSpaceWidth));
    textureSpaceSize["height"] = picojson::value(static_cast<double>(TextureSpaceHeight));
    root["texture_space_size"] = picojson::value(std::move(textureSpaceSize));

    picojson::object textureCoordinates;
    textureCoordinates["left"] = picojson::value(static_cast<double>(TextureCoordinatesBottomLeft.x));
    textureCoordinates["bottom"] = picojson::value(static_cast<double>(TextureCoordinatesBottomLeft.y));
    textureCoordinates["anchorCenterX"] = picojson::value(static_cast<double>(TextureCoordinatesAnchorCenter.x));
    textureCoordinates["anchorCenterY"] = picojson::value(static_cast<double>(TextureCoordinatesAnchorCenter.y));
    textureCoordinates["right"] = picojson::value(static_cast<double>(TextureCoordinatesTopRight.x));
    textureCoordinates["top"] = picojson::value(static_cast<double>(TextureCoordinatesTopRight.y));
    root["texture_coordinates"] = picojson::value(std::move(textureCoordinates));

    picojson::object frameCoordinates;
    frameCoordinates["left"] = picojson::value(static_cast<int64_t>(FrameLeftX));
    frameCoordinates["bottom"] = picojson::value(static_cast<int64_t>(FrameBottomY));
    root["frame_coordinates"] = picojson::value(std::move(frameCoordinates));

    picojson::object frameMetadata;
    FrameMetadata.Serialize(frameMetadata);
    root["frame"] = picojson::value(std::move(frameMetadata));
}

template <typename TextureGroups>
TextureAtlasFrameMetadata<TextureGroups> TextureAtlasFrameMetadata<TextureGroups>::Deserialize(picojson::object const & root)
{
    picojson::object const & textureSpaceSizeJson = root.at("texture_space_size").get<picojson::object>();
    float const textureSpaceWidth = static_cast<float>(textureSpaceSizeJson.at("width").get<double>());
    float const textureSpaceHeight = static_cast<float>(textureSpaceSizeJson.at("height").get<double>());

    picojson::object const & textureCoordinatesJson = root.at("texture_coordinates").get<picojson::object>();
    vec2f textureCoordinatesBottomLeft(
        static_cast<float>(textureCoordinatesJson.at("left").get<double>()),
        static_cast<float>(textureCoordinatesJson.at("bottom").get<double>()));
    vec2f textureCoordinatesAnchorCenter(
        static_cast<float>(textureCoordinatesJson.at("anchorCenterX").get<double>()),
        static_cast<float>(textureCoordinatesJson.at("anchorCenterY").get<double>()));
    vec2f textureCoordinatesTopRight(
        static_cast<float>(textureCoordinatesJson.at("right").get<double>()),
        static_cast<float>(textureCoordinatesJson.at("top").get<double>()));

    picojson::object const & frameCoordinatesJson = root.at("frame_coordinates").get<picojson::object>();
    int frameLeftX = static_cast<int>(frameCoordinatesJson.at("left").get<std::int64_t>());
    int frameBottomY = static_cast<int>(frameCoordinatesJson.at("bottom").get<std::int64_t>());

    picojson::object const & frameMetadataJson = root.at("frame").get<picojson::object>();
    TextureFrameMetadata<TextureGroups> frameMetadata = TextureFrameMetadata<TextureGroups>::Deserialize(frameMetadataJson);

    return TextureAtlasFrameMetadata<TextureGroups>(
        textureSpaceWidth,
        textureSpaceHeight,
        textureCoordinatesBottomLeft,
        textureCoordinatesAnchorCenter,        
        textureCoordinatesTopRight,
        frameLeftX,
        frameBottomY,
        frameMetadata);
}

template <typename TextureGroups>
void TextureAtlasMetadata<TextureGroups>::Serialize(picojson::object & root) const
{
    picojson::object size;
    size["width"] = picojson::value(static_cast<std::int64_t>(mSize.width));
    size["height"] = picojson::value(static_cast<std::int64_t>(mSize.height));
    root["size"] = picojson::value(size);

    root["options"] = picojson::value(static_cast<std::int64_t>(mOptions));

    picojson::array frames;
    for (auto const & frameMetadata : mFrameMetadata)
    {
        picojson::object frame;
        frameMetadata.Serialize(frame);
        frames.push_back(picojson::value(std::move(frame)));
    }

    root["frames"] = picojson::value(std::move(frames));
}

template <typename TextureGroups>
TextureAtlasMetadata<TextureGroups> TextureAtlasMetadata<TextureGroups>::Deserialize(picojson::object const & root)
{
    picojson::object const & sizeJson = root.at("size").get<picojson::object>();
    ImageSize size(
        static_cast<int>(sizeJson.at("width").get<std::int64_t>()),
        static_cast<int>(sizeJson.at("height").get<std::int64_t>()));

    AtlasOptions options = static_cast<AtlasOptions>(root.at("options").get<std::int64_t>());

    picojson::array const & framesJson = root.at("frames").get<picojson::array>();
    std::vector<TextureAtlasFrameMetadata<TextureGroups>> frames;
    for (auto const & frameJsonValue : framesJson)
    {
        picojson::object const & frameJson = frameJsonValue.get<picojson::object>();
        frames.push_back(TextureAtlasFrameMetadata<TextureGroups>::Deserialize(frameJson));
    }

    return TextureAtlasMetadata<TextureGroups>(
        size,
        options,
        std::move(frames));
}

template <typename TextureGroups>
void TextureAtlas<TextureGroups>::Serialize(
    std::string const & databaseName,
    std::filesystem::path const & outputDirectoryPath) const
{
    //
    // Metadata
    //

    picojson::object metadataJson;
    Metadata.Serialize(metadataJson);

    std::filesystem::path const metadataFilePath = outputDirectoryPath / MakeMetadataFilename(databaseName);
    Utils::SaveJSONFile(picojson::value(metadataJson), metadataFilePath);

    //
    // Image
    //

    std::filesystem::path const imageFilePath = outputDirectoryPath / MakeImageFilename(databaseName);
    ImageFileTools::SavePngImage(AtlasData, imageFilePath);
}

template <typename TextureGroups>
TextureAtlas<TextureGroups> TextureAtlas<TextureGroups>::Deserialize(
    std::string const & databaseName,
    std::filesystem::path const & databaseRootDirectoryPath)
{
    //
    // Metadata
    //

    std::filesystem::path const metadataFilePath = databaseRootDirectoryPath / "Atlases" / MakeMetadataFilename(databaseName);
    picojson::value metadataJsonValue = Utils::ParseJSONFile(metadataFilePath);
    if (!metadataJsonValue.is<picojson::object>())
    {
        throw GameException("Atlas metadata json is not an object");
    }

    picojson::object const & metadataJson = metadataJsonValue.get<picojson::object>();
    TextureAtlasMetadata<TextureGroups> metadata = TextureAtlasMetadata<TextureGroups>::Deserialize(metadataJson);

    //
    // Image
    //

    std::filesystem::path const imageFilePath = databaseRootDirectoryPath / "Atlases" / MakeImageFilename(databaseName);
    RgbaImageData atlasData = ImageFileTools::LoadImageRgba(imageFilePath);

    return TextureAtlas<TextureGroups>(std::move(metadata), std::move(atlasData));
}

////////////////////////////////////////////////////////////////////////////////
// Builder
////////////////////////////////////////////////////////////////////////////////

template <typename TextureGroups>
typename TextureAtlasBuilder<TextureGroups>::AtlasSpecification TextureAtlasBuilder<TextureGroups>::BuildAtlasSpecification(std::vector<TextureInfo> const & inputTextureInfos)
{
    //
    // Sort input texture info's by height, from tallest to shortest,
    // and then by width
    //

    std::vector<TextureInfo> sortedTextureInfos = inputTextureInfos;
    std::sort(
        sortedTextureInfos.begin(),
        sortedTextureInfos.end(),
        [](TextureInfo const & a, TextureInfo const & b)
        {
            return a.Size.height > b.Size.height
                || (a.Size.height == b.Size.height && a.Size.width > b.Size.width);
        });


    //
    // Calculate size of atlas
    //

    uint64_t totalArea = 0;
    for (auto const & ti : sortedTextureInfos)
    {
        totalArea += static_cast<uint64_t>(ti.Size.width * ti.Size.height);
    }

    // Square root of area, floor'd to next power of two, minimized
    int const atlasSide = ceil_power_of_two(static_cast<int>(std::floor(std::sqrt(static_cast<float>(totalArea))))) / 2;
    int atlasWidth = atlasSide;
    int atlasHeight = atlasSide;


    //
    // Place tiles
    //

    std::vector<typename AtlasSpecification::TexturePosition> texturePositions;
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

            if (currentPosition.x + t.Size.width < atlasWidth   // Fits at current position
                || positionStack.size() == 1                    // We can't backtrack
                || (ceil_power_of_two(currentPosition.x + t.Size.width) - atlasWidth) <= (ceil_power_of_two(positionStack.front().y + t.Size.height) - atlasHeight)) // Extra W <= Extra H
            {
                // Put it at the current location
                texturePositions.emplace_back(
                    t.FrameId,
                    currentPosition.x,
                    currentPosition.y);

                if (positionStack.size() == 1
                    || currentPosition.y + t.Size.height < std::next(positionStack.rbegin())->y)
                {
                    // Move current location up to top
                    positionStack.back().y += t.Size.height;
                }
                else
                {
                    // Current location is completed
                    assert(currentPosition.y + t.Size.height == std::next(positionStack.rbegin())->y);

                    // Pop it from stack
                    positionStack.pop_back();
                }

                // Add new location to the right of this tile
                positionStack.emplace_back(currentPosition.x + t.Size.width, currentPosition.y);

                // Adjust atlas dimensions
                atlasWidth = ceil_power_of_two(std::max(atlasWidth, currentPosition.x + t.Size.width));
                atlasHeight = ceil_power_of_two(std::max(atlasHeight, currentPosition.y + t.Size.height));

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

    atlasWidth = ceil_power_of_two(atlasWidth);
    atlasHeight = ceil_power_of_two(atlasHeight);


    //
    // Return atlas
    //

    return AtlasSpecification(
        std::move(texturePositions),
        ImageSize(atlasWidth, atlasHeight));
}

template <typename TextureGroups>
typename TextureAtlasBuilder<TextureGroups>::AtlasSpecification TextureAtlasBuilder<TextureGroups>::BuildMipMappableAtlasSpecification(std::vector<TextureInfo> const & inputTextureInfos)
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
            return a.Size.height > b.Size.height
                || (a.Size.height == b.Size.height && a.Size.width > b.Size.width);
        });


    //
    // Calculate size of atlas
    //

    uint64_t totalArea = 0;
    for (auto const & ti : sortedTextureInfos)
    {
        // Verify tile dimensions are powers of two
        if (ti.Size.width != ceil_power_of_two(ti.Size.width)
            || ti.Size.height != ceil_power_of_two(ti.Size.height))
        {
            throw GameException("Dimensions of texture frame \"" + ti.FrameId.ToString() + "\" are not a power of two");
        }

        totalArea += static_cast<uint64_t>(ti.Size.width * ti.Size.height);
    }

    // Square root of area, floor'd to next power of two, minimized
    int const atlasSide = ceil_power_of_two(static_cast<int>(std::floor(std::sqrt(static_cast<float>(totalArea))))) / 2;
    int atlasWidth = atlasSide;
    int atlasHeight = atlasSide;


    //
    // Place tiles
    //

    std::vector<typename AtlasSpecification::TexturePosition> texturePositions;
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

            if (currentPosition.x + t.Size.width < atlasWidth   // Fits at current position
                || positionStack.size() == 1                    // We can't backtrack
                || (ceil_power_of_two(currentPosition.x + t.Size.width) - atlasWidth) <= (ceil_power_of_two(positionStack.front().y + t.Size.height) - atlasHeight)) // Extra W <= Extra H
            {
                // Put it at the current location
                texturePositions.emplace_back(
                    t.FrameId,
                    currentPosition.x,
                    currentPosition.y);

                if (positionStack.size() == 1
                    || currentPosition.y + t.Size.height < std::next(positionStack.rbegin())->y)
                {
                    // Move current location up to top
                    positionStack.back().y += t.Size.height;
                }
                else
                {
                    // Current location is completed
                    assert(currentPosition.y + t.Size.height == std::next(positionStack.rbegin())->y);

                    // Pop it from stack
                    positionStack.pop_back();
                }

                // Add new location to the right of this tile
                positionStack.emplace_back(currentPosition.x + t.Size.width, currentPosition.y);

                // Adjust atlas dimensions
                atlasWidth = ceil_power_of_two(std::max(atlasWidth, currentPosition.x + t.Size.width));
                atlasHeight = ceil_power_of_two(std::max(atlasHeight, currentPosition.y + t.Size.height));

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

    atlasWidth = ceil_power_of_two(atlasWidth);
    atlasHeight = ceil_power_of_two(atlasHeight);


    //
    // Return atlas
    //

    return AtlasSpecification(
        std::move(texturePositions),
        ImageSize(atlasWidth, atlasHeight));
}

template <typename TextureGroups>
typename TextureAtlasBuilder<TextureGroups>::AtlasSpecification TextureAtlasBuilder<TextureGroups>::BuildRegularAtlasSpecification(std::vector<TextureInfo> const & inputTextureInfos)
{
    //
    // Verify frames
    //

    if (inputTextureInfos.empty())
    {
        throw GameException("Regular texture atlas cannot consist of an empty set of texture frames");
    }

    int const frameWidth = inputTextureInfos[0].Size.width;
    int const frameHeight = inputTextureInfos[0].Size.height;
    if (frameWidth != ceil_power_of_two(frameWidth)
        || frameHeight != ceil_power_of_two(frameHeight))
    {
        throw GameException("Dimensions of texture frame \"" + inputTextureInfos[0].FrameId.ToString() + "\" are not a power of two");
    }

    for (auto const & ti : inputTextureInfos)
    {
        // Verify tile dimensions are powers of two
        if (ti.Size.width != frameWidth
            || ti.Size.height != frameHeight)
        {
            throw GameException("Dimensions of texture frame \"" + ti.FrameId.ToString() + "\" differ from the dimensions of the other frames");
        }
    }


    //
    // Place tiles
    //

    // Number of frames, rounded up to next square of a power of two
    size_t virtualNumberOfFrames = ceil_square_power_of_two(inputTextureInfos.size());

    int const numberOfFramesPerSide = static_cast<int>(std::floor(std::sqrt(static_cast<float>(virtualNumberOfFrames))));
    assert(numberOfFramesPerSide > 0);
    int const atlasWidth = numberOfFramesPerSide * frameWidth;
    int const atlasHeight = numberOfFramesPerSide * frameHeight;

    std::vector<typename AtlasSpecification::TexturePosition> texturePositions;
    texturePositions.reserve(virtualNumberOfFrames);

    for (int i = 0; i < static_cast<int>(inputTextureInfos.size()); ++i)
    {
        int const c = i % numberOfFramesPerSide;
        int const r = i / numberOfFramesPerSide;

        texturePositions.emplace_back(
            inputTextureInfos[i].FrameId,
            c * frameWidth,
            r * frameHeight);
    }

    //
    // Return atlas
    //

    return AtlasSpecification(
        std::move(texturePositions),
        ImageSize(atlasWidth, atlasHeight));
}

template <typename TextureGroups>
TextureAtlas<TextureGroups> TextureAtlasBuilder<TextureGroups>::BuildAtlas(
    AtlasSpecification const & specification,
    AtlasOptions options,
    std::function<TextureFrame<TextureGroups>(TextureFrameId<TextureGroups> const &)> frameLoader,
    ProgressCallback const & progressCallback)
{
    float const dx = 0.5f / static_cast<float>(specification.AtlasSize.width);
    float const dy = 0.5f / static_cast<float>(specification.AtlasSize.height);

    // Allocate image
    size_t const imagePoints = specification.AtlasSize.width * specification.AtlasSize.height;
    std::unique_ptr<rgbaColor[]> atlasImage(new rgbaColor[imagePoints]);

    // Fill image - transparent black
    std::fill_n(atlasImage.get(), imagePoints, rgbaColor::zero());

    // Copy all textures into image, building metadata at the same time
    std::vector<TextureAtlasFrameMetadata<TextureGroups>> frameMetadata;
    for (auto const & texturePosition : specification.TexturePositions)
    {
        progressCallback(
            static_cast<float>(frameMetadata.size()) / static_cast<float>(specification.TexturePositions.size()),
            ProgressMessageType::None);

        // Load frame
        TextureFrame<TextureGroups> textureFrame = frameLoader(texturePosition.FrameId);

        // Copy frame
        CopyImage(
            std::move(textureFrame.TextureData.Data),
            textureFrame.TextureData.Size,
            atlasImage.get(),
            specification.AtlasSize,
            texturePosition.FrameLeftX,
            texturePosition.FrameBottomY);

        // Calculate frame dimensions in texture space - the whole thing, ignoring dx/dy
        float const textureSpaceFrameWidth = static_cast<float>(textureFrame.TextureData.Size.width) / static_cast<float>(specification.AtlasSize.width);
        float const textureSpaceFrameHeight = static_cast<float>(textureFrame.TextureData.Size.height) / static_cast<float>(specification.AtlasSize.height);

        // Store texture metadata
        frameMetadata.emplace_back(
            textureSpaceFrameWidth,
            textureSpaceFrameHeight,
            // Bottom-left
            vec2f(
                dx + static_cast<float>(texturePosition.FrameLeftX) / static_cast<float>(specification.AtlasSize.width),
                dy + static_cast<float>(texturePosition.FrameBottomY) / static_cast<float>(specification.AtlasSize.height)),
            // Anchor center
            vec2f(
                dx + static_cast<float>(texturePosition.FrameLeftX + textureFrame.Metadata.AnchorCenter.x) / static_cast<float>(specification.AtlasSize.width),
                dy + static_cast<float>(texturePosition.FrameBottomY + textureFrame.Metadata.AnchorCenter.y) / static_cast<float>(specification.AtlasSize.height)),
            // Top-right
            vec2f(
                static_cast<float>(texturePosition.FrameLeftX + textureFrame.TextureData.Size.width) / static_cast<float>(specification.AtlasSize.width) - dx,
                static_cast<float>(texturePosition.FrameBottomY + textureFrame.TextureData.Size.height) / static_cast<float>(specification.AtlasSize.height) - dy),
            texturePosition.FrameLeftX,
            texturePosition.FrameBottomY,
            textureFrame.Metadata);
    }

    RgbaImageData atlasImageData(
        specification.AtlasSize,
        std::move(atlasImage));

    // Pre-multiply alpha, if requested
    if (!!(options & AtlasOptions::AlphaPremultiply))
    {
        ImageTools::AlphaPreMultiply(atlasImageData);
    }

    progressCallback(1.0f, ProgressMessageType::None);

    // Return atlas
    return TextureAtlas<TextureGroups>(
        TextureAtlasMetadata<TextureGroups>(
            specification.AtlasSize,
            options,
            std::move(frameMetadata)),
        std::move(atlasImageData));
}

template <typename TextureGroups>
void TextureAtlasBuilder<TextureGroups>::CopyImage(
    std::unique_ptr<rgbaColor const []> sourceImage,
    ImageSize sourceImageSize,
    rgbaColor * destImage,
    ImageSize destImageSize,
    int destinationLeftX,
    int destinationBottomY)
{
    // From bottom to top
    for (int y = 0; y < sourceImageSize.height; ++y)
    {
        // From left to right
        size_t const srcIndex = y * sourceImageSize.width;
        size_t const dstIndex = (destinationBottomY + y) * destImageSize.width + destinationLeftX;
        std::copy_n(
            &(sourceImage[srcIndex]),
            sourceImageSize.width,
            &(destImage[dstIndex]));
    }
}

}
