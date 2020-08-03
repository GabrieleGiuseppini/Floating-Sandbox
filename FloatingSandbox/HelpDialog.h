/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "LocalizationManager.h"

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>

class HelpDialog : public wxDialog
{
public:

    HelpDialog(
        wxWindow* parent,
        ResourceLocator const & resourceLocator,
        LocalizationManager const & localizationManager);

	virtual ~HelpDialog();
};
