/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UIPreferences.h"

#include "StandardSystemPaths.h"

#include <Game/ResourceLoader.h>

#include <GameCore/Utils.h>

#include <algorithm>

const std::string Filename = "ui_preferences.json";

UIPreferences::UIPreferences()
    : mShipLoadDirectories()
{
    //
    // Set defaults
    //

    auto const defaultShipLoadDirectory = ResourceLoader::GetInstalledShipFolderPath();

    mShipLoadDirectories.push_back(defaultShipLoadDirectory);


    //
    // Load preferences
    //

    std::vector<std::filesystem::path> newShipLoadDirectories;

    try
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
                auto shipLoadDirectories = shipLoadDirectoriesIt->second.get<picojson::array>();
                for (auto shipLoadDirectory : shipLoadDirectories)
                {
                    if (shipLoadDirectory.is<std::string>())
                    {
                        auto shipLoadDirectoryPath = std::filesystem::path(shipLoadDirectory.get<std::string>());
                        if (std::filesystem::exists(shipLoadDirectoryPath)
                            && newShipLoadDirectories.end() == std::find(
                                newShipLoadDirectories.begin(),
                                newShipLoadDirectories.end(),
                                shipLoadDirectoryPath))
                        {
                            newShipLoadDirectories.push_back(shipLoadDirectoryPath);
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        // Ignore
    }

    // Make sure default is in, at leat at the end
    if (newShipLoadDirectories.end() == std::find(
        newShipLoadDirectories.begin(),
        newShipLoadDirectories.end(),
        defaultShipLoadDirectory))
    {
        newShipLoadDirectories.push_back(defaultShipLoadDirectory);
    }

    mShipLoadDirectories = newShipLoadDirectories;
}

UIPreferences::~UIPreferences()
{
    //
    // Save preferences
    //

    try
    {
        picojson::object preferencesRootObject;


        // Add ship load directories

        picojson::array shipLoadDirectories;
        for (auto shipDir : mShipLoadDirectories)
        {
            shipLoadDirectories.push_back(picojson::value(shipDir.string()));
        }

        preferencesRootObject["ship_load_directories"] = picojson::value(shipLoadDirectories);

        // Save
        Utils::SaveJSONFile(
            picojson::value(preferencesRootObject),
            StandardSystemPaths::GetInstance().GetUserSettingsGameFolderPath() / Filename);
    }
    catch (...)
    {
        // Ignore
    }
}