/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "DefaultMaterialDatabase.h"
#include "ElectricalPanel.h"
#include "ShipDefinition.h"
#include "ShipPreviewData.h"

#include <Core/DeSerializationBuffer.h>
#include <Core/GameTypes.h>
#include <Core/ImageData.h>
#include <Core/Streams.h>
#include <Core/Version.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define MAKE_TAG(ch1, ch2, ch3, ch4) \
    std::uint32_t( ((ch1 & 0xff) << 24) | ((ch2 & 0xff) << 16) | ((ch3 & 0xff) << 8) | (ch4 & 0xff) )

/*
 * All the logic to *load* and *save* ships from and to .shp2 files.
 */
class ShipDefinitionFormatDeSerializer
{
public:

    static ShipDefinition Load(
        BinaryReadStream & shipDefinitionInputStream,
        DefaultMaterialDatabase const & defaultMaterialDatabase);

    static ShipPreviewData LoadPreviewData(BinaryReadStream & shipDefinitionInputStream);

    static RgbaImageData LoadPreviewImage(
        BinaryReadStream & shipDefinitionInputStream,
        ImageSize const & maxSize);

    static void Save(
        ShipDefinition const & shipDefinition,
        Version const & currentGameVersion,
        BinaryWriteStream & shipDefinitionOutputStream);

    static PasswordHash CalculatePasswordHash(std::string const & password);

private:

    //
    // File format
    //

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
        Version FileFSVersion;
        ShipSpaceSize ShipSize;
        bool HasTextureLayer;
        bool HasElectricalLayer;

