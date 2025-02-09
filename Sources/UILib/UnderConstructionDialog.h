/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-24
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <wx/dialog.h>

class UnderConstructionDialog : public wxDialog
{
public:

	static void Show(wxWindow * parent, GameAssetManager const & gameAssetManager);

private:

	UnderConstructionDialog(wxWindow * parent, GameAssetManager const & gameAssetManager);
};
