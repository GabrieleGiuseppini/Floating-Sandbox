/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameLib/ResourceLoader.h>

#include <wx/dialog.h>
#include <wx/textctrl.h>

class AboutDialog : public wxDialog
{
public:

    AboutDialog(
        wxWindow* parent,
        ResourceLoader const & resourceLoader);
	
	virtual ~AboutDialog();

    void Open();

private:

	void OnClose(wxCloseEvent& event);

private:

	wxWindow * const mParent;

    wxTextCtrl * mTextCtrl;

	DECLARE_EVENT_TABLE()
};
