/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureAtlas.h"

#include "ImageFileMap.h"
#include "ImageTools.h"
#include "Log.h"
#include "SysSpecifics.h"
#include "Utils.h"

#include <cmath>
#include <cstring>

template <typename TTextureDatabase>
TextureAtlasMetadata<TTextureDatabase>::TextureAtlasMetadata(
    ImageSize size,
    TextureAtlasOptions options,
    std::vector<TextureAtlasFrameMetadata<TTextureDatabase>> && frames)
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
        [](TextureAtlasFrameMetadata<TTextureDatabase> const & f1, TextureAtlasFrameMetadata<TTextureDatabase> const & f2)
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

    //
    // Build index by filename stem
    //

    for (size_t i = 0; i < mFrameMetadata.size(); ++i)
    {
        auto const [_, isInserted] = mFrameMetadataByName.try_emplace(
            mFrameMetadata[i].FrameMetadata.FrameName,
            i);

        if (!isInserted)
        {
            // Note: this may happen if the same file is, for example, used with different world sizes; in such cases,
            // one cannot use an Atlas
            throw GameException("Atlas metadata frame \"" + mFrameMetadata[i].FrameMetadata.FrameName + "\" is duplicated");
        }
    }
}

template <typename TTextureDatabase>
int TextureAtlasMetadata<TTextureDatabase>::GetMaxDimension() const
{
    return std::accumulate(
        mFrameMetadata.cbegin(),
        mFrameMetadata.cend(),
        std::numeric_limits<int>::lowest(),
        [](int minDimension, TextureAtlasFrameMetadata<TTextureDatabase> const & frameMd)
        {
            return std::max(
                minDimension,
                std::max(frameMd.FrameMetadata.Size.width, frameMd.FrameMetadata.Size.height));
        });
}

template <typename TTextureDatabase>
void TextureAtlasFrameMetadata<TTextureDatabase>::Serialize(picojson::object & root) const
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

template <typename TTextureDatabase>
TextureAtlasFrameMetadata<TTextureDatabase> TextureAtlasFrameMetadata<TTextureDatabase>::Deserialize(picojson::object const & root)
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
    TextureFrameMetadata<TTextureDatabase> frameMetadata = TextureFrameMetadata<TTextureDatabase>::Deserialize(frameMetadataJson);

    return TextureAtlasFrameMetadata<TTextureDatabase>(
        textureSpaceWidth,
        textureSpaceHeight,
        textureCoordinatesBottomLeft,
        textureCoordinatesAnchorCenter,
        textureCoordinatesTopRight,
        frameLeftX,
        frameBottomY,
        frameMetadata);
}

template <typename TTextureDatabase>
void TextureAtlasMetadata<TTextureDatabase>::Serialize(picojson::object & root) const
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

template <typename TTextureDatabase>
TextureAtlasMetadata<TTextureDatabase> TextureAtlasMetadata<TTextureDatabase>::Deserialize(picojson::object const & root)
{
    picojson::object const & sizeJson = root.at("size").get<picojson::object>();
    ImageSize size(
        static_cast<int>(sizeJson.at("width").get<std::int64_t>()),
        static_cast<int>(sizeJson.at("height").get<std::int64_t>()));

    TextureAtlasOptions options = static_cast<TextureAtlasOptions>(root.at("options").get<std::int64_t>());

    picojson::array const & framesJson = root.at("frames").get<picojson::array>();
    std::vector<TextureAtlasFrameMetadata<TTextureDatabase>> frames;
    for (auto const & frameJsonValue : framesJson)
    {
        picojson::object const & frameJson = frameJsonValue.get<picojson::object>();
        frames.push_back(TextureAtlasFrameMetadata<TTextureDatabase>::Deserialize(frameJson));
    }

    return TextureAtlasMetadata<TTextureDatabase>(
        size,
        options,
        std::move(frames));
}

template <typename TTextureDatabase>
std::tuple<picojson::value, RgbaImageData const &> TextureAtlas<TTextureDatabase>::Serialize() const
{
    picojson::object metadataJson;
    Metadata.Serialize(metadataJson);

    return std::make_tuple(picojson::value(std::move(metadataJson)), std::ref(Image));
}

