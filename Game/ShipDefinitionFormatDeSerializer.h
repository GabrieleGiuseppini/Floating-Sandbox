/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ShipDefinition.h"
#include "ShipPreviewData.h"

#include <GameCore/DeSerializationBuffer.h>
#include <GameCore/ImageData.h>
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

    static ShipPreviewData LoadPreviewData(std::filesystem::path const & shipFilePath);

    static RgbaImageData LoadPreviewImage(
        std::filesystem::path const & previewFilePath,
        ImageSize const & maxSize);

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

    struct FileHeader
    {
        char Title[24];
        std::uint16_t FloatingSandboxVersionMaj;
        std::uint16_t FloatingSandboxVersionMin;
        std::uint16_t FileFormatVersion;
        char Pad[2];
    };

    static_assert(sizeof(FileHeader) == 32);

#pragma pack(pop)

    enum class MainSectionTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        StructuralLayer = 1,
        ElectricalLayer = 2,
        RopesLayer = 3,
        TextureLayer_PNG = 4,
        Metadata = 5,
        PhysicsData = 6,
        AutoTexturizationSettings = 7,
        ShipSize = 8,
        Preview_PNG = 9,

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
        ElectricalPanelMetadata_V1 = 6,
        Password = 7,
        DoHideElectricalsInPreview = 8,
        DoHideHDInPreview = 9,

        Tail = 0xffffffff
    };

    struct DeserializationContext
    {
        std::string const ShipFilename;
        int const FileFSVersionMaj;
        int const FileFSVersionMin;

        DeserializationContext(
            std::string shipFilename,
            int fileFSVersionMaj,
            int fileFSVersionMin)
            : ShipFilename(std::move(shipFilename))
            , FileFSVersionMaj(fileFSVersionMaj)
            , FileFSVersionMin(fileFSVersionMin)
        {}
    };

private:

    // Write

    template<typename TSectionAppender>
    static void AppendSection(
        std::ofstream & outputFile,
        std::uint32_t tag,
        TSectionAppender const & sectionAppender,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendPngImage(
        RgbaImageData const & rawImageDAta,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendFileHeader(
        std::ofstream & outputFile,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendFileHeader(DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendShipSize(
        ShipSpaceSize const & shipSize,
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

    static size_t AppendPngPreview(
        StructuralLayerBuffer const & structuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    // Read

    template<typename SectionHandler>
    static void Parse(
        std::filesystem::path const & shipFilePath,
        SectionHandler const & sectionHandler);

    static std::ifstream OpenFileForRead(std::filesystem::path const & shipFilePath);

    static void ThrowMaterialNotFound(DeserializationContext const & deserializationContext);

    static void ReadIntoBuffer(
        std::ifstream & inputFile,
        DeSerializationBuffer<BigEndianess> & buffer,
        size_t size);

    static SectionHeader ReadSectionHeader(
        std::ifstream & inputFile,
        DeSerializationBuffer<BigEndianess> & buffer);

    static SectionHeader ReadSectionHeader(
        DeSerializationBuffer<BigEndianess> const & buffer,
        size_t offset);

    static RgbaImageData ReadPngImage(DeSerializationBuffer<BigEndianess> & buffer);

    static RgbaImageData ReadPngImageAndResize(
        DeSerializationBuffer<BigEndianess> & buffer,
        ImageSize const & maxSize);

    static DeserializationContext ReadFileHeader(
        std::ifstream & inputFile,
        std::string shipFilename,
        DeSerializationBuffer<BigEndianess> & buffer);

    static DeserializationContext ReadFileHeader(
        DeSerializationBuffer<BigEndianess> & buffer,
        std::string shipFilename);

    static ShipSpaceSize ReadShipSize(DeSerializationBuffer<BigEndianess> const & buffer);

    static ShipMetadata ReadMetadata(
        DeSerializationBuffer<BigEndianess> const & buffer,
        DeserializationContext & deserializationContext);

    static void ReadStructuralLayer(
        DeSerializationBuffer<BigEndianess> const & buffer,
        DeserializationContext & deserializationContext,
        ShipSpaceSize const & shipSize,
        MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
        std::unique_ptr<StructuralLayerBuffer> & structuralLayerBuffer);

private:

    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_Test;
    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_UnrecognizedHeader_Test;
    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_UnsupportedFileFormatVersion_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Full_WithoutElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Minimal_WithoutElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_ElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests_VariousSizes_Uniform_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests_MidSize_Heterogeneous_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests_UnrecognizedMaterial_SameVersion_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests_UnrecognizedMaterial_LaterVersion_Test;
};
