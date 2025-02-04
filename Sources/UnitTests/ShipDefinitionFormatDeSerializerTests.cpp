#include <Simulation/ShipDefinitionFormatDeSerializer.h>

#include <Core/MemoryStreams.h>
#include <Core/UserGameException.h>
#include <Core/Version.h>

#include "gtest/gtest.h"

#include <algorithm>
#include <map>
#include <vector>

TEST(ShipDefinitionFormatDeSerializerTests, FileHeader)
{
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendFileHeader(buffer);
    buffer.WriteAt(std::uint16_t(200), offsetof(ShipDefinitionFormatDeSerializer::FileHeader, FileFormatVersion));

    std::uint16_t fileFormatVersion;
    buffer.ReadAt(offsetof(ShipDefinitionFormatDeSerializer::FileHeader, FileFormatVersion), fileFormatVersion);
    EXPECT_EQ(fileFormatVersion, 200);
}

TEST(ShipDefinitionFormatDeSerializerTests, FileHeader_UnrecognizedHeader)
{
    DeSerializationBuffer<BigEndianess> buffer(256);
    buffer.ReserveAndAdvance(sizeof(ShipDefinitionFormatDeSerializer::SectionHeader));
    buffer.WriteAt(std::uint32_t(0xaabbccdd), 0);

    try
    {
        ShipDefinitionFormatDeSerializer::ReadFileHeader(buffer);
        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::UnrecognizedShipFile);
        EXPECT_TRUE(exc.Parameters.empty());
    }
}

TEST(ShipDefinitionFormatDeSerializerTests, FileHeader_UnsupportedFileFormatVersion)
{
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendFileHeader(buffer);
    buffer.WriteAt(std::uint16_t(200), offsetof(ShipDefinitionFormatDeSerializer::FileHeader, FileFormatVersion));

    try
    {
        ShipDefinitionFormatDeSerializer::ReadFileHeader(buffer);
        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::UnsupportedShipFile);
        EXPECT_TRUE(exc.Parameters.empty());
    }
}

TEST(ShipDefinitionFormatDeSerializerTests, ShipAttributes)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipDefinitionFormatDeSerializer::ShipAttributes sourceShipAttributes(
        Version(200, 14, 54, 1),
        ShipSpaceSize(242, 409),
        true,
        false);

    ShipDefinitionFormatDeSerializer::AppendShipAttributes(sourceShipAttributes, buffer);

    // Read

    ShipDefinitionFormatDeSerializer::ShipAttributes const targetShipAttributes = ShipDefinitionFormatDeSerializer::ReadShipAttributes(buffer);

    EXPECT_EQ(sourceShipAttributes.FileFSVersion.GetMajor(), targetShipAttributes.FileFSVersion.GetMajor());
    EXPECT_EQ(sourceShipAttributes.FileFSVersion.GetMinor(), targetShipAttributes.FileFSVersion.GetMinor());
    EXPECT_EQ(sourceShipAttributes.FileFSVersion.GetPatch(), targetShipAttributes.FileFSVersion.GetPatch());
    EXPECT_EQ(sourceShipAttributes.FileFSVersion.GetBuild(), targetShipAttributes.FileFSVersion.GetBuild());
    EXPECT_EQ(sourceShipAttributes.ShipSize, targetShipAttributes.ShipSize);
    EXPECT_EQ(sourceShipAttributes.HasTextureLayer, targetShipAttributes.HasTextureLayer);
    EXPECT_EQ(sourceShipAttributes.HasElectricalLayer, targetShipAttributes.HasElectricalLayer);
}

TEST(ShipDefinitionFormatDeSerializerTests, Metadata_Full)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipMetadata sourceMd("Test ship");
    sourceMd.ArtCredits = "KillerWhale";
    sourceMd.Author = "Gabriele Giuseppini";
    sourceMd.Description = "Supercaligragilisticexpiralidocius";
    sourceMd.Password = 0x1122334455667788u;
    sourceMd.Scale = ShipSpaceToWorldSpaceCoordsRatio(4.0f, 100.5f);
    sourceMd.ShipName = "Best ship";
    sourceMd.YearBuilt = "2020-2021";
    sourceMd.DoHideElectricalsInPreview = true;
    sourceMd.DoHideHDInPreview = false;

    ShipDefinitionFormatDeSerializer::AppendMetadata(sourceMd, buffer);

    // Read

    ShipMetadata const targetMd = ShipDefinitionFormatDeSerializer::ReadMetadata(buffer);

    EXPECT_EQ(sourceMd.ArtCredits, targetMd.ArtCredits);
    EXPECT_EQ(sourceMd.Author, targetMd.Author);
    EXPECT_EQ(sourceMd.Description, targetMd.Description);
    EXPECT_EQ(sourceMd.Password, targetMd.Password);
    EXPECT_EQ(sourceMd.Scale, targetMd.Scale);
    EXPECT_EQ(sourceMd.ShipName, targetMd.ShipName);
    EXPECT_EQ(sourceMd.YearBuilt, targetMd.YearBuilt);
    EXPECT_EQ(sourceMd.DoHideElectricalsInPreview, targetMd.DoHideElectricalsInPreview);
    EXPECT_EQ(sourceMd.DoHideHDInPreview, targetMd.DoHideHDInPreview);
}

