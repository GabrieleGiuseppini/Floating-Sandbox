#include <Game/ShipDefinitionFormatDeSerializer.h>

#include "gtest/gtest.h"

#include <algorithm>
#include <map>
#include <vector>

TEST(ShipDefinitionFormatDeSerializerTests, Metadata_Full_WithoutElectricalPanel)
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

    ShipMetadata targetMd("Foo");
    ShipDefinitionFormatDeSerializer::ReadMetadata(buffer, targetMd);

    EXPECT_EQ(sourceMd.ArtCredits, targetMd.ArtCredits);
    EXPECT_EQ(sourceMd.Author, targetMd.Author);
    EXPECT_EQ(sourceMd.Description, targetMd.Description);
    EXPECT_EQ(sourceMd.Password, targetMd.Password);
    EXPECT_EQ(sourceMd.ShipName, targetMd.ShipName);
    EXPECT_EQ(sourceMd.YearBuilt, targetMd.YearBuilt);
    EXPECT_EQ(sourceMd.DoHideElectricalsInPreview, targetMd.DoHideElectricalsInPreview);
    EXPECT_EQ(sourceMd.DoHideHDInPreview, targetMd.DoHideHDInPreview);
}

TEST(ShipDefinitionFormatDeSerializerTests, Metadata_Minimal_WithoutElectricalPanel)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipMetadata sourceMd("Test ship");
    ShipDefinitionFormatDeSerializer::AppendMetadata(sourceMd, buffer);

    // Read

    ShipMetadata targetMd("Foo");
    ShipDefinitionFormatDeSerializer::ReadMetadata(buffer, targetMd);

    EXPECT_FALSE(sourceMd.ArtCredits.has_value());
    EXPECT_FALSE(sourceMd.Author.has_value());
    EXPECT_FALSE(sourceMd.Description.has_value());
    EXPECT_FALSE(sourceMd.Password.has_value());
    EXPECT_EQ(sourceMd.ShipName, targetMd.ShipName);
    EXPECT_FALSE(sourceMd.YearBuilt.has_value());
    EXPECT_FALSE(sourceMd.DoHideElectricalsInPreview);
    EXPECT_FALSE(sourceMd.DoHideHDInPreview);
}

TEST(ShipDefinitionFormatDeSerializerTests, Metadata_ElectricalPanel)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

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

    ShipMetadata sourceMd("Test ship");
    sourceMd.ElectricalPanelMetadata.emplace(ElectricalElementInstanceIndex(8), elem1);
    sourceMd.ElectricalPanelMetadata.emplace(ElectricalElementInstanceIndex(0), elem2);
    sourceMd.ElectricalPanelMetadata.emplace(ElectricalElementInstanceIndex(18), elem3);
    sourceMd.ElectricalPanelMetadata.emplace(ElectricalElementInstanceIndex(234), elem4);
    ShipDefinitionFormatDeSerializer::AppendMetadata(sourceMd, buffer);

    // Read

    ShipMetadata targetMd("Foo");
    ShipDefinitionFormatDeSerializer::ReadMetadata(buffer, targetMd);

    ASSERT_EQ(4, targetMd.ElectricalPanelMetadata.size());

    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.find(8) != targetMd.ElectricalPanelMetadata.end());
    EXPECT_FALSE(targetMd.ElectricalPanelMetadata.at(8).PanelCoordinates.has_value());
    EXPECT_FALSE(targetMd.ElectricalPanelMetadata.at(8).Label.has_value());
    EXPECT_TRUE(targetMd.ElectricalPanelMetadata.at(8).IsHidden);

    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.find(0) != targetMd.ElectricalPanelMetadata.end());
    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.at(0).PanelCoordinates.has_value());
    EXPECT_EQ(*targetMd.ElectricalPanelMetadata.at(0).PanelCoordinates, IntegralCoordinates(3, 127));
    EXPECT_FALSE(targetMd.ElectricalPanelMetadata.at(0).Label.has_value());
    EXPECT_FALSE(targetMd.ElectricalPanelMetadata.at(0).IsHidden);

    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.find(18) != targetMd.ElectricalPanelMetadata.end());
    EXPECT_FALSE(targetMd.ElectricalPanelMetadata.at(18).PanelCoordinates.has_value());
    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.at(18).Label.has_value());
    EXPECT_EQ(*targetMd.ElectricalPanelMetadata.at(18).Label, "Foo bar");
    EXPECT_TRUE(targetMd.ElectricalPanelMetadata.at(18).IsHidden);

    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.find(234) != targetMd.ElectricalPanelMetadata.end());
    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.at(234).PanelCoordinates.has_value());
    EXPECT_EQ(*targetMd.ElectricalPanelMetadata.at(234).PanelCoordinates, IntegralCoordinates(13, -45));
    ASSERT_TRUE(targetMd.ElectricalPanelMetadata.at(234).Label.has_value());
    EXPECT_EQ(*targetMd.ElectricalPanelMetadata.at(234).Label, "Foobar 2");
    EXPECT_FALSE(targetMd.ElectricalPanelMetadata.at(234).IsHidden);
}

class ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests : public testing::Test
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
        StructuralLayerBuffer const & sourceStructuralLayerBuffer,
        DeSerializationBuffer<BigEndianess> & buffer)
    {
        std::unique_ptr<StructuralLayerBuffer> targetStructuralLayerBuffer;
        ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
            buffer,
            TestMaterialMap,
            targetStructuralLayerBuffer);

        EXPECT_EQ(targetStructuralLayerBuffer->Size, sourceStructuralLayerBuffer.Size);
        ASSERT_EQ(targetStructuralLayerBuffer->GetByteSize(), sourceStructuralLayerBuffer.GetByteSize());
        EXPECT_EQ(
            std::memcmp(
                targetStructuralLayerBuffer->Data.get(),
                sourceStructuralLayerBuffer.Data.get(),
                targetStructuralLayerBuffer->GetByteSize()),
            0);
    }

    std::map<MaterialColorKey, StructuralMaterial> TestMaterialMap;
};

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests, VariousSizes_Uniform)
{
    for (int iParam = 1; iParam < 16384 * 2 + 1; ++iParam)
    {
        StructuralLayerBuffer sourceStructuralLayerBuffer(
            ShipSpaceSize(iParam, 1),
            StructuralElement(nullptr)); // Empty

        ASSERT_EQ(sourceStructuralLayerBuffer.Size.GetLinearSize(), iParam);

        DeSerializationBuffer<BigEndianess> buffer(256);
        ShipDefinitionFormatDeSerializer::AppendStructuralLayer(sourceStructuralLayerBuffer, buffer);

        // Verify ship size
        std::uint32_t width, height;
        size_t idx = buffer.ReadAt(0, width);
        idx += buffer.ReadAt(idx, height);
        ASSERT_EQ(width, uint32_t(iParam));
        ASSERT_EQ(height, uint32_t(1));

        //
        // Verify RLE:
        //  16383 times: EmptyMaterialKey
        //  16383 times: EmptyMaterialKey
        //  ...
        //      n times: EmptyMaterialKey
        //

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

        // Buffer is done
        EXPECT_EQ(idx, buffer.GetSize());

        //
        // Verify may be read
        //

        VerifyDeserializedStructuralLayer(
            sourceStructuralLayerBuffer,
            buffer);
    }
}

TEST_F(ShipDefinitionFormatDeSerializer_StructuralLayerBufferTests, MidSize_Heterogeneous)
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

    // Populate structural layer buffer
    StructuralLayerBuffer sourceStructuralLayerBuffer(ShipSpaceSize(10, 12));
    for (size_t i = 0; i < sourceStructuralLayerBuffer.Size.GetLinearSize(); ++i)
    {
        sourceStructuralLayerBuffer.Data[i].Material = materials[i % materials.size()];
    }

    // Serialize
    DeSerializationBuffer<BigEndianess> buffer(256);
    ShipDefinitionFormatDeSerializer::AppendStructuralLayer(sourceStructuralLayerBuffer, buffer);

    // Verify size
    std::uint32_t width, height;
    size_t idx = buffer.ReadAt(0, width);
    idx += buffer.ReadAt(idx, height);
    ASSERT_EQ(width, uint32_t(10));
    ASSERT_EQ(height, uint32_t(12));

    //
    // Verify RLE
    //

    for (size_t i = 0; i < sourceStructuralLayerBuffer.Size.GetLinearSize(); ++i)
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

    // Buffer is done
    EXPECT_EQ(idx, buffer.GetSize());

    //
    // Verify may be read
    //

    VerifyDeserializedStructuralLayer(
        sourceStructuralLayerBuffer,
        buffer);
}

// TODOHERE: all sizes, just write+read