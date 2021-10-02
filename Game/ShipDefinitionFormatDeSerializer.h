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
class ShipDefinitionFormatDeSerializer
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
        PhysicsData = 6,
        AutoTexturizationSettings = 7,

        Tail = 0xffffffff
    };

    enum class MetadataTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        ShipName = 1,
        Author = 2,
        ArtCredits = 3,
        YearBuilt = 4,
        Description = 5,
        ElectricalPanelMetadata = 6,
        Password = 7
    };

private:

    static void AppendHeader(DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendMetadata(
        ShipMetadata const & metadata,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendMetadataEntry(
        ShipDefinitionFormatDeSerializer::MetadataTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

private:

    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Full_WithoutElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Minimal_WithoutElectricalPanel_Test;
};
