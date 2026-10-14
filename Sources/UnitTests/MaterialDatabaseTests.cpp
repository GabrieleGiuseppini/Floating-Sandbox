#include <ShipBuilderLib/UI/MaterialDatabase.h>

#include <filesystem>

#include "gtest/gtest.h"

TEST(MaterialDatabaseTests, PrunesShip)
{
	//
	// Make default materials
	//

    std::map<MaterialColorKey, StructuralMaterial> TestDefaultMaterialColorMap;

    for (std::uint8_t i = 0; i < 250; ++i)
    {
        MaterialColorKey colorKey(
            i + 2,
            i + 1,
            i);

        StructuralMaterial defaultMaterial(
            colorKey,
            "Material " + std::to_string(i),
            rgbaColor(colorKey, 255));
        defaultMaterial.PaletteCoordinates = { "Category", "SubCategory", i };

        TestDefaultMaterialColorMap.try_emplace(
            colorKey,
            defaultMaterial);
    }

    //
    // Make DB
    //

    auto tmpDir = std::filesystem::temp_directory_path() / "_FSUTS";
    std::filesystem::remove_all(tmpDir);
    std::filesystem::create_directories(tmpDir);

    ShipBuilder::MaterialDatabase<LayerType::Structural> structuralMaterialDatabase(TestDefaultMaterialColorMap, tmpDir);

    //
    // Add two user custom materials
    //

    StructuralMaterial::VariantOverridesType commonOverrides1(
        "CommonOverrides1",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f);

    StructuralMaterial::VariantOverridesType commonOverrides2(
        "CommonOverrides2",
        rgbaColor(10, 20, 30, 40),
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f);

    structuralMaterialDatabase.AddUserCustomMaterial((*TestDefaultMaterialColorMap.begin()).second.MakeCustomMaterial(commonOverrides1));
    structuralMaterialDatabase.AddUserCustomMaterial((*std::next(TestDefaultMaterialColorMap.begin())).second.MakeCustomMaterial(commonOverrides2));

    //
    // Make ship custom materials
    //

    std::vector<std::unique_ptr<StructuralMaterial const>> shipCustomMaterials;
    for (size_t i = 0; i < 4; ++i)
    {
        StructuralMaterial::VariantOverridesType uniqueOverrides(
            "UniqueOverrides",
            rgbaColor(10, 20, 30, 40),
            1.0f,
            2.0f,
            3.0f,
            4.0f,
            5.0f,
            6.0f,
            7.0f + static_cast<float>(i));

        shipCustomMaterials.emplace_back((*TestDefaultMaterialColorMap.begin()).second.MakeCustomMaterial(uniqueOverrides));
    }

    shipCustomMaterials.emplace_back((*TestDefaultMaterialColorMap.begin()).second.MakeCustomMaterial(commonOverrides1));
    shipCustomMaterials.emplace_back((*std::next(TestDefaultMaterialColorMap.begin())).second.MakeCustomMaterial(commonOverrides2));

    // Save pointers for later
    std::vector<StructuralMaterial const *> shipCustomMaterialsPtr;
    for (auto const & uq_ptr : shipCustomMaterials)
    {
        shipCustomMaterialsPtr.emplace_back(uq_ptr.get());
    }

    //
    // Let the DB work
    //

    auto const replacementMap = structuralMaterialDatabase.SetShipCustomMaterials(std::move(shipCustomMaterials));

    // Verify replacement map

    ASSERT_EQ(replacementMap.size(), 2u);
    for (auto const & entry : replacementMap)
    {
        if (entry.first == shipCustomMaterials[4].get())
        {
            EXPECT_EQ(entry.second, structuralMaterialDatabase.GetUserCustomMaterials()[0].get());
        }
        else
        {
            EXPECT_EQ(entry.first, shipCustomMaterials[5].get());
            EXPECT_EQ(entry.second, structuralMaterialDatabase.GetUserCustomMaterials()[1].get());
        }
    }

    // Verify stored materials

    ASSERT_EQ(structuralMaterialDatabase.GetShipCustomMaterials().size(), 4u);
    for (size_t i = 0; i < structuralMaterialDatabase.GetShipCustomMaterials().size(); ++i)
    {
        EXPECT_EQ(structuralMaterialDatabase.GetShipCustomMaterials()[i].get(), shipCustomMaterialsPtr[i]);
    }

    std::filesystem::remove_all(tmpDir);
}
