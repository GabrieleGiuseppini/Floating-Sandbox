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
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class MaterialDatabase
{
public:

    using ColorKey = rgbColor;

    static ColorKey constexpr EmptyMaterialColorKey = ColorKey(255, 255, 255);

    template<typename TMaterial>
    struct Palette
    {
        struct Category
        {
            struct SubCategory
            {
                std::string Name;
                std::vector<std::reference_wrapper<TMaterial const>> Materials;

                SubCategory(std::string const & name)
                    : Name(name)
                    , Materials()
                {}
            };

            std::string Name;
            std::vector<SubCategory> SubCategories;

            Category(std::string const & name)
                : Name(name)
                , SubCategories()
            {}
        };

        std::vector<Category> Categories;

        static Palette Parse(picojson::array const & paletteCategoriesJson);

        void InsertMaterial(TMaterial const & material, MaterialPaletteCoordinatesType const & paletteCoordinates);

        void CheckComplete();
    };

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

    Palette<StructuralMaterial> const & GetStructuralMaterialPalette() const
    {
        return mStructuralMaterialPalette;
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

    Palette<ElectricalMaterial> const & GetElectricalMaterialPalette() const
    {
        return mElectricalMaterialPalette;
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

    static ElectricalElementInstanceIndex ExtractElectricalElementInstanceIndex(ColorKey const & colorKey)
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

private:

    MaterialDatabase(
        std::map<ColorKey, StructuralMaterial> structuralMaterialMap,
        Palette<StructuralMaterial> structuralMaterialPalette,
        std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> nonInstancedElectricalMaterialMap,
        std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> instancedElectricalMaterialMap,
        Palette<ElectricalMaterial> electricalMaterialPalette,
        UniqueStructuralMaterialsArray uniqueStructuralMaterials,
        float largestMass,
        float largestStrength)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mStructuralMaterialPalette(std::move(structuralMaterialPalette))
        , mNonInstancedElectricalMaterialMap(std::move(nonInstancedElectricalMaterialMap))
        , mInstancedElectricalMaterialMap(std::move(instancedElectricalMaterialMap))
        , mElectricalMaterialPalette(std::move(electricalMaterialPalette))
        , mUniqueStructuralMaterials(uniqueStructuralMaterials)
        , mLargestMass(largestMass)
        , mLargestStrength(largestStrength)
    {
    }

private:

    // Structural
    std::map<ColorKey, StructuralMaterial> mStructuralMaterialMap;
    Palette<StructuralMaterial> mStructuralMaterialPalette;

    // Electrical
    std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> mNonInstancedElectricalMaterialMap;
    std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> mInstancedElectricalMaterialMap;
    Palette<ElectricalMaterial> mElectricalMaterialPalette;

    UniqueStructuralMaterialsArray mUniqueStructuralMaterials;
    float mLargestMass;
    float mLargestStrength;
};
