/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-10-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Simulation/Materials.h>

namespace ShipBuilder {

/*
 * Interface of MaterialPalettesController to its UI subordinate.
 */
struct IMaterialPalettesController
{
    virtual void OnStructuralMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) = 0;
    virtual void OnElectricalMaterialSelected(ElectricalMaterial const * material, MaterialPlaneType plane) = 0;
    virtual void OnRopesMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) = 0;
};

}