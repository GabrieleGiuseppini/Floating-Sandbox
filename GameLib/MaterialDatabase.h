/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-04
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameException.h"
#include "Material.h"
#include "Utils.h"

#include <picojson/picojson.h>

#include <array>
#include <cassert>
#include <map>
#include <memory>
#include <vector>

class MaterialDatabase
{
public:
        
    static MaterialDatabase Create(picojson::value const & root)
    {
        std::vector<std::unique_ptr<Material const>> materials;

        if (!root.is<picojson::array>())
        {
            throw GameException("Materials database is not a JSON array");
        }

        picojson::array rootArray = root.get<picojson::array>();
        for (auto const & rootElem : rootArray)
        {
            if (!rootElem.is<picojson::object>())
            {
                throw GameException("Found a non-array in materials database");
            }

            materials.emplace_back(Material::Create(rootElem.get<picojson::object>()));
        }

        return Create(std::move(materials));
    }

    static MaterialDatabase Create(std::vector<std::unique_ptr<Material const>> materials)
    {
        std::map<std::array<uint8_t, 3u>, std::unique_ptr<Material const>> materialsMap;
        Material const * ropeMaterial = nullptr;

        for (auto & material : materials)
        {
            if (material->IsRope)
            {
                // Make sure we've only got one rope material
                if (nullptr != ropeMaterial)
                {
                    throw GameException("More than one Rope material found in materials database");
                }

                // Make sure structural color is black
                static std::array<uint8_t, 3u> BlackColour = { 0x00, 0x00, 0x00 };
                if (material->StructuralColourRgb != BlackColour)
                {
                    throw GameException("Rope material must have black as its structural colour");
                }

                ropeMaterial = material.get();
            }
            else
            {
                // Make sure no clash with Indexed Rope colours
                if (0x00 == material->StructuralColourRgb[0]
                    && 0 == (material->StructuralColourRgb[1] & 0xF0))
                {
                    throw GameException("Material \"" + material->Name + "\" has a structural colour that is reserved for ropes and rope endpoints");
                }
            }

            // Add to map
            auto result = materialsMap.emplace(material->StructuralColourRgb, std::move(material));

            // Make sure no duplicate colour
            if (!result.second)
            {
                throw GameException("Material \"" + material->Name + "\" has a duplicate structural colour");
            }
        }

        // Make sure we've got one rope material
        if (nullptr == ropeMaterial)
        {
            throw GameException("No Rope material found in materials database");
        }

        return MaterialDatabase(
            std::move(materialsMap),
            *ropeMaterial);
    }

    Material const * Get(std::array<uint8_t, 3u> const & structuralColourRgb) const
    {
        auto srchIt = mMaterialsMap.find(structuralColourRgb);
        if (srchIt != mMaterialsMap.end())
        {
            return srchIt->second.get();
        }
        else
        {
            return nullptr;
        }
    }

    Material const & GetRopeMaterial() const
    {
        return mRopeMaterial;
    }

private:

    MaterialDatabase(
        std::map<std::array<uint8_t, 3u>, std::unique_ptr<Material const>> && materialsMap,
        Material const & ropeMaterial)
        : mMaterialsMap(std::move(materialsMap))
        , mRopeMaterial(ropeMaterial)
    {
    }

    std::map<std::array<uint8_t, 3u>, std::unique_ptr<Material const>> mMaterialsMap;
    Material const & mRopeMaterial;
};
