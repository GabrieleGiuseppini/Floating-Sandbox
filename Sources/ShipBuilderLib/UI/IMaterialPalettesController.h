/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-10-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Simulation/Materials.h>

#include <vector>

namespace ShipBuilder {

/*
 * Interface of MaterialPalettesController to its UI subordinate.
 */
struct IMaterialPalettesController
{
    virtual bool IsAnyCustomMaterialChecked() const = 0;

    virtual void OnStructuralMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) = 0;
    virtual void OnElectricalMaterialSelected(ElectricalMaterial const * material, MaterialPlaneType plane) = 0;
    virtual void OnRopesMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) = 0;

    virtual void OnNewUserCustomStructuralMaterial(StructuralMaterial const * baseMaterial) = 0;
    virtual void OnNewUserCustomElectricalMaterial(ElectricalMaterial const * baseMaterial) = 0;
    virtual void OnNewUserCustomRopesMaterial(StructuralMaterial const * baseMaterial) = 0;

    virtual void OnEditCustomStructuralMaterial(StructuralMaterial const * customMaterial) = 0;
    virtual void OnEditCustomElectricalMaterial(ElectricalMaterial const * customMaterial) = 0;
    virtual void OnEditCustomRopesMaterial(StructuralMaterial const * customMaterial) = 0;

    virtual void OnRemoveCustomStructuralMaterials(std::vector<StructuralMaterial const *> customMaterials) = 0;
    virtual void OnRemoveCustomElectricalMaterials(std::vector<ElectricalMaterial const *> customMaterials) = 0;
    virtual void OnRemoveCustomRopesMaterials(std::vector<StructuralMaterial const *> customMaterials) = 0;

    virtual void OnImportCustomStructuralShipMaterials(std::vector<StructuralMaterial const *> customMaterials) = 0;
    virtual void OnImportCustomElectricalShipMaterials(std::vector<ElectricalMaterial const *> customMaterials) = 0;
    virtual void OnImportCustomRopesShipMaterials(std::vector<StructuralMaterial const *> customMaterials) = 0;

    virtual void OnExportCheckedCustomMaterials() const = 0;
};

}