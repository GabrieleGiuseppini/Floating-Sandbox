/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WorkbenchState.h"

#include <cassert>

namespace ShipBuilder {

WorkbenchState::WorkbenchState(MaterialDatabase const & materialDatabase)
{
    // Default structural foreground material: first structural material
    assert(!materialDatabase.GetStructuralMaterialPalette().Categories.empty()
        && !materialDatabase.GetStructuralMaterialPalette().Categories[0].SubCategories.empty()
        && !materialDatabase.GetStructuralMaterialPalette().Categories[0].SubCategories[0].Materials.empty());
    mForegroundStructuralMaterial = &(materialDatabase.GetStructuralMaterialPalette().Categories[0].SubCategories[0].Materials[0].get());

    // Default structural background material: none
    mBackgroundStructuralMaterial = nullptr;

    // Default electrical foreground material: first electrical material
    assert(!materialDatabase.GetElectricalMaterialPalette().Categories.empty()
        && !materialDatabase.GetElectricalMaterialPalette().Categories[0].SubCategories.empty()
        && !materialDatabase.GetElectricalMaterialPalette().Categories[0].SubCategories[0].Materials.empty());
    mForegroundElectricalMaterial = &(materialDatabase.GetElectricalMaterialPalette().Categories[0].SubCategories[0].Materials[0].get());

    // Default electrical background material: none
    mBackgroundElectricalMaterial = nullptr;
}

}