TEST(ShipDefinitionFormatDeSerializerTests, Metadata_Minimal)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipMetadata sourceMd("Test ship");
    ShipDefinitionFormatDeSerializer::AppendMetadata(sourceMd, buffer);

    // Read

    ShipMetadata const targetMd = ShipDefinitionFormatDeSerializer::ReadMetadata(buffer);

    EXPECT_FALSE(sourceMd.ArtCredits.has_value());
    EXPECT_FALSE(sourceMd.Author.has_value());
    EXPECT_FALSE(sourceMd.Description.has_value());
    EXPECT_FALSE(sourceMd.Password.has_value());
    EXPECT_EQ(sourceMd.ShipName, targetMd.ShipName);
    EXPECT_FALSE(sourceMd.YearBuilt.has_value());
    EXPECT_FALSE(sourceMd.DoHideElectricalsInPreview);
    EXPECT_FALSE(sourceMd.DoHideHDInPreview);
}

TEST(ShipDefinitionFormatDeSerializerTests, PhysicsData)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipPhysicsData sourcePd(vec2f(0.75f, 256.0f), 242.0f);
    ShipDefinitionFormatDeSerializer::AppendPhysicsData(sourcePd, buffer);

    // Read

    ShipPhysicsData const targetPd = ShipDefinitionFormatDeSerializer::ReadPhysicsData(buffer);

    EXPECT_EQ(sourcePd.Offset, targetPd.Offset);
    EXPECT_EQ(sourcePd.InternalPressure, targetPd.InternalPressure);
}

TEST(ShipDefinitionFormatDeSerializerTests, AutoTexturizationSettings)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipAutoTexturizationSettings sourceAts(ShipAutoTexturizationModeType::MaterialTextures, 0.5f, 0.75f);
    ShipDefinitionFormatDeSerializer::AppendAutoTexturizationSettings(sourceAts, buffer);

    // Read

    ShipAutoTexturizationSettings const targetAts = ShipDefinitionFormatDeSerializer::ReadAutoTexturizationSettings(buffer);

    EXPECT_EQ(sourceAts.Mode, targetAts.Mode);
    EXPECT_EQ(sourceAts.MaterialTextureMagnification, targetAts.MaterialTextureMagnification);
    EXPECT_EQ(sourceAts.MaterialTextureTransparency, targetAts.MaterialTextureTransparency);
}

class ShipDefinitionFormatDeSerializer_StructuralLayerTests : public testing::Test
{
protected:

    void SetUp()
    {
        for (std::uint8_t i = 0; i < 250; ++i)
        {
            MaterialColorKey colorKey(
                i + 2,
                i + 1,
                i);

            TestMaterialMap.try_emplace(
                colorKey,
                StructuralMaterial(
                    colorKey,
                    "Material " + std::to_string(i),
                    rgbaColor(colorKey, 255)));
        }
    }

    void VerifyDeserializedStructuralLayer(
        StructuralLayerData const & sourceStructuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer)
    {
        std::unique_ptr<StructuralLayerData> targetStructuralLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(Version(1, 16, 200, 4), sourceStructuralLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetStructuralLayer);

        EXPECT_EQ(targetStructuralLayer->Buffer.Size, sourceStructuralLayer.Buffer.Size);
        ASSERT_EQ(targetStructuralLayer->Buffer.GetByteSize(), sourceStructuralLayer.Buffer.GetByteSize());
        for (int y = 0; y < targetStructuralLayer->Buffer.Size.height; ++y)
        {
            for (int x = 0; x < targetStructuralLayer->Buffer.Size.width; ++x)
            {
                ShipSpaceCoordinates const coords{ x, y };
                EXPECT_EQ(sourceStructuralLayer.Buffer[coords], targetStructuralLayer->Buffer[coords]);
            }
        }
    }

