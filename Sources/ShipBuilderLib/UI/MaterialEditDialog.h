/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>

#include <optional>
#include <string>

namespace ShipBuilder {

template<typename TMaterial>
class MaterialEditDialog : public wxDialog
{
public:

    using TVariantOverridesType = typename TMaterial::VariantOverridesType;

    MaterialEditDialog(wxWindow * parent);

    std::optional<TVariantOverridesType> RunForNew(TVariantOverridesType & overrides, TMaterial const * baseMaterial);
    std::optional<TVariantOverridesType> RunForEdit(TVariantOverridesType & overrides, TMaterial const * baseMaterial);

private:

    std::optional<TVariantOverridesType> Run(TVariantOverridesType & overrides, TMaterial const * baseMaterial);

private:
};

}