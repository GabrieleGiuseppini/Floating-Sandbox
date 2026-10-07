#include <Simulation/Materials.h>

#include <Core/Utils.h>

#include <unordered_set>

#include "TestingUtils.h"

#include "gtest/gtest.h"

TEST(MaterialTests, Structural_VariantOverrides_Equality)
{
    auto const c1 = StructuralMaterial::VariantOverridesType(
        "test1",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.0f);

    auto c2 = c1;

    EXPECT_TRUE(c1 == c2);
    c2.Name = "test2";
    EXPECT_FALSE(c1 == c2);
    c2.Name = c1.Name;

    EXPECT_TRUE(c1 == c2);
    c2.RenderColor = rgbaColor(10, 20, 30, 41);
    EXPECT_FALSE(c1 == c2);
    c2.RenderColor = c1.RenderColor;

    EXPECT_TRUE(c1 == c2);
    c2.Strength = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.Strength = c1.Strength;

    EXPECT_TRUE(c1 == c2);
    c2.Density = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.Density = c1.Density;

    EXPECT_TRUE(c1 == c2);
    c2.IgnitionTemperature = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.IgnitionTemperature = c1.IgnitionTemperature;

    EXPECT_TRUE(c1 == c2);
    c2.MeltingTemperature = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.MeltingTemperature = c1.MeltingTemperature;

    EXPECT_TRUE(c1 == c2);
    c2.RotReceptivity = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.RotReceptivity = c1.RotReceptivity;

    EXPECT_TRUE(c1 == c2);
    c2.RustReceptivity = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.RustReceptivity = c1.RustReceptivity;

    EXPECT_TRUE(c1 == c2);
    c2.WaterSolubility = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.WaterSolubility = c1.WaterSolubility;
}

TEST(MaterialTests, Structural_VariantOverrides_LessThan)
{
    auto const c1 = StructuralMaterial::VariantOverridesType(
        "test1",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.2f);

    auto c2 = c1;

    EXPECT_FALSE(c1 < c2);
    EXPECT_FALSE(c2 < c1);

    EXPECT_TRUE(c1 == c2);

    c2.Name = "test";
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Name = c1.Name;

    EXPECT_TRUE(c1 == c2);

    c2.RenderColor = rgbaColor(10, 20, 30, 1);
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.RenderColor = c1.RenderColor;

    EXPECT_TRUE(c1 == c2);

    c2.Strength = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Strength = c1.Strength;

    EXPECT_TRUE(c1 == c2);

    c2.Density = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Density = c1.Density;

    EXPECT_TRUE(c1 == c2);

    c2.IgnitionTemperature = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.IgnitionTemperature = c1.IgnitionTemperature;

    EXPECT_TRUE(c1 == c2);

    c2.MeltingTemperature = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.MeltingTemperature = c1.MeltingTemperature;

    EXPECT_TRUE(c1 == c2);

    c2.RotReceptivity = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.RotReceptivity = c1.RotReceptivity;

    EXPECT_TRUE(c1 == c2);

    c2.RustReceptivity = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.RustReceptivity = c1.RustReceptivity;

    EXPECT_TRUE(c1 == c2);

    c2.WaterSolubility = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.WaterSolubility = c1.WaterSolubility;

    EXPECT_TRUE(c1 == c2);
}

TEST(MaterialTests, Structural_VariantOverrides_Hash)
{
    std::unordered_set<StructuralMaterial::VariantOverridesType> s;

    auto const c1 = StructuralMaterial::VariantOverridesType(
        "test1",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.0f);

    s.insert(c1);
    EXPECT_EQ(s.size(), 1);

    auto const c2 = StructuralMaterial::VariantOverridesType(
        "test1",
        rgbaColor(10, 20, 30, 41),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.0f);

    s.insert(c2);
    EXPECT_EQ(s.size(), 2);

    auto const c3 = StructuralMaterial::VariantOverridesType(
        "test1",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.0f);

    EXPECT_FALSE(c1 < c1);
    EXPECT_TRUE(c1 < c2);
    EXPECT_FALSE(c2 < c1);
    EXPECT_FALSE(c1 < c3);
    EXPECT_FALSE(c3 < c1);

    s.insert(c3);
    EXPECT_EQ(s.size(), 2);
}

