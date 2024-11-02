/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "AudioController.h"
#include "MusicController.h"

#include <UILib/LocalizationManager.h>

#include <Game/IGameController.h>
#include <Game/ShipLoadSpecifications.h>

#include <GameCore/GameTypes.h>
#include <GameCore/Version.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
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
        IGameController & gameController,
        MusicController & musicController,
        LocalizationManager & localizationManager);

    ~UIPreferencesManager();

    static std::optional<std::string> LoadPreferredLanguage();

public:

    std::vector<std::filesystem::path> const & GetShipLoadDirectories() const
    {
        return mShipLoadDirectories;
    }

    void AddShipLoadDirectory(std::filesystem::path shipLoadDirectory)
    {
        // Check if it's in already
        if (std::find(mShipLoadDirectories.cbegin(), mShipLoadDirectories.cend(), shipLoadDirectory) == mShipLoadDirectories.cend())
        {
            // Add in front
            mShipLoadDirectories.insert(mShipLoadDirectories.cbegin(), shipLoadDirectory);
        }
    }

    std::optional<ShipLoadSpecifications> const & GetLastShipLoadedSpecifications() const
    {
        return mLastShipLoadedSpecifications;
    }

    void SetLastShipLoadedSpecifications(ShipLoadSpecifications const & lastShipLoadedSpecs)
    {
        mLastShipLoadedSpecifications = lastShipLoadedSpecs;
    }

    bool GetReloadLastLoadedShipOnStartup() const
    {
        return mReloadLastLoadedShipOnStartup;
    }

    void SetReloadLastLoadedShipOnStartup(bool value)
    {
        mReloadLastLoadedShipOnStartup = value;
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

    bool GetStartInFullScreen() const
    {
        return mStartInFullScreen;
    }

    void SetStartInFullScreen(bool value)
    {
        mStartInFullScreen = value;
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

    ShipAutoTexturizationSettings const & GetShipAutoTexturizationSharedSettings() const
    {
        return mGameController.GetShipAutoTexturizationSharedSettings();
    }

    ShipAutoTexturizationSettings & GetShipAutoTexturizationSharedSettings()
    {
        return mGameController.GetShipAutoTexturizationSharedSettings();
    }

    void SetShipAutoTexturizationSharedSettings(ShipAutoTexturizationSettings const & settings)
    {
        mGameController.SetShipAutoTexturizationSharedSettings(settings);
    }

    bool GetShipAutoTexturizationForceSharedSettingsOntoShipDefinition() const
    {
        return mGameController.GetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings();
    }

    void SetShipAutoTexturizationForceSharedSettingsOntoShipDefinition(bool value)
    {
        mGameController.SetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings(value);
    }

    bool GetShowShipDescriptionsAtShipLoad() const
    {
        return mShowShipDescriptionsAtShipLoad;
    }

    void SetShowShipDescriptionsAtShipLoad(bool value)
    {
        mShowShipDescriptionsAtShipLoad = value;
    }

    float GetCameraSpeedAdjustment() const
    {
        return mGameController.GetCameraSpeedAdjustment();
    }

    void SetCameraSpeedAdjustment(float value)
    {
        mGameController.SetCameraSpeedAdjustment(value);
    }

    float GetMinCameraSpeedAdjustment() const
    {
        return mGameController.GetMinCameraSpeedAdjustment();
    }

    float GetMaxCameraSpeedAdjustment() const
    {
        return mGameController.GetMaxCameraSpeedAdjustment();
    }

    bool GetDoAutoFocusOnShipLoad() const
    {
        return mGameController.GetDoAutoFocusOnShipLoad();
    }

    void SetDoAutoFocusOnShipLoad(bool value)
    {
        mGameController.SetDoAutoFocusOnShipLoad(value);
    }

    std::optional<AutoFocusTargetKindType> GetAutoFocusTarget() const
    {
        return mGameController.GetAutoFocusTarget();
    }

    void SetAutoFocusTarget(std::optional<AutoFocusTargetKindType> value)
    {
        mGameController.SetAutoFocusTarget(value);
    }

    bool GetDoShowTsunamiNotifications() const
    {
        return mGameController.GetDoShowTsunamiNotifications();
    }

    void SetDoShowTsunamiNotifications(bool value)
    {
        mGameController.SetDoShowTsunamiNotifications(value);
    }

    UnitsSystem GetDisplayUnitsSystem() const
    {
        return mGameController.GetDisplayUnitsSystem();
    }

    void SetDisplayUnitsSystem(UnitsSystem value)
    {
        mGameController.SetDisplayUnitsSystem(value);
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
        return mGameController.GetDoShowElectricalNotifications();
    }

    void SetDoShowElectricalNotifications(bool value)
    {
        mGameController.SetDoShowElectricalNotifications(value);
    }

    float GetZoomIncrement() const
    {
        return mZoomIncrement;
    }

    void SetZoomIncrement(float value)
    {
        mZoomIncrement = value;
    }

    int GetPanIncrement() const
    {
        return mPanIncrement;
    }

    void SetPanIncrement(int value)
    {
        mPanIncrement = value;
    }

    bool GetShowStatusText() const
    {
        return mGameController.GetShowStatusText();
    }

    void SetShowStatusText(bool value)
    {
        mGameController.SetShowStatusText(value);
    }

    bool GetShowExtendedStatusText() const
    {
        return mGameController.GetShowExtendedStatusText();
    }

    void SetShowExtendedStatusText(bool value)
    {
        mGameController.SetShowExtendedStatusText(value);
    }

    //
    // NPCs
    //

    size_t GetMaxNpcs() const
    {
        return mGameController.GetMaxNpcs();
    }

    void SetMaxNpcs(size_t value)
    {
        mGameController.SetMaxNpcs(value);
    }

    size_t GetMinMaxNpcs() const
    {
        return mGameController.GetMinMaxNpcs();
    }

    size_t GetMaxMaxNpcs() const
    {
        return mGameController.GetMaxMaxNpcs();
    }

    size_t GetNpcsPerGroup() const
    {
        return mGameController.GetNpcsPerGroup();
    }

    void SetNpcsPerGroup(size_t value)
    {
        mGameController.SetNpcsPerGroup(value);
    }

    size_t GetMinNpcsPerGroup() const
    {
        return mGameController.GetMinNpcsPerGroup();
    }

    size_t GetMaxNpcsPerGroup() const
    {
        return mGameController.GetMaxNpcsPerGroup();
    }

    bool GetDoAutoFocusOnNpcPlacement() const
    {
        return mGameController.GetDoAutoFocusOnNpcPlacement();
    }

    void SetDoAutoFocusOnNpcPlacement(bool value)
    {
        mGameController.SetDoAutoFocusOnNpcPlacement(value);
    }

    bool GetDoShowNpcNotifications() const
    {
        return mGameController.GetDoShowNpcNotifications();
    }

    void SetDoShowNpcNotifications(bool value)
    {
        mGameController.SetDoShowNpcNotifications(value);
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
        mGameController.NotifySoundMuted(value);
    }

    float GetBackgroundMusicVolume() const
    {
        return mMusicController.GetBackgroundMusicVolume();
    }

    void SetBackgroundMusicVolume(float value)
    {
        mMusicController.SetBackgroundMusicVolume(value);
    }

    bool GetPlayBackgroundMusic() const
    {
        return mMusicController.GetPlayBackgroundMusic();
    }

    void SetPlayBackgroundMusic(bool value)
    {
        mMusicController.SetPlayBackgroundMusic(value);
    }

    float GetGameMusicVolume() const
    {
        return mMusicController.GetGameMusicVolume();
    }

    void SetGameMusicVolume(float value)
    {
        mMusicController.SetGameMusicVolume(value);
    }

    bool GetPlaySinkingMusic() const
    {
        return mMusicController.GetPlaySinkingMusic();
    }

    void SetPlaySinkingMusic(bool value)
    {
        mMusicController.SetPlaySinkingMusic(value);
    }

    //
    // Language
    //

    std::optional<std::string> GetDesiredLanguage() const
    {
        if (mLocalizationManager.GetDesiredLanguage().has_value())
            return mLocalizationManager.GetDesiredLanguage()->Identifier;
        else
            return std::nullopt;
    }

    void SetDesiredLanguage(std::optional<std::string> const & languageIdentifier)
    {
        mLocalizationManager.StoreDesiredLanguage(languageIdentifier);
    }

    auto GetAvailableLanguages() const
    {
        return mLocalizationManager.GetAvailableLanguages();
    }

private:

    static std::filesystem::path GetPreferencesFilePath();

    static std::optional<picojson::object> LoadPreferencesRootObject();

    void LoadPreferences();

    void SavePreferences() const;

private:

    // The owners/storage of our properties
    IGameController & mGameController;
    MusicController & mMusicController;
    LocalizationManager & mLocalizationManager;

    //
    // The preferences for which we are the owners/storage
    //

    std::vector<std::filesystem::path> mShipLoadDirectories;
    std::optional<ShipLoadSpecifications> mLastShipLoadedSpecifications;
    bool mReloadLastLoadedShipOnStartup;

    std::filesystem::path mScreenshotsFolderPath;

    std::vector<Version> mBlacklistedUpdates;
    bool mCheckUpdatesAtStartup;
    bool mStartInFullScreen;
    bool mShowStartupTip;
    bool mSaveSettingsOnExit;
    bool mShowShipDescriptionsAtShipLoad;
    bool mAutoShowSwitchboard;
    int mSwitchboardBackgroundBitmapIndex;

    float mZoomIncrement;
    int mPanIncrement;
};
