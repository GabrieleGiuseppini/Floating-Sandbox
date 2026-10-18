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

private:

    static SharedUIResources *sInstance;

    wxBitmap mAddNewMaterialPlusIcon;
    wxBitmap mCheckboxOnIcon;
    wxBitmap mDockIconLarge;
    wxBitmap mUndockIconLarge;
    wxBrush mWhiteSolidBackgroundBrush;
    wxBrush mSeparatorBrush;
};