    std::map<MaterialColorKey, StructuralMaterial> TestMaterialMap;
};

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerTests, VariousSizes_Uniform)
{
    for (int iParam = 1; iParam < 16384 * 2 + 1;)
    {
        StructuralLayerData sourceStructuralLayer(
            Buffer2D<StructuralElement, struct ShipSpaceTag>(
                ShipSpaceSize(iParam, 1),
                StructuralElement(nullptr))); // Empty

        ASSERT_EQ(sourceStructuralLayer.Buffer.Size.GetLinearSize(), static_cast<size_t>(iParam));

        DeSerializationBuffer<BigEndianess> buffer(256);
        ShipDefinitionFormatDeSerializer::AppendStructuralLayer(sourceStructuralLayer, buffer);

        //
        // Verify RLE:
        //  16383 times: EmptyMaterialKey
        //  16383 times: EmptyMaterialKey
        //  ...
        //      n times: EmptyMaterialKey
        //

        size_t idx = sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Buffer header

        int iRunSize = iParam;
        while (iRunSize >= 16383)
        {
            // Count
            var_uint16_t fullCount;
            idx += buffer.ReadAt(idx, fullCount);
            ASSERT_EQ(fullCount.value(), 16383);

            // Value
            MaterialColorKey fullColorKey;
            idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&fullColorKey), sizeof(fullColorKey));
            EXPECT_EQ(fullColorKey, EmptyMaterialColorKey);

            iRunSize -= 16383;
        }

        ASSERT_LE(iRunSize, 16383);

        if (iRunSize > 0)
        {
            // Count
            var_uint16_t partialCount;
            idx += buffer.ReadAt(idx, partialCount);
            ASSERT_EQ(partialCount.value(), static_cast<std::uint16_t>(iRunSize));

            // Value
            MaterialColorKey partialColorKey;
            idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&partialColorKey), sizeof(partialColorKey));
            EXPECT_EQ(partialColorKey, EmptyMaterialColorKey);
        }

        idx += sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Tail

        // Buffer is done
        EXPECT_EQ(idx, buffer.GetSize());

        //
        // Verify may be read
        //

        VerifyDeserializedStructuralLayer(
            sourceStructuralLayer,
            buffer);

        //
        // Jump to next size
        //

        int iStep;
        if ((iParam < 3) || (iParam >= 16384 - 20 && iParam < 16384 + 20) || (iParam >= 16384 * 2 - 40))
            iStep = 1;
        else
            iStep = 10;

        iParam += iStep;
    }
}

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerTests, MidSize_Heterogeneous)
{
    // Linearize materials
    std::vector<StructuralMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate structural layer
    StructuralLayerData sourceStructuralLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceStructuralLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceStructuralLayer.Buffer.Data[i].Material = materials[i % materials.size()];
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendStructuralLayer(sourceStructuralLayer, buffer);

    //
    // Verify RLE
    //

    size_t idx = sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Buffer header

    for (size_t i = 0; i < sourceStructuralLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        // Count
        var_uint16_t count;
        idx += buffer.ReadAt(idx, count);
        EXPECT_EQ(count.value(), 1);

        // Value
        MaterialColorKey colorKey;
        idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));
        EXPECT_EQ(colorKey, materials[i % materials.size()]->ColorKey);
    }

    idx += sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Tail

    // Buffer is done
    EXPECT_EQ(idx, buffer.GetSize());

    //
    // Verify may be read
    //

    VerifyDeserializedStructuralLayer(
        sourceStructuralLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerTests, Nulls)
{
    // Linearize materials
    std::vector<StructuralMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate structural layer
    StructuralLayerData sourceStructuralLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceStructuralLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceStructuralLayer.Buffer.Data[i].Material = (i % 2 == 0) ? nullptr : materials[i % materials.size()];
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendStructuralLayer(sourceStructuralLayer, buffer);

    //
    // Verify RLE
    //

    size_t idx = sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Buffer header

    for (size_t i = 0; i < sourceStructuralLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        // Count
        var_uint16_t count;
        idx += buffer.ReadAt(idx, count);
        EXPECT_EQ(count.value(), 1);

        // Value
        MaterialColorKey colorKey;
        idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));
        if (i % 2 == 0)
        {
            EXPECT_EQ(colorKey, EmptyMaterialColorKey);
        }
        else
        {
            EXPECT_EQ(colorKey, materials[i % materials.size()]->ColorKey);
        }
    }

    idx += sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Tail

    // Buffer is done
    EXPECT_EQ(idx, buffer.GetSize());

    //
    // Verify may be read
    //

    VerifyDeserializedStructuralLayer(
        sourceStructuralLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerTests, UnrecognizedMaterial)
{
    StructuralMaterial unrecognizedMaterial = StructuralMaterial(
        rgbColor(0x12, 0x34, 0x56),
        "Unrecognized Material",
        rgbaColor(0x12, 0x34, 0x56, 0xff));

    // Populate structural layer
    StructuralLayerData sourceStructuralLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceStructuralLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceStructuralLayer.Buffer.Data[i].Material = &unrecognizedMaterial;
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendStructuralLayer(sourceStructuralLayer, buffer);

    //
    // Verify exception
    //

    Version const fileFSVersion(1, 2, 3, 4);

    try
    {
        std::unique_ptr<StructuralLayerData> targetStructuralLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            fileFSVersion,
            sourceStructuralLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetStructuralLayer);

        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFound);
        EXPECT_EQ(exc.Parameters[0], fileFSVersion.ToMajorMinorPatchString());
    }
}

