/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewPanel.h"
#include "UIPreferences.h"

#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/dirctrl.h>

#include <memory>

class ShipLoadDialog : public wxDialog
{
public:

    ShipLoadDialog(
        wxWindow* parent,
        std::shared_ptr<UIPreferences> uiPreferences);

	virtual ~ShipLoadDialog();

private:

    void OnDirectorySelected(wxCommandEvent & event);

private:

	wxWindow * const mParent;
    std::shared_ptr<UIPreferences> mUIPreferences;

    wxGenericDirCtrl * mDirCtrl;
    ShipPreviewPanel * mShipPreviewPanel;
    wxComboBox * mDirectoriesComboBox;
};
