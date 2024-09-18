/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UIPreferencesManager.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/ResourceLocator.h>

#include <GameCore/Utils.h>

UIPreferencesManager::UIPreferencesManager(
    IGameController & gameController,
    MusicController & musicController,
    LocalizationManager & localizationManager)
    : mGameController(gameController)
    , mMusicController(musicController)
    , mLocalizationManager(localizationManager)    
{
    //
    // Set defaults for our preferences
    //

    mLastShipLoadedSpecifications.reset();
    mReloadLastLoadedShipOnStartup = false;

    mScreenshotsFolderPath = StandardSystemPaths::GetInstance().GetUserPicturesGameFolderPath();

    mBlacklistedUpdates = { };
    mCheckUpdatesAtStartup = true;
    mStartInFullScreen = true;
    mShowStartupTip = true;
    mSaveSettingsOnExit = true;
    mShowShipDescriptionsAtShipLoad = true;
    mAutoShowSwitchboard = true;
    mSwitchboardBackgroundBitmapIndex = 0;

    mZoomIncrement = 1.05f;
    mPanIncrement = 20;


    //
    // Load preferences
    //

    try
    {
        LoadPreferences();
    }
    catch (...)
    {
        // Ignore
    }
}

UIPreferencesManager::~UIPreferencesManager()
{
    //
    // Save preferences
    //

    try
    {
        SavePreferences();
    }
    catch (...)
    {
        // Ignore
    }
}

std::optional<std::string> UIPreferencesManager::LoadPreferredLanguage()
{
    auto const preferencesRootObject = LoadPreferencesRootObject();
    if (preferencesRootObject.has_value())
    {
        if (auto const it = preferencesRootObject->find("language");
            it != preferencesRootObject->end() && it->second.is<std::string>())
        {
            return it->second.get<std::string>();
        }
    }

    return std::nullopt;
}

std::filesystem::path UIPreferencesManager::GetPreferencesFilePath()
{
    return StandardSystemPaths::GetInstance().GetUserGameRootFolderPath() / "ui_preferences.json";
}

std::optional<picojson::object> UIPreferencesManager::LoadPreferencesRootObject()
{
    auto const preferencesFilePath = GetPreferencesFilePath();

    if (std::filesystem::exists(preferencesFilePath))
    {
        auto const preferencesRootValue = Utils::ParseJSONFile(preferencesFilePath);

        if (preferencesRootValue.is<picojson::object>())
        {
            return preferencesRootValue.get<picojson::object>();
        }
    }

    return std::nullopt;
}