class ShipDefinitionFormatDeSerializer_ElectricalLayerTests : public testing::Test
{
protected:

    void SetUp()
    {
        for (std::uint8_t i = 0; i < 200; ++i)
        {
            MaterialColorKey colorKey(
                i + 2,
                i + 1,
                i);

            TestMaterialMap.try_emplace(
                colorKey,
                ElectricalMaterial(
                    colorKey,
                    "Material " + std::to_string(i),
                    colorKey,
                    i >= 100));
        }
    }

    void VerifyDeserializedElectricalLayer(
        ElectricalLayerData const & sourceElectricalLayer,
        DeSerializationBuffer<BigEndianess> & buffer)
    {
        std::unique_ptr<ElectricalLayerData> targetElectricalLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(Version(1, 2, 3, 4), sourceElectricalLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadElectricalLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetElectricalLayer);

        // Buffer
        EXPECT_EQ(targetElectricalLayer->Buffer.Size, sourceElectricalLayer.Buffer.Size);
        ASSERT_EQ(targetElectricalLayer->Buffer.GetByteSize(), sourceElectricalLayer.Buffer.GetByteSize());
        for (int y = 0; y < targetElectricalLayer->Buffer.Size.height; ++y)
        {
            for (int x = 0; x < targetElectricalLayer->Buffer.Size.width; ++x)
            {
                ShipSpaceCoordinates const coords{ x, y };
                EXPECT_EQ(sourceElectricalLayer.Buffer[coords], targetElectricalLayer->Buffer[coords]);
            }
        }

        // Panel
        ASSERT_EQ(targetElectricalLayer->Panel.GetSize(), sourceElectricalLayer.Panel.GetSize());
        for (auto const & sourceEntry : sourceElectricalLayer.Panel)
        {
            ASSERT_TRUE(targetElectricalLayer->Panel.Contains(sourceEntry.first));

            auto const & targetElement = targetElectricalLayer->Panel[sourceEntry.first];
            EXPECT_EQ(targetElement.Label, sourceEntry.second.Label);
            EXPECT_EQ(targetElement.PanelCoordinates, sourceEntry.second.PanelCoordinates);
            EXPECT_EQ(targetElement.IsHidden, sourceEntry.second.IsHidden);
        }
    }

