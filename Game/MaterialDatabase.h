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

    static MaterialDatabase Load(ResourceLocator const & resourceLocator)
    {
        return Load(resourceLocator.GetMaterialDatabaseRootFilePath());
    }

    static MaterialDatabase Load(std::filesystem::path materialsRootDirectory)
    {
        //
        // Structural
        //

        std::map<ColorKey, StructuralMaterial> structuralMaterialsMap;
        UniqueStructuralMaterialsArray uniqueStructuralMaterials;

        for (size_t i = 0; i < uniqueStructuralMaterials.size(); ++i)
            uniqueStructuralMaterials[i].second = nullptr;

        picojson::value const structuralMaterialsRoot = Utils::ParseJSONFile(
            materialsRootDirectory / "materials_structural.json");

        if (!structuralMaterialsRoot.is<picojson::array>())
        {
            throw GameException("Structural materials definition is not a JSON array");
        }

        picojson::array const & structuralMaterialsRootArray = structuralMaterialsRoot.get<picojson::array>();
        for (auto const & materialElem : structuralMaterialsRootArray)
        {
            if (!materialElem.is<picojson::object>())
            {
                throw GameException("Found a non-object in structural materials definition");
            }

            picojson::object const & materialObject = materialElem.get<picojson::object>();

            ColorKey colorKey = Utils::Hex2RgbColor(
                Utils::GetMandatoryJsonMember<std::string>(materialObject, "color_key"));

            StructuralMaterial material = StructuralMaterial::Create(materialObject);

            // Make sure there are no dupes
            if (structuralMaterialsMap.count(colorKey) != 0)
            {
                throw GameException("Structural material \"" + material.Name + "\" has a duplicate color key");
            }

            // Store
            auto const storedEntry = structuralMaterialsMap.emplace(
                std::make_pair(
                    colorKey,
                    material));

            // Check if it's unique, and if so, check for dupes and store it
            if (!!material.UniqueType)
            {
                size_t uniqueTypeIndex = static_cast<size_t>(*(material.UniqueType));
                if (nullptr != uniqueStructuralMaterials[uniqueTypeIndex].second)
                {
                    throw GameException("More than one unique material of type \"" + std::to_string(uniqueTypeIndex) + "\" found in structural materials definition");
                }

                uniqueStructuralMaterials[uniqueTypeIndex] = std::make_pair(
                    colorKey,
                    &(storedEntry.first->second));
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
        for (auto const & entry : structuralMaterialsMap)
        {
            if ((!entry.second.UniqueType || StructuralMaterial::MaterialUniqueType::Rope != *(entry.second.UniqueType))
                && entry.first.r == uniqueStructuralMaterials[RopeUniqueMaterialIndex].first.r
                && (entry.first.g & 0xF0) == (uniqueStructuralMaterials[RopeUniqueMaterialIndex].first.g & 0xF0))
            {
                throw GameException("Structural material \"" + entry.second.Name + "\" has a color key that is reserved for ropes and rope endpoints");
            }
        }


        //
        // Electrical materials
        //

        std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> nonInstancedElectricalMaterialsMap;
        std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> instancedElectricalMaterialsMap;

        picojson::value const electricalMaterialsRoot = Utils::ParseJSONFile(
            materialsRootDirectory / "materials_electrical.json");

        if (!electricalMaterialsRoot.is<picojson::array>())
        {
            throw GameException("Electrical materials definition is not a JSON array");
        }

        picojson::array const & electricalMaterialsRootArray = electricalMaterialsRoot.get<picojson::array>();
        for (auto const & materialElem : electricalMaterialsRootArray)
        {
            if (!materialElem.is<picojson::object>())
            {
                throw GameException("Found a non-object in electrical materials definition");
            }

            picojson::object const & materialObject = materialElem.get<picojson::object>();

            ColorKey colorKey = Utils::Hex2RgbColor(
                Utils::GetMandatoryJsonMember<std::string>(materialObject, "color_key"));

            ElectricalMaterial material = ElectricalMaterial::Create(materialObject);

            // Make sure there are no dupes
            if (nonInstancedElectricalMaterialsMap.count(colorKey) != 0
                || instancedElectricalMaterialsMap.count(colorKey) != 0)
            {
                throw GameException("Electrical material \"" + material.Name + "\" has a duplicate color key");
            }

            // Store
            if (material.IsInstanced)
                instancedElectricalMaterialsMap.emplace(
                    std::make_pair(
                        colorKey,
                        material));
            else
                nonInstancedElectricalMaterialsMap.emplace(
                    std::make_pair(
                        colorKey,
                        material));
        }


        //
        // Make sure there are no structural materials whose key appears
        // in electrical materials, with the exception for "legacy" electrical
        // materials
        //

        for (auto const & kv : structuralMaterialsMap)
        {
            if (!kv.second.IsLegacyElectrical
                && (0 != nonInstancedElectricalMaterialsMap.count(kv.first)
                    || 0 != instancedElectricalMaterialsMap.count(kv.first)))
            {
                throw GameException("color key of structural material \"" + kv.second.Name + "\" is also present among electrical materials");
            }
        }



        return MaterialDatabase(
            std::move(structuralMaterialsMap),
            std::move(nonInstancedElectricalMaterialsMap),
            std::move(instancedElectricalMaterialsMap),
            uniqueStructuralMaterials);
    }

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

    MaterialDatabase(
        std::map<ColorKey, StructuralMaterial> structuralMaterialMap,
        std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> nonInstancedElectricalMaterialMap,
        std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> instancedElectricalMaterialMap,
        UniqueStructuralMaterialsArray uniqueStructuralMaterials)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mNonInstancedElectricalMaterialMap(std::move(nonInstancedElectricalMaterialMap))
        , mInstancedElectricalMaterialMap(std::move(instancedElectricalMaterialMap))
        , mUniqueStructuralMaterials(uniqueStructuralMaterials)
    {
    }

    std::map<ColorKey, StructuralMaterial> mStructuralMaterialMap;
    std::map<ColorKey, ElectricalMaterial, NonInstancedColorKeyComparer> mNonInstancedElectricalMaterialMap;
    std::map<ColorKey, ElectricalMaterial, InstancedColorKeyComparer> mInstancedElectricalMaterialMap;

    UniqueStructuralMaterialsArray mUniqueStructuralMaterials;
};
