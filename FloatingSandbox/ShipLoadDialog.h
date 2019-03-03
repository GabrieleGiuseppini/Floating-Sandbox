/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewPanel.h"
#include "UIPreferences.h"

#include <Game/ResourceLoader.h>

#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/dirctrl.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

class ShipLoadDialog : public wxDialog
{
public:

    ShipLoadDialog(
        wxWindow* parent,
        std::shared_ptr<UIPreferences> uiPreferences,
        ResourceLoader const & resourceLoader);

	virtual ~ShipLoadDialog();

    void Open();

private:

    void OnDirCtrlDirSelected(wxCommandEvent & event);
    void OnShipFileSelected(fsShipFileSelectedEvent & event);
    void OnShipFileChosen(fsShipFileChosenEvent & event);
    void OnRecentDirectorySelected(wxCommandEvent & event);
    void OnHomeDirButtonClicked(wxCommandEvent & event);
    void OnLoadButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnCloseWindow(wxCloseEvent & event);

    void OnDirectorySelected(std::filesystem::path directoryPath);
    void OnShipFileChosen(std::filesystem::path shipFilepath);

private:

    void Close();
    void RepopulateRecentDirectoriesComboBox();

private:

	wxWindow * const mParent;
    std::shared_ptr<UIPreferences> mUIPreferences;

    wxGenericDirCtrl * mDirCtrl;
    ShipPreviewPanel * mShipPreviewPanel;
    wxComboBox * mRecentDirectoriesComboBox;
    wxButton * mLoadButton;

private:

    std::optional<std::filesystem::path> mSelectedShipFilepath;
};
