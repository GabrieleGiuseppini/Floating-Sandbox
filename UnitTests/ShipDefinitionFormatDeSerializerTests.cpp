#include <Game/ShipDefinitionFormatDeSerializer.h>

#include "gtest/gtest.h"

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