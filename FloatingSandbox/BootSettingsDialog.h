/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-01-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <GameCore/BootSettings.h>

#include <wx/checkbox.h>
#include <wx/dialog.h>

class BootSettingsDialog : public wxDialog
{
public:

    BootSettingsDialog(
        wxWindow* parent,
        ResourceLocator const & resourceLocator);

    virtual ~BootSettingsDialog();

private:

    void PopulateCheckboxes(BootSettings const & settings);

    void OnRevertToDefaultsButton(wxCommandEvent & event);
    void OnSaveAndQuitButton(wxCommandEvent & event);

    wxCheckBox * mDoForceNoGlFinishCheckBox;
    wxCheckBox * mDoForceNoMultithrededRenderingCheckBox;

private:

    ResourceLocator const & mResourceLocator;
};
