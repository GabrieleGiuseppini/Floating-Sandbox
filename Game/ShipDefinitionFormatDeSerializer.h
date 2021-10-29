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

#define MAKE_TAG(ch1, ch2, ch3, ch4) \
    std::uint32_t( ((ch1 & 0xff) << 24) | ((ch2 & 0xff) << 16) | ((ch3 & 0xff) << 8) | (ch4 & 0xff) )

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
        std::uint16_t FileFormatVersion;
        char Pad[6];
    };

    static_assert(sizeof(FileHeader) == 32);

#pragma pack(pop)

    struct ShipAttributes
    {
        int FileFSVersionMaj;
        int FileFSVersionMin;
        ShipSpaceSize ShipSize;
        bool HasTextureLayer;
        bool HasElectricalLayer;

        ShipAttributes(
            int fileFSVersionMaj,
            int fileFSVersionMin,
            ShipSpaceSize shipSize,
            bool hasTextureLayer,
            bool hasElectricalLayer)
            : FileFSVersionMaj(fileFSVersionMaj)
            , FileFSVersionMin(fileFSVersionMin)
            , ShipSize(shipSize)
            , HasTextureLayer(hasTextureLayer)
            , HasElectricalLayer(hasElectricalLayer)
        {}
    };

    enum class MainSectionTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        StructuralLayer = MAKE_TAG('S', 'T', 'R', '1'),
        ElectricalLayer = MAKE_TAG('E', 'L', 'C', '1'),
        RopesLayer = MAKE_TAG('R', 'P', 'S', '1'),
        TextureLayer_PNG = MAKE_TAG('T', 'X', 'P', '1'),
        Metadata = MAKE_TAG('M', 'E', 'T', '1'),
        PhysicsData = MAKE_TAG('P', 'H', 'S', '1'),
        AutoTexturizationSettings = MAKE_TAG('A', 'T', 'X', '1'),
        ShipAttributes = MAKE_TAG('A', 'T', 'T', '1'),
        Preview_PNG = MAKE_TAG('P', 'V', 'P', '1'),

        Tail = 0xffffffff
    };

    enum class ShipAttributesTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        FSVersion = MAKE_TAG('F', 'S', 'V', '1'),
        ShipSize = MAKE_TAG('S', 'I', 'X', '1'),
        HasTextureLayer = MAKE_TAG('H', 'T', 'X', '1'),
        HasElectricalLayer = MAKE_TAG('H', 'E', 'L', '1'),

        Tail = 0xffffffff
    };

    enum class MetadataTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        ShipName = MAKE_TAG('N', 'A', 'M', '1'),
        Author = MAKE_TAG('A', 'U', 'T', '1'),
        ArtCredits = MAKE_TAG('A', 'C', 'R', '1'),
        YearBuilt = MAKE_TAG('Y', 'R', 'B', '1'),
        Description = MAKE_TAG('D', 'E', 'S', '1'),
        Password = MAKE_TAG('P', 'P', 'P', '1'),
        DoHideElectricalsInPreview = MAKE_TAG('H', 'E', 'P', '1'),
        DoHideHDInPreview = MAKE_TAG('H', 'H', 'P', '1'),

        Tail = 0xffffffff
    };

    enum class PhysicsDataTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        OffsetX = MAKE_TAG('O', 'F', 'X', '1'),
        OffsetY = MAKE_TAG('O', 'F', 'Y', '1'),
        InternalPressure = MAKE_TAG('I', 'P', 'R', '1'),

        Tail = 0xffffffff
    };

    enum class StructuralLayerTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Buffer = MAKE_TAG('B', 'U', 'F', '1'),

        Tail = 0xffffffff
    };

    enum class ElectricalLayerTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Buffer = MAKE_TAG('B', 'U', 'F', '1'),
        Panel = MAKE_TAG('P', 'N', 'L', '1'),

        Tail = 0xffffffff
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
        RgbaImageData const & rawImageData,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendFileHeader(
        std::ofstream & outputFile,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendFileHeader(DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendShipAttributes(
        ShipAttributes const & shipAttributes,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendShipAttributesEntry(
        ShipDefinitionFormatDeSerializer::ShipAttributesTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendMetadata(
        ShipMetadata const & metadata,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendMetadataEntry(
        ShipDefinitionFormatDeSerializer::MetadataTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendPhysicsData(
        ShipPhysicsData const & physicsData,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendPhysicsDataEntry(
        ShipDefinitionFormatDeSerializer::PhysicsDataTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayer(
        StructuralLayerData const & structuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayerBuffer(
        Buffer2D<StructuralElement, struct ShipSpaceTag> const & structuralLayerBuffer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayer(
        ElectricalLayerData const & electricalLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayerBuffer(
        Buffer2D<ElectricalElement, struct ShipSpaceTag> const & electricalLayerBuffer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayerPanel(
        ElectricalPanelMetadata const & electricalPanel,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendPngPreview(
        StructuralLayerData const & structuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    // Read

    template<typename SectionHandler>
    static void Parse(
        std::filesystem::path const & shipFilePath,
        SectionHandler const & sectionHandler);

    static std::ifstream OpenFileForRead(std::filesystem::path const & shipFilePath);

    static void ThrowMaterialNotFound(ShipAttributes const & shipAttributes);

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

    static void ReadFileHeader(
        std::ifstream & inputFile,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void ReadFileHeader(DeSerializationBuffer<BigEndianess> & buffer);

    static ShipAttributes ReadShipAttributes(DeSerializationBuffer<BigEndianess> const & buffer);

    static ShipMetadata ReadMetadata(DeSerializationBuffer<BigEndianess> const & buffer);

    static ShipPhysicsData ReadPhysicsData(DeSerializationBuffer<BigEndianess> const & buffer);

    static void ReadStructuralLayer(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
        std::unique_ptr<StructuralLayerData> & structuralLayer);

    static void ReadElectricalLayer(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialDatabase::MaterialMap<ElectricalMaterial> const & materialMap,
        std::unique_ptr<ElectricalLayerData> & electricalLayer);

private:

    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_Test;
    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_UnrecognizedHeader_Test;
    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_UnsupportedFileFormatVersion_Test;
    friend class ShipDefinitionFormatDeSerializerTests_ShipAttributes_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Full_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Minimal_Test;
    friend class ShipDefinitionFormatDeSerializerTests_PhysicsData_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerTests;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerTests_VariousSizes_Uniform_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerTests_MidSize_Heterogeneous_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerTests_UnrecognizedMaterial_SameVersion_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerTests_UnrecognizedMaterial_LaterVersion_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerTests;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerTests_MidSize_NonInstanced_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerTests_MidSize_Instanced_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerTests_ElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerTests_UnrecognizedMaterial_SameVersion_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerTests_UnrecognizedMaterial_LaterVersion_Test;
};
