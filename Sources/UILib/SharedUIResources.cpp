/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SharedUIResources.h"

#include "WxHelpers.h"

SharedUIResources * SharedUIResources::sInstance = nullptr;

void SharedUIResources::Initialize(GameAssetManager const & gameAssetManager)
{
	sInstance = new SharedUIResources();
	sInstance->mAddNewMaterialPlusIcon = WxHelpers::LoadBitmap("add_new_material_plus_icon", gameAssetManager);
	sInstance->mWhiteSolidBackgroundBrush = wxBrush(wxColour("WHITE"), wxBRUSHSTYLE_SOLID);
}