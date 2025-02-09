#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/MaterialDatabase.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/ImageData.h>
#include <Core/GameTypes.h>

#include <benchmark/benchmark.h>

#include <filesystem>

static constexpr ShipSpaceSize StructureSize = ShipSpaceSize(800, 400);
static constexpr size_t Repetitions = 10;

//
// Original perf @ 800x400, 10 repetitions:
// 3,470,411,200 ns 3,468,750,000 ns
//
static void AutoTexturization_AutoTexturizeInto(benchmark::State& state)
{
    GameAssetManager const gameAssetManager = GameAssetManager((std::filesystem::current_path() / "Data").string());
    MaterialDatabase const materialDatabase = MaterialDatabase::Load(gameAssetManager);
    ShipTexturizer texturizer(materialDatabase, gameAssetManager);

    // Create structural layer
    StructuralLayerData structuralLayer(StructureSize);
    auto const & materialCategories = materialDatabase.GetStructuralMaterialPalette().Categories;
    size_t currentCategory = 0;
    size_t currentSubCategory = 0;
    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            StructuralMaterial const * material = &materialCategories[currentCategory].SubCategories[currentSubCategory].Materials[0].get();
            structuralLayer.Buffer[{x, y}].Material = material;

            // Move to next sub-category
            ++currentSubCategory;
            if (currentSubCategory >= materialCategories[currentCategory].SubCategories.size())
            {
                currentSubCategory = 0;
                ++currentCategory;
                if (currentCategory >= materialCategories.size())
                {
                    currentCategory = 0;
                }
            }
        }
    }

    // Create target texture
    int const magnificationFactor = ShipTexturizer::CalculateHighDefinitionTextureMagnificationFactor(StructureSize, 4096);
    ImageSize const textureSize = ImageSize(
        StructureSize.width * magnificationFactor,
        StructureSize.height * magnificationFactor);
    RgbaImageData targetTextureImage = RgbaImageData(textureSize);

    // Create settings
    ShipAutoTexturizationSettings settings;
    settings.Mode = ShipAutoTexturizationModeType::MaterialTextures;

    // Test
    for (auto _ : state)
    {
        for (size_t i = 0; i < Repetitions; ++i)
        {
            texturizer.AutoTexturizeInto(
                structuralLayer,
                ShipSpaceRect({ 0, 0 }, StructureSize),
                targetTextureImage,
                magnificationFactor,
                settings,
                gameAssetManager);
        }
    }
}
BENCHMARK(AutoTexturization_AutoTexturizeInto);

//
// Original perf @ 800x400, 40 repetitions:
// 3,341,784,000 ns 3,343,750,000 ns
//
static void AutoTexturization_RenderShipInto(benchmark::State & state)
{
    GameAssetManager const gameAssetManager = GameAssetManager((std::filesystem::current_path() / "Data").string());
    MaterialDatabase const materialDatabase = MaterialDatabase::Load(gameAssetManager);
    ShipTexturizer texturizer(materialDatabase, gameAssetManager);

    // Create structural layer
    StructuralLayerData structuralLayer(StructureSize);
    auto const & materialCategories = materialDatabase.GetStructuralMaterialPalette().Categories;
    size_t currentCategory = 0;
    size_t currentSubCategory = 0;
    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            StructuralMaterial const * material = ((x + y) % 5 == 0) ? nullptr : &materialCategories[currentCategory].SubCategories[currentSubCategory].Materials[0].get();
            structuralLayer.Buffer[{x, y}].Material = material;

            // Move to next sub-category
            ++currentSubCategory;
            if (currentSubCategory >= materialCategories[currentCategory].SubCategories.size())
            {
                currentSubCategory = 0;
                ++currentCategory;
                if (currentCategory >= materialCategories.size())
                {
                    currentCategory = 0;
                }
            }
        }
    }

    // Create source texture
    ImageSize const sourceTextureSize = ImageSize(
        StructureSize.width * 18,
        StructureSize.height * 18);
    RgbaImageData sourceTextureImage = RgbaImageData(sourceTextureSize);

    // Create target texture
    int const magnificationFactor = ShipTexturizer::CalculateHighDefinitionTextureMagnificationFactor(StructureSize, 4096);
    ImageSize const targetTextureSize = ImageSize(
        StructureSize.width * magnificationFactor,
        StructureSize.height * magnificationFactor);
    RgbaImageData targetTextureImage = RgbaImageData(targetTextureSize);

    // Test
    for (auto _ : state)
    {
        for (size_t i = 0; i < Repetitions * 4; ++i)
        {
            texturizer.RenderShipInto(
                structuralLayer,
                ShipSpaceRect({ 0, 0 }, StructureSize),
                sourceTextureImage,
                targetTextureImage,
                magnificationFactor);
        }
    }
}
BENCHMARK(AutoTexturization_RenderShipInto);
