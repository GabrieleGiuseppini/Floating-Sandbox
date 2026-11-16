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
	sInstance->mCheckboxOnIcon = WxHelpers::LoadBitmap("checkbox_on_small", gameAssetManager);
	sInstance->mDockIconLarge = WxHelpers::LoadBitmap("dock_undock_dock_icon_large", gameAssetManager);
	sInstance->mUndockIconLarge = WxHelpers::LoadBitmap("dock_undock_undock_icon_large", gameAssetManager);
	sInstance->mWhiteSolidBackgroundBrush = wxBrush(*wxWHITE, wxBRUSHSTYLE_SOLID);
	sInstance->mSeparatorBrush = wxBrush(wxColor(0xa0, 0xa0, 0xa0), wxBRUSHSTYLE_SOLID);

	sInstance->mPencilIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("pencil_icon"));
	sInstance->mPencilIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("pencil_icon"));
	sInstance->mEraserIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("eraser_icon"));
	sInstance->mEraserIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("eraser_icon"));
	sInstance->mLineIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("line_icon"));
	sInstance->mLineIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("line_icon"));
	sInstance->mSamplerIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("sampler_icon"));
	sInstance->mSamplerIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("sampler_icon"));
	sInstance->mSelectionIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("selection_icon"));
	sInstance->mSelectionIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("selection_icon"));
	sInstance->mPasteIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("paste_icon"));
	sInstance->mPasteIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("paste_icon"));
	sInstance->mTranslateIconBase = WxHelpers::MakeBaseButtonBitmap(gameAssetManager.GetIconFilePath("translate_icon"));
	sInstance->mTranslateIconSelected = WxHelpers::MakeSelectedButtonBitmap(gameAssetManager.GetIconFilePath("translate_icon"));
}