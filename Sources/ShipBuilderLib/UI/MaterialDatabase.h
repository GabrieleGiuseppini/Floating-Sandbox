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

private:

	using Pod = typename std::vector<std::unique_ptr<typename TMaterial>>;

	static Pod LoadDefaultUserDatabase();

	static Pod LoadUserDatabase(std::filesystem::path const & databaseFilepath);

	static std::filesystem::path GetDefaultUserDatabaseFilePath();

private:

	MaterialColorMap<TMaterial> const & mDefaultMaterialColorMap; // For color_key lookups
	Pod mUserCustomMaterials;
};

}