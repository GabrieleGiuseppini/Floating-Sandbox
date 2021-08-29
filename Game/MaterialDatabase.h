/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-04
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"
#include "ResourceLocator.h"

#include <GameCore/Colors.h>
#include <GameCore/GameException.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Utils.h>

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

class MaterialDatabase
{
public:

    using ColorKey = rgbColor;

private:

    using UniqueStructuralMaterialsArray = std::array<std::pair<ColorKey, StructuralMaterial const *>, static_cast<size_t>(StructuralMaterial::MaterialUniqueType::_Last) + 1>;

    static constexpr auto RopeUniqueMaterialIndex = static_cast<size_t>(StructuralMaterial::MaterialUniqueType::Rope);

public:

    // Only movable
    MaterialDatabase(MaterialDatabase const & other) = delete;
    MaterialDatabase(MaterialDatabase && other) = default;
    MaterialDatabase & operator=(MaterialDatabase const & other) = delete;
    MaterialDatabase & operator=(MaterialDatabase && other) = default;

    static MaterialDatabase Load(ResourceLocator const & resourceLocator)
    {
        return Load(resourceLocator.GetMaterialDatabaseRootFilePath());
    }

    static MaterialDatabase Load(std::filesystem::path materialsRootDirectory);

    StructuralMaterial const * FindStructuralMaterial(ColorKey const & colorKey) const
    {
        if (auto srchIt = mStructuralMaterialMap.find(colorKey);
            srchIt != mStructuralMaterialMap.end())
        {
            // Found color key verbatim!
            return &(srchIt->second);
        }

        // Check whether it's a rope endpoint
        if (colorKey.r == mUniqueStructuralMaterials[RopeUniqueMaterialIndex].first.r
            && ((colorKey.g & 0xF0) == (mUniqueStructuralMaterials[RopeUniqueMaterialIndex].first.g & 0xF0)))
        {
            return mUniqueStructuralMaterials[RopeUniqueMaterialIndex].second;
        }

        // No luck
        return nullptr;
    }

    auto const & GetStructuralMaterialsByColorKeys() const
    {
        return mStructuralMaterialMap;
    }

    ElectricalMaterial const * FindElectricalMaterial(ColorKey const & colorKey) const
    {
        // Try non-instanced first
        if (auto srchIt = mNonInstancedElectricalMaterialMap.find(colorKey);
            srchIt != mNonInstancedElectricalMaterialMap.end())
        {
            return &(srchIt->second);
        }

        // Try instanced now
        if (auto srchIt = mInstancedElectricalMaterialMap.find(colorKey);
            srchIt != mInstancedElectricalMaterialMap.end())
        {
            return &(srchIt->second);
        }

        // No luck
        return nullptr;
    }

    StructuralMaterial const & GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType uniqueType) const
    {
        assert(static_cast<size_t>(uniqueType) < mUniqueStructuralMaterials.size());
        assert(nullptr != mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);

        return *(mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);
    }

    bool IsUniqueStructuralMaterialColorKey(
        StructuralMaterial::MaterialUniqueType uniqueType,
        ColorKey const & colorKey) const
    {
        assert(static_cast<size_t>(uniqueType) < mUniqueStructuralMaterials.size());
        assert(nullptr != mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);

        return colorKey == mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].first;
    }

    static ElectricalElementInstanceIndex GetElectricalElementInstanceIndex(ColorKey const & colorKey)
    {
        static_assert(sizeof(ElectricalElementInstanceIndex) >= sizeof(ColorKey::data_type));
        return static_cast<ElectricalElementInstanceIndex>(colorKey.b);
    }

    float GetLargestMass() const
    {
        return mLargestMass;
    }

    float GetLargestStrength() const
    {
        return mLargestStrength;
    }

private:

    struct NonInstancedColorKeyComparer
    {
        size_t operator()(ColorKey const & lhs, ColorKey const & rhs) const
        {
            return lhs < rhs;
        }
    };

    struct InstancedColorKeyComparer
    {
        size_t operator()(ColorKey const & lhs, ColorKey const & rhs) const
        {
            return lhs.r < rhs.r
                || (lhs.r == rhs.r && lhs.g < rhs.g);
        }
    };

    struct PaletteCategory
    {
        std::string CategoryName;
        std::vector<std::string> CategoryGroups;

        PaletteCategory(
            std::string && categoryName,
            std::vector<std::string> && categoryGroups)
            : CategoryName(std::move(categoryName))
            , CategoryGroups(std::move(categoryGroups))
        {}
    };

private:

    MaterialDatabase(
        std::map<ColorKey, StructuralMaterial> structuralMaterialMap,
        std::vector<PaletteCategory> structuralPaletteCategories,
        std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> nonInstancedElectricalMaterialMap,
        std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> instancedElectricalMaterialMap,
        UniqueStructuralMaterialsArray uniqueStructuralMaterials,
        float largestMass,
        float largestStrength)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mStructuralPaletteCategories(std::move(structuralPaletteCategories))
        , mNonInstancedElectricalMaterialMap(std::move(nonInstancedElectricalMaterialMap))
        , mInstancedElectricalMaterialMap(std::move(instancedElectricalMaterialMap))
        , mUniqueStructuralMaterials(uniqueStructuralMaterials)
        , mLargestMass(largestMass)
        , mLargestStrength(largestStrength)
    {
    }

    static std::vector<PaletteCategory> ParsePaletteCategories(picojson::array const & paletteCategoriesJson)
    {
        std::vector<PaletteCategory> categories;

        for (auto const & categoryJson : paletteCategoriesJson)
        {
            picojson::object const & categoryObj = Utils::GetJsonValueAs<picojson::object>(categoryJson, "palette_category");

            std::string categoryName = Utils::GetMandatoryJsonMember<std::string>(categoryObj, "category");

            std::vector<std::string> categoryGroups;
            for (auto const & groupJson : Utils::GetMandatoryJsonArray(categoryObj, "groups"))
            {
                categoryGroups.emplace_back(Utils::GetJsonValueAs<std::string>(groupJson, "group"));
            }

            categories.emplace_back(
                std::move(categoryName),
                std::move(categoryGroups));
        }

        return categories;
    }

private:

    // Structural
    std::map<ColorKey, StructuralMaterial> mStructuralMaterialMap;
    std::vector<PaletteCategory> mStructuralPaletteCategories;

    // Electrical
    std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> mNonInstancedElectricalMaterialMap;
    std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> mInstancedElectricalMaterialMap;
    std::vector<PaletteCategory> mElectricalPaletteCategories;

    UniqueStructuralMaterialsArray mUniqueStructuralMaterials;
    float const mLargestMass;
    float const mLargestStrength;
};
