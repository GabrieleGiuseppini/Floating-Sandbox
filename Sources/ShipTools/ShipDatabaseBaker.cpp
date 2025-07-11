/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-07-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDatabaseBaker.h"

#include <Game/FileStreams.h>

#include <Simulation/ShipDatabase.h>

#include <Core/GameException.h>
#include <Core/PngTools.h>
#include <Core/Utils.h>

ShipDatabaseBaker::ShipDirectory ShipDatabaseBaker::ShipDirectory::Deserialize(picojson::value const & specification)
{
    if (!specification.is<picojson::array>())
    {
        throw GameException("ShipDirectory specification is not a JSON array");
    }

    std::vector<ShipLocator> locators;

    for (auto const & entry : specification.get<picojson::array>())
    {
        locators.emplace_back(ShipLocator::Deserialize(entry));
    }

    return ShipDirectory(std::move(locators));
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

    // Instantiate builder
    ShipDatabaseBuilder builder(maxPreviewImageSize);

    // Add ships
    for (auto const & locator : shipDirectory.Locators)
    {
        builder.AddShip(
            FileBinaryReadStream(inputShipRootDirectoryPath / locator.RelativeFilePath),
            locator);
    }

    // Build
    auto output = builder.Build();

    // Save outcome

    // Ship database specification
    auto const shipDatabaseSpecificationFilePath = outputDirectoryPath / ShipDatabase::SpecificationFilename;
    FileTextWriteStream(shipDatabaseSpecificationFilePath).Write(output.Database.Serialize().serialize(true));

    // Preview atlases
    for (size_t iAtlas = 0; iAtlas < output.AtlasImages.size(); ++iAtlas)
    {
        auto writeStream = FileBinaryWriteStream(outputDirectoryPath / ShipDatabase::MakePreviewAtlasFilename(iAtlas));
        PngTools::EncodeImage(output.AtlasImages[iAtlas], writeStream);
    }
}