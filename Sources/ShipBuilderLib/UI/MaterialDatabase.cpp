/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialDatabase.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/GameAssetManager.h>
#include <Game/GameVersion.h>

#include <Core/Log.h>
#include <Core/Utils.h>

#include <picojson.h>

#include <algorithm>

namespace ShipBuilder {

template<LayerType TLayer>
MaterialDatabase<TLayer>::MaterialDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap)
    : mDefaultMaterialColorMap(defaultMaterialColorMap)
    , mUserCustomMaterials(LoadDefaultUserDatabase(defaultMaterialColorMap))
    , mShipCustomMaterials()
{
}

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::Pod MaterialDatabase<TLayer>::LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap)
{
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

    SaveUserMaterials(
        materials,
        GetDefaultUserDatabaseFilePath());
}

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::Pod MaterialDatabase<TLayer>::LoadUserDatabase(
    std::filesystem::path const & databaseFilepath,
    MaterialColorMap<TMaterial> const & defaultMaterialColorMap)
{
    if (!std::filesystem::exists(databaseFilepath))
    {
        return Pod();
    }

    auto const rootJsonValue = GameAssetManager::LoadJson(databaseFilepath);

    if (!rootJsonValue.is<picojson::object>())
    {
        LogMessage("ERROR: root of user database file \"", databaseFilepath, "\" is not a json object");
        return Pod();
    }

    picojson::object const & rootJsonObject = rootJsonValue.get<picojson::object>();

    // De-serialize materials

    Pod pod;

    picojson::array const & materialsJsonArray = Utils::GetMandatoryJsonArray(rootJsonObject, "custom_materials");
    for (auto const & materialJsonValue : materialsJsonArray)
    {
        picojson::object const & materialJsonObject = Utils::GetJsonValueAsObject(materialJsonValue, "custom_materials");

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

    return pod;
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::SaveUserMaterials(
    std::vector<TMaterial const *> const & materials,
    std::filesystem::path const & databaseFilepath)
{
    picojson::object rootJsonObject;

    // Add version
    rootJsonObject["_version"] = picojson::value(CurrentGameVersion.ToString());

    // Add timestamp
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

    rootJsonObject["custom_materials"] = picojson::value(materialsJsonArray);

    GameAssetManager::SaveJson(
        picojson::value(rootJsonObject),
        databaseFilepath);
}

template<LayerType TLayer>
std::filesystem::path MaterialDatabase<TLayer>::GetDefaultUserDatabaseFilePath()
{
    return StandardSystemPaths::GetInstance().GetUserGameRootFolderPath() / (std::string("shipbuilder_user_custom_") + LayerTypeToStr_snake(TLayer) + "_materials.json");
}

//
// Explicit specializations for all layers
//

template class MaterialDatabase<LayerType::Structural>;
template class MaterialDatabase<LayerType::Electrical>;
template class MaterialDatabase<LayerType::Ropes>;

}