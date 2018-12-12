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
        std::optional<ColorKey> ropeMaterialColorKey;
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

            // Check whether this is *the* rope material
            if (material.IsRope)
            {
                if (!!ropeMaterialColorKey)
                {
                    throw GameException("More than one Rope material found in structural materials definition");
                }

                ropeMaterialColorKey.emplace(colorKey);
            }

            structuralMaterialsMap.emplace(
                std::make_pair(
                    colorKey,
                    material));
        }

        // Make sure we did find the rope material
        if (!ropeMaterialColorKey)
        {
            throw GameException("No Rope material found in structural materials definition");
        }

        // Make sure there are no clashes with indexed rope colors
        for (auto const & entry : structuralMaterialsMap)
        {
            if (!entry.second.IsRope
                && (*ropeMaterialColorKey)[0] == entry.first[0]
                && ((*ropeMaterialColorKey)[1] & 0xF0) == (entry.first[1] & 0xF0))
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

        return MaterialDatabase(
            std::move(structuralMaterialsMap),
            std::move(electricalMaterialsMap));
    }

    StructuralMaterial const * FindStructuralMaterial(ColorKey const & colorKey) const
    {
        auto srchIt = mStructuralMaterialMap.find(colorKey);
        if (srchIt != mStructuralMaterialMap.end())
        {
            return &(srchIt->second);
        }

        // Check whether it's a rope endpoint
        if (colorKey[0] == mRopeMaterialBaseColorKey[0]
            && ((colorKey[1] & 0xF0) == (mRopeMaterialBaseColorKey[1] & 0xF0)))
        {
            return &mRopeMaterial;
        }

        // No luck
        return nullptr;
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

private:

    MaterialDatabase(
        std::map<ColorKey, StructuralMaterial> structuralMaterialMap,
        std::map<ColorKey, ElectricalMaterial> electricalMaterialMap)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
        , mElectricalMaterialMap(std::move(electricalMaterialMap))
        , mRopeMaterialBaseColorKey(GetRopeMaterialColorKey(mStructuralMaterialMap))
        , mRopeMaterial(GetRopeMaterial(mStructuralMaterialMap))
    {
    }

    static ColorKey const & GetRopeMaterialColorKey(std::map<ColorKey, StructuralMaterial> const & structuralMaterialMap)
    {
        for (auto const & entry : structuralMaterialMap)
        {
            if (entry.second.IsRope)
                return entry.first;
        }

        assert(false);
        throw GameException("Shouldn't be here!");
    }

    static StructuralMaterial const & GetRopeMaterial(std::map<ColorKey, StructuralMaterial> const & structuralMaterialMap)
    {
        for (auto const & entry : structuralMaterialMap)
        {
            if (entry.second.IsRope)
                return entry.second;
        }

        assert(false);
        throw GameException("Shouldn't be here!");
    }

    std::map<ColorKey, StructuralMaterial> mStructuralMaterialMap;
    std::map<ColorKey, ElectricalMaterial> mElectricalMaterialMap;
    ColorKey const mRopeMaterialBaseColorKey;
    StructuralMaterial const & mRopeMaterial;
};
