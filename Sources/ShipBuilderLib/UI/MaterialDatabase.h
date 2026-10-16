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

	TMaterial const * AddUserCustomMaterial(std::unique_ptr<TMaterial> && customMaterial);

	std::vector<std::unique_ptr<typename TMaterial const>> const & GetShipCustomMaterials() const
	{
		return mShipCustomMaterials;
	}

	MaterialReplacementMap<TMaterial> SetShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> && shipMaterials);

private:

	using SinglePod = typename std::vector<std::unique_ptr<typename TMaterial const>>;

	SinglePod LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap) const;

	void SaveDefaultUserDatabase() const;

	static SinglePod LoadUserDatabase(
		std::filesystem::path const & databaseFilepath,
		MaterialColorMap<TMaterial> const & defaultMaterialColorMap);

	static void SaveUserMaterials(
		std::vector<TMaterial const *> const & materials,
		std::filesystem::path const & databaseFilepath);

	static picojson::object LoadUserDatabaseJson(std::filesystem::path const & databaseFilepath);

	std::filesystem::path GetDefaultUserDatabaseFilePath() const;

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