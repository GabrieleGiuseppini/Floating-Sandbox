/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipLoadDialog.h"

ShipLoadDialog::ShipLoadDialog(wxWindow * parent)
	: mParent(parent)
{
	Create(
		mParent,
		wxID_ANY,
		_("Load Ship"),
		wxDefaultPosition,
        wxSize(800, 250),
		wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxFRAME_SHAPED,
		_T("Load Ship Dialog"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}

ShipLoadDialog::~ShipLoadDialog()
{
}

void ShipLoadDialog::Open()
{
	this->Show();
}