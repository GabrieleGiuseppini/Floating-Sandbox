/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/IGameController.h>

#include <GameCore/Version.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <memory>
#include <vector>

/*
 * This class manages UI preferences, and takes care of persisting and loading them.
 *
 * The class also serves as the storage for some of the preferences.
 */
class UIPreferencesManager
{
public:

    UIPreferencesManager(std::shared_ptr<IGameController> gameController);

    ~UIPreferencesManager();

public:

    std::vector<std::filesystem::path> const & GetShipLoadDirectories() const
    {
        return mShipLoadDirectories;
    }

    void AddShipLoadDirectory(std::filesystem::path shipLoadDirectory)
    {
        // We always have the default ship directory in the first position
        assert(mShipLoadDirectories.size() >= 1);

        if (shipLoadDirectory != mShipLoadDirectories[0])
        {
            // Check if we have one already
            for (auto it = mShipLoadDirectories.begin(); it != mShipLoadDirectories.end(); ++it)
            {
                if (*it == shipLoadDirectory)
                {
                    // Move it to second place
                    std::rotate(std::next(mShipLoadDirectories.begin()), it, it + 1);

                    return;
                }
            }

            // Add to second place
            mShipLoadDirectories.insert(std::next(mShipLoadDirectories.cbegin()), shipLoadDirectory);
        }
    }

    std::filesystem::path const & GetScreenshotsFolderPath() const
    {
        return mScreenshotsFolderPath;
    }

    void SetScreenshotsFolderPath(std::filesystem::path screenshotsFolderPath)
    {
        mScreenshotsFolderPath = std::move(screenshotsFolderPath);
    }

    bool GetCheckUpdatesAtStartup() const
    {
        return mCheckUpdatesAtStartup;
    }

    void SetCheckUpdatesAtStartup(bool value)
    {
        mCheckUpdatesAtStartup = value;
    }

    bool IsUpdateBlacklisted(Version const & version) const
    {
        return std::find(
            mBlacklistedUpdates.cbegin(),
            mBlacklistedUpdates.cend(),
            version) != mBlacklistedUpdates.cend();
    }

    void AddUpdateToBlacklist(Version const & version)
    {
        if (std::find(
            mBlacklistedUpdates.cbegin(),
            mBlacklistedUpdates.cend(),
            version) == mBlacklistedUpdates.cend())
        {
            mBlacklistedUpdates.push_back(version);
        }
    }

    void RemoveUpdateFromBlacklist(Version const & version)
    {
        mBlacklistedUpdates.erase(
            std::remove(
                mBlacklistedUpdates.begin(),
                mBlacklistedUpdates.end(),
                version));
    }

    void ResetUpdateBlacklist()
    {
        mBlacklistedUpdates.clear();
    }

    bool GetShowStartupTip() const
    {
        return mShowStartupTip;
    }

    void SetShowStartupTip(bool value)
    {
        mShowStartupTip = value;
    }

    bool GetSaveSettingsOnExit() const
    {
        return mSaveSettingsOnExit;
    }

    void SetSaveSettingsOnExit(bool value)
    {
        mSaveSettingsOnExit = value;
    }

    bool GetShowShipDescriptionsAtShipLoad() const
    {
        return mShowShipDescriptionsAtShipLoad;
    }

    void SetShowShipDescriptionsAtShipLoad(bool value)
    {
        mShowShipDescriptionsAtShipLoad = value;
    }

    bool GetShowTsunamiNotifications() const
    {
        return mGameController->GetShowTsunamiNotifications();
    }

    void SetShowTsunamiNotifications(bool value)
    {
        mGameController->SetShowTsunamiNotifications(value);
    }

    float GetZoomIncrement() const
    {
        return mZoomIncrement;
    }

    void SetZoomIncrement(float value)
    {
        mZoomIncrement = value;
    }

    float GetPanIncrement() const
    {
        return mPanIncrement;
    }

    void SetPanIncrement(float value)
    {
        mPanIncrement = value;
    }

	bool GetShowStatusText() const
	{
		return mGameController->GetShowStatusText();
	}

	void SetShowStatusText(bool value)
	{
		mGameController->SetShowStatusText(value);
	}

	bool GetShowExtendedStatusText() const
	{
		return mGameController->GetShowExtendedStatusText();
	}

	void SetShowExtendedStatusText(bool value)
	{
		mGameController->SetShowExtendedStatusText(value);
	}

private:

    void LoadPreferences();

    void SavePreferences() const;

private:

    std::filesystem::path const mDefaultShipLoadDirectory;

    // The owners/storage of our properties
    std::shared_ptr<IGameController> mGameController;

    //
    // The preferences for which we are the owners/storage

    std::vector<std::filesystem::path> mShipLoadDirectories;

    std::filesystem::path mScreenshotsFolderPath;

    std::vector<Version> mBlacklistedUpdates;
    bool mCheckUpdatesAtStartup;
    bool mShowStartupTip;
    bool mSaveSettingsOnExit;
    bool mShowShipDescriptionsAtShipLoad;

    float mZoomIncrement;
    float mPanIncrement;
};
