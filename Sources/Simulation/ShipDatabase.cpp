/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-07-09
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDatabase.h"

#include "ShipDefinitionFormatDeSerializer.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <optional>

std::string const ShipDatabase::SpecificationFilename = "ship_database.json";

std::string ShipDatabase::MakePreviewAtlasFilename(size_t atlasIndex)
{
    return "preview_atlas_" + std::to_string(atlasIndex) + ".png";
}

picojson::value ShipDatabase::ShipEntry::Serialize() const
{
    picojson::object root;

    root["locator"] = Locator.Serialize();
    root["preview_data"] = PreviewData.Serialize();
    root["atlas_index"] = picojson::value(static_cast<std::int64_t>(AtlasIndex));
    root["frame_index"] = picojson::value(static_cast<std::int64_t>(FrameIndex));

    return picojson::value(root);
}

ShipDatabase::ShipEntry ShipDatabase::ShipEntry::Deserialize(picojson::value const & entryRoot)
{
    auto const & entryRootAsObject = Utils::GetJsonValueAsObject(entryRoot, "ShipDatabase::ShipEntry");

    ShipLocator locator = ShipLocator::Deserialize(entryRootAsObject.at("locator"));
    ShipPreviewData previewData = ShipPreviewData::Deserialize(entryRootAsObject.at("preview_data"));
    size_t atlasIndex = static_cast<size_t>(Utils::GetJsonValueAs<std::int64_t>(entryRootAsObject.at("atlas_index"), "atlas_index"));
    TextureFrameIndex frameIndex = static_cast<TextureFrameIndex>(Utils::GetJsonValueAs<std::int64_t>(entryRootAsObject.at("frame_index"), "frame_index"));

    return ShipEntry(
        std::move(locator),
        std::move(previewData),
        atlasIndex,
        frameIndex);
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
    // Atlases
    //

    picojson::array serializedAtlases;

    for (auto const & atlasMetadata : PreviewAtlasMetadatas)
    {
        picojson::object serializedAtlas;
        atlasMetadata.Serialize(serializedAtlas);
        serializedAtlases.push_back(picojson::value(serializedAtlas));
    }

    root["atlases"] = picojson::value(serializedAtlases);

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
    // Atlases
    //

    std::vector<TextureAtlasMetadata<ShipPreviewTextureDatabase>> atlases;
    for (auto const & entry : Utils::GetMandatoryJsonArray(specificationAsObject, "atlases"))
    {
        atlases.emplace_back(TextureAtlasMetadata<ShipPreviewTextureDatabase>::Deserialize(Utils::GetJsonValueAsObject(entry, "ShipDatabase::ShipEntry")));
    }

    return ShipDatabase(
        std::move(ships),
        std::move(atlases));
}

////////////////////////////////////////////////////////////////////////////////////

void ShipDatabaseBuilder::AddShip(BinaryReadStream && inputStream, ShipLocator locator)
{
    // Preview data
    auto const initialPosition = inputStream.GetCurrentPosition();
    ShipPreviewData previewData = ShipDefinitionFormatDeSerializer::LoadPreviewData(inputStream);

    // Preview image
    inputStream.SetPosition(initialPosition);
    RgbaImageData previewImage = ShipDefinitionFormatDeSerializer::LoadPreviewImage(inputStream, mMaxPreviewImageSize);

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
    std::vector<TextureAtlasMetadata<ShipDatabase::ShipPreviewTextureDatabase>> outputAtlasMetadata;
    std::vector<RgbaImageData> outputAtlasImages;

    std::optional<TextureAtlas<ShipDatabase::ShipPreviewTextureDatabase>> currentAtlas;
    std::vector<TextureFrame<ShipDatabase::ShipPreviewTextureDatabase>> currentTextureFrames;
    size_t currentAtlasFrameStart = 0;

    // Process all ships
    currentTextureFrames.emplace_back(std::move(mWorkingList[0].PreviewImageFrame)); // Initialize first frame
    for (size_t ship = 0; ship < mWorkingList.size(); /*incremented in loop*/)
    {
        assert(ship >= currentAtlasFrameStart);

        // Make candidate atlas with all ships from currentAtlasFrameStart up to ship, included
        auto candidateAtlas = TextureAtlasBuilder<ShipDatabase::ShipPreviewTextureDatabase>::BuildAtlas(
            currentTextureFrames,
            TextureAtlasOptions::BinaryTransparencySmoothing);

        if (candidateAtlas.Metadata.GetSize().width < ShipDatabase::MaxAtlasSize
            && candidateAtlas.Metadata.GetSize().height < ShipDatabase::MaxAtlasSize)
        {
            // Candidate atlas is good
            currentAtlas.emplace(std::move(candidateAtlas));

            // Consume current ship
            outputShips.emplace_back(
                ShipDatabase::ShipEntry(
                    std::move(mWorkingList[ship].Locator),
                    std::move(mWorkingList[ship].PreviewData),
                    outputAtlasImages.size(), // atlasIndex
                    static_cast<TextureFrameIndex>(currentTextureFrames.size()))); // frameIndex

            // Proceed to next ship
            ++ship;
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
            outputAtlasMetadata.emplace_back(currentAtlas->Metadata);
            outputAtlasImages.emplace_back(std::move(currentAtlas->Image));

            // Restart clean
            currentAtlas.reset();
            currentTextureFrames.clear();
            currentAtlasFrameStart = ship;

            // Retry this ship
        }
    }

    if (currentAtlas.has_value())
    {
        // Consume currentAtlas
        outputAtlasMetadata.emplace_back(currentAtlas->Metadata);
        outputAtlasImages.emplace_back(std::move(currentAtlas->Image));
    }

    // Prepare for next session
    mWorkingList.clear();

    return Output(
        ShipDatabase(
            std::move(outputShips),
            std::move(outputAtlasMetadata)),
        std::move(outputAtlasImages));
}