template <typename TTextureDatabase>
TextureAtlas<TTextureDatabase> TextureAtlas<TTextureDatabase>::Deserialize(IAssetManager const & assetManager)
{
    //
    // Metadata
    //

    picojson::value metadataJsonValue = assetManager.LoadTetureAtlasSpecification(TTextureDatabase::DatabaseName);
    if (!metadataJsonValue.is<picojson::object>())
    {
        throw GameException("Atlas metadata json is not an object");
    }

    picojson::object const & metadataJson = metadataJsonValue.get<picojson::object>();
    TextureAtlasMetadata<TTextureDatabase> metadata = TextureAtlasMetadata<TTextureDatabase>::Deserialize(metadataJson);

    //
    // Image
    //

    RgbaImageData atlasData = assetManager.LoadTextureAtlasImageRGBA(TTextureDatabase::DatabaseName);

    return TextureAtlas<TTextureDatabase>(std::move(metadata), std::move(atlasData));
}

////////////////////////////////////////////////////////////////////////////////
// Builder
////////////////////////////////////////////////////////////////////////////////

template<typename TTextureDatabase>
typename TextureAtlasBuilder<TTextureDatabase>::AtlasSpecification TextureAtlasBuilder<TTextureDatabase>::BuildAtlasSpecification(
    std::vector<TextureInfo> const & inputTextureInfos,
    TextureAtlasOptions options,
    std::function<TextureFrame<TTextureDatabase>(TextureFrameId<TTextureGroups> const &)> frameLoader)
{
    //
    // Welcome to the poor man's BinPack
    //
    // We sort tiles by height, from tallest to shortest,
    // and then by width; then we place them in a dynamically-built
    // flexgrid
    //

    //
    // Sort input texture info's
    //

    std::vector<TextureInfo> sortedTextureInfos = inputTextureInfos;
    std::sort(
        sortedTextureInfos.begin(),
        sortedTextureInfos.end(),
        [](TextureInfo const & a, TextureInfo const & b)
        {
            return a.InAtlasSize.height > b.InAtlasSize.height
                || (a.InAtlasSize.height == b.InAtlasSize.height && a.InAtlasSize.width > b.InAtlasSize.width);
        });

    //
    // Initialize atlas size
    //

    int atlasWidth = 0;
    int atlasHeight = 0;

    //
    // Place tiles
    //

    std::vector<typename AtlasSpecification::TextureLocationInfo> textureLocationInfos;
    textureLocationInfos.reserve(inputTextureInfos.size());

    std::vector<typename AtlasSpecification::DuplicateTextureInfo> duplicateTextureInfos;

    ImageFileMap<rgbaColor, TextureFrameMetadata<TTextureDatabase>> dupeMap; // For duplicate suppression

    // Bands for the FlexGrid

    // The width of a VBand is the width of the first (bottom) item in it.
    struct VBand
    {
        int const Width;
        int const LeftX; // For convenience; matches left's VBand right (or zero)
        int TopY; // Relative to HBand

        VBand(
            int width,
            int leftX)
            : Width(width)
            , LeftX(leftX)
            , TopY(0)
        {}
    };

    // The height of a HBand is the height of the first (left) item in it.
    struct HBand
    {
        int const Height;
        int const BottomY; // For convenience; matches b-1's top (or zero)
        int RightmostX;

        std::vector<VBand> VBands;

        HBand(
            int height,
            int bottomY)
            : Height(height)
            , BottomY(bottomY)
            , RightmostX(0)
            , VBands()
        {}
    };

    std::vector<HBand> hBands;

    int totalTopmostY = 0; // Convenience: top of last (top) HBand
    int totalRightmostX = 0; // Convenience: max(right) among all last (right) VBand's

    for (TextureInfo const & t : sortedTextureInfos)
    {
        // Check whether we need to look for duplicates
        bool isDuplicate = false;
        if (!!(options & TextureAtlasOptions::SuppressDuplicates))
        {
            // Load this frame
            TextureFrame<TTextureDatabase> frame = frameLoader(t.FrameId);

            assert(frame.Metadata.FrameId == t.FrameId);

            // Check if it's a duplicate of a frame we have already seen
            size_t imageHash = frame.TextureData.Hash();
            auto originalFrameMetadata = dupeMap.Find(
                imageHash,
                frame.TextureData,
                [frameLoader](TextureFrameMetadata<TTextureDatabase> const & frameMetadata) -> ImageData<rgbaColor>
                {
                    return frameLoader(frameMetadata.FrameId).TextureData;
                });

            if (originalFrameMetadata.has_value())
            {
                // It's a duplicate

                LogMessage("Frame \"", frame.Metadata.FrameName, "\" is a duplicate of \"", originalFrameMetadata->FrameName, "\"");

                isDuplicate = true;

                // Store duplicate information
                duplicateTextureInfos.emplace_back(
                    frame.Metadata,
                    originalFrameMetadata->FrameId);
            }
            else
            {
                // Add this original

                dupeMap.Add(
                    imageHash,
                    frame.TextureData.Size,
                    frame.Metadata);
            }
        }

        if (!isDuplicate)
        {
            //
            // Place frame
            //

            while (true)
            {
                // Look for a band that is "viable" for this frame,
                // and place texture frame in it
                bool hasBeenPlaced = false;
                for (size_t h = 0; h < hBands.size(); /* incremented in loop */)
                {
                    // Check if band's height would fit this frame
                    if (hBands[h].Height >= t.InAtlasSize.height)
                    {
                        // Check if there's an already-existing V band that fits it
                        for (size_t v = 0; v < hBands[h].VBands.size(); ++v)
                        {
                            if (hBands[h].VBands[v].Width >= t.InAtlasSize.width
                                && hBands[h].Height >= hBands[h].VBands[v].TopY + t.InAtlasSize.height)
                            {
                                // Found!

                                // Place here
                                textureLocationInfos.emplace_back(
                                    t.FrameId,
                                    vec2i(hBands[h].VBands[v].LeftX, hBands[h].BottomY + hBands[h].VBands[v].TopY),
                                    t.InAtlasSize);

                                // Update V band's top Y
                                hBands[h].VBands[v].TopY += t.InAtlasSize.height;

                                // Update H band's rightmost X
                                hBands[h].RightmostX = std::max(hBands[h].RightmostX, hBands[h].VBands[v].LeftX + t.InAtlasSize.width);

                                // Update extrema
                                totalTopmostY = std::max(totalTopmostY, hBands[h].BottomY + t.InAtlasSize.height);
                                totalRightmostX = std::max(totalRightmostX, hBands[h].RightmostX);

                                hasBeenPlaced = true;
                                break;
                            }
                        }

                        if (hasBeenPlaced)
                        {
                            break;
                        }

                        //
                        // No luck
                        //
                        // Check if can get away with a new V band
                        //

                        if (hBands[h].RightmostX + t.InAtlasSize.width <= atlasWidth)
                        {
                            // Add a new V band

                            hBands[h].VBands.emplace_back(t.InAtlasSize.width, hBands[h].RightmostX);

                            // Retry this H band
                            continue;
                        }
                    }

                    ++h;
                }

                if (hasBeenPlaced)
                {
                    break;
                }

                //
                // No luck
                //
                // See if can get away with a new H band
                //

                assert(hBands.empty() || totalTopmostY == hBands.back().BottomY + hBands.back().Height);
                if (totalTopmostY + t.InAtlasSize.height <= atlasHeight
                    && t.InAtlasSize.width <= atlasWidth) // And Atlas must fit the frame horizontally
                {
                    // Add a new H band

                    hBands.emplace_back(t.InAtlasSize.height, totalTopmostY);

                    // Retry the frame
                    continue;
                }

                //
                // No luck
                //
                // Enlarge atlas and retry the frame
                //

                int const candidateNewAtlasHeight = ceil_power_of_two(totalTopmostY + t.InAtlasSize.height);
                int const candidateNewAtlasWidth = ceil_power_of_two(totalRightmostX + t.InAtlasSize.width);
                if (atlasWidth < t.InAtlasSize.width)
                {
                    // Compelled to go wide - with guarantee
                    atlasWidth = candidateNewAtlasWidth;
                }
                else if (atlasHeight < t.InAtlasSize.height)
                {
                    // Compelled to go high - with guarantee
                    atlasHeight = candidateNewAtlasHeight;
                }
                // Minimize waste
                else if ((candidateNewAtlasHeight - (totalTopmostY + t.InAtlasSize.height)) >= (candidateNewAtlasWidth - (totalRightmostX + t.InAtlasSize.width)))
                {
                    // Go wide
                    atlasWidth = candidateNewAtlasWidth;
                }
                else
                {
                    // Go high
                    atlasHeight = candidateNewAtlasHeight;
                }
            }
        }
    }

    assert(atlasWidth == ceil_power_of_two(atlasWidth));
    assert(atlasHeight == ceil_power_of_two(atlasHeight));

    //
    // Return spec
    //

    assert(textureLocationInfos.size() + duplicateTextureInfos.size() == inputTextureInfos.size());

    return AtlasSpecification(
        std::move(textureLocationInfos),
        std::move(duplicateTextureInfos),
        ImageSize(atlasWidth, atlasHeight));
}