void UIPreferencesManager::LoadPreferences()
{
    auto const preferencesRootObject = LoadPreferencesRootObject();
    if (preferencesRootObject.has_value())
    {
        //
        // Load version
        //

        Version settingsVersion(1, 16, 7, 0); // Last version _without_ a "version" field
        if (auto versionIt = preferencesRootObject->find("version");
            versionIt != preferencesRootObject->end() && versionIt->second.is<std::string>())
        {
            settingsVersion = Version::FromString(versionIt->second.get<std::string>());
        }

        //
        // Ship load directories
        //

        if (auto shipLoadDirectoriesIt = preferencesRootObject->find("ship_load_directories");
            shipLoadDirectoriesIt != preferencesRootObject->end() && shipLoadDirectoriesIt->second.is<picojson::array>())
        {
            mShipLoadDirectories.clear();

            auto shipLoadDirectories = shipLoadDirectoriesIt->second.get<picojson::array>();
            for (auto const shipLoadDirectory : shipLoadDirectories)
            {
                if (shipLoadDirectory.is<std::string>())
                {
                    auto shipLoadDirectoryPath = std::filesystem::path(shipLoadDirectory.get<std::string>());

                    // Make sure dir still exists, and it's not in the vector already
                    if (std::filesystem::exists(shipLoadDirectoryPath)
                        && std::find(
                            mShipLoadDirectories.cbegin(),
                            mShipLoadDirectories.cend(),
                            shipLoadDirectoryPath) == mShipLoadDirectories.cend())
                    {
                        mShipLoadDirectories.push_back(shipLoadDirectoryPath);
                    }
                }
            }
        }

        //
        // Last ship loaded
        //

        if (auto const lastShipLoadedSpecsIt = preferencesRootObject->find("last_ship_loaded_specifications"); // First introduced in 1.17.4
            lastShipLoadedSpecsIt != preferencesRootObject->cend() && lastShipLoadedSpecsIt->second.is<picojson::object>())
        {
            picojson::object const & specsObject = lastShipLoadedSpecsIt->second.get<picojson::object>();
            mLastShipLoadedSpecifications = ShipLoadSpecifications::FromJson(specsObject);
        }
        else if (auto const lastShipLoadedFilePathIt = preferencesRootObject->find("last_ship_loaded_file_path"); // No more since 1.17.4
            lastShipLoadedFilePathIt != preferencesRootObject->cend() && lastShipLoadedFilePathIt->second.is<std::string>())
        {
            mLastShipLoadedSpecifications = ShipLoadSpecifications(lastShipLoadedFilePathIt->second.get<std::string>());
        }

        //
        // Reload last loaded ship on startup
        //

        if (auto reloadLastLoadedShipOnStartupIt = preferencesRootObject->find("reload_last_loaded_ship_on_startup");
            reloadLastLoadedShipOnStartupIt != preferencesRootObject->end() && reloadLastLoadedShipOnStartupIt->second.is<bool>())
        {
            mReloadLastLoadedShipOnStartup = reloadLastLoadedShipOnStartupIt->second.get<bool>();
        }

        //
        // Screenshots folder path
        //

        if (auto screenshotsFolderPathIt = preferencesRootObject->find("screenshots_folder_path");
            screenshotsFolderPathIt != preferencesRootObject->end() && screenshotsFolderPathIt->second.is<std::string>())
        {
            mScreenshotsFolderPath = screenshotsFolderPathIt->second.get<std::string>();
        }

        //
        // Blacklisted updates
        //

        if (auto blacklistedUpdatedIt = preferencesRootObject->find("blacklisted_updates");
            blacklistedUpdatedIt != preferencesRootObject->end() && blacklistedUpdatedIt->second.is<picojson::array>())
        {
            mBlacklistedUpdates.clear();

            auto blacklistedUpdates = blacklistedUpdatedIt->second.get<picojson::array>();
            for (auto blacklistedUpdate : blacklistedUpdates)
            {
                if (blacklistedUpdate.is<std::string>())
                {
                    auto blacklistedVersion = Version::FromString(blacklistedUpdate.get<std::string>());

                    if (mBlacklistedUpdates.cend() == std::find(
                        mBlacklistedUpdates.cbegin(),
                        mBlacklistedUpdates.cend(),
                        blacklistedVersion))
                    {
                        mBlacklistedUpdates.push_back(blacklistedVersion);
                    }
                }
            }
        }

        //
        // Check updates at startup
        //

        if (auto checkUpdatesAtStartupIt = preferencesRootObject->find("check_updates_at_startup");
            checkUpdatesAtStartupIt != preferencesRootObject->end() && checkUpdatesAtStartupIt->second.is<bool>())
        {
            mCheckUpdatesAtStartup = checkUpdatesAtStartupIt->second.get<bool>();
        }

        //
        // Start in full screen
        //

        if (auto startInFullScreenIt = preferencesRootObject->find("start_in_full_screen");
            startInFullScreenIt != preferencesRootObject->end() && startInFullScreenIt->second.is<bool>())
        {
            mStartInFullScreen = startInFullScreenIt->second.get<bool>();
        }

        //
        // Show startup tip
        //

        if (auto showStartupTipIt = preferencesRootObject->find("show_startup_tip");
            showStartupTipIt != preferencesRootObject->end() && showStartupTipIt->second.is<bool>())
        {
            mShowStartupTip = showStartupTipIt->second.get<bool>();
        }

        //
        // Save settings on exit
        //

        if (auto saveSettingsOnExitIt = preferencesRootObject->find("save_settings_on_exit");
            saveSettingsOnExitIt != preferencesRootObject->end() && saveSettingsOnExitIt->second.is<bool>())
        {
            mSaveSettingsOnExit = saveSettingsOnExitIt->second.get<bool>();
        }

        //
        // Show ship descriptions at ship load
        //

        if (auto showShipDescriptionAtShipLoadIt = preferencesRootObject->find("show_ship_descriptions_at_ship_load");
            showShipDescriptionAtShipLoadIt != preferencesRootObject->end() && showShipDescriptionAtShipLoadIt->second.is<bool>())
        {
            mShowShipDescriptionsAtShipLoad = showShipDescriptionAtShipLoadIt->second.get<bool>();
        }

        //
        // Show tsunami notifications
        //

        if (auto showTsunamiNotificationsIt = preferencesRootObject->find("show_tsunami_notifications");
            showTsunamiNotificationsIt != preferencesRootObject->end() && showTsunamiNotificationsIt->second.is<bool>())
        {
            mGameController.SetDoShowTsunamiNotifications(showTsunamiNotificationsIt->second.get<bool>());
        }

        //
        // Display units system
        //

        if (auto displayUnitsSystemIt = preferencesRootObject->find("display_units_system");
            displayUnitsSystemIt != preferencesRootObject->end() && displayUnitsSystemIt->second.is<std::int64_t>())
        {
            mGameController.SetDisplayUnitsSystem(static_cast<UnitsSystem>(displayUnitsSystemIt->second.get<std::int64_t>()));
        }

        //
        // Ship auto-texturization shared settings
        //

        if (auto shipAutoTexturizationSharedSettingsIt = preferencesRootObject->find("ship_auto_texturization_default_settings");
            shipAutoTexturizationSharedSettingsIt != preferencesRootObject->end() && shipAutoTexturizationSharedSettingsIt->second.is<picojson::object>())
        {
            mGameController.SetShipAutoTexturizationSharedSettings(ShipAutoTexturizationSettings::FromJSON(shipAutoTexturizationSharedSettingsIt->second.get<picojson::object>()));
        }

        // We don't load/save this setting on purpose
        //////
        ////// Ship auto-texturization force shared settings onto ship
        //////

        ////if (auto shipAutoTexturizationForceDefaultsOntoShipIt = preferencesRootObject->find("ship_auto_texturization_force_defaults_onto_ship");
        ////    shipAutoTexturizationForceDefaultsOntoShipIt != preferencesRootObject->end()
        ////    && shipAutoTexturizationForceDefaultsOntoShipIt->second.is<bool>())
        ////{
        ////    mGameController.SetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings(shipAutoTexturizationForceSharedSettingsOntoShipIt->second.get<bool>());
        ////}

        //
        // Camera speed adjustment
        //

        if (auto it = preferencesRootObject->find("camera_speed_adjustment");
            it != preferencesRootObject->end() && it->second.is<double>())
        {
            mGameController.SetCameraSpeedAdjustment(static_cast<float>(it->second.get<double>()));
        }

        //
        // Auto-focus at ship load
        //

        if (auto it = preferencesRootObject->find("auto_zoom_at_ship_load");
            it != preferencesRootObject->end() && it->second.is<bool>())
        {
            mGameController.SetDoAutoFocusOnShipLoad(it->second.get<bool>());
        }

        //
        // Continuous auto-focus on ships
        //
        // If set: ships; else: none (we don't save SelectedNpc)
        //

        if (auto continuousAutoFocusIt = preferencesRootObject->find("continuous_auto_focus");
            continuousAutoFocusIt != preferencesRootObject->end() && continuousAutoFocusIt->second.is<bool>())
        {
            bool const value = continuousAutoFocusIt->second.get<bool>();
            mGameController.SetAutoFocusTarget(value ? std::optional<AutoFocusTargetKindType>(AutoFocusTargetKindType::Ship) : std::nullopt);
        }

        //
        // Auto show switchboard
        //

        if (auto autoShowSwitchboardIt = preferencesRootObject->find("auto_show_switchboard");
            autoShowSwitchboardIt != preferencesRootObject->end() && autoShowSwitchboardIt->second.is<bool>())
        {
            mAutoShowSwitchboard = autoShowSwitchboardIt->second.get<bool>();
        }

        //
        // Switchboard background bitmap index
        //

        if (auto switchboardBackgroundBitmapIndexIt = preferencesRootObject->find("switchboard_background_bitmap_index");
            switchboardBackgroundBitmapIndexIt != preferencesRootObject->end() && switchboardBackgroundBitmapIndexIt->second.is<std::int64_t>())
        {
            mSwitchboardBackgroundBitmapIndex = static_cast<int>(switchboardBackgroundBitmapIndexIt->second.get<std::int64_t>());
        }

        //
        // Show electrical notifications
        //

        if (auto showElectricalNotificationsIt = preferencesRootObject->find("show_electrical_notifications");
            showElectricalNotificationsIt != preferencesRootObject->end() && showElectricalNotificationsIt->second.is<bool>())
        {
            mGameController.SetDoShowElectricalNotifications(showElectricalNotificationsIt->second.get<bool>());
        }

        //
        // Zoom increment
        //

        if (auto zoomIncrementIt = preferencesRootObject->find("zoom_increment");
            zoomIncrementIt != preferencesRootObject->end() && zoomIncrementIt->second.is<double>())
        {
            mZoomIncrement = static_cast<float>(zoomIncrementIt->second.get<double>());
        }

        //
        // Pan increment
        //

        if (auto panIncrementIt = preferencesRootObject->find("pan_increment");
            panIncrementIt != preferencesRootObject->end() && panIncrementIt->second.is<std::int64_t>())
        {
            mPanIncrement = static_cast<int>(panIncrementIt->second.get<std::int64_t>());
        }

        //
        // Show status text
        //

        if (auto showStatusTextIt = preferencesRootObject->find("show_status_text");
            showStatusTextIt != preferencesRootObject->end() && showStatusTextIt->second.is<bool>())
        {
            mGameController.SetShowStatusText(showStatusTextIt->second.get<bool>());
        }

        //
        // Show extended status text
        //

        if (auto showExtendedStatusTextIt = preferencesRootObject->find("show_extended_status_text");
            showExtendedStatusTextIt != preferencesRootObject->end() && showExtendedStatusTextIt->second.is<bool>())
        {
            mGameController.SetShowExtendedStatusText(showExtendedStatusTextIt->second.get<bool>());
        }

        //
        // Sound and Music
        //

        {
            // Global mute
            if (auto globalMuteIt = preferencesRootObject->find("global_mute");
                globalMuteIt != preferencesRootObject->end() && globalMuteIt->second.is<bool>())
            {
                bool const isSoundMuted = globalMuteIt->second.get<bool>();
                AudioController::SetGlobalMute(isSoundMuted);
                mGameController.NotifySoundMuted(isSoundMuted);
            }

            // Background music volume
            if (auto backgroundMusicVolumeIt = preferencesRootObject->find("background_music_volume");
                backgroundMusicVolumeIt != preferencesRootObject->end() && backgroundMusicVolumeIt->second.is<double>())
            {
                mMusicController.SetBackgroundMusicVolume(static_cast<float>(backgroundMusicVolumeIt->second.get<double>()));
            }

            // Play background music
            if (auto playBackgroundMusicIt = preferencesRootObject->find("play_background_music");
                playBackgroundMusicIt != preferencesRootObject->end() && playBackgroundMusicIt->second.is<bool>())
            {
                mMusicController.SetPlayBackgroundMusic(playBackgroundMusicIt->second.get<bool>());
            }

            // Last-played background music
            if (auto lastPlayedBackgroundMusicIt = preferencesRootObject->find("last_played_background_music");
                lastPlayedBackgroundMusicIt != preferencesRootObject->end() && lastPlayedBackgroundMusicIt->second.is<int64_t>())
            {
                mMusicController.SetLastPlayedBackgroundMusic(static_cast<size_t>(lastPlayedBackgroundMusicIt->second.get<int64_t>()));
            }

            // Game music volume
            if (auto gameMusicVolumeIt = preferencesRootObject->find("game_music_volume");
                gameMusicVolumeIt != preferencesRootObject->end() && gameMusicVolumeIt->second.is<double>())
            {
                mMusicController.SetGameMusicVolume(static_cast<float>(gameMusicVolumeIt->second.get<double>()));
            }

            // Play sinking music
            if (auto playSinkingMusicIt = preferencesRootObject->find("play_sinking_music");
                playSinkingMusicIt != preferencesRootObject->end() && playSinkingMusicIt->second.is<bool>())
            {
                mMusicController.SetPlaySinkingMusic(playSinkingMusicIt->second.get<bool>());
            }
        }

        // Note: we do not load language, as it has been loaded already and passed
        // to the LocalizationManager
    }
}