        ShipAttributes(
            Version fileFSVersion,
            ShipSpaceSize shipSize,
            bool hasTextureLayer,
            bool hasElectricalLayer)
            : FileFSVersion(fileFSVersion)
            , ShipSize(shipSize)
            , HasTextureLayer(hasTextureLayer)
            , HasElectricalLayer(hasElectricalLayer)
        {}
    };

    enum class MainSectionTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        StructuralLayer_V1 = MAKE_TAG('S', 'T', 'R', '1'),
        StructuralLayer_V2 = MAKE_TAG('S', 'T', 'R', '2'),
        ElectricalLayer_V1 = MAKE_TAG('E', 'L', 'C', '1'),
        ElectricalLayer_V2 = MAKE_TAG('E', 'L', 'C', '2'),
        RopesLayer_V1 = MAKE_TAG('R', 'P', 'S', '1'),
        RopesLayer_V2 = MAKE_TAG('R', 'P', 'S', '2'),
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

        FSVersion1 = MAKE_TAG('F', 'S', 'V', '1'),  // Legacy, used until 1.17.2 included
        FSVersion2 = MAKE_TAG('F', 'S', 'V', '2'),  // Current, used from 1.17.3 included
        ShipSize = MAKE_TAG('S', 'S', 'Z', '1'),
        HasTextureLayer = MAKE_TAG('H', 'T', 'X', '1'),
        HasElectricalLayer = MAKE_TAG('H', 'E', 'L', '1'),
        LastWriteTime = MAKE_TAG('W', 'R', 'T', '1'),  // Legacy as of multi-platform rearc

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
        Category = MAKE_TAG('C', 'T', 'G', '1'),
        Description = MAKE_TAG('D', 'E', 'S', '1'),
        Scale = MAKE_TAG('S', 'C', 'L', '1'),
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
        InternalAirPressure = MAKE_TAG('I', 'P', 'R', '1'),

        Tail = 0xffffffff
    };

    enum class AutoTexturizationSettingsTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Mode = MAKE_TAG('M', 'D', 'E', '1'),
        MaterialTextureMagnification = MAKE_TAG('M', 'T', 'M', '1'),
        MaterialTextureTransparency = MAKE_TAG('M', 'T', 'T', '1'),

        Tail = 0xffffffff
    };

    enum class CustomMaterialMapEntryTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Name = MAKE_TAG('C', 'N', 'M', '1'),

        RenderColor = MAKE_TAG('C', 'R', 'C', '1'),
        Strength = MAKE_TAG('C', 'S', 'T', '1'),
        Density = MAKE_TAG('C', 'D', 'N', '1'),
        IgnitionTemperature = MAKE_TAG('C', 'I', 'T', '1'),
        MeltingTemperature = MAKE_TAG('C', 'M', 'T', '1'),
        RotReceptivity = MAKE_TAG('C', 'R', 'R', '1'),
        RustReceptivity = MAKE_TAG('C', 'S', 'R', '1'),
        WaterSolubility = MAKE_TAG('C', 'W', 'S', '1'),

        HeatGenerated = MAKE_TAG('C', 'H', 'G', '1'),
        Luminiscence = MAKE_TAG('C', 'L', 'M', '1'),
        LightSpread = MAKE_TAG('C', 'L', 'S', '1'),
        EnginePower = MAKE_TAG('C', 'E', 'P', '1'),
        WaterPumpNominalForce = MAKE_TAG('C', 'W', 'F', '1'),
        TimerDurationSeconds = MAKE_TAG('C', 'T', 'D', '1'),
        ThermalSwitchTransitionTemperature = MAKE_TAG('C', 'T', 'T', '1'),

        Tail = 0xffffffff
    };

    enum class StructuralLayerTagType_V1 : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Buffer = MAKE_TAG('B', 'U', 'F', '1'),

        Tail = 0xffffffff
    };

    enum class StructuralLayerTagType_V2 : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        CustomMaterialsMap = MAKE_TAG('C', 'M', 'M', '1'),
        Buffer = MAKE_TAG('B', 'U', 'F', '2'),

        Tail = 0xffffffff
    };

    enum class ElectricalLayerTagType_V1 : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Buffer = MAKE_TAG('B', 'U', 'F', '1'),
        Panel = MAKE_TAG('P', 'N', 'L', '1'),

        Tail = 0xffffffff
    };

    enum class ElectricalLayerTagType_V2 : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        CustomMaterialsMap = MAKE_TAG('C', 'M', 'M', '1'),
        Buffer = MAKE_TAG('B', 'U', 'F', '2'),
        Panel = MAKE_TAG('P', 'N', 'L', '1'),

        Tail = 0xffffffff
    };

    enum class RopesLayerTagType : std::uint32_t
    {
        // Numeric values are serialized in ship files, changing them will result
        // in ship files being un-deserializable!

        Buffer = MAKE_TAG('B', 'U', 'F', '1'),

        Tail = 0xffffffff
    };

    //
    // Other data structures
    //

    // Key of a custom material found in a ship
    template<typename TMaterial>
    struct CustomMaterialOverridesKey
    {
        MaterialColorKey BaseColorKey;
        typename TMaterial::VariantOverridesType VariantOverrides;

        CustomMaterialOverridesKey(
            MaterialColorKey const & baseColorKey,
            typename TMaterial::VariantOverridesType const & variantOverrides)
            : BaseColorKey(baseColorKey)
            , VariantOverrides(variantOverrides)
        { }

        bool operator==(CustomMaterialOverridesKey const & other) const
        {
            return BaseColorKey == other.BaseColorKey
                && VariantOverrides == other.VariantOverrides;
        }

        bool operator<(CustomMaterialOverridesKey const & other) const
        {
            return std::tie(
                BaseColorKey,
                VariantOverrides) < std::tie(
                    other.BaseColorKey,
                    other.VariantOverrides);
        }
    };

    // Custom material map extracted from a ship
    template<typename TMaterial>
    using CustomMaterialOverridesMap = std::map<CustomMaterialOverridesKey<TMaterial>, std::uint16_t>; // Key -> Index in serialized map

