/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-17
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AskPasswordDialog.h"

// TODO
#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

bool AskPasswordDialog::CheckPasswordProtectedEdit(
    ShipDefinition const & shipDefinition,
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
{
    if (!shipDefinition.Metadata.Password.has_value())
        return true;

    // TODOHERE
}

AskPasswordDialog::AskPasswordDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("Provide Password"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // TODOHERE

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

}