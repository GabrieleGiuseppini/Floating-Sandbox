/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialDatabase.h"

#include <Core/Log.h>

#include <algorithm>

MaterialDatabase MaterialDatabase::Load(IAssetManager const & assetManager)
{
    //
    // Structural
    //

    MaterialColorMap<StructuralMaterial> structuralMaterialColorMap;
    MaterialNameMap<StructuralMaterial> structuralMaterialNameMap;

    // Prepare unique structural materials
    UniqueStructuralMaterialsArray uniqueStructuralMaterials;
    for (size_t i = 0; i < uniqueStructuralMaterials.size(); ++i)
        uniqueStructuralMaterials[i].second = nullptr;

    // Load file
    picojson::value const structuralMaterialsRoot = assetManager.LoadStructuralMaterialDatabase();

    if (!structuralMaterialsRoot.is<picojson::object>())
    {
        throw GameException("Structural materials definition is not a JSON object");
    }

    picojson::object const & structuralMaterialsRootObj = structuralMaterialsRoot.get<picojson::object>();

    float largestStructuralMass = 0.0f;

    // Parse structural palette
    Palette<StructuralMaterial> structuralMaterialPalette = Palette<StructuralMaterial>::Parse(
        Utils::GetMandatoryJsonObject(structuralMaterialsRootObj, "palettes"),
        "structural_palette");

    // Parse ropes palette
    Palette<StructuralMaterial> ropeMaterialPalette = Palette<StructuralMaterial>::Parse(
        Utils::GetMandatoryJsonObject(structuralMaterialsRootObj, "palettes"),
        "ropes_palette");

    // Parse materials
    picojson::array const & structuralMaterialsRootArray = Utils::GetMandatoryJsonArray(structuralMaterialsRootObj, "materials");
    for (auto const & materialElem : structuralMaterialsRootArray)
    {
        if (!materialElem.is<picojson::object>())
        {
            throw GameException("Found a non-object in structural materials definition");
        }

        picojson::object const & materialObject = materialElem.get<picojson::object>();

        // Normalize color keys
        std::vector<MaterialColorKey> colorKeys;
        {
            auto const & memberIt = materialObject.find("color_key");
            if (materialObject.end() == memberIt)
            {
                throw GameException("Error parsing JSON: cannot find member \"color_key\"");
            }

            if (memberIt->second.is<std::string>())
            {
                colorKeys.emplace_back(
                    Utils::Hex2RgbColor(
                        memberIt->second.get<std::string>()));
            }
            else if (memberIt->second.is<picojson::array>())
            {
                auto const & memberArray = memberIt->second.get<picojson::array>();
                for (auto const & memberArrayElem : memberArray)
                {
                    if (!memberArrayElem.is<std::string>())
                    {
                        throw GameException("Error parsing JSON: an element of the material's \"color_key\" array is not a 'string'");
                    }

                    colorKeys.emplace_back(
                        Utils::Hex2RgbColor(
                            memberArrayElem.get<std::string>()));
                }
            }
            else
            {
                throw GameException("Error parsing JSON: material's \"color_key\" member is neither a 'string' nor an 'array'");
            }
        }

        // Process all color keys
        for (size_t iColorKey = 0; iColorKey < colorKeys.size(); ++iColorKey)
        {
            // Get color key
            rgbColor const colorKey = colorKeys[iColorKey];

            // Get/make render color
            rgbColor renderColor = colorKey;
            if (auto const & renderColorIt = materialObject.find("render_color");
                materialObject.end() != renderColorIt)
            {
                // The material carries its own render color

                if (colorKeys.size() > 1)
                {
                    throw GameException("Error parsing JSON: material with multiple \"color_key\" members cannot specify custom \"render_color\" members");
                }

                if (!renderColorIt->second.is<std::string>())
                {
                    throw GameException("Error parsing JSON: member \"render_color\" is not of type 'string'");
                }

                renderColor = Utils::Hex2RgbColor(renderColorIt->second.get<std::string>());
            }

            // Create instance of this material
            StructuralMaterial const material = StructuralMaterial::Create(
                colorKey,
                static_cast<unsigned int>(iColorKey),
                renderColor,
                materialObject);

            // Make sure color key does not match the "empty" color key
            if (colorKey == EmptyMaterialColorKey)
            {
                throw GameException("Structural material \"" + material.Name + "\" has the same color key as the \"empty material\"");
            }

            // Store by color - making sure there are no dupes
            auto const [instanceIt, isColorInserted] = structuralMaterialColorMap.emplace(
                std::make_pair(
                    colorKey,
                    material));
            if (!isColorInserted)
            {
                throw GameException("Color key \"" + Utils::RgbColor2Hex(colorKey) + "\" of structural material \"" + material.Name + "\" already belongs to another material");
            }

            // Store by name (first instance only) - making sure there are no dupes
            if (iColorKey == 0)
            {
                auto const [_, isNameInserted] = structuralMaterialNameMap.emplace(
                    std::make_pair(
                        material.Name,
                        colorKey));
                if (!isNameInserted)
                {
                    throw GameException("Material name \"" + material.Name + "\" already belongs to another material");
                }
            }

            // Add to palettes
            if (material.PaletteCoordinates.has_value())
            {
                if (structuralMaterialPalette.HasCategory(material.PaletteCoordinates->Category))
                {
                    structuralMaterialPalette.InsertMaterial(instanceIt->second, *material.PaletteCoordinates);
                }
                else if (ropeMaterialPalette.HasCategory(material.PaletteCoordinates->Category))
                {
                    ropeMaterialPalette.InsertMaterial(instanceIt->second, *material.PaletteCoordinates);
                }
                else
                {
                    throw GameException("Category \"" + material.PaletteCoordinates->Category + "\" of structural material \"" + material.Name + "\" may not be found in any palette");
                }
            }

            // Check if it's a unique material, and if so, check for dupes and store it
            if (material.UniqueType.has_value())
            {
                size_t uniqueTypeIndex = static_cast<size_t>(*(material.UniqueType));
                if (nullptr != uniqueStructuralMaterials[uniqueTypeIndex].second)
                {
                    throw GameException("More than one unique material of type \"" + std::to_string(uniqueTypeIndex) + "\" found in structural materials definition");
                }

                uniqueStructuralMaterials[uniqueTypeIndex] = std::make_pair(
                    colorKey,
                    &(instanceIt->second));
            }

            // Update extremes
            largestStructuralMass = std::max(material.GetMass(), largestStructuralMass);
        }
    }

    // Make sure we did find all the unique materials
    for (size_t i = 0; i < uniqueStructuralMaterials.size(); ++i)
    {
        if (nullptr == uniqueStructuralMaterials[i].second)
        {
            throw GameException("No material found in structural materials definition for unique type \"" + std::to_string(i) + "\"");
        }
    }

    // Make sure there are no clashes with indexed rope colors
    for (auto const & entry : structuralMaterialColorMap)
    {
        if ((!entry.second.UniqueType || StructuralMaterial::MaterialUniqueType::Rope != *(entry.second.UniqueType))
            && entry.first.r == uniqueStructuralMaterials[RopeUniqueMaterialIndex].first.r
            && (entry.first.g & 0xF0) == (uniqueStructuralMaterials[RopeUniqueMaterialIndex].first.g & 0xF0))
        {
            throw GameException("Structural material \"" + entry.second.Name + "\" has a color key (\"" + Utils::RgbColor2Hex(entry.first) + "\") which is reserved for ropes and rope endpoints");
        }
    }

    // Make sure the palettes are fully-populated
    structuralMaterialPalette.CheckComplete();
    ropeMaterialPalette.CheckComplete();

    LogMessage("Loaded " + std::to_string(structuralMaterialColorMap.size()) + " structural materials.");

    //
    // Electrical materials
    //

    MaterialColorMap<ElectricalMaterial> electricalMaterialColorMap;
    std::map<MaterialColorKey, ElectricalMaterial const *, InstancedColorKeyComparer> instancedElectricalMaterialMap;

    picojson::value const electricalMaterialsRoot = assetManager.LoadElectricalMaterialDatabase();

    if (!electricalMaterialsRoot.is<picojson::object>())
    {
        throw GameException("Structural materials definition is not a JSON object");
    }

    picojson::object const & electricalMaterialsRootObj = electricalMaterialsRoot.get<picojson::object>();

    // Parse palette structure
    Palette<ElectricalMaterial> electricalMaterialPalette = Palette<ElectricalMaterial>::Parse(
        Utils::GetMandatoryJsonObject(electricalMaterialsRootObj, "palettes"),
        "electrical_palette");

    // Parse materials
    picojson::array const & electricalMaterialsRootArray = Utils::GetMandatoryJsonArray(electricalMaterialsRootObj, "materials");
    for (auto const & materialElem : electricalMaterialsRootArray)
    {
        if (!materialElem.is<picojson::object>())
        {
            throw GameException("Found a non-object in electrical materials definition");
        }

        picojson::object const & materialObject = materialElem.get<picojson::object>();

        // Get color key
        MaterialColorKey const colorKey = Utils::Hex2RgbColor(
            Utils::GetMandatoryJsonMember<std::string>(materialObject, "color_key"));

        // Get/make render color
        rgbColor renderColor = colorKey;
        if (auto const & renderColorIt = materialObject.find("render_color");
            materialObject.end() != renderColorIt)
        {
            if (!renderColorIt->second.is<std::string>())
            {
                throw GameException("Error parsing JSON: member \"render_color\" is not of type 'string'");
            }

            renderColor = Utils::Hex2RgbColor(renderColorIt->second.get<std::string>());
        }

        // Create instance of this material
        ElectricalMaterial const material = ElectricalMaterial::Create(
            colorKey,
            0,
            renderColor,
            materialObject);

        // Make sure color key does not match the "empty" color key
        if (colorKey == EmptyMaterialColorKey)
        {
            throw GameException("Electrical material \"" + material.Name + "\" has the same color key as the \"empty material\"");
        }

        // Make sure there are no dupes
        if (auto const searchIt = electricalMaterialColorMap.find(colorKey);
            searchIt != electricalMaterialColorMap.cend())
        {
            throw GameException("Electrical material \"" + material.Name + "\" has a color key conflicting with the \"" + searchIt->second.Name+ "\" material.");
        }
        if (auto const searchIt = instancedElectricalMaterialMap.find(colorKey);
            searchIt != instancedElectricalMaterialMap.cend())
        {
            throw GameException("Electrical material \"" + material.Name + "\" has a color key conflicting with the \"" + searchIt->second->Name + "\" material.");
        }

        // Store
        auto const [instanceIt, isInserted] = electricalMaterialColorMap.emplace(
            std::make_pair(
                colorKey,
                material));
        assert(isInserted);

        // Add to palette
        if (material.PaletteCoordinates.has_value())
        {
            if (electricalMaterialPalette.HasCategory(material.PaletteCoordinates->Category))
            {
                electricalMaterialPalette.InsertMaterial(instanceIt->second, *material.PaletteCoordinates);
            }
            else
            {
                throw GameException("Category \"" + material.PaletteCoordinates->Category + "\" of electrical material \"" + material.Name + "\" may not be found in any palette");
            }
        }

        if (material.IsInstanced)
        {
            // Add also to instanced material map, for legacy r+g lookup
            instancedElectricalMaterialMap.emplace(
                std::make_pair(
                    colorKey,
                    &(instanceIt->second)));
        }
    }

    // Make sure the palette is fully-populated
    electricalMaterialPalette.CheckComplete();

    LogMessage("Loaded " + std::to_string(electricalMaterialColorMap.size()) + " electrical materials.");

    //
    // Make sure there are no structural materials whose key appears
    // in electrical materials, with the exception for "legacy" electrical
    // materials
    //

    for (auto const & kv : structuralMaterialColorMap)
    {
        if (!kv.second.IsLegacyElectrical
            && (0 != electricalMaterialColorMap.count(kv.first)
                || 0 != instancedElectricalMaterialMap.count(kv.first)))
        {
            throw GameException("Color key of structural material \"" + kv.second.Name + "\" is also present among electrical materials");
        }
    }

    //
    // Wrap it up
    //

    return MaterialDatabase(
        std::move(structuralMaterialColorMap),
        std::move(structuralMaterialNameMap),
        uniqueStructuralMaterials,
        std::move(structuralMaterialPalette),
        std::move(ropeMaterialPalette),
        largestStructuralMass,
        std::move(electricalMaterialColorMap),
        std::move(instancedElectricalMaterialMap),
        std::move(electricalMaterialPalette));
}

