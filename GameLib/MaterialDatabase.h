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

    // The (single) structural material with this color is assumed to be Rope
    static constexpr ColorKey RopeMaterialBaseColorKey{ 0x00, 0x00, 0x00 };

public:

    static MaterialDatabase Create(
        picojson::value const & structuralMaterialsRoot,
        picojson::value const & electricalMaterialsRoot)
    {
        //
        // Structural
        //

        std::map<ColorKey, StructuralMaterial> structuralMaterialsMap;

        if (!structuralMaterialsRoot.is<picojson::array>())
        {
            throw GameException("Structural materials definition is not a JSON array");
        }

        picojson::array structuralMaterialsRootArray = structuralMaterialsRoot.get<picojson::array>();
        StructuralMaterial const * ropeMaterial = nullptr;
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
            auto const entry = structuralMaterialsMap.emplace(
                std::make_pair(
                    colorKey,
                    material));

            // Check whether this is *the* rope material
            if (colorKey == RopeMaterialBaseColorKey)
            {
                if (nullptr != ropeMaterial)
                {
                    throw GameException("More than one Rope material found in structural materials definition");
                }

                ropeMaterial = &(entry.first->second);
            }
        }

        // Make sure we did find the rope material
        if (nullptr == ropeMaterial)
        {
            throw GameException("No Rope material found in structural materials definition");
        }

        // Make sure there are no clashes with indexed rope colors
        for (auto const & entry : structuralMaterialsMap)
        {
            if (!IsRopeMaterialColorKey(entry.first)
                && entry.first[0] == RopeMaterialBaseColorKey[0]
                && (entry.first[1] & 0xF0) == (RopeMaterialBaseColorKey[1] & 0xF0))
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
            *ropeMaterial);
    }

    StructuralMaterial const * FindStructuralMaterial(ColorKey const & colorKey) const
    {
        auto srchIt = mStructuralMaterialMap.find(colorKey);
        if (srchIt != mStructuralMaterialMap.end())
        {
            return &(srchIt->second);
        }

        // Check whether it's a rope endpoint
        if (colorKey[0] == RopeMaterialBaseColorKey[0]
            && ((colorKey[1] & 0xF0) == (RopeMaterialBaseColorKey[1] & 0xF0)))
        {
            return &mRopeMaterial;
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

    StructuralMaterial const & GetRopeMaterial() const
    {
        return mRopeMaterial;
    }

    static bool IsRopeMaterialColorKey(ColorKey const & colorKey)
    {
        return colorKey == RopeMaterialBaseColorKey;
    }

    bool IsRopeMaterial(StructuralMaterial const & material) const
    {
        return (&material) == (&mRopeMaterial);
    }

private:

    MaterialDatabase(
        std::map<ColorKey, StructuralMaterial> structuralMaterialMap,
        std::map<ColorKey, ElectricalMaterial> electricalMaterialMap,
        StructuralMaterial const & ropeMaterial)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mElectricalMaterialMap(std::move(electricalMaterialMap))
        , mRopeMaterial(ropeMaterial)
    {
    }

    std::map<ColorKey, StructuralMaterial> mStructuralMaterialMap;
    std::map<ColorKey, ElectricalMaterial> mElectricalMaterialMap;
    StructuralMaterial const & mRopeMaterial;
};
