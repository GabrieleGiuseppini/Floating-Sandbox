/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-07-09
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDatabase.h"

#include "ShipDefinitionFormatDeSerializer.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cassert>
#include <optional>

std::string const ShipDatabase::SpecificationFilename = "ship_database.json";

std::string ShipDatabase::MakePreviewAtlasFilename(size_t previewAtlasIndex)
{
    return "preview_atlas_" + std::to_string(previewAtlasIndex) + ".png";
}

picojson::value ShipDatabase::ShipEntry::Serialize() const
{
    picojson::object root;

    root["locator"] = Locator.Serialize();
    root["preview_data"] = PreviewData.Serialize();
    root["preview_atlas_index"] = picojson::value(static_cast<std::int64_t>(PreviewAtlasIndex));
    root["preview_frame_index"] = picojson::value(static_cast<std::int64_t>(PreviewFrameIndex));

    return picojson::value(root);
}

ShipDatabase::ShipEntry ShipDatabase::ShipEntry::Deserialize(picojson::value const & entryRoot)
{
    auto const & entryRootAsObject = Utils::GetJsonValueAsObject(entryRoot, "ShipDatabase::ShipEntry");

    ShipLocator locator = ShipLocator::Deserialize(entryRootAsObject.at("locator"));
    ShipPreviewData previewData = ShipPreviewData::Deserialize(entryRootAsObject.at("preview_data"));
    size_t previewAtlasIndex = static_cast<size_t>(Utils::GetJsonValueAs<std::int64_t>(entryRootAsObject.at("preview_atlas_index"), "preview_atlas_index"));
    TextureFrameIndex previewFrameIndex = static_cast<TextureFrameIndex>(Utils::GetJsonValueAs<std::int64_t>(entryRootAsObject.at("preview_frame_index"), "preview_frame_index"));

    return ShipEntry(
        std::move(locator),
        std::move(previewData),
        previewAtlasIndex,
        previewFrameIndex);
}

picojson::value ShipDatabase::Serialize() const
{
    picojson::object root;

    //
    // Ships
    //

    picojson::array serializedShips;

    for (auto const & ship : Ships)
    {
        serializedShips.push_back(picojson::value(ship.Serialize()));
    }

    root["ships"] = picojson::value(serializedShips);


    //
    // Preview atlases
    //

    picojson::array serializedPreviewAtlases;

    for (auto const & previewAtlasMetadata : PreviewAtlasMetadatas)
    {
        picojson::object serializedPreviewAtlasMetadata;
        previewAtlasMetadata.Serialize(serializedPreviewAtlasMetadata);
        serializedPreviewAtlases.push_back(picojson::value(serializedPreviewAtlasMetadata));
    }

    root["preview_atlases"] = picojson::value(serializedPreviewAtlases);

    return picojson::value(root);
}

ShipDatabase ShipDatabase::Deserialize(picojson::value const & specification)
{
    if (!specification.is<picojson::object>())
    {
        throw GameException("ShipDatabase specification is not a JSON object");
    }

    auto const & specificationAsObject = Utils::GetJsonValueAsObject(specification, "ShipDatabase");

    //
    // Ships
    //

    std::vector<ShipEntry> ships;
    for (auto const & entry : Utils::GetMandatoryJsonArray(specificationAsObject, "ships"))
    {
        ships.emplace_back(ShipEntry::Deserialize(entry));
    }

    //
    // Preview atlases
    //

    std::vector<TextureAtlasMetadata<ShipPreviewTextureDatabase>> previewAtlasMetadatas;
    for (auto const & entry : Utils::GetMandatoryJsonArray(specificationAsObject, "preview_atlases"))
    {
        previewAtlasMetadatas.emplace_back(TextureAtlasMetadata<ShipPreviewTextureDatabase>::Deserialize(Utils::GetJsonValueAsObject(entry, "ShipDatabase::ShipEntry")));
    }

    return ShipDatabase(
        std::move(ships),
        std::move(previewAtlasMetadatas));
}

////////////////////////////////////////////////////////////////////////////////////

void ShipDatabaseBuilder::AddShip(BinaryReadStream && inputStream, ShipLocator locator)
{
    // Load preview image
    auto const initialPosition = inputStream.GetCurrentPosition();
    RgbaImageData previewImage = ShipDefinitionFormatDeSerializer::LoadPreviewImage(inputStream, mMaxPreviewImageSize);
    inputStream.SetPosition(initialPosition);

    // Trim image on transparency
    RgbaImageData trimmedPreviewImage = ImageTools::TrimTransparent(std::move(previewImage));

    AddShip(
        std::move(inputStream),
        std::move(trimmedPreviewImage),
        std::move(locator));
}