MaterialDatabase MaterialDatabase::Make(
    std::vector<StructuralMaterial const *> const & structuralMaterials,
    std::vector<ElectricalMaterial const *> const & electricalMaterials)
{
    //
    // Structural
    //

    MaterialColorMap<StructuralMaterial> structuralMaterialColorMap;
    MaterialNameMap<StructuralMaterial> structuralMaterialNameMap;

    for (auto s : structuralMaterials)
    {
        auto [_1, isInserted1] = structuralMaterialColorMap.insert({ s->ColorKey, *s });
        assert(isInserted1);
        (void)_1;
        (void)isInserted1;

        auto [_2, isInserted2] = structuralMaterialNameMap.insert({ s->Name, s->ColorKey });
        assert(isInserted2);
        (void)_2;
        (void)isInserted2;
    }

    UniqueStructuralMaterialsArray uniqueStructuralMaterials;
    for (size_t i = 0; i < uniqueStructuralMaterials.size(); ++i)
        uniqueStructuralMaterials[i].second = nullptr;

    Palette<StructuralMaterial> structuralMaterialPalette;
    Palette<StructuralMaterial> ropeMaterialPalette;

    //
    // Electrical
    //

    MaterialColorMap<ElectricalMaterial> electricalMaterialColorMap;
    std::map<MaterialColorKey, ElectricalMaterial const *, InstancedColorKeyComparer> instancedElectricalMaterialMap;

    for (auto e : electricalMaterials)
    {
        auto [_1, isInserted1] = electricalMaterialColorMap.insert({ e->ColorKey, *e });
        assert(isInserted1);
        (void)_1;
        (void)isInserted1;

        if (e->IsInstanced)
        {
            auto [_2, isInserted2] = instancedElectricalMaterialMap.insert({ e->ColorKey, e });
            assert(isInserted2);
            (void)_2;
            (void)isInserted2;
        }
    }

    Palette<ElectricalMaterial> electricalMaterialPalette;

    return MaterialDatabase(
        std::move(structuralMaterialColorMap),
        std::move(structuralMaterialNameMap),
        uniqueStructuralMaterials,
        std::move(structuralMaterialPalette),
        std::move(ropeMaterialPalette),
        1000.0f,
        std::move(electricalMaterialColorMap),
        std::move(instancedElectricalMaterialMap),
        std::move(electricalMaterialPalette));
}

