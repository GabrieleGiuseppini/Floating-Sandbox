/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Simulation/CustomMaterialsPod.h>
#include <Simulation/DefaultMaterialDatabase.h>
#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/GameTypes.h>

#include <picojson.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace ShipBuilder {

class BaseMaterialDatabase
{
public:

	static void ExportCustomMaterials(
		std::vector<StructuralMaterial const *> const & customStructuralMaterials,
		std::vector<ElectricalMaterial const *> const & customElectricalMaterials,
		std::vector<StructuralMaterial const *> const & customRopesMaterials,
		std::filesystem::path const & databaseFilepath);

	static CustomMaterialsPod LoadCustomMaterialsFile(
		std::filesystem::path const & databaseFilepath,
		DefaultMaterialDatabase const & defaultMaterialDatabase);

protected:

	static void InitializeDatabaseFile(picojson::object & rootJsonObject);

	template<typename TMaterialOrUqPtr, LayerType TLayer>
	static void AppendMaterialsToDatabaseFile(
		std::vector<TMaterialOrUqPtr> const & materials,
		picojson::object & rootJsonObject);

	template<LayerType TLayer>
	static std::string MakeLayerTypeMaterialsNodeName()
	{
		return "custom_" + LayerTypeToStr_snake(TLayer) + "_materials";
	}

	static picojson::object LoadDatabaseJson(std::filesystem::path const & databaseFilepath);

	template<LayerType TLayer>
	static std::vector<std::unique_ptr<typename LayerTypeTraits<TLayer>::material_type const>> LoadCustomMaterials(
		picojson::object const & rootJsonObject,
		MaterialColorMap<typename LayerTypeTraits<TLayer>::material_type> const & defaultMaterialColorMap);
};

template<LayerType TLayer>
class MaterialDatabase final : public BaseMaterialDatabase
{
public:

	using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

	MaterialDatabase(
		MaterialColorMap<TMaterial> const & defaultMaterialColorMap,
		std::filesystem::path userGameRootFolderPath);

	std::vector<std::unique_ptr<TMaterial const>> const & GetUserCustomMaterials() const
	{
		return mUserCustomMaterials;
	}

	std::vector<std::unique_ptr<TMaterial const>> const & GetShipCustomMaterials() const
	{
		return mShipCustomMaterials;
	}

	std::vector<bool> DetectEquivalentUserCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> const & customMaterials) const;

	TMaterial const * AddUserCustomMaterial(std::unique_ptr<TMaterial const> && customMaterial);

	void AddUserCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && customMaterials);

	void AddShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && customMaterials);

	MaterialReplacementMap<TMaterial> SetShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && shipMaterials);

	std::unique_ptr<TMaterial const> ReplaceCustomMaterial(
		TMaterial const * oldMaterial,
		std::unique_ptr<TMaterial const> && newMaterial);

	std::tuple<std::vector<std::unique_ptr<TMaterial const>>, std::vector<std::unique_ptr<TMaterial const>>> ExtractCustomMaterials(std::vector<TMaterial const *> const & materials);

	void MoveShipCustomMaterialsToUser(std::vector<TMaterial const *> const & shipMaterials);
	void MoveUserCustomMaterialsToShip(std::vector<TMaterial const *> const & userMaterials);

private:

	static bool AreCustomMaterialsEquivalent(TMaterial const & material1, TMaterial const & material2)
	{
		assert(material1.VariantOverrides.has_value());
		assert(material2.VariantOverrides.has_value());

		return material1.ColorKey == material2.ColorKey
			&& material1.VariantOverrides == material2.VariantOverrides;
	}

	using SinglePod = typename std::vector<std::unique_ptr<TMaterial const>>;

	SinglePod LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap) const;

	void SaveDefaultUserDatabase() const;

	static SinglePod LoadUserDatabase(
		std::filesystem::path const & databaseFilepath,
		MaterialColorMap<TMaterial> const & defaultMaterialColorMap);

	static void SaveUserMaterials(
		SinglePod const & materials,
		std::filesystem::path const & databaseFilepath);

	std::filesystem::path MakeDefaultUserDatabaseFilePath() const;
	std::filesystem::path MakeDefaultUserDatabaseBackupFilePath() const;

private:

	std::filesystem::path const mUserGameRootFolderPath;

	MaterialColorMap<TMaterial> const & mDefaultMaterialColorMap; // For color_key lookups
	SinglePod mUserCustomMaterials;
	SinglePod mShipCustomMaterials;
};

}