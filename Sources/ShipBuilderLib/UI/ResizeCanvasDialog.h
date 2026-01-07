/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-01-07
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "BaseResizeDialog.h"
#include "ShipCanvasResizeVisualizationControl.h"

#include <UILib/BitmapToggleButton.h>
#include <UILib/EditSpinBox.h>
#include <wx/textctrl.h>

#include <array>
#include <memory>
#include <optional>

namespace ShipBuilder {

class ResizeCanvasDialog final : public BaseResizeDialog
{
public:

    static std::unique_ptr<ResizeCanvasDialog> Create(
        wxWindow * parent,
        GameAssetManager const & gameAssetManager)
    {
        auto dlg = std::unique_ptr<ResizeCanvasDialog>(new ResizeCanvasDialog());
        dlg->CreateLayout(
            parent,
            _("Resize Ship"),
            gameAssetManager);

        return dlg;
    }

    IntegralRectSize GetTargetSize() const;

    // Position in final buffer of bottom-left corner wrt. bottom-left corner of target
    IntegralCoordinates GetOffset() const;

protected:

    void InternalCreateLayout(
        wxBoxSizer * dialogVSizer,
        GameAssetManager const & gameAssetManager) override;

    void InternalReconciliateUI(
        RgbaImageData const & image,
        ShipSpaceSize const & shipSize) override;

    void InternalOnClose() override;

private:

    ResizeCanvasDialog()
        : BaseResizeDialog()
        , mShipSize(0, 0) // Will be set later
    {
    }

    void ReconciliateUIWithAnchorCoordinates(std::optional<IntegralCoordinates> const & anchorCoordinates);

private:

    wxTextCtrl * mSourceWidthTextCtrl;
    wxTextCtrl * mSourceHeightTextCtrl;
    EditSpinBox<int> * mTargetWidthSpinBox;
    EditSpinBox<int> * mTargetHeightSpinBox;
    BitmapToggleButton * mTargetSizeDimensionLockButton;
    std::array<wxToggleButton *, 9> mAnchorButtons;
    ShipCanvasResizeVisualizationControl * mShipCanvasResizeVisualizationControl;

    ShipSpaceSize mShipSize;
};

}