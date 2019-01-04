/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ResourceLoader.h>

#include <wx/dialog.h>

class HelpDialog : public wxDialog
{
public:

    HelpDialog(
        wxWindow* parent,
        ResourceLoader const & resourceLoader);

	virtual ~HelpDialog();
};