///////////////////////////////////////////////////////////////////////

template<typename TMaterial>
MaterialDatabase::Palette<TMaterial> MaterialDatabase::Palette<TMaterial>::Parse(
    picojson::object const & palettesRoot,
    std::string const & paletteName)
{
    Palette<TMaterial> palette;

    size_t uniqueGroupId = 0;

    picojson::array const & paletteCategoriesJson = Utils::GetMandatoryJsonMember<picojson::array>(palettesRoot, paletteName);
    for (auto const & categoryJson : paletteCategoriesJson)
    {
        picojson::object const & categoryObj = Utils::GetJsonValueAsObject(categoryJson, "palette_category");

        Category category(Utils::GetMandatoryJsonMember<std::string>(categoryObj, "category"));
        for (auto const & groupJson : Utils::GetMandatoryJsonArray(categoryObj, "groups"))
        {
            picojson::object const & groupObj = Utils::GetJsonValueAsObject(groupJson, "group");

            typename Category::SubCategory::Group const parentGroup(
                Utils::GetMandatoryJsonMember<std::string>(groupObj, "name"),
                uniqueGroupId++);

            for (auto const & subCategoryJson : Utils::GetMandatoryJsonArray(groupObj, "sub_categories"))
            {
                category.SubCategories.emplace_back(
                    Utils::GetJsonValueAs<std::string>(subCategoryJson, "sub_category"),
                    parentGroup);
            }
        }

        palette.Categories.emplace_back(std::move(category));
    }

    return palette;
}

