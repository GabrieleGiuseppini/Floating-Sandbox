/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewWindow.h"

#include <Game/ResourceLocator.h>
#include <Game/ShipLoadSpecifications.h>

#include <UILib/BitmapButton.h>
#include <UILib/BitmapToggleButton.h>

#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/dirctrl.h>
#include <wx/popupwin.h>
#include <wx/srchctrl.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

enum class ShipLoadDialogUsageType
{
    ForGame,
    ForShipBuilder
};

template<ShipLoadDialogUsageType TUsageType>
class ShipLoadDialog : public wxDialog
{
public:

    ShipLoadDialog(
        wxWindow* parent,
        ResourceLocator const & resourceLocator);

    virtual ~ShipLoadDialog();

    int ShowModal(std::vector<std::filesystem::path> const & shipLoadDirectories);

    auto GetChosenShip() const
    {
        assert(mChosenShipFilepath.has_value());

        if constexpr (TUsageType == ShipLoadDialogUsageType::ForGame)
        {
            assert(mFlipHButton);
            assert(mFlipVButton);
            assert(mRotate90CWButton);

            return ShipLoadSpecifications(
                *mChosenShipFilepath,
                ShipLoadOptions(
                mFlipHButton->GetValue(),
                mFlipVButton->GetValue(),
                mRotate90CWButton->GetValue()));
        }
        else
        {
            static_assert(TUsageType == ShipLoadDialogUsageType::ForShipBuilder);
            return *mChosenShipFilepath;
        }
    }

private:

    void OnDirCtrlDirSelected(wxCommandEvent & event);
    void OnShipFileSelected(ShipPreviewWindow::fsShipFileSelectedEvent & event);
    void OnShipFileChosen(ShipPreviewWindow::fsShipFileChosenEvent & event);
    void OnRecentDirectorySelected(wxCommandEvent & event);
    void OnShipSearchCtrlText(wxCommandEvent & event);
    void OnShipSearchCtrlSearchBtn(wxCommandEvent & event);
    void OnShipSearchCtrlCancelBtn(wxCommandEvent & event);
    void OnSearchNextButtonClicked(wxCommandEvent & event);
    void OnStandardHomeDirButtonClicked(wxCommandEvent & event);
    void OnUserHomeDirButtonClicked(wxCommandEvent & event);
    void OnSortMethodChanged(ShipPreviewWindow::SortMethod sortMethod);
    void OnSortDirectionChanged(bool isSortDescending);
    void OnInfoButtonClicked(wxCommandEvent & event);
    void OnLoadButton(wxCommandEvent & event);
    void OnLoadRandomButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnCloseWindow(wxCloseEvent & event);

    void OnDirectorySelected(std::filesystem::path directoryPath);
    void OnShipFileChosen(std::filesystem::path shipFilepath);

private:

    using wxDialog::ShowModal;
    virtual void EndModal(int retCode) override;

    void ReconciliateUIWithSortMethod();
    void StartShipSearch();
    void RepopulateRecentDirectoriesComboBox(std::vector<std::filesystem::path> const & shipLoadDirectories);

private:

    wxWindow * const mParent;
    ResourceLocator const & mResourceLocator;

    wxBitmap mSortByNameIcon;
    wxBitmap mSortByLastModifiedIcon;
    wxBitmap mSortByYearBuiltIcon;
    wxBitmap mSortByFeaturesIcon;
    wxBitmap mSortAscendingIcon;
    wxBitmap mSortDescendingIcon;

    wxGenericDirCtrl * mDirCtrl;
    wxBitmapButton * mInfoButton;
    wxBitmapButton * mPasswordProtectedButton;
    BitmapToggleButton * mFlipHButton;
    BitmapToggleButton * mFlipVButton;
    BitmapToggleButton * mRotate90CWButton;
    wxPopupTransientWindow * mSortMethodSelectionPopupWindow;
    wxBitmapButton * mSortMethodButton;
    wxBitmapButton * mSortDirectionButton;
    wxSearchCtrl * mShipSearchCtrl;
    ShipPreviewWindow * mShipPreviewWindow;
    wxComboBox * mRecentDirectoriesComboBox;
    wxButton * mLoadButton;
    wxBitmapButton * mLoadRandomButton;
    wxBitmapButton * mSearchNextButton;

    wxDialog * mPasswordProtectionInfoDialog;

private:

    std::filesystem::path const mStandardInstalledShipFolderPath;
    std::filesystem::path const mUserShipFolderPath;

    std::optional<ShipMetadata> mSelectedShipMetadata;
    std::optional<std::filesystem::path> mSelectedShipFilepath;
    std::optional<std::filesystem::path> mChosenShipFilepath;
};
