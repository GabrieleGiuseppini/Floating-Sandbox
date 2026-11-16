/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <wx/wx.h>

#include <cassert>

class SharedUIResources
{
public:

    static void Initialize(GameAssetManager const & gameAssetManager);

    static SharedUIResources const & GetInstance()
    {
        assert(sInstance != nullptr);
        return *sInstance;
    }

    wxBitmap const & GetAddNewMaterialPlusIcon() const
    {
        assert(mAddNewMaterialPlusIcon.IsOk());
        return mAddNewMaterialPlusIcon;
    }

    wxBitmap const & GetCheckboxOnIcon() const
    {
        assert(mCheckboxOnIcon.IsOk());
        return mCheckboxOnIcon;
    }

    wxBitmap const & GetDockedIconLarge() const
    {
        assert(mDockIconLarge.IsOk());
        return mDockIconLarge;
    }

    wxBitmap const & GetUndockedIconLarge() const
    {
        assert(mUndockIconLarge.IsOk());
        return mUndockIconLarge;
    }

    wxBrush const & GetWhiteSolidBackgroundBrush() const
    {
        return mWhiteSolidBackgroundBrush;
    }

    wxBrush const & GetSeparatorBrush() const
    {
        return mSeparatorBrush;
    }

    wxBitmap const & GetPencilIconBase() const
    {
        assert(mPencilIconBase.IsOk());
        return mPencilIconBase;
    }

    wxBitmap const & GetPencilIconSelected() const
    {
        assert(mPencilIconSelected.IsOk());
        return mPencilIconSelected;
    }

    wxBitmap const & GetEraserIconBase() const
    {
        assert(mEraserIconBase.IsOk());
        return mEraserIconBase;
    }

    wxBitmap const & GetEraserIconSelected() const
    {
        assert(mEraserIconSelected.IsOk());
        return mEraserIconSelected;
    }

    wxBitmap const & GetLineIconBase() const
    {
        assert(mLineIconBase.IsOk());
        return mLineIconBase;
    }

    wxBitmap const & GetLineIconSelected() const
    {
        assert(mLineIconSelected.IsOk());
        return mLineIconSelected;
    }

    wxBitmap const & GetSamplerIconBase() const
    {
        assert(mSamplerIconBase.IsOk());
        return mSamplerIconBase;
    }

    wxBitmap const & GetSamplerIconSelected() const
    {
        assert(mSamplerIconSelected.IsOk());
        return mSamplerIconSelected;
    }

    wxBitmap const & GetSelectionIconBase() const
    {
        assert(mSelectionIconBase.IsOk());
        return mSelectionIconBase;
    }

    wxBitmap const & GetSelectionIconSelected() const
    {
        assert(mSelectionIconSelected.IsOk());
        return mSelectionIconSelected;
    }

    wxBitmap const & GetPasteIconBase() const
    {
        assert(mPasteIconBase.IsOk());
        return mPasteIconBase;
    }

    wxBitmap const & GetPasteIconSelected() const
    {
        assert(mPasteIconSelected.IsOk());
        return mPasteIconSelected;
    }

    wxBitmap const & GetTranslateIconBase() const
    {
        assert(mTranslateIconBase.IsOk());
        return mTranslateIconBase;
    }

    wxBitmap const & GetTranslateIconSelected() const
    {
        assert(mTranslateIconSelected.IsOk());
        return mTranslateIconSelected;
    }

private:

    static SharedUIResources *sInstance;

    wxBitmap mAddNewMaterialPlusIcon;
    wxBitmap mCheckboxOnIcon;
    wxBitmap mDockIconLarge;
    wxBitmap mUndockIconLarge;
    wxBrush mWhiteSolidBackgroundBrush;
    wxBrush mSeparatorBrush;

    wxBitmap mPencilIconBase;
    wxBitmap mPencilIconSelected;
    wxBitmap mEraserIconBase;
    wxBitmap mEraserIconSelected;
    wxBitmap mLineIconBase;
    wxBitmap mLineIconSelected;
    wxBitmap mSamplerIconBase;
    wxBitmap mSamplerIconSelected;
    wxBitmap mSelectionIconBase;
    wxBitmap mSelectionIconSelected;
    wxBitmap mPasteIconBase;
    wxBitmap mPasteIconSelected;
    wxBitmap mTranslateIconBase;
    wxBitmap mTranslateIconSelected;
};
