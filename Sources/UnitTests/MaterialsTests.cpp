#include <Simulation/Materials.h>

#include <unordered_set>

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

    ASSERT_TRUE(c1 == c2);
    c2.Name = "test2";
    ASSERT_FALSE(c1 == c2);
    c2.Name = c1.Name;

    ASSERT_TRUE(c1 == c2);
    c2.RenderColor = rgbaColor(10, 20, 30, 41);
    ASSERT_FALSE(c1 == c2);
    c2.RenderColor = c1.RenderColor;

    ASSERT_TRUE(c1 == c2);
    c2.Strength = 42.0f;
    ASSERT_FALSE(c1 == c2);
    c2.Strength = c1.Strength;

    ASSERT_TRUE(c1 == c2);
    c2.Density = 42.0f;
    ASSERT_FALSE(c1 == c2);
    c2.Density = c1.Density;

    ASSERT_TRUE(c1 == c2);
    c2.IgnitionTemperature = 42.0f;
    ASSERT_FALSE(c1 == c2);
    c2.IgnitionTemperature = c1.IgnitionTemperature;

    ASSERT_TRUE(c1 == c2);
    c2.MeltingTemperature = 42.0f;
    ASSERT_FALSE(c1 == c2);
    c2.MeltingTemperature = c1.MeltingTemperature;

    ASSERT_TRUE(c1 == c2);
    c2.RotReceptivity = 42.0f;
    ASSERT_FALSE(c1 == c2);
    c2.RotReceptivity = c1.RotReceptivity;

    ASSERT_TRUE(c1 == c2);
    c2.RustReceptivity = 42.0f;
    ASSERT_FALSE(c1 == c2);
    c2.RustReceptivity = c1.RustReceptivity;

    ASSERT_TRUE(c1 == c2);
    c2.WaterSolubility = 42.0f;
    ASSERT_FALSE(c1 == c2);
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
        0.0f);

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

    ASSERT_FALSE(c1 < c1);
    ASSERT_TRUE(c1 < c2);
    ASSERT_FALSE(c2 < c1);
    ASSERT_FALSE(c1 < c3);
    ASSERT_FALSE(c3 < c1);
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
    ASSERT_EQ(s.size(), 1);

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
    ASSERT_EQ(s.size(), 2);

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

    ASSERT_FALSE(c1 < c1);
    ASSERT_TRUE(c1 < c2);
    ASSERT_FALSE(c2 < c1);
    ASSERT_FALSE(c1 < c3);
    ASSERT_FALSE(c3 < c1);

    s.insert(c3);
    ASSERT_EQ(s.size(), 2);
}
