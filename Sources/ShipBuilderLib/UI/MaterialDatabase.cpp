/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialDatabase.h"

#include <UILib/StandardSystemPaths.h>

#include <Core/Utils.h>

namespace ShipBuilder {

template<LayerType TLayer>
MaterialDatabase<TLayer>::MaterialDatabase(MaterialColorMap<TMaterial> const & defaultMaterialColorMap)
    : mDefaultMaterialColorMap(defaultMaterialColorMap)
    , mUserCustomMaterials(LoadDefaultUserDatabase())
{
}

template<LayerType TLayer>
MaterialDatabase<TLayer>::Pod MaterialDatabase<TLayer>::LoadDefaultUserDatabase()
{
    // TODOHERE
}

template<LayerType TLayer>
MaterialDatabase<TLayer>::Pod MaterialDatabase<TLayer>::LoadUserDatabase(std::filesystem::path const & databaseFilepath)
{
    // TODOHERE
}

template<LayerType TLayer>
std::filesystem::path MaterialDatabase<TLayer>::GetDefaultUserDatabaseFilePath()
{
    // TODO: layer name
    return StandardSystemPaths::GetInstance().GetUserGameRootFolderPath() / "shipbuilder_preferences.json";
}

//
// Explicit specializations for all layers
//

template class MaterialDatabase<LayerType::Structural>;
template class MaterialDatabase<LayerType::Electrical>;
template class MaterialDatabase<LayerType::Ropes>;

}