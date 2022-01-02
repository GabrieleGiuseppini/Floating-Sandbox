/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-24
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>

class UnderConstructionDialog : public wxDialog
{
public:

	static void Show(wxWindow * parent, ResourceLocator const & resourceLocator);

private:

	UnderConstructionDialog(wxWindow * parent, ResourceLocator const & resourceLocator);
};
