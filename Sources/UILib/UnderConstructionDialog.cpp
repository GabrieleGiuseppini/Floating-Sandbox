/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-24
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UnderConstructionDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/sizer.h>
#include <wx/statbmp.h>

UnderConstructionDialog::UnderConstructionDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
{
	Create(
        parent,
		wxID_ANY,
		_("Under Construction"),
		wxDefaultPosition,
        wxSize(600, 600),
		wxCAPTION | wxCLOSE_BOX);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    {
        auto temp = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", gameAssetManager));
        dialogVSizer->Add(temp, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
    }

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void UnderConstructionDialog::Show(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
{
    UnderConstructionDialog dlg(parent, gameAssetManager);
    dlg.ShowModal();
}