TEST(MaterialTests, Structural_VariantOverrides_SerializeDeserialize)
{
    auto const overrides = StructuralMaterial::VariantOverridesType(
        "test1-variant",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.2f);

    auto const jsonValue = overrides.Serialize();

    EXPECT_TRUE(jsonValue.is<picojson::object>());
    auto const jsonObject = Utils::GetJsonValueAsObject(jsonValue, "test");

    auto const overrides2 = StructuralMaterial::VariantOverridesType::Deserialize(jsonObject);

    EXPECT_EQ(overrides, overrides2);
}

TEST(MaterialTests, Structural_MakeCustomMaterial)
{
    auto baseMaterial = MakeTestStructuralMaterial("test1", rgbColor(1, 2, 3));
    baseMaterial.PaletteCoordinates = MaterialPaletteCoordinatesType({ "Category1", "SubCategory1", 12u });
    baseMaterial.PaletteSubCategoryBaseMaterialOrdinal = 5;

    auto const overrides = StructuralMaterial::VariantOverridesType(
        "test1-variant",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        100.0f,
        250.0f,
        300.0f,
        1.0,
        0.9f,
        0.2f);

    auto customMaterial = baseMaterial.MakeCustomMaterial(overrides);

    EXPECT_EQ(customMaterial->Name, overrides.Name);
    EXPECT_EQ(customMaterial->RenderColor, overrides.RenderColor);
    EXPECT_EQ(customMaterial->Strength, overrides.Strength);
    EXPECT_EQ(customMaterial->Density, overrides.Density);
    EXPECT_EQ(customMaterial->IgnitionTemperature, overrides.IgnitionTemperature);
    EXPECT_EQ(customMaterial->MeltingTemperature, overrides.MeltingTemperature);
    EXPECT_EQ(customMaterial->RotReceptivity, overrides.RotReceptivity);
    EXPECT_EQ(customMaterial->RustReceptivity, overrides.RustReceptivity);
    EXPECT_EQ(customMaterial->WaterSolubility, overrides.WaterSolubility);

    ASSERT_TRUE(customMaterial->PaletteCoordinates.has_value());
    EXPECT_EQ(customMaterial->PaletteCoordinates->SubCategoryOrdinal, 0u);
    EXPECT_EQ(customMaterial->PaletteSubCategoryBaseMaterialOrdinal, 5u);
}

//////////////////////////////////////////////////////

TEST(MaterialTests, Electrical_VariantOverrides_Equality)
{
    auto const c1 = ElectricalMaterial::VariantOverridesType(
        "test1",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.75f);

    auto c2 = c1;

    EXPECT_TRUE(c1 == c2);
    c2.Name = "test2";
    EXPECT_FALSE(c1 == c2);
    c2.Name = c1.Name;

    EXPECT_TRUE(c1 == c2);
    c2.HeatGenerated = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.HeatGenerated = c1.HeatGenerated;

    EXPECT_TRUE(c1 == c2);
    c2.Luminiscence = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.Luminiscence = c1.Luminiscence;

    EXPECT_TRUE(c1 == c2);
    c2.LightSpread = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.LightSpread = c1.LightSpread;

    EXPECT_TRUE(c1 == c2);
    c2.EnginePower = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.EnginePower = c1.EnginePower;

    EXPECT_TRUE(c1 == c2);
    c2.WaterPumpNominalForce = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.WaterPumpNominalForce = c1.WaterPumpNominalForce;

    EXPECT_TRUE(c1 == c2);
    c2.TimerDurationSeconds = 42.0f;
    EXPECT_FALSE(c1 == c2);
    c2.TimerDurationSeconds = c1.TimerDurationSeconds;
}

