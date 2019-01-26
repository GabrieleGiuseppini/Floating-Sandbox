/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/dialog.h>

class ShipLoadDialog : public wxDialog
{
public:

    ShipLoadDialog(wxWindow* parent);

	virtual ~ShipLoadDialog();

	void Open();

private:

	wxWindow * const mParent;
};
