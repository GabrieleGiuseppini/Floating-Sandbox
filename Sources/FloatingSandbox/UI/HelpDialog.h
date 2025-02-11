/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <UILib/LocalizationManager.h>

#include <Game/GameAssetManager.h>

#include <wx/dialog.h>

class HelpDialog : public wxDialog
{
public:

    HelpDialog(
        wxWindow* parent,
        GameAssetManager const & gameAssetManager,
        LocalizationManager const & localizationManager);

	virtual ~HelpDialog();
};
