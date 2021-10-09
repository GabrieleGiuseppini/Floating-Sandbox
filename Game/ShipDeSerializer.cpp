/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDeSerializer.h"

#include "ImageFileTools.h"
#include "ShipDefinitionFormatDeSerializer.h"
#include "ShipLegacyFormatDeSerializer.h"

#include <GameCore/GameException.h>
#include <GameCore/ImageTools.h>

#include <memory>

bool ShipDeSerializer::IsShipDefinitionFile(std::filesystem::path const & shipFilePath)
{
    return Utils::CaseInsensitiveEquals(shipFilePath.extension().string(), GetShipDefinitionFileExtension());
}

ShipDefinition ShipDeSerializer::LoadShip(
    std::filesystem::path const & shipFilePath,
    MaterialDatabase const & materialDatabase)
{
    if (IsShipDefinitionFile(shipFilePath))
    {
        return ShipDefinitionFormatDeSerializer::Load(shipFilePath, materialDatabase);
    }
    else if (IsImageDefinitionFile(shipFilePath))
    {
        return ShipLegacyFormatDeSerializer::LoadShipFromImageDefinition(shipFilePath, materialDatabase);
    }
    else if (IsLegacyShpShipDefinitionFile(shipFilePath))
    {
        return ShipLegacyFormatDeSerializer::LoadShipFromLegacyShpShipDefinition(shipFilePath, materialDatabase);
    }
    else
    {
        throw GameException("Ship filename \"" + shipFilePath.filename().string() + "\" is not recognized as a ship file");
    }
}

ShipPreviewData ShipDeSerializer::LoadShipPreviewData(std::filesystem::path const & shipFilePath)
{
    if (IsShipDefinitionFile(shipFilePath))
    {
        return ShipDefinitionFormatDeSerializer::LoadPreviewData(shipFilePath);
    }
    else if (IsImageDefinitionFile(shipFilePath))
    {
        return ShipLegacyFormatDeSerializer::LoadShipPreviewDataFromImageDefinition(shipFilePath);
    }
    else if (IsLegacyShpShipDefinitionFile(shipFilePath))
    {
        return ShipLegacyFormatDeSerializer::LoadShipPreviewDataFromLegacyShpShipDefinition(shipFilePath);
    }
    else
    {
        throw GameException("Ship filename \"" + shipFilePath.filename().string() + "\" is not recognized as a valid ship file");
    }
}

RgbaImageData ShipDeSerializer::LoadShipPreviewImage(
    ShipPreviewData const & previewData,
    ImageSize const & maxSize)
{
    RgbaImageData previewImage = IsShipDefinitionFile(previewData.PreviewFilePath)
        ? ShipDefinitionFormatDeSerializer::LoadPreviewImage(previewData.PreviewFilePath, maxSize)
        : ShipLegacyFormatDeSerializer::LoadPreviewImage(previewData.PreviewFilePath, maxSize);

    // Trim
    return ImageTools::TrimWhiteOrTransparent(std::move(previewImage));
}

void ShipDeSerializer::SaveShip(
    ShipDefinition const & shipDefinition,
    std::filesystem::path const & shipFilePath)
{
    ShipDefinitionFormatDeSerializer::Save(
        shipDefinition,
        shipFilePath);
}

///////////////////////////////////////////////////////

bool ShipDeSerializer::IsImageDefinitionFile(std::filesystem::path const & shipFilePath)
{
    return Utils::CaseInsensitiveEquals(shipFilePath.extension().string(), GetImageDefinitionFileExtension());
}

bool ShipDeSerializer::IsLegacyShpShipDefinitionFile(std::filesystem::path const & shipFilePath)
{
    return Utils::CaseInsensitiveEquals(shipFilePath.extension().string(), GetLegacyShpShipDefinitionFileExtension());
}