void UIPreferencesManager::SavePreferences() const
{
    picojson::object preferencesRootObject;

    // Add version
    preferencesRootObject["version"] = picojson::value(Version::CurrentVersion().ToString());

    // Add ship load directories
    {
        picojson::array shipLoadDirectories;
        for (auto shipDir : mShipLoadDirectories)
        {
            shipLoadDirectories.push_back(picojson::value(shipDir.string()));
        }

        preferencesRootObject["ship_load_directories"] = picojson::value(shipLoadDirectories);
    }

    // Add last ship loaded
    if (mLastShipLoadedSpecifications.has_value())
    {
        preferencesRootObject["last_ship_loaded_specifications"] = picojson::value(mLastShipLoadedSpecifications->ToJson());
    }

    // Add reload last loaded ship on startup
    preferencesRootObject["reload_last_loaded_ship_on_startup"] = picojson::value(mReloadLastLoadedShipOnStartup);

    // Add screenshots folder path
    preferencesRootObject["screenshots_folder_path"] = picojson::value(mScreenshotsFolderPath.string());

    // Add blacklisted updates

    picojson::array blacklistedUpdates;
    for (auto blacklistedUpdate : mBlacklistedUpdates)
    {
        blacklistedUpdates.push_back(picojson::value(blacklistedUpdate.ToString()));
    }

    preferencesRootObject["blacklisted_updates"] = picojson::value(blacklistedUpdates);

    // Add check updates at startup
    preferencesRootObject["check_updates_at_startup"] = picojson::value(mCheckUpdatesAtStartup);

    // Add start in full screen
    preferencesRootObject["start_in_full_screen"] = picojson::value(mStartInFullScreen);

    // Add show startup tip
    preferencesRootObject["show_startup_tip"] = picojson::value(mShowStartupTip);

    // Add save settings on exit
    preferencesRootObject["save_settings_on_exit"] = picojson::value(mSaveSettingsOnExit);

    // Add show ship descriptions at ship load
    preferencesRootObject["show_ship_descriptions_at_ship_load"] = picojson::value(mShowShipDescriptionsAtShipLoad);

    // Add show tsunami notification
    preferencesRootObject["show_tsunami_notifications"] = picojson::value(mGameController.GetDoShowTsunamiNotifications());

    // Add display units system
    preferencesRootObject["display_units_system"] = picojson::value(static_cast<std::int64_t>(mGameController.GetDisplayUnitsSystem()));

    // Add ship auto-texturization shared settings
    preferencesRootObject["ship_auto_texturization_default_settings"] = picojson::value(mGameController.GetShipAutoTexturizationSharedSettings().ToJSON());

    // We don't load/save this setting on purpose
    ////// Add ship auto-texturization force shared settings onto ship
    ////preferencesRootObject["ship_auto_texturization_force_defaults_onto_ship"] = picojson::value(mGameController.GetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings());

    // Add camera speed adjustment
    preferencesRootObject["camera_speed_adjustment"] = picojson::value(static_cast<double>(mGameController.GetCameraSpeedAdjustment()));

    // Add auto focus at ship load
    preferencesRootObject["auto_zoom_at_ship_load"] = picojson::value(mGameController.GetDoAutoFocusOnShipLoad());

    // Add continuous auto focus
    preferencesRootObject["continuous_auto_focus"] = picojson::value(mGameController.GetAutoFocusTarget() == AutoFocusTargetKindType::Ship);

    // Add auto show switchboard
    preferencesRootObject["auto_show_switchboard"] = picojson::value(mAutoShowSwitchboard);

    // Add switchboard background bitmap index
    preferencesRootObject["switchboard_background_bitmap_index"] = picojson::value(static_cast<std::int64_t>(mSwitchboardBackgroundBitmapIndex));

    // Add show electrical notifications
    preferencesRootObject["show_electrical_notifications"] = picojson::value(mGameController.GetDoShowElectricalNotifications());

    // Add zoom increment
    preferencesRootObject["zoom_increment"] = picojson::value(static_cast<double>(mZoomIncrement));

    // Add pan increment
    preferencesRootObject["pan_increment"] = picojson::value(static_cast<std::int64_t>(mPanIncrement));

    // Add show status text
    preferencesRootObject["show_status_text"] = picojson::value(mGameController.GetShowStatusText());

    // Add show extended status text
    preferencesRootObject["show_extended_status_text"] = picojson::value(mGameController.GetShowExtendedStatusText());

    //
    // Sounds and Music
    //

    {
        preferencesRootObject["global_mute"] = picojson::value(AudioController::GetGlobalMute());

        preferencesRootObject["background_music_volume"] = picojson::value(static_cast<double>(mMusicController.GetBackgroundMusicVolume()));

        preferencesRootObject["play_background_music"] = picojson::value(mMusicController.GetPlayBackgroundMusic());

        preferencesRootObject["last_played_background_music"] = picojson::value(static_cast<int64_t>(mMusicController.GetLastPlayedBackgroundMusic()));

        preferencesRootObject["game_music_volume"] = picojson::value(static_cast<double>(mMusicController.GetGameMusicVolume()));

        preferencesRootObject["play_sinking_music"] = picojson::value(mMusicController.GetPlaySinkingMusic());
    }

    // Language
    if (mLocalizationManager.GetDesiredLanguage().has_value())
        preferencesRootObject["language"] = picojson::value(mLocalizationManager.GetDesiredLanguage()->Identifier);

    // Save
    Utils::SaveJSONFile(
        picojson::value(preferencesRootObject),
        GetPreferencesFilePath());
}