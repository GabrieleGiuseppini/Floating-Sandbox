/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <filesystem>
#include <optional>

namespace ShipBuilder {

/*
 * This class manages ShipBuilder preferences, and takes care of loading, persisting, and persisting them.
 */
class Preferences
{
public:

    Preferences();

    ~Preferences();

public:

    UnitsSystem GetDisplayUnitsSystem() const
    {
        return mDisplayUnitsSystem;
    }

    void SetDisplayUnitsSystem(UnitsSystem value)
    {
        mDisplayUnitsSystem = value;
    }

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

private:

    static std::filesystem::path GetPreferencesFilePath();

    static std::optional<picojson::object> LoadPreferencesRootObject();

    void LoadPreferences();

    void SavePreferences() const;

private:

    UnitsSystem mDisplayUnitsSystem;
    std::vector<std::filesystem::path> mShipLoadDirectories;
};

}