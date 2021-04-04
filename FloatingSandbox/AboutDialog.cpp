/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-03-12
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AboutDialog.h"

#include "CreditsPanel.h"

#include <GameCore/SysSpecifics.h>
#include <GameCore/Version.h>

#include <wx/sizer.h>

AboutDialog::AboutDialog(wxWindow * parent)
{
    wxString dialogTitle;
    dialogTitle.Printf(_("About %s"), std::string(APPLICATION_NAME_WITH_SHORT_VERSION));

    Create(
        parent,
        wxID_ANY,
        dialogTitle,
        wxDefaultPosition,
        wxSize(780, 620),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    //
    // Setup dialog
    //

    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

    // Credits panel
    {
        CreditsPanel * creditsPanel = new CreditsPanel(this);

        mainSizer->Add(
            creditsPanel,
            1,
            wxEXPAND,
            0);
    }

    SetSizer(mainSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

AboutDialog::~AboutDialog()
{
}