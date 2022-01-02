/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ShipDefinition.h"
#include "ShipPreviewData.h"

#include <GameCore/ImageData.h>

#include <cstdint>
#include <filesystem>
#include <optional>

/*
 * All the logic to load and save ships from and to files.
 */
class ShipDeSerializer
{
public:

    static std::string GetShipDefinitionFileExtension()
    {
        return ".shp2";
    }

    static std::string GetImageDefinitionFileExtension()
    {
        return ".png";
    }

    static std::string GetLegacyShpShipDefinitionFileExtension()
    {
        return ".shp";
    }

    static bool IsShipDefinitionFile(std::filesystem::path const & shipFilePath);

    static bool IsAnyShipDefinitionFile(std::filesystem::path const & filepath)
    {
        return IsShipDefinitionFile(filepath)
            || IsImageDefinitionFile(filepath)
            || IsLegacyShpShipDefinitionFile(filepath);
    }

    static ShipDefinition LoadShip(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipPreviewData LoadShipPreviewData(std::filesystem::path const & shipFilePath);

    static RgbaImageData LoadShipPreviewImage(
        ShipPreviewData const & previewData,
        ImageSize const & maxSize);

    static void SaveShip(
        ShipDefinition const & shipDefinition,
        std::filesystem::path const & shipFilePath);

private:

    static bool IsImageDefinitionFile(std::filesystem::path const & shipFilePath);

    static bool IsLegacyShpShipDefinitionFile(std::filesystem::path const & shipFilePath);
};