    std::map<MaterialColorKey, ElectricalMaterial> TestMaterialMap;
};

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, MidSize_NonInstanced)
{
    // Linearize materials
    std::vector<ElectricalMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate electrical layer with non-instanced materials
    ElectricalLayerData sourceElectricalLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceElectricalLayer.Buffer.Data[i] = ElectricalElement(materials[i % 100], NoneElectricalElementInstanceIndex);
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendElectricalLayer(sourceElectricalLayer, buffer);

    //
    // Verify RLE
    //

    size_t idx = sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Buffer header

    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        // Count
        var_uint16_t count;
        idx += buffer.ReadAt(idx, count);
        EXPECT_EQ(count.value(), 1);

        // RGB key
        MaterialColorKey colorKey;
        idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));
        EXPECT_EQ(colorKey, materials[i % 100]->ColorKey);
    }

    idx += sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Tail

    // Buffer is done
    EXPECT_EQ(idx, buffer.GetSize());

    //
    // Verify may be read
    //

    VerifyDeserializedElectricalLayer(
        sourceElectricalLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, MidSize_Instanced)
{
    // Linearize materials
    std::vector<ElectricalMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate electrical layer with instanced materials
    ElectricalLayerData sourceElectricalLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceElectricalLayer.Buffer.Data[i] = ElectricalElement(materials[100 + i % 100], ElectricalElementInstanceIndex(i));
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendElectricalLayer(sourceElectricalLayer, buffer);

    //
    // Verify RLE
    //

    size_t idx = sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Buffer header

    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        // Count
        var_uint16_t count;
        idx += buffer.ReadAt(idx, count);
        EXPECT_EQ(count.value(), 1);

        // RGB key
        MaterialColorKey colorKey;
        idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));
        EXPECT_EQ(colorKey, materials[100 + i % 100]->ColorKey);

        // InstanceId
        var_uint16_t instanceId;
        idx += buffer.ReadAt(idx, instanceId);
        EXPECT_EQ(static_cast<ElectricalElementInstanceIndex>(instanceId.value()), ElectricalElementInstanceIndex(i));
    }

    idx += sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Tail

    // Buffer is done
    EXPECT_EQ(idx, buffer.GetSize());

    //
    // Verify may be read
    //

    VerifyDeserializedElectricalLayer(
        sourceElectricalLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, Nulls)
{
    // Linearize materials
    std::vector<ElectricalMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate electrical layer with non-instanced materials
    ElectricalLayerData sourceElectricalLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceElectricalLayer.Buffer.Data[i] = ElectricalElement((i % 2 == 0) ? nullptr : materials[i % 100], NoneElectricalElementInstanceIndex);
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendElectricalLayer(sourceElectricalLayer, buffer);

    //
    // Verify RLE
    //

    size_t idx = sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Buffer header

    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        // Count
        var_uint16_t count;
        idx += buffer.ReadAt(idx, count);
        EXPECT_EQ(count.value(), 1);

        // RGB key
        MaterialColorKey colorKey;
        idx += buffer.ReadAt(idx, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));
        if (i % 2 == 0)
        {
            EXPECT_EQ(colorKey, EmptyMaterialColorKey);
        }
        else
        {
            EXPECT_EQ(colorKey, materials[i % 100]->ColorKey);
        }
    }

    idx += sizeof(ShipDefinitionFormatDeSerializer::SectionHeader); // Skip Tail

    // Buffer is done
    EXPECT_EQ(idx, buffer.GetSize());

    //
    // Verify may be read
    //

    VerifyDeserializedElectricalLayer(
        sourceElectricalLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, ElectricalPanel)
{
    // Linearize materials
    std::vector<ElectricalMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate electrical layer with instanced materials
    ElectricalLayerData sourceElectricalLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceElectricalLayer.Buffer.Data[i] = ElectricalElement(materials[100 + i % 100], ElectricalElementInstanceIndex(i));
    }

    // Populate electrical panel

    ElectricalPanel::ElementMetadata elem1 = ElectricalPanel::ElementMetadata(
        std::nullopt,
        std::nullopt,
        true);

    ElectricalPanel::ElementMetadata elem2 = ElectricalPanel::ElementMetadata(
        IntegralCoordinates(3, 127),
        std::nullopt,
        false);

    ElectricalPanel::ElementMetadata elem3 = ElectricalPanel::ElementMetadata(
        std::nullopt,
        "Foo bar",
        true);

    ElectricalPanel::ElementMetadata elem4 = ElectricalPanel::ElementMetadata(
        IntegralCoordinates(13, -45),
        "Foobar 2",
        false);

    sourceElectricalLayer.Panel.Add(ElectricalElementInstanceIndex(8), elem1);
    sourceElectricalLayer.Panel.Add(ElectricalElementInstanceIndex(0), elem2);
    sourceElectricalLayer.Panel.Add(ElectricalElementInstanceIndex(18), elem3);
    sourceElectricalLayer.Panel.Add(ElectricalElementInstanceIndex(234), elem4);

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendElectricalLayer(sourceElectricalLayer, buffer);

    //
    // Verify may be read
    //

    VerifyDeserializedElectricalLayer(
        sourceElectricalLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, UnrecognizedMaterial)
{
    ElectricalMaterial unrecognizedMaterial = ElectricalMaterial(
        rgbColor(0x12, 0x34, 0x56),
        "Unrecognized Material",
        rgbColor(0x12, 0x34, 0x56),
        false);

    // Populate electrical layer
    ElectricalLayerData sourceElectricalLayer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceElectricalLayer.Buffer.Data[i] = ElectricalElement(&unrecognizedMaterial, NoneElectricalElementInstanceIndex);
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendElectricalLayer(sourceElectricalLayer, buffer);

    //
    // Verify exception
    //

    Version const fileFSVersion(1, 2, 3, 4);

    try
    {
        std::unique_ptr<ElectricalLayerData> targetElectricalLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            fileFSVersion, sourceElectricalLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadElectricalLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetElectricalLayer);

        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFound);
        EXPECT_EQ(exc.Parameters[0], fileFSVersion.ToMajorMinorPatchString());
    }
}

class ShipDefinitionFormatDeSerializer_RopesLayerTests : public testing::Test
{
protected:

    void SetUp()
    {
        for (std::uint8_t i = 0; i < 250; ++i)
        {
            MaterialColorKey colorKey(
                i + 2,
                i + 1,
                i);

            TestMaterialMap.try_emplace(
                colorKey,
                StructuralMaterial(
                    colorKey,
                    "Material " + std::to_string(i),
                    rgbaColor(colorKey, 255)));
        }
    }

