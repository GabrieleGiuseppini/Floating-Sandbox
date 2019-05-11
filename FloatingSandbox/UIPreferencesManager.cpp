/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UIPreferencesManager.h"

#include "StandardSystemPaths.h"

#include <Game/ResourceLoader.h>

#include <GameCore/Utils.h>

const std::string Filename = "ui_preferences.json";

UIPreferencesManager::UIPreferencesManager(std::shared_ptr<GameController> gameController)
    : mGameController(std::move(gameController))
{
    //
    // Set defaults for our preferences
    //

    auto const defaultShipLoadDirectory = ResourceLoader::GetInstalledShipFolderPath();
    mShipLoadDirectories.push_back(defaultShipLoadDirectory);

    mScreenshotsFolderPath = StandardSystemPaths::GetInstance().GetUserPicturesGameFolderPath();

    mShowStartupTip = true;
    mShowShipDescriptionsAtShipLoad = true;


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

void UIPreferencesManager::LoadPreferences()
{
    auto preferencesRootValue = Utils::ParseJSONFile(
        StandardSystemPaths::GetInstance().GetUserSettingsGameFolderPath() / Filename);

    if (preferencesRootValue.is<picojson::object>())
    {
        auto preferencesRootObject = preferencesRootValue.get<picojson::object>();

        //
        // Ship load directories
        //

        auto shipLoadDirectoriesIt = preferencesRootObject.find("ship_load_directories");
        if (shipLoadDirectoriesIt != preferencesRootObject.end()
            && shipLoadDirectoriesIt->second.is<picojson::array>())
        {
            mShipLoadDirectories.clear();

            auto shipLoadDirectories = shipLoadDirectoriesIt->second.get<picojson::array>();
            for (auto shipLoadDirectory : shipLoadDirectories)
            {
                if (shipLoadDirectory.is<std::string>())
                {
                    auto shipLoadDirectoryPath = std::filesystem::path(shipLoadDirectory.get<std::string>());

                    // Make sure dir still exists, and it's not in the vector already
                    if (std::filesystem::exists(shipLoadDirectoryPath)
                        && mShipLoadDirectories.end() == std::find(
                            mShipLoadDirectories.begin(),
                            mShipLoadDirectories.end(),
                            shipLoadDirectoryPath))
                    {
                        mShipLoadDirectories.push_back(shipLoadDirectoryPath);
                    }
                }
            }
        }

        //
        // Screenshots folder path
        //

        auto screenshotsFolderPathIt = preferencesRootObject.find("screenshots_folder_path");
        if (screenshotsFolderPathIt != preferencesRootObject.end()
            && screenshotsFolderPathIt->second.is<std::string>())
        {
            mScreenshotsFolderPath = screenshotsFolderPathIt->second.get<std::string>();
        }

        //
        // Show startup tip
        //

        auto showStartupTipIt = preferencesRootObject.find("show_startup_tip");
        if (showStartupTipIt != preferencesRootObject.end()
            && showStartupTipIt->second.is<bool>())
        {
            mShowStartupTip = showStartupTipIt->second.get<bool>();
        }

        //
        // Show ship descriptions at ship load
        //

        auto showShipDescriptionAtShipLoadIt = preferencesRootObject.find("show_ship_descriptions_at_ship_load");
        if (showShipDescriptionAtShipLoadIt != preferencesRootObject.end()
            && showShipDescriptionAtShipLoadIt->second.is<bool>())
        {
            mShowShipDescriptionsAtShipLoad = showShipDescriptionAtShipLoadIt->second.get<bool>();
        }

        //
        // Show tsunami notifications
        //

        auto showTsunamiNotificationsIt = preferencesRootObject.find("show_tsunami_notifications");
        if (showTsunamiNotificationsIt != preferencesRootObject.end()
            && showTsunamiNotificationsIt->second.is<bool>())
        {
            mGameController->SetShowTsunamiNotifications(showTsunamiNotificationsIt->second.get<bool>());
        }
    }
}

void UIPreferencesManager::SavePreferences() const
{
    picojson::object preferencesRootObject;


    // Add ship load directories

    picojson::array shipLoadDirectories;
    for (auto shipDir : mShipLoadDirectories)
    {
        shipLoadDirectories.push_back(picojson::value(shipDir.string()));
    }

    preferencesRootObject["ship_load_directories"] = picojson::value(shipLoadDirectories);

    // Add screenshots folder path
    preferencesRootObject["screenshots_folder_path"] = picojson::value(mScreenshotsFolderPath.string());

    // Add show startup tip
    preferencesRootObject["show_startup_tip"] = picojson::value(mShowStartupTip);

    // Add show ship descriptions at ship load
    preferencesRootObject["show_ship_descriptions_at_ship_load"] = picojson::value(mShowShipDescriptionsAtShipLoad);

    // Add show tsunami notification
    preferencesRootObject["show_tsunami_notifications"] = picojson::value(mGameController->GetShowTsunamiNotifications());

    // Save
    Utils::SaveJSONFile(
        picojson::value(preferencesRootObject),
        StandardSystemPaths::GetInstance().GetUserSettingsGameFolderPath() / Filename);
}