void ShipDatabaseBuilder::AddShip(BinaryReadStream && inputStream, RgbaImageData && previewImage, ShipLocator locator)
{
    // Load preview data
    ShipPreviewData previewData = ShipDefinitionFormatDeSerializer::LoadPreviewData(inputStream);

    // Store in working set
    mWorkingList.emplace_back(
        std::move(locator),
        std::move(previewData),
        TextureFrame<ShipDatabase::ShipPreviewTextureDatabase>(
            TextureFrameMetadata<ShipDatabase::ShipPreviewTextureDatabase>(
                previewImage.Size,
                1.0f, // Irrelevant
                1.0f, // Irrelevant
                false, // Irrelevant
                ImageCoordinates(0, 0), // Irrelevant
                vec2f::zero(), // Irrelevant
                vec2f::zero(), // Irrelevant
                TextureFrameId<ShipDatabase::ShipPreviewTextureDatabase::TextureGroupsType>(
                    ShipDatabase::ShipPreviewTextureDatabase::TextureGroupsType::Preview,
                    0), // FrameIndex, preliminary (will be overwritten)
                locator.RelativeFilePath,
                locator.RelativeFilePath),
            std::move(previewImage)));
}

ShipDatabaseBuilder::Output ShipDatabaseBuilder::Build()
{
    std::vector<ShipDatabase::ShipEntry> outputShips;
    std::vector<TextureAtlasMetadata<ShipDatabase::ShipPreviewTextureDatabase>> outputPreviewAtlasMetadata;
    std::vector<RgbaImageData> outputPreviewAtlasImages;

    std::optional<TextureAtlas<ShipDatabase::ShipPreviewTextureDatabase>> currentAtlas;
    std::vector<TextureFrame<ShipDatabase::ShipPreviewTextureDatabase>> currentTextureFrames;
    size_t currentAtlasFrameStart = 0;

    // Process all ships
    assert(mWorkingList.size() > 0);
    currentTextureFrames.emplace_back(std::move(mWorkingList[0].PreviewImageFrame)); // Initialize first frame; no need to adjust
    assert(currentTextureFrames.back().Metadata.FrameId.FrameIndex == 0);
    for (size_t ship = 0; ; /*incremented in loop*/)
    {
        assert(ship >= currentAtlasFrameStart);

        // Make candidate atlas with all ships from currentAtlasFrameStart up to ship, included
        auto candidateAtlas = TextureAtlasBuilder<ShipDatabase::ShipPreviewTextureDatabase>::BuildAtlas(
            currentTextureFrames,
            TextureAtlasOptions::BinaryTransparencySmoothing);

        if (candidateAtlas.Metadata.GetSize().width <= ShipDatabase::MaxPreviewAtlasSize
            && candidateAtlas.Metadata.GetSize().height <= ShipDatabase::MaxPreviewAtlasSize)
        {
            // Candidate atlas is good
            currentAtlas.emplace(std::move(candidateAtlas));

            // Consume current ship
            assert(currentTextureFrames.size() > 0); // Already contains this ship
            outputShips.emplace_back(
                ShipDatabase::ShipEntry(
                    std::move(mWorkingList[ship].Locator),
                    std::move(mWorkingList[ship].PreviewData),
                    outputPreviewAtlasImages.size(), // atlasIndex
                    static_cast<TextureFrameIndex>(currentTextureFrames.size() - 1))); // frameIndex

            // Proceed to next ship
            ++ship;
            if (ship == mWorkingList.size())
            {
                break;
            }

            // Initialize new ship
            currentTextureFrames.emplace_back(std::move(mWorkingList[ship].PreviewImageFrame));
            // Update frame index
            currentTextureFrames.back().Metadata.FrameId = TextureFrameId<ShipDatabase::ShipPreviewTextureDatabase::TextureGroupsType>(
                ShipDatabase::ShipPreviewTextureDatabase::TextureGroupsType::Preview,
                static_cast<TextureFrameIndex>(ship - currentAtlasFrameStart));
        }
        else
        {
            // Cannot grow this atlas anymore

            // Produce current atlas
            if (!currentAtlas.has_value())
            {
                // This means that the current ship preview image is itself too large
                throw GameException("Ship preview for \"" + mWorkingList[ship].Locator.RelativeFilePath + "\" is too large");
            }

            // Consume currentAtlas, containing[currentAtlasFrameStart, ship - 1]
            assert(ship >= 1);
            outputPreviewAtlasMetadata.emplace_back(currentAtlas->Metadata);
            outputPreviewAtlasImages.emplace_back(std::move(currentAtlas->Image));

            // Restart clean
            currentAtlas.reset();
            currentTextureFrames.erase(currentTextureFrames.begin(), currentTextureFrames.end() - 1); // Leave last one
            // Update frame index of last one (it's now the first of its atlas)
            currentTextureFrames.back().Metadata.FrameId = TextureFrameId<ShipDatabase::ShipPreviewTextureDatabase::TextureGroupsType>(
                ShipDatabase::ShipPreviewTextureDatabase::TextureGroupsType::Preview,
                static_cast<TextureFrameIndex>(0));
            currentAtlasFrameStart = ship;

            // Retry this ship
        }
    }

    if (currentAtlas.has_value())
    {
        // Consume currentAtlas
        outputPreviewAtlasMetadata.emplace_back(currentAtlas->Metadata);
        outputPreviewAtlasImages.emplace_back(std::move(currentAtlas->Image));
    }

    // Prepare for next session
    mWorkingList.clear();

    return Output(
        ShipDatabase(
            std::move(outputShips),
            std::move(outputPreviewAtlasMetadata)),
        std::move(outputPreviewAtlasImages));
}