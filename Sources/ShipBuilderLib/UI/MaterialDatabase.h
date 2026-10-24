/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/GameTypes.h>

#include <picojson.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace ShipBuilder {

template<LayerType TLayer>
class MaterialDatabase final
{
public:

	using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

	MaterialDatabase(
		MaterialColorMap<TMaterial> const & defaultMaterialColorMap,
		std::filesystem::path userGameRootFolderPath);

	std::vector<std::unique_ptr<typename TMaterial const>> const & GetUserCustomMaterials() const
	{
		return mUserCustomMaterials;
	}

	std::vector<std::unique_ptr<typename TMaterial const>> const & GetShipCustomMaterials() const
	{
		return mShipCustomMaterials;
	}

	TMaterial const * AddUserCustomMaterial(std::unique_ptr<TMaterial const> && customMaterial);

	void AddUserCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && customMaterials);

	void AddShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && customMaterials);

	MaterialReplacementMap<TMaterial> SetShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && shipMaterials);

	std::unique_ptr<TMaterial const> ReplaceCustomMaterial(
		TMaterial const * oldMaterial,
		std::unique_ptr<TMaterial const> && newMaterial);

	std::tuple<std::vector<std::unique_ptr<typename TMaterial const>>, std::vector<std::unique_ptr<typename TMaterial const>>> ExtractCustomMaterials(std::vector<TMaterial const *> const & materials);

private:

	using SinglePod = typename std::vector<std::unique_ptr<typename TMaterial const>>;

	SinglePod LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap) const;

	void SaveDefaultUserDatabase() const;

	static SinglePod LoadUserDatabase(
		std::filesystem::path const & databaseFilepath,
		MaterialColorMap<TMaterial> const & defaultMaterialColorMap);

	static void SaveUserMaterials(
		SinglePod const & materials,
		std::filesystem::path const & databaseFilepath);

	static picojson::object LoadUserDatabaseJson(std::filesystem::path const & databaseFilepath);

	std::filesystem::path MakeDefaultUserDatabaseFilePath() const;
	std::filesystem::path MakeDefaultUserDatabaseBackupFilePath() const;

	static std::string MakeLayerTypeMaterialsNodeName()
	{
		return "custom_" + LayerTypeToStr_snake(TLayer) + "_materials";
	}

private:

	std::filesystem::path const mUserGameRootFolderPath;

	MaterialColorMap<TMaterial> const & mDefaultMaterialColorMap; // For color_key lookups
	SinglePod mUserCustomMaterials;
	SinglePod mShipCustomMaterials;
};

}