#include <Game/ShipDefinitionFormatDeSerializer.h>

#include <GameCore/UserGameException.h>

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
        200,
        14,
        ShipSpaceSize(242, 409),
        true,
        false);

    ShipDefinitionFormatDeSerializer::AppendShipAttributes(sourceShipAttributes, buffer);

    // Read

    ShipDefinitionFormatDeSerializer::ShipAttributes const targetShipAttributes = ShipDefinitionFormatDeSerializer::ReadShipAttributes(buffer);

    EXPECT_EQ(sourceShipAttributes.FileFSVersionMaj, targetShipAttributes.FileFSVersionMaj);
    EXPECT_EQ(sourceShipAttributes.FileFSVersionMin, targetShipAttributes.FileFSVersionMin);
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
    sourceMd.Password = 0x1122334455667788;
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

    EXPECT_EQ(targetPd.Offset, targetPd.Offset);
    EXPECT_EQ(targetPd.InternalPressure, targetPd.InternalPressure);
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
                    colorKey));
        }
    }

    void VerifyDeserializedStructuralLayer(
        StructuralLayerData const & sourceStructuralLayer,
        DeSerializationBuffer<BigEndianess> & buffer)
    {
        std::unique_ptr<StructuralLayerData> targetStructuralLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(1, 16, sourceStructuralLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetStructuralLayer);

        EXPECT_EQ(targetStructuralLayer->Buffer.Size, sourceStructuralLayer.Buffer.Size);
        ASSERT_EQ(targetStructuralLayer->Buffer.GetByteSize(), sourceStructuralLayer.Buffer.GetByteSize());
        EXPECT_EQ(
            std::memcmp(
                targetStructuralLayer->Buffer.Data.get(),
                sourceStructuralLayer.Buffer.Data.get(),
                targetStructuralLayer->Buffer.GetByteSize()),
            0);
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

        ASSERT_EQ(sourceStructuralLayer.Buffer.Size.GetLinearSize(), iParam);

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

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerTests, UnrecognizedMaterial_SameVersion)
{
    StructuralMaterial unrecognizedMaterial = StructuralMaterial(
        rgbColor(0x12, 0x34, 0x56),
        "Unrecognized Material",
        rgbColor(0x12, 0x34, 0x56));

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

    try
    {
        std::unique_ptr<StructuralLayerData> targetStructuralLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            Version::CurrentVersion().GetMajor(),
            Version::CurrentVersion().GetMinor(),
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
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFoundSameVersion);
        EXPECT_TRUE(exc.Parameters.empty());
    }
}

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerTests, UnrecognizedMaterial_LaterVersion)
{
    StructuralMaterial unrecognizedMaterial = StructuralMaterial(
        rgbColor(0x12, 0x34, 0x56),
        "Unrecognized Material",
        rgbColor(0x12, 0x34, 0x56));

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

    try
    {
        std::unique_ptr<StructuralLayerData> targetStructuralLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            2999,
            4,
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
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFoundLaterVersion);
        ASSERT_FALSE(exc.Parameters.empty());
        EXPECT_EQ(exc.Parameters[0], "2999.4");
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
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(1, 16, sourceElectricalLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadElectricalLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetElectricalLayer);

        // Buffer
        EXPECT_EQ(targetElectricalLayer->Buffer.Size, sourceElectricalLayer.Buffer.Size);
        ASSERT_EQ(targetElectricalLayer->Buffer.GetByteSize(), sourceElectricalLayer.Buffer.GetByteSize());
        EXPECT_EQ(
            std::memcmp(
                targetElectricalLayer->Buffer.Data.get(),
                sourceElectricalLayer.Buffer.Data.get(),
                targetElectricalLayer->Buffer.GetByteSize()),
            0);

        // Panel
        ASSERT_EQ(targetElectricalLayer->Panel.size(), sourceElectricalLayer.Panel.size());
        for (auto const & sourceEntry : sourceElectricalLayer.Panel)
        {
            ASSERT_TRUE(targetElectricalLayer->Panel.count(sourceEntry.first) == 1);

            auto const & targetElement = targetElectricalLayer->Panel.at(sourceEntry.first);
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

    ElectricalPanelElementMetadata elem1 = ElectricalPanelElementMetadata(
        std::nullopt,
        std::nullopt,
        true);

    ElectricalPanelElementMetadata elem2 = ElectricalPanelElementMetadata(
        IntegralCoordinates(3, 127),
        std::nullopt,
        false);

    ElectricalPanelElementMetadata elem3 = ElectricalPanelElementMetadata(
        std::nullopt,
        "Foo bar",
        true);

    ElectricalPanelElementMetadata elem4 = ElectricalPanelElementMetadata(
        IntegralCoordinates(13, -45),
        "Foobar 2",
        false);

    sourceElectricalLayer.Panel.emplace(ElectricalElementInstanceIndex(8), elem1);
    sourceElectricalLayer.Panel.emplace(ElectricalElementInstanceIndex(0), elem2);
    sourceElectricalLayer.Panel.emplace(ElectricalElementInstanceIndex(18), elem3);
    sourceElectricalLayer.Panel.emplace(ElectricalElementInstanceIndex(234), elem4);

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

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, UnrecognizedMaterial_SameVersion)
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

    try
    {
        std::unique_ptr<ElectricalLayerData> targetElectricalLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            Version::CurrentVersion().GetMajor(),
            Version::CurrentVersion().GetMinor(),
            sourceElectricalLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadElectricalLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetElectricalLayer);

        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFoundSameVersion);
        EXPECT_TRUE(exc.Parameters.empty());
    }
}

TEST_F(ShipDefinitionFormatDeSerializer_ElectricalLayerTests, UnrecognizedMaterial_LaterVersion)
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

    try
    {
        std::unique_ptr<ElectricalLayerData> targetElectricalLayer;
        ShipDefinitionFormatDeSerializer::ShipAttributes shipAttributes(
            2999,
            4,
            sourceElectricalLayer.Buffer.Size, false, false);
        ShipDefinitionFormatDeSerializer::ReadElectricalLayer(
            buffer,
            shipAttributes,
            TestMaterialMap,
            targetElectricalLayer);

        FAIL();
    }
    catch (UserGameException const & exc)
    {
        EXPECT_EQ(exc.MessageId, UserGameException::MessageIdType::LoadShipMaterialNotFoundLaterVersion);
        ASSERT_FALSE(exc.Parameters.empty());
        EXPECT_EQ(exc.Parameters[0], "2999.4");
    }
}