template<typename TMaterial>
bool MaterialDatabase::Palette<TMaterial>::HasCategory(std::string const & categoryName)
{
    auto const categoryIt = std::find_if(
        Categories.cbegin(),
        Categories.cend(),
        [&categoryName](Category const & c)
        {
            return c.Name == categoryName;
        });

    return categoryIt != Categories.cend();
}

template<typename TMaterial>
void MaterialDatabase::Palette<TMaterial>::InsertMaterial(
    TMaterial const & material,
    MaterialPaletteCoordinatesType const & paletteCoordinates)
{
    //
    // Find category
    //

    auto categoryIt = std::find_if(
        Categories.begin(),
        Categories.end(),
        [&paletteCoordinates](Category const & c)
        {
            return c.Name == paletteCoordinates.Category;
        });

    assert(categoryIt != Categories.end());

    Category & category = *categoryIt;

    //
    // Find sub-category
    //

    auto subCategoryIt = std::find_if(
        category.SubCategories.begin(),
        category.SubCategories.end(),
        [&paletteCoordinates](typename Category::SubCategory const & s)
        {
            return s.Name == paletteCoordinates.SubCategory;
        });

    if (subCategoryIt == category.SubCategories.end())
    {
        throw GameException("Sub-category \"" + paletteCoordinates.SubCategory + "\" of material \""
            + material.Name + "\" is not a valid sub-category of category \"" + paletteCoordinates.Category + "\"");
    }

    //
    // Store material at righ position for its ordinal
    //

    auto const insertIt = std::lower_bound(
        subCategoryIt->Materials.cbegin(),
        subCategoryIt->Materials.cend(),
        paletteCoordinates.SubCategoryOrdinal,
        [](TMaterial const & m, auto const ordinal)
        {
            assert(m.PaletteCoordinates.has_value());
            return m.PaletteCoordinates->SubCategoryOrdinal < ordinal;
        });

    if (insertIt != subCategoryIt->Materials.cend())
    {
        TMaterial const & conflictingMaterial = *insertIt;
        assert(conflictingMaterial.PaletteCoordinates.has_value());
        if (conflictingMaterial.PaletteCoordinates->SubCategoryOrdinal == paletteCoordinates.SubCategoryOrdinal)
        {
            throw GameException("Material \"" + material.Name + "\" has a palette category ordinal that conflicts with material \"" + conflictingMaterial.Name + "\"");
        }
    }

    subCategoryIt->Materials.insert(insertIt, material);
}

template<typename TMaterial>
void MaterialDatabase::Palette<TMaterial>::CheckComplete()
{
    for (auto const & category : Categories)
    {
        if (category.SubCategories.empty())
        {
            throw GameException("Material palette category \"" + category.Name + "\" is empty");
        }
        else
        {
            for (auto const & subCategory : category.SubCategories)
            {
                if (subCategory.Materials.empty())
                {
                    throw GameException("Material palette sub-category \"" + subCategory.Name + "\" of category \"" + category.Name + "\" is empty");
                }
            }
        }
    }
}