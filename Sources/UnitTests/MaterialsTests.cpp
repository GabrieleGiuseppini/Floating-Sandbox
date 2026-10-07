#include <Simulation/Materials.h>

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

    c2.Name = "test";
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Name = c1.Name;

    c2.RenderColor = rgbaColor(10, 20, 30, 1);
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.RenderColor = c1.RenderColor;

    c2.Strength = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Strength = c1.Strength;

    c2.Density = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.Density = c1.Density;

    c2.IgnitionTemperature = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.IgnitionTemperature = c1.IgnitionTemperature;

    c2.MeltingTemperature = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.MeltingTemperature = c1.MeltingTemperature;

    c2.RotReceptivity = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.RotReceptivity = c1.RotReceptivity;

    c2.RustReceptivity = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.RustReceptivity = c1.RustReceptivity;

    c2.WaterSolubility = 0.1f;
    EXPECT_TRUE(c2 < c1);
    EXPECT_FALSE(c1 < c2);
    c2.WaterSolubility = c1.WaterSolubility;
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

TEST(MaterialTests, Structural_MakeCustomMaterial)
{
    auto const baseMaterial = MakeTestStructuralMaterial("test1", rgbColor(1, 2, 3));

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
}
