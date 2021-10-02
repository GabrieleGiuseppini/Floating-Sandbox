/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipDefinition.h"
#include "ShipPreview.h"

#include <GameCore/DeSerializationBuffer.h>

#include <cstdint>
#include <filesystem>

/*
 * All the logic to load and save ships from and to .shp2 files.
 */
class ShipDefinitionDeSerializer
{
public:

    static ShipDefinition Load(std::filesystem::path const & shipFilePath);

    static ShipPreview LoadPreview(std::filesystem::path const & shipFilePath);

    static void Save(
        ShipDefinition const & shipDefinition,
        std::filesystem::path const & shipFilePath);

private:

    enum class SectionTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        StructuralLayer = 1,
        ElectricalLayer = 2,
        RopesLayer = 3,
        TextureLayer = 4,
        Metadata = 5,
        PasswordHash = 6
    };

private:

    static void WriteHeader(DeSerializationBuffer<BigEndianess> & buffer);

    static void WriteMetadata(
        ShipMetadata const & metadata,
        DeSerializationBuffer<BigEndianess> & buffer);
};
