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

#include <set>

namespace ShipBuilder {

void BaseMaterialDatabase::ExportCustomMaterials(
    std::vector<StructuralMaterial const *> const & customStructuralMaterials,
    std::vector<ElectricalMaterial const *> const & customElectricalMaterials,
    std::vector<StructuralMaterial const *> const & customRopesMaterials,
    std::filesystem::path const & databaseFilepath)
{
    picojson::object rootJsonObject;

    // Initialize DB file
    InitializeDatabaseFile(rootJsonObject);

    // Append materials
    AppendMaterialsToDatabaseFile<StructuralMaterial const *, LayerType::Structural>(customStructuralMaterials, rootJsonObject);
    AppendMaterialsToDatabaseFile<ElectricalMaterial const *, LayerType::Electrical>(customElectricalMaterials, rootJsonObject);
    AppendMaterialsToDatabaseFile<StructuralMaterial const *, LayerType::Ropes>(customRopesMaterials, rootJsonObject);

    // Save
    GameAssetManager::SaveJson(
        picojson::value(rootJsonObject),
        databaseFilepath);
}

CustomMaterialsPod BaseMaterialDatabase::LoadCustomMaterialsFile(
    std::filesystem::path const & databaseFilepath,
    DefaultMaterialDatabase const & defaultMaterialDatabase)
{
    picojson::object const rootJsonObject = LoadDatabaseJson(databaseFilepath);

    return CustomMaterialsPod(
        LoadCustomMaterials<LayerType::Structural>(rootJsonObject, defaultMaterialDatabase.GetStructuralMaterialColorMap()),
        LoadCustomMaterials<LayerType::Electrical>(rootJsonObject, defaultMaterialDatabase.GetElectricalMaterialColorMap()),
        LoadCustomMaterials<LayerType::Ropes>(rootJsonObject, defaultMaterialDatabase.GetStructuralMaterialColorMap()));
}

void BaseMaterialDatabase::InitializeDatabaseFile(picojson::object & rootJsonObject)
{
    // Set version
    rootJsonObject["_version"] = picojson::value(CurrentGameVersion.ToString());

    // Set timestamp
    rootJsonObject["_saved_at"] = picojson::value(Utils::MakeNowDateAndTimeString());
}

template<typename TMaterialOrUqPtr, LayerType TLayer>
void BaseMaterialDatabase::AppendMaterialsToDatabaseFile(
    std::vector<TMaterialOrUqPtr> const & materials,
    picojson::object & rootJsonObject)
{
    picojson::array materialsJsonArray;
    for (auto const & material : materials)
    {
        assert(material->VariantOverrides.has_value()); // Or else wouldn't be a custom material

        picojson::object materialJsonObject;
        materialJsonObject["base_color_key"] = picojson::value(Utils::RgbColor2Hex(material->ColorKey));
        materialJsonObject["variant_overrides"] = material->VariantOverrides->Serialize();

        materialsJsonArray.push_back(picojson::value(materialJsonObject));
    }

    rootJsonObject[MakeLayerTypeMaterialsNodeName<TLayer>()] = picojson::value(materialsJsonArray);
}

picojson::object BaseMaterialDatabase::LoadDatabaseJson(std::filesystem::path const & databaseFilepath)
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
std::vector<std::unique_ptr<typename LayerTypeTraits<TLayer>::material_type const>> BaseMaterialDatabase::LoadCustomMaterials(
    picojson::object const & rootJsonObject,
    MaterialColorMap<typename LayerTypeTraits<TLayer>::material_type> const & defaultMaterialColorMap)
{
    using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

    // De-serialize materials for this layer

    std::vector<std::unique_ptr<TMaterial const>> pod;

    std::optional<picojson::array> const & materialsJsonArray = Utils::GetOptionalJsonArray(rootJsonObject, MakeLayerTypeMaterialsNodeName<TLayer>());
    if (materialsJsonArray.has_value())
    {
        for (auto const & materialJsonValue : *materialsJsonArray)
        {
            picojson::object const & materialJsonObject = Utils::GetJsonValueAsObject(materialJsonValue, "custom_material");

            rgbColor const baseColorKey = Utils::Hex2RgbColor(Utils::GetMandatoryJsonMember<std::string>(materialJsonObject, "base_color_key"));

            // Lookup material
            auto const srchIt = defaultMaterialColorMap.find(baseColorKey);
            if (srchIt != defaultMaterialColorMap.cend()) // Safety
            {
                TMaterial const & baseMaterial = srchIt->second;

                // Deserialize overrides
                typename TMaterial::VariantOverridesType const variantOverrides = TMaterial::VariantOverridesType::Deserialize(
                    Utils::GetMandatoryJsonValue(materialJsonObject, "variant_overrides"),
                    baseMaterial.MakeStartingVariantOverrides());

                // Store material
                pod.emplace_back(baseMaterial.MakeCustomMaterial(variantOverrides));
            }
        }
    }

    return pod;
}

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
std::vector<bool> MaterialDatabase<TLayer>::DetectEquivalentUserCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> const & customMaterials) const
{
    std::vector<bool> equivalentMarkers(customMaterials.size(), false);

    for (size_t cm = 0; cm < customMaterials.size(); ++cm)
    {
        assert(customMaterials[cm]);

        if (std::find_if(
            mUserCustomMaterials.cbegin(),
            mUserCustomMaterials.cend(),
            [&](auto const & ucm)
            {
                assert(ucm);
                return AreCustomMaterialsEquivalent(*customMaterials[cm], *ucm);
            }) != mUserCustomMaterials.cend())
        {
            equivalentMarkers[cm] = true;
            break;
        }
    }

    return equivalentMarkers;
}

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::TMaterial const * MaterialDatabase<TLayer>::AddUserCustomMaterial(std::unique_ptr<TMaterial const> && customMaterial)
{
    mUserCustomMaterials.emplace_back(std::move(customMaterial));

    SaveDefaultUserDatabase();

    return mUserCustomMaterials.back().get();
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::AddUserCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && customMaterials)
{
    for (auto & uq : customMaterials)
    {
        mUserCustomMaterials.emplace_back(std::move(uq));
    }

    SaveDefaultUserDatabase();
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::AddShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && customMaterials)
{
    for (auto & uq : customMaterials)
    {
        mShipCustomMaterials.emplace_back(std::move(uq));
    }
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
            assert(userMaterial);
            assert(shipMaterial);

            if (AreCustomMaterialsEquivalent(*userMaterial, *shipMaterial))
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

template<LayerType TLayer>
std::unique_ptr<typename MaterialDatabase<TLayer>::TMaterial const> typename MaterialDatabase<TLayer>::ReplaceCustomMaterial(
    TMaterial const * oldMaterial,
    std::unique_ptr<TMaterial const> && newMaterial)
{
    // Find among all

    for (auto & userMaterial : mUserCustomMaterials)
    {
        if (userMaterial.get() == oldMaterial)
        {
            // Found

            // Swap
            auto old = std::move(userMaterial);
            userMaterial = std::move(newMaterial);

            // Save DB
            SaveDefaultUserDatabase();

            return old;
        }
    }

    for (auto & shipMaterial : mShipCustomMaterials)
    {
        if (shipMaterial.get() == oldMaterial)
        {
            // Found
            auto old = std::move(shipMaterial);
            shipMaterial = std::move(newMaterial);
            return old;
        }
    }

    assert(false);
    return std::unique_ptr<TMaterial const>();
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::RemoveUserCustomMaterials(std::vector<TMaterial const *> const & customMaterials)
{
    // Find among user's
    for (auto it = mUserCustomMaterials.begin(); it != mUserCustomMaterials.end(); /*incremented in loop*/)
    {
        auto const srchIt = std::find(
            customMaterials.cbegin(),
            customMaterials.cend(),
            it->get());

        if (srchIt != customMaterials.cend())
        {
            // Found

            it = mUserCustomMaterials.erase(it);
        }
        else
        {
            ++it;
        }
    }

    SaveDefaultUserDatabase();
}

template<LayerType TLayer>
std::tuple<std::vector<std::unique_ptr<typename MaterialDatabase<TLayer>::TMaterial const>>, std::vector<std::unique_ptr<typename MaterialDatabase<TLayer>::TMaterial const>>> MaterialDatabase<TLayer>::ExtractCustomMaterials(std::vector<TMaterial const *> const & materials)
{
    // Find among all

    std::vector<std::unique_ptr<typename MaterialDatabase<TLayer>::TMaterial const>> extractedUserMaterials;
    for (auto it = mUserCustomMaterials.begin(); it != mUserCustomMaterials.end(); /*incremented in loop*/)
    {
        auto const srchIt = std::find(
            materials.cbegin(),
            materials.cend(),
            it->get());

        if (srchIt != materials.cend())
        {
            // Found

            // Extract
            extractedUserMaterials.emplace_back(std::move(*it));
            it = mUserCustomMaterials.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!extractedUserMaterials.empty())
    {
        SaveDefaultUserDatabase();
    }

    std::vector<std::unique_ptr<typename MaterialDatabase<TLayer>::TMaterial const>> extractedShipMaterials;
    for (auto it = mShipCustomMaterials.begin(); it != mShipCustomMaterials.end(); /*incremented in loop*/)
    {
        auto const srchIt = std::find(
            materials.cbegin(),
            materials.cend(),
            it->get());

        if (srchIt != materials.cend())
        {
            // Found

            // Extract
            extractedShipMaterials.emplace_back(std::move(*it));
            it = mShipCustomMaterials.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return std::make_tuple(std::move(extractedUserMaterials), std::move(extractedShipMaterials));
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::MoveShipCustomMaterialsToUser(std::vector<TMaterial const *> const & shipMaterials)
{
    bool isUserDirty = false;
    for (auto it = mShipCustomMaterials.begin(); it != mShipCustomMaterials.end(); /*incremented in loop*/)
    {
        if (std::find(shipMaterials.cbegin(), shipMaterials.cend(), it->get()) != shipMaterials.cend())
        {
            mUserCustomMaterials.emplace_back(std::move(*it));
            it = mShipCustomMaterials.erase(it);
            isUserDirty = true;
        }
        else
        {
            ++it;
        }
    }

    if (isUserDirty)
    {
        SaveDefaultUserDatabase();
    }
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::MoveUserCustomMaterialsToShip(std::vector<TMaterial const *> const & userMaterials)
{
    bool isUserDirty = false;
    for (auto it = mUserCustomMaterials.begin(); it != mUserCustomMaterials.end(); /*incremented in loop*/)
    {
        if (std::find(userMaterials.cbegin(), userMaterials.cend(), it->get()) != userMaterials.cend())
        {
            mShipCustomMaterials.emplace_back(std::move(*it));
            it = mUserCustomMaterials.erase(it);
            isUserDirty = true;
        }
        else
        {
            ++it;
        }
    }

    if (isUserDirty)
    {
        SaveDefaultUserDatabase();
    }
}

//////////////////////////////////////////////////

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::SinglePod MaterialDatabase<TLayer>::LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap) const
{
    std::filesystem::path const defaultUserDatabaseFilePath = MakeDefaultUserDatabaseFilePath();
    if (std::filesystem::exists(defaultUserDatabaseFilePath))
    {
        try
        {
            return LoadUserDatabase(defaultUserDatabaseFilePath, defaultMaterialColorMap);
        }
        catch(std::exception const & ex)
        {
            LogMessage("ERROR loading ShipBuilder's user custom material database: ", ex.what());
        }
    }

    // Try backup
    return LoadUserDatabase(MakeDefaultUserDatabaseBackupFilePath(), defaultMaterialColorMap);
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::SaveDefaultUserDatabase() const
{
    auto const defaultUserDatabaseFilePath = MakeDefaultUserDatabaseFilePath();
    if (std::filesystem::exists(defaultUserDatabaseFilePath))
    {
        // Make backup

        // Delete old backup
        std::filesystem::path const defaultUserDatabaseBackupFilePath = MakeDefaultUserDatabaseBackupFilePath();
        if (std::filesystem::exists(defaultUserDatabaseBackupFilePath))
        {
            std::filesystem::remove(defaultUserDatabaseBackupFilePath);
        }

        // Make new backup
        std::filesystem::copy_file(defaultUserDatabaseFilePath, defaultUserDatabaseBackupFilePath);
    }

    SaveUserMaterials(
        mUserCustomMaterials,
        defaultUserDatabaseFilePath);
}

template<LayerType TLayer>
typename MaterialDatabase<TLayer>::SinglePod MaterialDatabase<TLayer>::LoadUserDatabase(
    std::filesystem::path const & databaseFilepath,
    MaterialColorMap<TMaterial> const & defaultMaterialColorMap)
{
    picojson::object const rootJsonObject = LoadDatabaseJson(databaseFilepath);

    return LoadCustomMaterials<TLayer>(rootJsonObject, defaultMaterialColorMap);
}

template<LayerType TLayer>
void MaterialDatabase<TLayer>::SaveUserMaterials(
    SinglePod const & materials,
    std::filesystem::path const & databaseFilepath)
{
    // Load existing database, if any
    picojson::object rootJsonObject = LoadDatabaseJson(databaseFilepath);

    // Initialize DB file
    InitializeDatabaseFile(rootJsonObject);

    // Append materials
    AppendMaterialsToDatabaseFile<std::unique_ptr<TMaterial const>, TLayer>(materials, rootJsonObject);

    // Save
    GameAssetManager::SaveJson(
        picojson::value(rootJsonObject),
        databaseFilepath);
}

template<LayerType TLayer>
std::filesystem::path MaterialDatabase<TLayer>::MakeDefaultUserDatabaseFilePath() const
{
    return mUserGameRootFolderPath / std::string("shipbuilder_user_custom_materials.json");
}

template<LayerType TLayer>
std::filesystem::path MaterialDatabase<TLayer>::MakeDefaultUserDatabaseBackupFilePath() const
{
    auto const defaultUserDatabaseFilePath = MakeDefaultUserDatabaseFilePath();
    return (defaultUserDatabaseFilePath.parent_path() / defaultUserDatabaseFilePath.filename()).concat(".bak");
}

//
// Explicit specializations for all layers
//

template class MaterialDatabase<LayerType::Structural>;
template class MaterialDatabase<LayerType::Electrical>;
template class MaterialDatabase<LayerType::Ropes>;

}