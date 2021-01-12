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

    // Application and build info
    std::string application;
    std::string buildInfo;
    {
        application = std::string(APPLICATION_NAME_WITH_LONG_VERSION);

#if defined(FS_ARCHITECTURE_ARM_32)
        buildInfo += " ARM 32-bit";
#elif defined(FS_ARCHITECTURE_ARM_64)
        buildInfo += " ARM 64-bit";
#elif defined(FS_ARCHITECTURE_X86_32)
        buildInfo += " x86 32-bit";
#elif defined(FS_ARCHITECTURE_X86_64)
        buildInfo += " x86 64-bit";
#else
        buildInfo += " <ARCH?>";
#endif

#if defined(FS_OS_LINUX)
        buildInfo += " Linux";
#elif defined(FS_OS_MACOS)
        buildInfo += " MacOS";
#elif defined(FS_OS_WINDOWS)
        buildInfo += " Windows";
#else
        buildInfo += " <OS?>";
#endif

        buildInfo += " (" __DATE__ ")";
    }

    //
    // Setup dialog
    //

    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

    // Credits panel
    {
        CreditsPanel * creditsPanel = new CreditsPanel(this, application, buildInfo);

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