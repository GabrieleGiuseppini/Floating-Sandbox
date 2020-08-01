/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "AudioController.h"
#include "MusicController.h"

#include <Game/IGameController.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>
#include <GameCore/Version.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

/*
 * This class manages UI preferences, and takes care of persisting and loading them.
 *
 * The class also serves as the storage for some of the preferences.
 */
class UIPreferencesManager
{
public:

    UIPreferencesManager(
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<MusicController> musicController,
        ResourceLocator const & resourceLocator);

    ~UIPreferencesManager();

    static std::optional<int> LoadPreferredLanguage();

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

    ShipAutoTexturizationSettings const & GetShipAutoTexturizationDefaultSettings() const
    {
        return mGameController->GetShipAutoTexturizationDefaultSettings();
    }

    ShipAutoTexturizationSettings & GetShipAutoTexturizationDefaultSettings()
    {
        return mGameController->GetShipAutoTexturizationDefaultSettings();
    }

    void SetShipAutoTexturizationDefaultSettings(ShipAutoTexturizationSettings const & settings)
    {
        mGameController->SetShipAutoTexturizationDefaultSettings(settings);
    }

    bool GetShipAutoTexturizationForceDefaultsOntoShipDefinition() const
    {
        return mGameController->GetShipAutoTexturizationDoForceDefaultSettingsOntoShipSettings();
    }

    void SetShipAutoTexturizationForceDefaultsOntoShipDefinition(bool value)
    {
        mGameController->SetShipAutoTexturizationDoForceDefaultSettingsOntoShipSettings(value);
    }

    bool GetShowShipDescriptionsAtShipLoad() const
    {
        return mShowShipDescriptionsAtShipLoad;
    }

    void SetShowShipDescriptionsAtShipLoad(bool value)
    {
        mShowShipDescriptionsAtShipLoad = value;
    }

    bool GetDoAutoZoomAtShipLoad() const
    {
        return mGameController->GetDoAutoZoomOnShipLoad();
    }

    void SetDoAutoZoomAtShipLoad(bool value)
    {
        mGameController->SetDoAutoZoomOnShipLoad(value);
    }

    bool GetDoShowTsunamiNotifications() const
    {
        return mGameController->GetDoShowTsunamiNotifications();
    }

    void SetDoShowTsunamiNotifications(bool value)
    {
        mGameController->SetDoShowTsunamiNotifications(value);
    }

    bool GetAutoShowSwitchboard() const
    {
        return mAutoShowSwitchboard;
    }

    void SetAutoShowSwitchboard(bool value)
    {
        mAutoShowSwitchboard = value;
    }

    int GetSwitchboardBackgroundBitmapIndex() const
    {
        return mSwitchboardBackgroundBitmapIndex;
    }

    void SetSwitchboardBackgroundBitmapIndex(int value)
    {
        mSwitchboardBackgroundBitmapIndex = value;
    }

    bool GetDoShowElectricalNotifications() const
    {
        return mGameController->GetDoShowElectricalNotifications();
    }

    void SetDoShowElectricalNotifications(bool value)
    {
        mGameController->SetDoShowElectricalNotifications(value);
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

    //
    // Sounds
    //

    bool GetGlobalMute() const
    {
        return AudioController::GetGlobalMute();
    }

    void SetGlobalMute(bool value)
    {
        AudioController::SetGlobalMute(value);
        mGameController->NotifySoundMuted(value);
    }

    float GetBackgroundMusicVolume() const
    {
        return mMusicController->GetBackgroundMusicVolume();
    }

    void SetBackgroundMusicVolume(float value)
    {
        mMusicController->SetBackgroundMusicVolume(value);
    }

    bool GetPlayBackgroundMusic() const
    {
        return mMusicController->GetPlayBackgroundMusic();
    }

    void SetPlayBackgroundMusic(bool value)
    {
        mMusicController->SetPlayBackgroundMusic(value);
    }

    float GetGameMusicVolume() const
    {
        return mMusicController->GetGameMusicVolume();
    }

    void SetGameMusicVolume(float value)
    {
        mMusicController->SetGameMusicVolume(value);
    }

    bool GetPlaySinkingMusic() const
    {
        return mMusicController->GetPlaySinkingMusic();
    }

    void SetPlaySinkingMusic(bool value)
    {
        mMusicController->SetPlaySinkingMusic(value);
    }

private:

    static std::optional<picojson::object> LoadPreferencesRootObject();

    void LoadPreferences();

    void SavePreferences() const;    

private:

    std::filesystem::path const mDefaultShipLoadDirectory;

    // The owners/storage of our properties
    std::shared_ptr<IGameController> mGameController;
    std::shared_ptr<MusicController> mMusicController;

    //
    // The preferences for which we are the owners/storage
    //

    std::vector<std::filesystem::path> mShipLoadDirectories;

    std::filesystem::path mScreenshotsFolderPath;

    std::vector<Version> mBlacklistedUpdates;
    bool mCheckUpdatesAtStartup;
    bool mShowStartupTip;
    bool mSaveSettingsOnExit;
    bool mShowShipDescriptionsAtShipLoad;
    bool mAutoShowSwitchboard;
    int mSwitchboardBackgroundBitmapIndex;

    float mZoomIncrement;
    float mPanIncrement;
};
