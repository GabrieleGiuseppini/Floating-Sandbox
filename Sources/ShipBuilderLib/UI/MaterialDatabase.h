/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace ShipBuilder {

template<LayerType TLayer>
class MaterialDatabase final
{
public:

	using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

	MaterialDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap);

	std::vector<std::unique_ptr<typename TMaterial const>> const & GetUserCustomMaterials() const
	{
		return mUserCustomMaterials;
	}

	TMaterial const * AddUserCustomMaterial(std::unique_ptr<TMaterial> && customMaterial);

private:

	using SinglePod = typename std::vector<std::unique_ptr<typename TMaterial const>>;

	static SinglePod LoadDefaultUserDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap);
	void SaveDefaultUserDatabase() const;

	static SinglePod LoadUserDatabase(
		std::filesystem::path const & databaseFilepath,
		MaterialColorMap<TMaterial> const & defaultMaterialColorMap);
	static void SaveUserMaterials(
		std::vector<TMaterial const *> const & materials,
		std::filesystem::path const & databaseFilepath);

	static std::filesystem::path GetDefaultUserDatabaseFilePath();

private:

	MaterialColorMap<TMaterial> const & mDefaultMaterialColorMap; // For color_key lookups
	SinglePod mUserCustomMaterials;
	SinglePod mShipCustomMaterials;
};

}