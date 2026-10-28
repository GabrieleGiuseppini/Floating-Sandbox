/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-28
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/Materials.h>

#include <wx/dialog.h>

#include <memory>
#include <vector>

namespace ShipBuilder {

class CustomMaterialsImportPickDialog : public wxDialog
{
public:

    CustomMaterialsImportPickDialog(
        wxWindow * parent,
        std::vector<std::unique_ptr<StructuralMaterial const>> const & structuralCustomMaterials,
        std::vector<bool> const & duplicateStructuralCustomMaterials,
        std::vector<std::unique_ptr<ElectricalMaterial const>> const & electricalCustomMaterials,
        std::vector<bool> const & duplicateElectricalCustomMaterials,
        std::vector<std::unique_ptr<StructuralMaterial const>> const & ropesCustomMaterials,
        std::vector<bool> const & duplicateRopesCustomMaterials);

private:

};

}