    void VerifyDeserializedRopesLayer(
        RopesLayerData const & sourceRopesLayer,
        DeSerializationBuffer<BigEndianess> & buffer)
    {
        std::unique_ptr<RopesLayerData> targetRopesLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(Version(1, 2, 3, 4), sourceRopesLayer.Buffer.GetSize(), false, false);
        ShipDefinitionFormatDeSerializer::ReadRopesLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetRopesLayer);

        EXPECT_EQ(targetRopesLayer->Buffer.GetSize(), sourceRopesLayer.Buffer.GetSize());
        ASSERT_EQ(targetRopesLayer->Buffer.GetElementCount(), sourceRopesLayer.Buffer.GetElementCount());
        for (size_t i = 0; i < sourceRopesLayer.Buffer.GetElementCount(); ++i)
        {
            EXPECT_EQ(targetRopesLayer->Buffer[i], sourceRopesLayer.Buffer[i]);
        }
    }

    std::map<MaterialColorKey, StructuralMaterial> TestMaterialMap;
};

TEST_F(ShipDefinitionFormatDeSerializer_RopesLayerTests, TwoElements)
{
    // Linearize materials
    std::vector<StructuralMaterial const *> materials;
    std::transform(
        TestMaterialMap.cbegin(),
        TestMaterialMap.cend(),
        std::back_inserter(materials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate ropes layer
    RopesLayerData sourceRopesLayer({400, 200});
    sourceRopesLayer.Buffer.EmplaceBack(
        ShipSpaceCoordinates(0, 1),
        ShipSpaceCoordinates(90, 91),
        materials[0],
        rgbaColor(0x02, 0x11, 0x90, 0xfe));
    sourceRopesLayer.Buffer.EmplaceBack(
        ShipSpaceCoordinates(200, 201),
        ShipSpaceCoordinates(100010, 100011),
        materials[1],
        rgbaColor(0xff, 0xff, 0xff, 0xff));

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendRopesLayer(sourceRopesLayer, buffer);

    //
    // Verify may be read
    //

    VerifyDeserializedRopesLayer(
        sourceRopesLayer,
        buffer);
}

TEST_F(ShipDefinitionFormatDeSerializer_RopesLayerTests, UnrecognizedMaterial)
{
    StructuralMaterial unrecognizedMaterial = StructuralMaterial(
        rgbColor(0x12, 0x34, 0x56),
        "Unrecognized Material",
        rgbaColor(0x12, 0x34, 0x56, 0xff));

    // Populate ropes layer
    RopesLayerData sourceRopesLayer({400, 200});
    sourceRopesLayer.Buffer.EmplaceBack(
        ShipSpaceCoordinates(0, 1),
        ShipSpaceCoordinates(90, 91),
        &unrecognizedMaterial,
        rgbaColor(0x02, 0x11, 0x90, 0xfe));

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendRopesLayer(sourceRopesLayer, buffer);

    //
    // Verify exception
    //

    Version const fileFSVersion(1, 2, 3, 4);

    try
    {
        std::unique_ptr<RopesLayerData> targetRopesLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            fileFSVersion, sourceRopesLayer.Buffer.GetSize(), false, false);
        ShipDefinitionFormatDeSerializer::ReadRopesLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetRopesLayer);

        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFound);
        EXPECT_EQ(exc.Parameters[0], fileFSVersion.ToMajorMinorPatchString());
    }
}

