/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ShipDefinition.h"
#include "ShipPreview.h"

#include <GameCore/DeSerializationBuffer.h>
#include <GameCore/Version.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>

/*
 * All the logic to load and save ships from and to .shp2 files.
 */
class ShipDefinitionFormatDeSerializer
{
public:

    static ShipDefinition Load(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipPreview LoadPreview(std::filesystem::path const & shipFilePath);

    static void Save(
        ShipDefinition const & shipDefinition,
        std::filesystem::path const & shipFilePath);

private:

#pragma pack(push, 1)

    struct SectionHeader
    {
        std::uint32_t Tag;
        std::uint32_t SectionBodySize; // Excluding header
    };

#pragma pack(pop)

    enum class MainSectionTagType : std::uint32_t
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
        ElectricalPanelMetadataV1 = 6,
        Password = 7,
        DoHideElectricalsInPreview = 8,
        DoHideHDInPreview = 9,

        Tail = 0xffffffff
    };

private:

    // Write

    static void AppendFileHeader(
        std::ofstream & outputFile,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename TSectionAppender>
    static void AppendSection(
        std::ofstream & outputFile,
        std::uint32_t tag,
        TSectionAppender const & sectionAppender,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendMetadata(
        ShipMetadata const & metadata,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendMetadataEntry(
        ShipDefinitionFormatDeSerializer::MetadataTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayer(
        StructuralLayerBuffer const & structuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    // Read

    static std::ifstream OpenFileForRead(std::filesystem::path const & shipFilePath);

    static void ReadIntoBuffer(
        std::ifstream & inputFile,
        DeSerializationBuffer<BigEndianess> & buffer,
        size_t size);

    static void ReadFileHeader(
        std::ifstream & inputFile,
        MajorMinorVersion & fileFloatingSandboxVersion,
        DeSerializationBuffer<BigEndianess> & buffer);

    static SectionHeader ReadSectionHeader(
        std::ifstream & inputFile,
        DeSerializationBuffer<BigEndianess> & buffer);

    static SectionHeader ReadSectionHeader(
        DeSerializationBuffer<BigEndianess> const & buffer,
        size_t offset);

    static void ReadMetadata(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipMetadata & metadata);

    static void ReadStructuralLayer(
        DeSerializationBuffer<BigEndianess> const & buffer,
        MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
        std::unique_ptr<StructuralLayerBuffer> & structuralLayerBuffer);

    static void ThrowInvalidFile();

private:

    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Full_WithoutElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Minimal_WithoutElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_ElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests_VariousSizes_Uniform_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests_MidSize_Heterogeneous_Test;
};