TEST(MaterialTests, Electrical_VariantOverrides_LessThan)
{
    auto const c1 = ElectricalMaterial::VariantOverridesType(
        "test1",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.75f);

    auto c2 = c1;

    EXPECT_FALSE(c1 < c2);
    EXPECT_FALSE(c2 < c1);

    EXPECT_TRUE(c1 == c2);

    c2.Name = "test";
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Name = c1.Name;

    EXPECT_TRUE(c1 == c2);

    c2.HeatGenerated = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.HeatGenerated = c1.HeatGenerated;

    EXPECT_TRUE(c1 == c2);

    c2.Luminiscence = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Luminiscence = c1.Luminiscence;

    EXPECT_TRUE(c1 == c2);

    c2.LightSpread = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.LightSpread = c1.LightSpread;

    EXPECT_TRUE(c1 == c2);

    c2.EnginePower = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.EnginePower = c1.EnginePower;

    EXPECT_TRUE(c1 == c2);

    c2.WaterPumpNominalForce = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.WaterPumpNominalForce = c1.WaterPumpNominalForce;

    EXPECT_TRUE(c1 == c2);

    c2.TimerDurationSeconds = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.TimerDurationSeconds = c1.TimerDurationSeconds;

    EXPECT_TRUE(c1 == c2);
}

TEST(MaterialTests, Electrical_VariantOverrides_Hash)
{
    std::unordered_set<ElectricalMaterial::VariantOverridesType> s;

    auto const c1 = ElectricalMaterial::VariantOverridesType(
        "test1",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.75f);

    s.insert(c1);
    EXPECT_EQ(s.size(), 1);

    auto const c2 = ElectricalMaterial::VariantOverridesType(
        "test1",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.76f);

    s.insert(c2);
    EXPECT_EQ(s.size(), 2);

    auto const c3 = ElectricalMaterial::VariantOverridesType(
        "test1",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.75f);

    EXPECT_FALSE(c1 < c1);
    EXPECT_TRUE(c1 < c2);
    EXPECT_FALSE(c2 < c1);
    EXPECT_FALSE(c1 < c3);
    EXPECT_FALSE(c3 < c1);

    s.insert(c3);
    EXPECT_EQ(s.size(), 2);
}

TEST(MaterialTests, Electrical_VariantOverrides_SerializeDeserialize)
{
    auto const overrides = ElectricalMaterial::VariantOverridesType(
        "test1-variant",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.75f);

    auto const jsonValue = overrides.Serialize();

    EXPECT_TRUE(jsonValue.is<picojson::object>());
    auto const jsonObject = Utils::GetJsonValueAsObject(jsonValue, "test");

    auto const overrides2 = ElectricalMaterial::VariantOverridesType::Deserialize(jsonObject);

    EXPECT_EQ(overrides, overrides2);
}

TEST(MaterialTests, Electrical_MakeCustomMaterial)
{
    auto baseMaterial = MakeTestElectricalMaterial("test1", rgbColor(1, 2, 3));
    baseMaterial.PaletteCoordinates = MaterialPaletteCoordinatesType({ "Category1", "SubCategory1", 12u });
    baseMaterial.PaletteSubCategoryBaseMaterialOrdinal = 5;

    auto const overrides = ElectricalMaterial::VariantOverridesType(
        "test1-variant",
        100.0f,
        0.9f,
        0.8f,
        300.0f,
        1.5,
        0.75f);

    auto customMaterial = baseMaterial.MakeCustomMaterial(overrides);

    EXPECT_EQ(customMaterial->Name, overrides.Name);
    EXPECT_EQ(customMaterial->HeatGenerated, overrides.HeatGenerated);
    EXPECT_EQ(customMaterial->Luminiscence, overrides.Luminiscence);
    EXPECT_EQ(customMaterial->LightSpread, overrides.LightSpread);
    EXPECT_EQ(customMaterial->EnginePower, overrides.EnginePower);
    EXPECT_EQ(customMaterial->WaterPumpNominalForce, overrides.WaterPumpNominalForce);
    EXPECT_EQ(customMaterial->TimerDurationSeconds, overrides.TimerDurationSeconds);
    ASSERT_TRUE(customMaterial->PaletteCoordinates.has_value());
    EXPECT_EQ(customMaterial->PaletteCoordinates->SubCategoryOrdinal, 0u);
    EXPECT_EQ(customMaterial->PaletteSubCategoryBaseMaterialOrdinal, 5u);
}