TEST(ShipDefinitionFormatDeSerializer, Roundtrip)
{
    ShipSpaceSize const shipSize(4, 2);

    //
    // Structural
    //

    std::map<MaterialColorKey, StructuralMaterial> TestStructuralMaterialMap;
    for (std::uint8_t i = 0; i < 250; ++i)
    {
        MaterialColorKey colorKey(
            i + 2,
            i + 1,
            i);

        TestStructuralMaterialMap.try_emplace(
            colorKey,
            StructuralMaterial(
                colorKey,
                "Material " + std::to_string(i),
                rgbaColor(colorKey, 255)));
    }

    // Linearize
    std::vector<StructuralMaterial const *> structuralMaterials;
    std::transform(
        TestStructuralMaterialMap.cbegin(),
        TestStructuralMaterialMap.cend(),
        std::back_inserter(structuralMaterials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    StructuralLayerData sourceStructuralLayer(shipSize);
    for (size_t i = 0; i < sourceStructuralLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceStructuralLayer.Buffer.Data[i].Material = structuralMaterials[i % structuralMaterials.size()];
    }

    //
    // Electrical
    //

    std::map<MaterialColorKey, ElectricalMaterial> TestElectricalMaterialMap;
    for (std::uint8_t i = 0; i < 200; ++i)
    {
        MaterialColorKey colorKey(
            i + 2,
            i + 1,
            i);

        TestElectricalMaterialMap.try_emplace(
            colorKey,
            ElectricalMaterial(
                colorKey,
                "Material " + std::to_string(i),
                colorKey,
                i == 1));
    }

    // Linearize materials
    std::vector<ElectricalMaterial const *> electricalMaterials;
    std::transform(
        TestElectricalMaterialMap.cbegin(),
        TestElectricalMaterialMap.cend(),
        std::back_inserter(electricalMaterials),
        [](auto const & entry)
        {
            return &(entry.second);
        });

    // Populate electrical layer with non-instanced materials
    ElectricalLayerData sourceElectricalLayer(shipSize);
    for (size_t i = 0; i < sourceElectricalLayer.Buffer.Size.GetLinearSize(); ++i)
    {
        sourceElectricalLayer.Buffer.Data[i] = ElectricalElement(electricalMaterials[i % 100], i == 1 ? 22 : NoneElectricalElementInstanceIndex);
    }

    sourceElectricalLayer.Panel = ElectricalPanel();
    sourceElectricalLayer.Panel.Add(22, ElectricalPanel::ElementMetadata(std::nullopt, "FOO", true));

    //
    // Ropes
    //

    // Populate ropes layer
    RopesLayerData sourceRopesLayer(shipSize);
    sourceRopesLayer.Buffer.EmplaceBack(
        ShipSpaceCoordinates(0, 1),
        ShipSpaceCoordinates(90, 91),
        structuralMaterials[0],
        rgbaColor(0x02, 0x11, 0x90, 0xfe));
    sourceRopesLayer.Buffer.EmplaceBack(
        ShipSpaceCoordinates(200, 201),
        ShipSpaceCoordinates(100010, 100011),
        structuralMaterials[1],
        rgbaColor(0xff, 0xff, 0xff, 0xff));

    //
    // Exterior layer
    //

    RgbaImageData sourceExteriorTexture(ImageSize(4, 4));
    for (int x = 0; x < sourceExteriorTexture.Size.width; ++x)
    {
        for (int y = 0; y < sourceExteriorTexture.Size.height; ++y)
        {
            sourceExteriorTexture[{x, y}] = rgbaColor(0, 0, 0x80, 0xff);
        }
    }

    TextureLayerData sourceExteriorLayer(sourceExteriorTexture.Clone());

    //
    // Interior layer
    //

    // TODOHERE

    //
    // Serialize
    //

    ShipLayers layers(
        shipSize,
        std::make_unique<StructuralLayerData>(sourceStructuralLayer.Clone()),
        std::make_unique<ElectricalLayerData>(sourceElectricalLayer.Clone()),
        std::make_unique<RopesLayerData>(sourceRopesLayer.Clone()),
        std::make_unique<TextureLayerData>(sourceExteriorTexture.Clone()),
        nullptr); // TODO

    ShipDefinition shipDefinition = ShipDefinition(
        std::move(layers),
        ShipMetadata("TestShipName"),
        ShipPhysicsData(vec2f(242.0f, -242.0f), 2420.0f),
        ShipAutoTexturizationSettings(ShipAutoTexturizationModeType::MaterialTextures, 10.0f, 0.5f));

    MemoryBinaryWriteStream outputStream;
    ShipDefinitionFormatDeSerializer::Save(
        shipDefinition,
        Version(1, 2, 3, 4),
        outputStream);

    //
    // Deserialize whole
    //

    MaterialDatabase const materialDatabase = MaterialDatabase::Make(structuralMaterials, electricalMaterials);

    auto inputStream1 = outputStream.MakeReadStreamCopy();

    ShipDefinition const sd = ShipDefinitionFormatDeSerializer::Load(
        inputStream1,
        materialDatabase);

    // Layers

    EXPECT_EQ(sd.Layers.Size, shipSize);

    ASSERT_TRUE(sd.Layers.StructuralLayer);
    ASSERT_EQ(sd.Layers.StructuralLayer->Buffer.Size, shipSize);
    for (size_t i = 0; i < shipSize.GetLinearSize(); ++i)
    {
        EXPECT_EQ(sd.Layers.StructuralLayer->Buffer.Data[i].Material->ColorKey, structuralMaterials[i % structuralMaterials.size()]->ColorKey);
    }

    ASSERT_TRUE(sd.Layers.ElectricalLayer);
    ASSERT_EQ(sd.Layers.ElectricalLayer->Buffer.Size, shipSize);
    for (size_t i = 0; i < shipSize.GetLinearSize(); ++i)
    {
        ASSERT_NE(sd.Layers.ElectricalLayer->Buffer.Data[i].Material, nullptr);
        EXPECT_EQ(sd.Layers.ElectricalLayer->Buffer.Data[i].Material->ColorKey, electricalMaterials[i % 100]->ColorKey);
        EXPECT_EQ(sd.Layers.ElectricalLayer->Buffer.Data[i].InstanceIndex, sourceElectricalLayer.Buffer.Data[i].InstanceIndex);
    }
    ASSERT_EQ(sd.Layers.ElectricalLayer->Panel.GetSize(), 1u);
    ASSERT_TRUE(sd.Layers.ElectricalLayer->Panel.Contains(22));
    EXPECT_EQ(sd.Layers.ElectricalLayer->Panel[22].PanelCoordinates, std::nullopt);
    EXPECT_EQ(sd.Layers.ElectricalLayer->Panel[22].Label, "FOO");
    EXPECT_EQ(sd.Layers.ElectricalLayer->Panel[22].IsHidden, true);

    ASSERT_TRUE(sd.Layers.RopesLayer);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer.GetSize(), shipSize);
    ASSERT_EQ(sd.Layers.RopesLayer->Buffer.GetElementCount(), 2u);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[0].StartCoords, sourceRopesLayer.Buffer[0].StartCoords);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[0].EndCoords, sourceRopesLayer.Buffer[0].EndCoords);
    ASSERT_NE(sd.Layers.RopesLayer->Buffer[0].Material, nullptr);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[0].Material->ColorKey, structuralMaterials[0]->ColorKey);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[0].RenderColor, sourceRopesLayer.Buffer[0].RenderColor);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[1].StartCoords, sourceRopesLayer.Buffer[1].StartCoords);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[1].EndCoords, sourceRopesLayer.Buffer[1].EndCoords);
    ASSERT_NE(sd.Layers.RopesLayer->Buffer[1].Material, nullptr);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[1].Material->ColorKey, structuralMaterials[1]->ColorKey);
    EXPECT_EQ(sd.Layers.RopesLayer->Buffer[1].RenderColor, sourceRopesLayer.Buffer[1].RenderColor);

    ASSERT_TRUE(sd.Layers.ExteriorTextureLayer);
    EXPECT_EQ(sd.Layers.ExteriorTextureLayer->Buffer.Size, sourceExteriorTexture.Size);
    for (int x = 0; x < sourceExteriorTexture.Size.width; ++x)
    {
        for (int y = 0; y < sourceExteriorTexture.Size.height; ++y)
        {
            auto const c = typename RgbaImageData::coordinates_type(x, y);
            EXPECT_EQ(sd.Layers.ExteriorTextureLayer->Buffer[c], sourceExteriorTexture[c]);
        }
    }

    // TODOHERE: InteriorTextureLayer

    // Metadata

    EXPECT_EQ(sd.Metadata.ShipName, shipDefinition.Metadata.ShipName);

    // Physics Data

    EXPECT_EQ(sd.PhysicsData.Offset, shipDefinition.PhysicsData.Offset);
    EXPECT_EQ(sd.PhysicsData.InternalPressure, shipDefinition.PhysicsData.InternalPressure);

    // Auto-texturization settings

    ASSERT_TRUE(sd.AutoTexturizationSettings.has_value());
    EXPECT_EQ(sd.AutoTexturizationSettings->Mode, shipDefinition.AutoTexturizationSettings->Mode);
    EXPECT_EQ(sd.AutoTexturizationSettings->MaterialTextureMagnification, shipDefinition.AutoTexturizationSettings->MaterialTextureMagnification);
    EXPECT_EQ(sd.AutoTexturizationSettings->MaterialTextureTransparency, shipDefinition.AutoTexturizationSettings->MaterialTextureTransparency);

    //
    // Deserialize preview data
    //

    auto inputStream2 = outputStream.MakeReadStreamCopy();

    ShipPreviewData previewData = ShipDefinitionFormatDeSerializer::LoadPreviewData(inputStream2);

    EXPECT_EQ(previewData.ShipSize, shipSize);
    EXPECT_EQ(previewData.Metadata.ShipName, shipDefinition.Metadata.ShipName);
    EXPECT_EQ(previewData.IsHD, true);
    EXPECT_EQ(previewData.HasElectricals, true);

    //
    // Deserialize preview image
    //

    auto inputStream3 = outputStream.MakeReadStreamCopy();

    RgbaImageData previewImage = ShipDefinitionFormatDeSerializer::LoadPreviewImage(inputStream3, sourceExteriorTexture.Size);

    ASSERT_EQ(previewImage.Size, sourceExteriorTexture.Size);
    for (int x = 0; x < sourceExteriorTexture.Size.width; ++x)
    {
        for (int y = 0; y < sourceExteriorTexture.Size.height; ++y)
        {
            auto const c = typename RgbaImageData::coordinates_type(x, y);
            EXPECT_EQ(previewImage[c], sourceExteriorTexture[c]);
        }
    }
}
