/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Preferences.h"

#include <UILib/StandardSystemPaths.h>

#include <GameCore/Utils.h>
#include <GameCore/Version.h>

namespace ShipBuilder {

Preferences::Preferences()
{
    //
    // Set defaults for our preferences
    //

    mDisplayUnitsSystem = UnitsSystem::SI_Celsius;


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

Preferences::~Preferences()
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

std::filesystem::path Preferences::GetPreferencesFilePath()
{
    return StandardSystemPaths::GetInstance().GetUserGameRootFolderPath() / "shipbuilder_preferences.json";
}

std::optional<picojson::object> Preferences::LoadPreferencesRootObject()
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

void Preferences::LoadPreferences()
{
    auto const preferencesRootObject = LoadPreferencesRootObject();
    if (preferencesRootObject.has_value())
    {
        //
        // Load version
        //

        Version settingsVersion(Version::Zero());
        if (auto versionIt = preferencesRootObject->find("version");
            versionIt != preferencesRootObject->end() && versionIt->second.is<std::string>())
        {
            settingsVersion = Version::FromString(versionIt->second.get<std::string>());
        }

        //
        // Display units system
        //

        if (auto displayUnitsSystemIt = preferencesRootObject->find("display_units_system");
            displayUnitsSystemIt != preferencesRootObject->end() && displayUnitsSystemIt->second.is<std::int64_t>())
        {
            mDisplayUnitsSystem = static_cast<UnitsSystem>(displayUnitsSystemIt->second.get<std::int64_t>());
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
    }
}

void Preferences::SavePreferences() const
{
    picojson::object preferencesRootObject;

    // Add version
    preferencesRootObject["version"] = picojson::value(Version::CurrentVersion().ToString());

    // Add display units system
    preferencesRootObject["display_units_system"] = picojson::value(static_cast<std::int64_t>(mDisplayUnitsSystem));

    // Add ship load directories
    {
        picojson::array shipLoadDirectories;
        for (auto shipDir : mShipLoadDirectories)
        {
            shipLoadDirectories.push_back(picojson::value(shipDir.string()));
        }

        preferencesRootObject["ship_load_directories"] = picojson::value(shipLoadDirectories);
    }

    // Save
    Utils::SaveJSONFile(
        picojson::value(preferencesRootObject),
        GetPreferencesFilePath());
}

}