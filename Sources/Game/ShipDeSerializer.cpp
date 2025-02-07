/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDeSerializer.h"

#include "FileStreams.h"
#include "FileSystem.h"
#include "GameAssetManager.h"
#include "GameVersion.h"
#include "ShipLegacyFormatDeSerializer.h"

#include <Simulation/ShipDefinitionFormatDeSerializer.h>

#include <Core/GameException.h>

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
        auto inputStream = FileBinaryReadStream(shipFilePath);
        return ShipDefinitionFormatDeSerializer::Load(inputStream, materialDatabase);
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

EnhancedShipPreviewData ShipDeSerializer::LoadShipPreviewData(std::filesystem::path const & shipFilePath)
{
    if (IsShipDefinitionFile(shipFilePath))
    {
        auto inputStream = FileBinaryReadStream(shipFilePath);
        auto const shipPreviewData = ShipDefinitionFormatDeSerializer::LoadPreviewData(inputStream);
        return EnhancedShipPreviewData(
            shipFilePath,
            shipPreviewData.ShipSize,
            shipPreviewData.Metadata,
            shipPreviewData.IsHD,
            shipPreviewData.HasElectricals,
            PortableTimepoint::FromTime(FileSystem::GetLastModifiedTime(shipFilePath)));
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
    EnhancedShipPreviewData const & previewData,
    ImageSize const & maxSize)
{
    if (IsShipDefinitionFile(previewData.PreviewFilePath))
    {
        auto inputStream = FileBinaryReadStream(previewData.PreviewFilePath);
        return ShipDefinitionFormatDeSerializer::LoadPreviewImage(inputStream, maxSize);
    }
    else
    {
        return ShipLegacyFormatDeSerializer::LoadPreviewImage(previewData.PreviewFilePath, maxSize);
    }
}

void ShipDeSerializer::SaveShip(
    ShipDefinition const & shipDefinition,
    std::filesystem::path const & shipFilePath)
{
    auto outputStream = FileBinaryWriteStream(shipFilePath);
    ShipDefinitionFormatDeSerializer::Save(
        shipDefinition,
        CurrentGameVersion,
        outputStream);
}

void ShipDeSerializer::SaveStructuralLayerImage(
    StructuralLayerData const & structuralLayer,
    std::filesystem::path const & shipFilePath)
{
    RgbaImageData structuralLayerImage = RgbaImageData(
        ImageSize(
            structuralLayer.Buffer.Size.width,
            structuralLayer.Buffer.Size.height));

    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            StructuralElement const & element = structuralLayer.Buffer[{x, y}];
            structuralLayerImage[{x, y}] = element.Material != nullptr
                ? element.Material->RenderColor
                : rgbaColor(EmptyMaterialColorKey, 255);
        }
    }

    GameAssetManager::SavePngImage(
        structuralLayerImage,
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