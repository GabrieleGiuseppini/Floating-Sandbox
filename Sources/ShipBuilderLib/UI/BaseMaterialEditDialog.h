/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once


#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

#include <optional>

namespace ShipBuilder {

template<typename TMaterial>
class BaseMaterialEditDialog : public wxDialog
{
public:

    std::optional<typename TMaterial::VariantOverridesType> RunForNew(typename TMaterial::VariantOverridesType & overrides, TMaterial const * baseMaterial);
    std::optional<typename TMaterial::VariantOverridesType> RunForEdit(typename TMaterial::VariantOverridesType & overrides, TMaterial const * baseMaterial);

protected:

    static int constexpr DialogWidth = 300;
    static int constexpr DialogHeight = 600;

    static int constexpr MarginSize = 4;
    static int constexpr SliderWidth = 72; // Min

    static int constexpr VMarginAroundButtons = 10;
    static int constexpr MarginAroundCells = 8;

protected:

    BaseMaterialEditDialog(wxWindow * parent);

    void LayoutBase();

    virtual void LayoutControls(wxBoxSizer * dialogVSizer) = 0;

    virtual std::optional<typename TMaterial::VariantOverridesType> Run(typename TMaterial::VariantOverridesType & overrides, TMaterial const * baseMaterial) = 0;

    std::optional<typename TMaterial::VariantOverridesType> BaseRun();

protected:

    //
    // UI
    //

    // Main panel
    wxPanel * mMainPanel;

    // Shared controls (created by derived)
    wxTextCtrl * mNameTextCtrl;

    // Buttons
    wxButton * mOkButton;
    wxButton * mCancelButton;

    //
    // State
    //

    std::optional<typename TMaterial::VariantOverridesType> mOverridesUnderEdit;
};

}