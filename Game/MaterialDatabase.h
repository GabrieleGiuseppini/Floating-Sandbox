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

    template<typename TMaterial>
    struct Palette
    {
        struct Category
        {
            struct SubCategory
            {
                struct Group
                {
                    std::string Name;
                    size_t UniqueId;

                    Group(
                        std::string const & name,
                        size_t uniqueId)
                        : Name(name)
                        , UniqueId(uniqueId)
                    {}
                };

                std::string Name;
                Group ParentGroup;
                std::vector<std::reference_wrapper<TMaterial const>> Materials;

                SubCategory(
                    std::string const & name,
                    Group const & parentGroup)
                    : Name(name)
                    , ParentGroup(parentGroup)
                    , Materials()
                {}
            };

            std::string Name;
            std::vector<SubCategory> SubCategories;

            Category(std::string const & name)
                : Name(name)
                , SubCategories()
            {}

            size_t GetMaxWidth() const
            {
                size_t count = 0;
                for (auto const & sc : SubCategories)
                {
                    if (sc.Materials.size() > count)
                    {
                        count = sc.Materials.size();
                    }
                }

                return count;
            }
        };

        std::vector<Category> Categories;

        static Palette Parse(
            picojson::object const & palettesRoot,
            std::string const & paletteName);

        bool HasCategory(std::string const & categoryName);

        void InsertMaterial(TMaterial const & material, MaterialPaletteCoordinatesType const & paletteCoordinates);

        void CheckComplete();
    };

    template<typename TMaterial>
    using MaterialMap = std::map<MaterialColorKey, TMaterial>;

private:

    using UniqueStructuralMaterialsArray = std::array<std::pair<MaterialColorKey, StructuralMaterial const *>, static_cast<size_t>(StructuralMaterial::MaterialUniqueType::_Last) + 1>;

    static constexpr auto RopeUniqueMaterialIndex = static_cast<size_t>(StructuralMaterial::MaterialUniqueType::Rope);

    using NpcMaterialMap = std::map<std::string, NpcMaterial>;

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

    StructuralMaterial const * FindStructuralMaterial(MaterialColorKey const & colorKey) const
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

    StructuralMaterial const & GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType uniqueType) const
    {
        assert(static_cast<size_t>(uniqueType) < mUniqueStructuralMaterials.size());
        assert(nullptr != mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);

        return *(mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);
    }

    bool IsUniqueStructuralMaterialColorKey(
        StructuralMaterial::MaterialUniqueType uniqueType,
        MaterialColorKey const & colorKey) const
    {
        assert(static_cast<size_t>(uniqueType) < mUniqueStructuralMaterials.size());
        assert(nullptr != mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);

        return colorKey == mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].first;
    }

    MaterialMap<StructuralMaterial> const & GetStructuralMaterialMap() const
    {
        return mStructuralMaterialMap;
    }

    Palette<StructuralMaterial> const & GetStructuralMaterialPalette() const
    {
        return mStructuralMaterialPalette;
    }

    Palette<StructuralMaterial> const & GetRopeMaterialPalette() const
    {
        return mRopeMaterialPalette;
    }

    float GetLargestStructuralMass() const
    {
        return mLargestStructuralMass;
    }

    // ------------------------

    ElectricalMaterial const * FindElectricalMaterial(MaterialColorKey const & colorKey) const
    {
        if (auto srchIt = mElectricalMaterialMap.find(colorKey);
            srchIt != mElectricalMaterialMap.end())
        {
            // Found color key verbatim!
            return &(srchIt->second);
        }

        // No luck
        return nullptr;
    }

    ElectricalMaterial const * FindElectricalMaterialLegacy(MaterialColorKey const & colorKey) const
    {
        // Try verbatim first
        ElectricalMaterial const * verbatimMatch = FindElectricalMaterial(colorKey);
        if (verbatimMatch != nullptr)
        {
            // Found!
            return verbatimMatch;
        }

        // Try just instanced now (i.e. matching on r and g only)
        if (auto srchIt = mInstancedElectricalMaterialMap.find(colorKey);
            srchIt != mInstancedElectricalMaterialMap.end())
        {
            return srchIt->second;
        }

        // No luck
        return nullptr;
    }

    MaterialMap<ElectricalMaterial> const & GetElectricalMaterialMap() const
    {
        return mElectricalMaterialMap;
    }

    Palette<ElectricalMaterial> const & GetElectricalMaterialPalette() const
    {
        return mElectricalMaterialPalette;
    }

    static ElectricalElementInstanceIndex ExtractElectricalElementInstanceIndex(MaterialColorKey const & colorKey)
    {
        static_assert(sizeof(ElectricalElementInstanceIndex) >= sizeof(MaterialColorKey::data_type));
        return static_cast<ElectricalElementInstanceIndex>(colorKey.b);
    }

    // ------------------------

    // Note: not supposed to be invoked at runtime - only at NpcDatabase initialization
    NpcMaterial const & GetNpcMaterial(std::string const & name) const
    {
        if (auto const srchIt = mNpcMaterialMap.find(name);
            srchIt != mNpcMaterialMap.end())
        {
            return srchIt->second;
        }

        throw GameException("Cannot find NPC material \"" + name + "\"");
    }

private:

    struct InstancedColorKeyComparer
    {
        size_t operator()(MaterialColorKey const & lhs, MaterialColorKey const & rhs) const
        {
            return lhs.r < rhs.r
                || (lhs.r == rhs.r && lhs.g < rhs.g);
        }
    };

private:

    MaterialDatabase(
        MaterialMap<StructuralMaterial> structuralMaterialMap,
        UniqueStructuralMaterialsArray uniqueStructuralMaterials,
        Palette<StructuralMaterial> structuralMaterialPalette,
        Palette<StructuralMaterial> ropeMaterialPalette,
        float largestStructuralMass,
        MaterialMap<ElectricalMaterial> electricalMaterialMap,
        std::map<MaterialColorKey, ElectricalMaterial const *, InstancedColorKeyComparer> instancedElectricalMaterialMap,
        Palette<ElectricalMaterial> electricalMaterialPalette,
        NpcMaterialMap npcMaterialMap)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mUniqueStructuralMaterials(uniqueStructuralMaterials)
        , mStructuralMaterialPalette(std::move(structuralMaterialPalette))
        , mRopeMaterialPalette(std::move(ropeMaterialPalette))
        , mLargestStructuralMass(largestStructuralMass)
        , mElectricalMaterialMap(std::move(electricalMaterialMap))
        , mInstancedElectricalMaterialMap(std::move(instancedElectricalMaterialMap))
        , mElectricalMaterialPalette(std::move(electricalMaterialPalette))
        , mNpcMaterialMap(std::move(npcMaterialMap))
    {
    }

private:

    // Structural
    MaterialMap<StructuralMaterial> mStructuralMaterialMap;
    UniqueStructuralMaterialsArray mUniqueStructuralMaterials;
    Palette<StructuralMaterial> mStructuralMaterialPalette;
    Palette<StructuralMaterial> mRopeMaterialPalette;
    float mLargestStructuralMass;

    // Electrical
    MaterialMap<ElectricalMaterial> mElectricalMaterialMap;
    std::map<MaterialColorKey, ElectricalMaterial const *, InstancedColorKeyComparer> mInstancedElectricalMaterialMap; // Redundant map for (legacy) instanced material lookup
    Palette<ElectricalMaterial> mElectricalMaterialPalette;

    // NPC
    NpcMaterialMap mNpcMaterialMap;
};
