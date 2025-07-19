/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-07-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDatabaseBaker.h"

#include <Game/FileStreams.h>

#include <Simulation/ShipDatabase.h>

#include <Core/GameException.h>
#include <Core/Log.h>
#include <Core/PngTools.h>
#include <Core/Utils.h>

ShipDatabaseBaker::ShipDirectory ShipDatabaseBaker::ShipDirectory::Deserialize(picojson::value const & specification)
{
    if (!specification.is<picojson::array>())
    {
        throw GameException("ShipDirectory specification is not a JSON array");
    }

    std::vector<ShipDirectory::Entry> entries;

    for (auto const & entry : specification.get<picojson::array>())
    {
        auto const & entryAsObject = Utils::GetJsonValueAsObject(entry, "ShipDirectory::Entry");

        auto const locator = ShipLocator::Deserialize(entryAsObject.at("locator"));
        auto const hasExternalPreviewImage = Utils::GetOptionalJsonMember<bool>(entryAsObject, "has_external_preview_image", false);

        entries.emplace_back(locator, hasExternalPreviewImage);
    }

    return ShipDirectory(std::move(entries));
}

void ShipDatabaseBaker::Bake(
    std::filesystem::path const & inputShipDirectoryFilePath,
    std::filesystem::path const & inputShipRootDirectoryPath,
    std::filesystem::path const & outputDirectoryPath,
    ImageSize const & maxPreviewImageSize)
{
    if (!std::filesystem::exists(inputShipDirectoryFilePath))
    {
        throw std::runtime_error("Input ship directory file '" + inputShipDirectoryFilePath.string() + "' does not exist");
    }

    if (!std::filesystem::exists(inputShipRootDirectoryPath))
    {
        throw std::runtime_error("Input ship directory '" + inputShipRootDirectoryPath.string() + "' does not exist");
    }

    if (!std::filesystem::exists(outputDirectoryPath))
    {
        throw std::runtime_error("Output directory '" + outputDirectoryPath.string() + "' does not exist");
    }

    // Read directory
    ShipDirectory shipDirectory = ShipDirectory::Deserialize(Utils::ParseJSONString(FileTextReadStream(inputShipDirectoryFilePath).ReadAll()));

    if (shipDirectory.Entries.empty())
    {
        throw std::runtime_error("Input ship directory file '" + inputShipDirectoryFilePath.string() + "' contains an empty directory");
    }

    // Instantiate builder
    ShipDatabaseBuilder builder(maxPreviewImageSize);

    // Add ships
    for (auto const & entry : shipDirectory.Entries)
    {
        std::filesystem::path shipFilePath = inputShipRootDirectoryPath / entry.Locator.RelativeFilePath;

        if (!entry.HasExternalPreviewImage)
        {
            builder.AddShip(
                FileBinaryReadStream(shipFilePath),
                entry.Locator);
        }
        else
        {
            // Load the preview file from the same directory as the database specification
            auto const previewImageFilePath = inputShipDirectoryFilePath.parent_path() / shipFilePath.filename().replace_extension("png");
            auto previewImageFileStream = FileBinaryReadStream(previewImageFilePath);

            builder.AddShip(
                FileBinaryReadStream(shipFilePath),
                PngTools::DecodeImageRgba(previewImageFileStream),
                entry.Locator);
        }
    }

    // Build
    auto output = builder.Build();

    LogMessage("Database ready: ", output.Database.Ships.size(), " ship(s), ", output.PreviewAtlasImages.size(), " preview atlas(es).");

    // Save outcome

    // Ship database specification
    auto const shipDatabaseSpecificationFilePath = outputDirectoryPath / ShipDatabase::SpecificationFilename;
    FileTextWriteStream(shipDatabaseSpecificationFilePath).Write(output.Database.Serialize().serialize(true));

    // Preview atlases
    for (size_t iAtlas = 0; iAtlas < output.PreviewAtlasImages.size(); ++iAtlas)
    {
        auto writeStream = FileBinaryWriteStream(outputDirectoryPath / ShipDatabase::MakePreviewAtlasFilename(iAtlas));
        PngTools::EncodeImage(output.PreviewAtlasImages[iAtlas], writeStream);
    }
}