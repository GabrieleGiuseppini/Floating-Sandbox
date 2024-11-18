/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../Controller.h"
#include "../WorkbenchState.h"

#include <UILib/EditSpinBox.h>

#include <Game/ResourceLocator.h>

#include <wx/bmpcbox.h>
#include <wx/clrpicker.h>
#include <wx/dialog.h>

#include <optional>

namespace ShipBuilder {

class PreferencesDialog : public wxDialog
{
public:

    PreferencesDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void ShowModal(
        Controller & controller,
        WorkbenchState const & workbenchState);

private:

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnCloseWindow(wxCloseEvent & event);

    void OnCancel();
    void OnCanvasBackgroundColorSelected(rgbColor const & color);

    void ReconciliateUI(WorkbenchState const & workbenchState);

private:

    EditSpinBox<int> * mNewShipSizeWidthSpinBox;
    EditSpinBox<int> * mNewShipSizeHeightSpinBox;
    wxColourPickerCtrl * mCanvasBackgroundColorColourPicker;
    wxBitmapComboBox * mPresetColorsComboBox;

private:

    struct SessionData
    {
        Controller & BuilderController;
        rgbColor const OriginalCanvasBackgroundColor;

        SessionData(
            Controller & controller,
            rgbColor const & originalCanvasBackgroundColor)
            : BuilderController(controller)
            , OriginalCanvasBackgroundColor(originalCanvasBackgroundColor)
        {}
    };

    std::optional<SessionData> mSessionData;
};

}