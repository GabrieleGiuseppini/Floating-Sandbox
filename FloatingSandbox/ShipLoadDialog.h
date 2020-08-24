/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewWindow.h"
#include "UIPreferencesManager.h"

#include <Game/ResourceLocator.h>

#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/dirctrl.h>
#include <wx/srchctrl.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

class ShipLoadDialog : public wxDialog
{
public:

    ShipLoadDialog(
        wxWindow* parent,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
        std::shared_ptr<ResourceLocator> const & resourceLocator);

	virtual ~ShipLoadDialog();

    virtual int ShowModal() override;

    std::filesystem::path GetChosenShipFilepath() const
    {
        assert(!!mChosenShipFilepath);
        return *mChosenShipFilepath;
    }

private:

    void OnDirCtrlDirSelected(wxCommandEvent & event);
    void OnShipFileSelected(fsShipFileSelectedEvent & event);
    void OnShipFileChosen(fsShipFileChosenEvent & event);
    void OnRecentDirectorySelected(wxCommandEvent & event);
    void OnShipSearchCtrlText(wxCommandEvent & event);
    void OnShipSearchCtrlSearchBtn(wxCommandEvent & event);
    void OnShipSearchCtrlCancelBtn(wxCommandEvent & event);
    void OnSearchNextButtonClicked(wxCommandEvent & event);
    void OnHomeDirButtonClicked(wxCommandEvent & event);
    void OnInfoButtonClicked(wxCommandEvent & event);
    void OnLoadButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnCloseWindow(wxCloseEvent & event);

    void OnDirectorySelected(std::filesystem::path directoryPath);
    void OnShipFileChosen(std::filesystem::path shipFilepath);

private:

    virtual void EndModal(int retCode) override;

    void StartShipSearch();
    void RepopulateRecentDirectoriesComboBox();

private:

	wxWindow * const mParent;
    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
    std::shared_ptr<ResourceLocator> mResourceLocator;

    wxGenericDirCtrl * mDirCtrl;
    ShipPreviewWindow * mShipPreviewWindow;
    wxComboBox * mRecentDirectoriesComboBox;
    wxSearchCtrl * mShipSearchCtrl;
    wxButton * mInfoButton;
    wxButton * mLoadButton;
    wxButton * mSearchNextButton;

private:

    std::optional<ShipMetadata> mSelectedShipMetadata;
    std::optional<std::filesystem::path> mSelectedShipFilepath;
    std::optional<std::filesystem::path> mChosenShipFilepath;
};