private:

    // Write

    template<typename TSectionAppender>
    static void AppendSection(
        BinaryWriteStream & shipDefinitionOutputStream,
        std::uint32_t tag,
        TSectionAppender const & sectionAppender,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendPngImage(
        RgbaImageData const & rawImageData,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendFileHeader(
        std::uint16_t fileFormatVersion,
        BinaryWriteStream & shipDefinitionOutputStream,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void AppendFileHeader(
        std::uint16_t fileFormatVersion,
        DeSerializationBuffer<BigEndianess> & buffer);

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

    static size_t AppendAutoTexturizationSettings(
        ShipAutoTexturizationSettings const & autoTexturizationSettings,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendAutoTexturizationSettingsEntry(
        ShipDefinitionFormatDeSerializer::AutoTexturizationSettingsTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename TMaterial>
    static size_t AppendCustomMaterialMap(
        CustomMaterialOverridesMap<TMaterial> const & customMaterialMap,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendCustomMaterialVariantOverrides(
        StructuralMaterial::VariantOverridesType const & variantOverrides,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendCustomMaterialVariantOverrides(
        ElectricalMaterial::VariantOverridesType const & variantOverrides,
        DeSerializationBuffer<BigEndianess> & buffer);

    template<typename T>
    static size_t AppendCustomMaterialVariantOverrideEntry(
        ShipDefinitionFormatDeSerializer::CustomMaterialMapEntryTagType tag,
        T const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendCustomMaterialVariantOverrideEntry(
        ShipDefinitionFormatDeSerializer::CustomMaterialMapEntryTagType tag,
        rgbaColor const & value,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayer_V1(
        StructuralLayerData const & structuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayerBuffer_V1(
        Buffer2D<StructuralElement, struct ShipSpaceTag> const & structuralLayerBuffer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayer_V2(
        StructuralLayerData const & structuralLayer,
        CustomMaterialOverridesMap<StructuralMaterial> const & customMaterialOverridesMap,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendStructuralLayerBuffer_V2(
        Buffer2D<StructuralElement, struct ShipSpaceTag> const & structuralLayerBuffer,
        CustomMaterialOverridesMap<StructuralMaterial> const & customMaterialOverridesMap,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayer_V1(
        ElectricalLayerData const & electricalLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayerBuffer_V1(
        Buffer2D<ElectricalElement, struct ShipSpaceTag> const & electricalLayerBuffer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayer_V2(
        ElectricalLayerData const & electricalLayer,
        CustomMaterialOverridesMap<ElectricalMaterial> const & customMaterialOverridesMap,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendElectricalLayerPanel(
        ElectricalPanel const & electricalPanel,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendRopesLayer_V1(
        RopesLayerData const & ropesLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendRopesLayerBuffer_V1(
        RopeBuffer const & ropesLayerBuffer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static size_t AppendPngPreview(
        StructuralLayerData const & structuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer);

    static CustomMaterialOverridesMap<StructuralMaterial> ExtractCustomMaterialOverridesMap(StructuralLayerData const & layerData);

    static CustomMaterialOverridesMap<ElectricalMaterial> ExtractCustomMaterialOverridesMap(ElectricalLayerData const & layerData);

    static CustomMaterialOverridesMap<StructuralMaterial> ExtractCustomMaterialOverridesMap(RopesLayerData const & layerData);

    // Read

    template<typename SectionHandler>
    static void Parse(
        BinaryReadStream & shipDefinitionInputStream,
        SectionHandler const & sectionHandler);

    static void ThrowMaterialNotFound(ShipAttributes const & shipAttributes);

    static void ReadIntoBuffer(
        BinaryReadStream & shipDefinitionInputStream,
        DeSerializationBuffer<BigEndianess> & buffer,
        size_t size);

    static SectionHeader ReadSectionHeader(
        BinaryReadStream & shipDefinitionInputStream,
        DeSerializationBuffer<BigEndianess> & buffer);

    static SectionHeader ReadSectionHeader(
        DeSerializationBuffer<BigEndianess> const & buffer,
        size_t offset);

    static RgbaImageData ReadPngImage(
        BinaryReadStream & shipDefinitionInputStream,
        size_t imageDataSize);

    static RgbaImageData ReadPngImageAndMakePreview(
        BinaryReadStream & shipDefinitionInputStream,
        size_t imageDataSize,
        ImageSize const & maxSize);

    static void ReadFileHeader(
        BinaryReadStream & shipDefinitionInputStream,
        DeSerializationBuffer<BigEndianess> & buffer);

    static void ReadFileHeader(DeSerializationBuffer<BigEndianess> & buffer);

    static ShipAttributes ReadShipAttributes(DeSerializationBuffer<BigEndianess> const & buffer);

    static ShipMetadata ReadMetadata(DeSerializationBuffer<BigEndianess> const & buffer);

    static ShipPhysicsData ReadPhysicsData(DeSerializationBuffer<BigEndianess> const & buffer);

    static ShipAutoTexturizationSettings ReadAutoTexturizationSettings(DeSerializationBuffer<BigEndianess> const & buffer);

    static std::vector<std::unique_ptr<StructuralMaterial const>> ReadCustomMaterialMap(
        DeSerializationBuffer<BigEndianess> const & buffer,
        size_t bufferReadOffset,
        MaterialColorMap<StructuralMaterial> const & baseMaterialColorMap);

    static std::vector<std::unique_ptr<ElectricalMaterial const>> ReadCustomMaterialMap(
        DeSerializationBuffer<BigEndianess> const & buffer,
        size_t bufferReadOffset,
        MaterialColorMap<ElectricalMaterial> const & baseMaterialColorMap);

    static void ReadStructuralLayer_V1(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialColorMap<StructuralMaterial> const & defaultMaterialColorMap,
        std::unique_ptr<StructuralLayerData> & structuralLayer);

    static std::vector<std::unique_ptr<StructuralMaterial const>> ReadStructuralLayer_V2(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialColorMap<StructuralMaterial> const & defaultMaterialColorMap,
        std::unique_ptr<StructuralLayerData> & structuralLayer);

    static void ReadElectricalLayer_V1(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialColorMap<ElectricalMaterial> const & defaultMaterialColorMap,
        std::unique_ptr<ElectricalLayerData> & electricalLayer);

    static std::vector<std::unique_ptr<ElectricalMaterial const>> ReadElectricalLayer_V2(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialColorMap<ElectricalMaterial> const & defaultMaterialColorMap,
        std::unique_ptr<ElectricalLayerData> & electricalLayer);

    static void ReadRopesLayer_V1(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ShipAttributes const & shipAttributes,
        MaterialColorMap<StructuralMaterial> const & defaultMaterialColorMap,
        std::unique_ptr<RopesLayerData> & ropesLayer);

private:

    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_Test;
    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_UnrecognizedHeader_Test;
    friend class ShipDefinitionFormatDeSerializerTests_FileHeader_UnsupportedFileFormatVersion_Test;
    friend class ShipDefinitionFormatDeSerializerTests_ShipAttributes_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Full_Test;
    friend class ShipDefinitionFormatDeSerializerTests_Metadata_Minimal_Test;
    friend class ShipDefinitionFormatDeSerializerTests_PhysicsData_Test;
    friend class ShipDefinitionFormatDeSerializerTests_AutoTexturizationSettings_Test;

    friend class ShipDefinitionFormatDeSerializer_StructuralCustomMaterialMapTests;
    friend class ShipDefinitionFormatDeSerializer_StructuralCustomMaterialMapTests_Basic_Test;

    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV1Tests;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV1Tests_VariousSizes_Uniform_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV1Tests_MidSize_Heterogeneous_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV1Tests_Nulls_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV1Tests_UnrecognizedMaterial_Test;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV2Tests;
    friend class ShipDefinitionFormatDeSerializer_StructuralLayerV2Tests_Basic_Test;

    friend class ShipDefinitionFormatDeSerializer_ElectricalCustomMaterialMapTests;
    friend class ShipDefinitionFormatDeSerializer_ElectricalCustomMaterialMapTests_Basic_Test;

    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV1Tests;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV1Tests_MidSize_NonInstanced_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV1Tests_MidSize_Instanced_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV1Tests_Nulls_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV1Tests_ElectricalPanel_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV1Tests_UnrecognizedMaterial_Test;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV2Tests;
    friend class ShipDefinitionFormatDeSerializer_ElectricalLayerV2Tests_Basic_Test;

    friend class ShipDefinitionFormatDeSerializer_RopesLayerV1Tests;
    friend class ShipDefinitionFormatDeSerializer_RopesLayerV1Tests_TwoElements_Test;
    friend class ShipDefinitionFormatDeSerializer_RopesLayerV1Tests_UnrecognizedMaterial_Test;
};
