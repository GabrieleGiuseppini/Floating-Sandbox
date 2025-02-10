/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "ShipResizeVisualizationControl.h"

#include <UILib/BitmapToggleButton.h>
#include <UILib/EditSpinBox.h>

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>

#include <wx/dialog.h>
#include <wx/textctrl.h>

#include <array>

namespace ShipBuilder {

class ResizeDialog : public wxDialog
{
public:

    ResizeDialog(
        wxWindow * parent,
        GameAssetManager const & gameAssetManager);

    bool ShowModalForResize(
        RgbaImageData const & image,
        IntegralRectSize const & targetSize);

    bool ShowModalForTexture(
        RgbaImageData const & image,
        IntegralRectSize const & targetSize);

    IntegralRectSize GetTargetSize() const;

    // Position in final buffer of bottom-left corner wrt. bottom-left corner of target
    IntegralCoordinates GetOffset() const;

private:

    enum class ModeType
    {
        ForResize,
        ForTexture
    };

    using wxDialog::ShowModal;

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void ReconciliateUIWithAnchorCoordinates(std::optional<IntegralCoordinates> const & anchorCoordinates);

    void ReconciliateUI(
        RgbaImageData const & image,
        IntegralRectSize const & targetSize,
        ModeType mode);

private:

    GameAssetManager const & mGameAssetManager;

    wxTextCtrl * mSourceWidthTextCtrl;
    wxTextCtrl * mSourceHeightTextCtrl;
    EditSpinBox<int> * mTargetWidthSpinBox;
    EditSpinBox<int> * mTargetHeightSpinBox;
    BitmapToggleButton * mTargetSizeDimensionLockButton;
    std::array<wxToggleButton *, 9> mAnchorButtons;
    ShipResizeVisualizationControl * mShipResizeVisualizationControl;

    IntegralRectSize mSourceSize;
};

}