/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialDatabase.h"

#include <Game/GameAssetManager.h>
#include <Game/GameVersion.h>

#include <Core/Log.h>
#include <Core/Utils.h>

#include <algorithm>
#include <set>

namespace ShipBuilder {

template<LayerType TLayer>
MaterialDatabase<TLayer>::MaterialDatabase(
    MaterialColorMap<TMaterial> const & defaultMaterialColorMap,
    std::filesystem::path userGameRootFolderPath)
    : mUserGameRootFolderPath(userGameRootFolderPath)
    , mDefaultMaterialColorMap(defaultMaterialColorMap)
    , mUserCustomMaterials(LoadDefaultUserDatabase(defaultMaterialColorMap))
    , mShipCustomMaterials()
{
}

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::TMaterial const * MaterialDatabase<TLayer>::AddUserCustomMaterial(std::unique_ptr<TMaterial> && customMaterial)
{
    mUserCustomMaterials.emplace_back(std::move(customMaterial));

    SaveDefaultUserDatabase();

    return mUserCustomMaterials.back().get();
}

template<LayerType TLayer>
MaterialReplacementMap<typename MaterialDatabase<TLayer>::TMaterial> MaterialDatabase<TLayer>::SetShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && shipMaterials)
{
    //
    // We do not want users reloading their ships after editing to get duplicate materials between user's and ship's,
    // as that would be confusing, and lead to two copies that may be edited independently.
    //
    // Se here we prune the ship materials with those already among the user's,
    // and return enough information to replace those in the ship
    //

    MaterialReplacementMap<typename MaterialDatabase<TLayer>::TMaterial> replacementMap;

    // Nuke all ship materials
    mShipCustomMaterials.clear();

    // Find if dupe
    for (auto & shipMaterial : shipMaterials)
    {
        assert(shipMaterial->VariantOverrides.has_value());

        TMaterial const * equivalentUserMaterial = nullptr;
        for (auto const & userMaterial : mUserCustomMaterials)
        {
            assert(userMaterial->VariantOverrides.has_value());

            if (userMaterial->ColorKey == shipMaterial->ColorKey
                && userMaterial->VariantOverrides == shipMaterial->VariantOverrides)
            {
                equivalentUserMaterial = userMaterial.get();
                break;
            }
        }

        if (equivalentUserMaterial)
        {
            // Ignore this one; add to replacement map
            auto const & [entry, isInserted] = replacementMap.try_emplace(
                shipMaterial.get(),
                equivalentUserMaterial);

            assert(isInserted);
        }
        else
        {
            // Store this one
            mShipCustomMaterials.emplace_back(std::move(shipMaterial));
        }
    }

    return replacementMap;
}

//////////////////////////////////////////////////

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::SinglePod MaterialDatabase<TLayer>::LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap) const
{
    // TODO: backup mechanism

    return LoadUserDatabase(GetDefaultUserDatabaseFilePath(), defaultMaterialColorMap);
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::SaveDefaultUserDatabase() const
{
    std::vector<TMaterial const *> materials;
    std::transform(
        mUserCustomMaterials.cbegin(),
        mUserCustomMaterials.cend(),
        std::back_inserter(materials),
        [](auto const & uq_ptr) -> TMaterial const *
        {
            return uq_ptr.get();
        });

    // TODO: backup mechanism

    SaveUserMaterials(
        materials,
        GetDefaultUserDatabaseFilePath());
}

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::SinglePod MaterialDatabase<TLayer>::LoadUserDatabase(
    std::filesystem::path const & databaseFilepath,
    MaterialColorMap<TMaterial> const & defaultMaterialColorMap)
{
    picojson::object const rootJsonObject = LoadUserDatabaseJson(databaseFilepath);

    // De-serialize materials for this layer

    SinglePod pod;

    std::optional<picojson::array> const & materialsJsonArray = Utils::GetOptionalJsonArray(rootJsonObject, MakeLayerTypeMaterialsNodeName());
    if (materialsJsonArray.has_value())
    {
        for (auto const & materialJsonValue : *materialsJsonArray)
        {
            picojson::object const & materialJsonObject = Utils::GetJsonValueAsObject(materialJsonValue, "custom_material");

            rgbColor const baseColorKey = Utils::Hex2RgbColor(Utils::GetMandatoryJsonMember<std::string>(materialJsonObject, "base_color_key"));
            typename TMaterial::VariantOverridesType const variantOverrides = TMaterial::VariantOverridesType::Deserialize(Utils::GetMandatoryJsonValue(materialJsonObject, "variant_overrides"));

            // Lookup material
            auto const srchIt = defaultMaterialColorMap.find(baseColorKey);
            if (srchIt != defaultMaterialColorMap.cend())
            {
                TMaterial const & baseMaterial = srchIt->second;
                pod.emplace_back(baseMaterial.MakeCustomMaterial(variantOverrides));
            }
        }
    }

    return pod;
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::SaveUserMaterials(
    std::vector<TMaterial const *> const & materials,
    std::filesystem::path const & databaseFilepath)
{
    picojson::object rootJsonObject = LoadUserDatabaseJson(databaseFilepath);

    // Set version
    rootJsonObject["_version"] = picojson::value(CurrentGameVersion.ToString());

    // Set timestamp
    rootJsonObject["_saved_at"] = picojson::value(Utils::MakeNowDateAndTimeString());

    // Materials

    picojson::array materialsJsonArray;
    for (TMaterial const * material : materials)
    {
        assert(material->VariantOverrides.has_value()); // Or else wouldn't be a custom material

        picojson::object materialJsonObject;
        materialJsonObject["base_color_key"] = picojson::value(Utils::RgbColor2Hex(material->ColorKey));
        materialJsonObject["variant_overrides"] = material->VariantOverrides->Serialize();

        materialsJsonArray.push_back(picojson::value(materialJsonObject));
    }

    rootJsonObject[MakeLayerTypeMaterialsNodeName()] = picojson::value(materialsJsonArray);

    GameAssetManager::SaveJson(
        picojson::value(rootJsonObject),
        databaseFilepath);
}

template<LayerType TLayer>
picojson::object MaterialDatabase<TLayer>::LoadUserDatabaseJson(std::filesystem::path const & databaseFilepath)
{
    if (!std::filesystem::exists(databaseFilepath))
    {
        return picojson::object();
    }

    auto const rootJsonValue = GameAssetManager::LoadJson(databaseFilepath);

    if (!rootJsonValue.is<picojson::object>())
    {
        LogMessage("ERROR: root of user database file \"", databaseFilepath, "\" is not a json object");
        return picojson::object();
    }

    return rootJsonValue.get<picojson::object>();
}

template<LayerType TLayer>
std::filesystem::path MaterialDatabase<TLayer>::GetDefaultUserDatabaseFilePath() const
{
    return mUserGameRootFolderPath / std::string("shipbuilder_user_custom_materials.json");
}

//
// Explicit specializations for all layers
//

template class MaterialDatabase<LayerType::Structural>;
template class MaterialDatabase<LayerType::Electrical>;
template class MaterialDatabase<LayerType::Ropes>;

}