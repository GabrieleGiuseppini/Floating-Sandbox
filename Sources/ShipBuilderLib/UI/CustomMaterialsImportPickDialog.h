/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-28
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/sizer.h>

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

    std::vector<bool> const & GetCheckedStructuralMaterials() const
    {
        return mCheckedStructuralMaterials;
    }

    std::vector<bool> GetCheckedElectricalMaterials() const
    {
        return mCheckedElectricalMaterials;
    }

    std::vector<bool> GetCheckedRopesMaterials() const
    {
        return mCheckedRopesMaterials;
    }

private:

    template<typename TMaterial>
    wxSizer * MakeSelectionPanel(
        std::vector<std::unique_ptr<TMaterial const>> const & customMaterials,
        std::vector<bool> const & isDuplicate,
        wxString const & title,
        std::vector<bool> & checkedMaterialsVector);

    void ReconciliateUIWithSelection();

    wxButton * mClearSelectionButton;

    std::vector<wxCheckBox *> mCheckboxes;
    std::vector<std::vector<bool>::iterator> mCheckboxCheckedFlags;

    int mCheckedMaterialsCount;
    std::vector<bool> mCheckedStructuralMaterials;
    std::vector<bool> mCheckedElectricalMaterials;
    std::vector<bool> mCheckedRopesMaterials;
};

}