template <typename TTextureDatabase>
typename TextureAtlasBuilder<TTextureDatabase>::AtlasSpecification TextureAtlasBuilder<TTextureDatabase>::BuildRegularAtlasSpecification(
    std::vector<TextureInfo> const & inputTextureInfos)
{
    //
    // Verify frames
    //

    if (inputTextureInfos.empty())
    {
        throw GameException("Regular texture atlas cannot consist of an empty set of texture frames");
    }

    int const frameInAtlasWidth = inputTextureInfos[0].InAtlasSize.width;
    int const frameInAtlasHeight = inputTextureInfos[0].InAtlasSize.height;
    if (frameInAtlasWidth != ceil_power_of_two(frameInAtlasWidth)
        || frameInAtlasHeight != ceil_power_of_two(frameInAtlasHeight))
    {
        throw GameException("Dimensions of texture frame \"" + inputTextureInfos[0].FrameId.ToString() + "\" are not a power of two");
    }

    for (auto const & ti : inputTextureInfos)
    {
        // Verify tile dimensions are powers of two
        if (ti.InAtlasSize.width != frameInAtlasWidth
            || ti.InAtlasSize.height != frameInAtlasHeight)
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
    int const atlasWidth = numberOfFramesPerSide * frameInAtlasWidth;
    int const atlasHeight = numberOfFramesPerSide * frameInAtlasHeight;

    std::vector<typename AtlasSpecification::TextureLocationInfo> textureLocationInfos;
    textureLocationInfos.reserve(virtualNumberOfFrames);

    std::vector<typename AtlasSpecification::DuplicateTextureInfo> duplicateTextureInfos;

    for (int i = 0; i < static_cast<int>(inputTextureInfos.size()); ++i)
    {
        int const c = i % numberOfFramesPerSide;
        int const r = i / numberOfFramesPerSide;

        textureLocationInfos.emplace_back(
            inputTextureInfos[i].FrameId,
            vec2i(c * frameInAtlasWidth, r * frameInAtlasHeight),
            inputTextureInfos[i].InAtlasSize);
    }

    //
    // Return atlas
    //

    return AtlasSpecification(
        std::move(textureLocationInfos),
        std::move(duplicateTextureInfos),
        ImageSize(atlasWidth, atlasHeight));
}

template <typename TTextureDatabase>
TextureAtlas<TTextureDatabase> TextureAtlasBuilder<TTextureDatabase>::InternalBuildAtlas(
    AtlasSpecification const & specification,
    TextureAtlasOptions options,
    std::function<TextureFrame<TTextureDatabase>(TextureFrameId<TTextureGroups> const &)> frameLoader,
    ProgressCallback const & progressCallback)
{
    // The DX's to sample pixels in their dead center
    float const dx = 0.5f / static_cast<float>(specification.AtlasSize.width);
    float const dy = 0.5f / static_cast<float>(specification.AtlasSize.height);

    // Allocate atlas image
    size_t const imagePoints = specification.AtlasSize.width * specification.AtlasSize.height;
    std::unique_ptr<rgbaColor[]> atlasImage(new rgbaColor[imagePoints]);

    // Fill atlas image - transparent black
    std::fill_n(atlasImage.get(), imagePoints, rgbaColor::zero());

    //
    // Copy all textures into image, building metadata at the same time
    //

    std::vector<TextureAtlasFrameMetadata<TTextureDatabase>> allAtlasFrameMetadata;

    for (auto const & textureLocationInfo : specification.TextureLocationInfos)
    {
        progressCallback(
            static_cast<float>(allAtlasFrameMetadata.size()) / static_cast<float>(specification.TextureLocationInfos.size()),
            ProgressMessageType::None);

        // Load frame
        TextureFrame<TTextureDatabase> textureFrame = frameLoader(textureLocationInfo.FrameId);
        ImageData<rgbaColor> textureImageData = ImageData<rgbaColor>(
            textureFrame.TextureData.Size,
            std::move(textureFrame.TextureData.Data));

        // Pre-multiply alpha, if requested
        if (!!(options & TextureAtlasOptions::AlphaPremultiply))
        {
            ImageTools::AlphaPreMultiply(textureImageData);
        }

        // Apply binary transparency smoothing, if requested
        if (!!(options & TextureAtlasOptions::BinaryTransparencySmoothing))
        {
            ImageTools::ApplyBinaryTransparencySmoothing(textureImageData);
        }

        // Calculate offset for frame into in-atlas frame
        assert(textureFrame.TextureData.Size.width <= textureLocationInfo.InAtlasSize.width);
        assert(textureFrame.TextureData.Size.height <= textureLocationInfo.InAtlasSize.height);
        vec2i framePositionOffset = vec2i(
            (textureLocationInfo.InAtlasSize.width - textureFrame.TextureData.Size.width) / 2,
            (textureLocationInfo.InAtlasSize.height - textureFrame.TextureData.Size.height) / 2);

        // Calculate actual position of frame in atlas
        vec2i frameActualPosition = textureLocationInfo.InAtlasBottomLeft + framePositionOffset;

        // Copy frame
        CopyImage(
            std::move(textureImageData),
            atlasImage.get(),
            specification.AtlasSize,
            frameActualPosition);

        // Calculate frame dimensions in texture space - the whole thing, ignoring dx/dy
        float const textureSpaceFrameWidth = static_cast<float>(textureFrame.TextureData.Size.width) / static_cast<float>(specification.AtlasSize.width);
        float const textureSpaceFrameHeight = static_cast<float>(textureFrame.TextureData.Size.height) / static_cast<float>(specification.AtlasSize.height);

        // Create atlas frame metadata
        TextureAtlasFrameMetadata<TTextureDatabase> atlasFrameMetadata(
            textureSpaceFrameWidth,
            textureSpaceFrameHeight,
            // Bottom-left
            vec2f(
                dx + static_cast<float>(frameActualPosition.x) / static_cast<float>(specification.AtlasSize.width),
                dy + static_cast<float>(frameActualPosition.y) / static_cast<float>(specification.AtlasSize.height)),
            // Anchor center
            vec2f(
                dx + static_cast<float>(frameActualPosition.x + textureFrame.Metadata.AnchorCenter.x) / static_cast<float>(specification.AtlasSize.width),
                dy + static_cast<float>(frameActualPosition.y + textureFrame.Metadata.AnchorCenter.y) / static_cast<float>(specification.AtlasSize.height)),
            // Top-right
            vec2f(
                static_cast<float>(frameActualPosition.x + textureFrame.TextureData.Size.width) / static_cast<float>(specification.AtlasSize.width) - dx,
                static_cast<float>(frameActualPosition.y + textureFrame.TextureData.Size.height) / static_cast<float>(specification.AtlasSize.height) - dy),
            frameActualPosition.x,
            frameActualPosition.y,
            textureFrame.Metadata);

        // Store
        allAtlasFrameMetadata.push_back(atlasFrameMetadata);
    }

    // Process dupes, if any
    for (auto const & duplicateTextureInfo : specification.DuplicateTextureInfos)
    {
        // Find the original among the ones we've just processed

        auto const findIt = std::find_if(
            allAtlasFrameMetadata.cbegin(),
            allAtlasFrameMetadata.cend(),
            [duplicateTextureInfo](TextureAtlasFrameMetadata<TTextureDatabase> const & v)
            {
                return v.FrameMetadata.FrameId == duplicateTextureInfo.OriginalFrameId;
            });

        assert(findIt != allAtlasFrameMetadata.cend());

        // Create atlas frame metadata
        TextureAtlasFrameMetadata<TTextureDatabase> atlasFrameMetadata = findIt->CloneForNewTextureFrame(duplicateTextureInfo.DuplicateFrameMetadata);

        // Store
        allAtlasFrameMetadata.push_back(atlasFrameMetadata);
    }

    progressCallback(1.0f, ProgressMessageType::None);

    // Create atlas image
    RgbaImageData atlasImageData(
        specification.AtlasSize,
        std::move(atlasImage));

    // Return atlas
    return TextureAtlas<TTextureDatabase>(
        TextureAtlasMetadata<TTextureDatabase>(
            specification.AtlasSize,
            options,
            std::move(allAtlasFrameMetadata)),
        std::move(atlasImageData));
}

template <typename TTextureDatabase>
void TextureAtlasBuilder<TTextureDatabase>::CopyImage(
    ImageData<rgbaColor> && sourceImage,
    rgbaColor * destImage,
    ImageSize destImageSize,
    vec2i const & destinationBottomLeftPosition)
{
    // From bottom to top
    for (int y = 0; y < sourceImage.Size.height; ++y)
    {
        // From left to right
        size_t const srcIndex = y * sourceImage.Size.width;
        size_t const dstIndex = (destinationBottomLeftPosition.y + y) * destImageSize.width + destinationBottomLeftPosition.x;
        std::copy_n(
            &(sourceImage.Data[srcIndex]),
            sourceImage.Size.width,
            &(destImage[dstIndex]));
    }
}

