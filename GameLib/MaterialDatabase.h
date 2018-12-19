/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-04
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameException.h"
#include "Materials.h"
#include "Utils.h"

#include <picojson/picojson.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <optional>

class MaterialDatabase
{
public:

    using ColorKey = std::array<uint8_t, 3>;

private:

    using UniqueMaterialsArray = std::array<std::pair<ColorKey, StructuralMaterial const *>, static_cast<size_t>(StructuralMaterial::MaterialUniqueType::_Last) + 1>;

    static constexpr auto RopeUniqueMaterialIndex = static_cast<size_t>(StructuralMaterial::MaterialUniqueType::Rope);

public:

    static MaterialDatabase Create(
        picojson::value const & structuralMaterialsRoot,
        picojson::value const & electricalMaterialsRoot)
    {
        //
        // Structural
        //

        std::map<ColorKey, StructuralMaterial> structuralMaterialsMap;
        UniqueMaterialsArray uniqueStructuralMaterials;

        if (!structuralMaterialsRoot.is<picojson::array>())
        {
            throw GameException("Structural materials definition is not a JSON array");
        }

        picojson::array structuralMaterialsRootArray = structuralMaterialsRoot.get<picojson::array>();
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
                && entry.first[0] == uniqueStructuralMaterials[RopeUniqueMaterialIndex].first[0]
                && (entry.first[1] & 0xF0) == (uniqueStructuralMaterials[RopeUniqueMaterialIndex].first[1] & 0xF0))
            {
                throw GameException("Structural material \"" + entry.second.Name + "\" has a color key that is reserved for ropes and rope endpoints");
            }
        }


        //
        // Electrical materials
        //

        std::map<ColorKey, ElectricalMaterial> electricalMaterialsMap;

        if (!electricalMaterialsRoot.is<picojson::array>())
        {
            throw GameException("Electrical materials definition is not a JSON array");
        }

        picojson::array electricalMaterialsRootArray = electricalMaterialsRoot.get<picojson::array>();
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
            if (electricalMaterialsMap.count(colorKey) != 0)
            {
                throw GameException("Electrical material \"" + material.Name + "\" has a duplicate color key");
            }

            electricalMaterialsMap.emplace(
                std::make_pair(
                    colorKey,
                    material));
        }

        assert(nullptr != ropeMaterial);

        return MaterialDatabase(
            std::move(structuralMaterialsMap),
            std::move(electricalMaterialsMap),
            uniqueStructuralMaterials);
    }

    StructuralMaterial const * FindStructuralMaterial(ColorKey const & colorKey) const
    {
        auto srchIt = mStructuralMaterialMap.find(colorKey);
        if (srchIt != mStructuralMaterialMap.end())
        {
            return &(srchIt->second);
        }

        // Check whether it's a rope endpoint
        if (colorKey[0] == mUniqueStructuralMaterials[RopeUniqueMaterialIndex].first[0]
            && ((colorKey[1] & 0xF0) == (mUniqueStructuralMaterials[RopeUniqueMaterialIndex].first[1] & 0xF0)))
        {
            return mUniqueStructuralMaterials[RopeUniqueMaterialIndex].second;
        }

        // No luck
        return nullptr;
    }

    auto const & GetStructuralMaterials() const
    {
        return mStructuralMaterialMap;
    }

    ElectricalMaterial const * FindElectricalMaterial(ColorKey const & colorKey) const
    {
        auto srchIt = mElectricalMaterialMap.find(colorKey);
        if (srchIt != mElectricalMaterialMap.end())
        {
            return &(srchIt->second);
        }

        // No luck
        return nullptr;
    }

    StructuralMaterial const & GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType uniqueType) const
    {
        assert(static_cast<size_t>(uniqueType) < mUniqueStructuralMaterials.size());
        assert(nullptr != mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)]);

        return *(mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].second);
    }

    bool IsUniqueStructuralMaterialColorKey(
        StructuralMaterial::MaterialUniqueType uniqueType,
        ColorKey const & colorKey) const
    {
        assert(static_cast<size_t>(uniqueType) < mUniqueStructuralMaterials.size());
        assert(nullptr != mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)]);

        return colorKey == mUniqueStructuralMaterials[static_cast<size_t>(uniqueType)].first;
    }

private:

    MaterialDatabase(
        std::map<ColorKey, StructuralMaterial> structuralMaterialMap,
        std::map<ColorKey, ElectricalMaterial> electricalMaterialMap,
        UniqueMaterialsArray uniqueStructuralMaterials)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mElectricalMaterialMap(std::move(electricalMaterialMap))
        , mUniqueStructuralMaterials(uniqueStructuralMaterials)
    {
    }

    std::map<ColorKey, StructuralMaterial> mStructuralMaterialMap;
    std::map<ColorKey, ElectricalMaterial> mElectricalMaterialMap;
    UniqueMaterialsArray mUniqueStructuralMaterials;
};
