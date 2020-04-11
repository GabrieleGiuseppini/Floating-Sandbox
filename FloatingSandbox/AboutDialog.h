/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>
#include <wx/scrolwin.h>

class AboutDialog : public wxDialog
{
public:

    AboutDialog(
        wxWindow* parent,
        ResourceLocator const & resourceLocator);

	virtual ~AboutDialog();

    void Open();

private:

	void OnClose(wxCloseEvent& event);

private:

	wxWindow * const mParent;

    wxScrolled<wxPanel> * mCreditsPanel;

	DECLARE_EVENT_TABLE()
};
