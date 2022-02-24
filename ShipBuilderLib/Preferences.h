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

    Preferences(std::optional<UnitsSystem> displayUnitsSystem);

    ~Preferences();

public:

    UnitsSystem GetDisplayUnitsSystem() const
    {
        return mDisplayUnitsSystem;
    }

private:

    static std::filesystem::path GetPreferencesFilePath();

    static std::optional<picojson::object> LoadPreferencesRootObject();

    void LoadPreferences();

    void SavePreferences() const;

private:

    UnitsSystem mDisplayUnitsSystem;
};

}